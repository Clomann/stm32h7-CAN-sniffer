"""
Check frame continuity in binary (.bin) or Vector ASCII (.asc) CAN logs.

Two independent checks are available and can be combined:

Counter check (default):
  Parses the first data byte of every frame and verifies that it increases by
  1 (modulo 256) for each CAN ID/channel combination.

ID-sequence check (--check-id-sequence):
  Verifies that the CAN ID itself increments by 1 (modulo --id-modulo, default
  256) for each channel.  Useful when the firmware encodes a frame counter
  directly in the arbitration ID.

Fast ID-only check (--id-sequence-only):
  For binary logs, checks only the CAN ID sequence without copying payloads or
  allocating objects for each frame.
"""

from __future__ import annotations

import argparse
import subprocess
import time
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Dict, Iterable, Iterator, List, Optional, Tuple


@dataclass(slots=True)
class Frame:
    """Minimal frame info extracted from a CAN log."""

    timestamp: float
    channel: int
    can_id: int
    counter: Optional[int]  # None when DLC=0 (no payload to read counter from)
    line_no: int
    position_unit: str = "line"


_PROGRESS_INTERVAL = 30.0
_PROGRESS_CHECK_LINES = 10_000


def describe_git_version() -> str:
    """Return a human-readable repository revision for report provenance."""
    try:
        return subprocess.check_output(
            ["git", "describe", "--long", "--tags", "--dirty"],
            cwd=Path(__file__).resolve().parent,
            text=True,
            stderr=subprocess.DEVNULL,
        ).strip()
    except (OSError, subprocess.CalledProcessError):
        return "unavailable"


def format_duration(seconds: float) -> str:
    """Format a duration as HH:MM:SS."""
    total_seconds = max(0, round(seconds))
    hours, remainder = divmod(total_seconds, 3600)
    minutes, seconds = divmod(remainder, 60)
    return f"{hours:02d}:{minutes:02d}:{seconds:02d}"


def parse_vector_ascii(path: Path) -> Iterable[Frame]:
    """
    Yield frames from a Vector ASCII (.asc) CAN log.

    Only lines that look like a frame are parsed; headers and comments are skipped.
    """
    with path.open(encoding="ascii", errors="ignore") as fh:
        for line_no, raw in enumerate(fh, start=1):
            parts = raw.split()  # split() strips whitespace implicitly
            if len(parts) < 6:
                continue

            try:
                timestamp = float(parts[0])
                channel = int(parts[1])
                can_id = int(parts[2].rstrip("xX"), 16)
                dlc = int(parts[5])
            except ValueError:
                # Header or malformed line
                continue

            data_tokens = parts[6 : 6 + dlc]
            try:
                counter: Optional[int] = int(data_tokens[0], 16) if data_tokens else None
            except ValueError:
                counter = None

            yield Frame(timestamp=timestamp, channel=channel, can_id=can_id, counter=counter, line_no=line_no)


def make_byte_progress_reporter(path: Path) -> Callable[[int, int], None]:
    """Create a percentage/ETA reporter for a binary input file."""
    total_bytes = path.stat().st_size
    next_percent = 1
    started_at = time.monotonic()
    print(f"Parsed bytes: 0% (0/{total_bytes}), ETA: calculating", flush=True)

    def report_progress(bytes_read: int, file_size: int) -> None:
        nonlocal next_percent
        if file_size <= 0:
            return
        completed_percent = min(100, bytes_read * 100 // file_size)
        elapsed = time.monotonic() - started_at
        remaining = elapsed * max(0, file_size - bytes_read) / bytes_read if bytes_read else 0
        while next_percent <= completed_percent:
            print(
                f"Parsed bytes: {next_percent}% ({bytes_read}/{file_size}), "
                f"ETA: {format_duration(remaining)}",
                flush=True,
            )
            next_percent += 1

    return report_progress


def parse_binary(
    path: Path,
    abort_on_gap: bool = False,
    continue_on_gap: bool = False,
) -> Iterator[Frame]:
    """Yield frames directly from a binary CAN logger trace."""
    # Keep this import lazy: checking an ASC file does not need the generated
    # firmware layout used by the binary parser.
    from log_parser import iter_can_frames

    for frame_no, decoded in enumerate(
        iter_can_frames(
            path,
            abort_on_gap=abort_on_gap,
            continue_on_gap=continue_on_gap,
            on_progress=make_byte_progress_reporter(path),
        ),
        start=1,
    ):
        yield Frame(
            timestamp=decoded.timestamp_us / 1_000_000,
            channel=decoded.channel,
            can_id=decoded.can_id,
            counter=decoded.data[0] if decoded.data else None,
            line_no=frame_no,
            position_unit="frame",
        )


def resolve_input_format(path: Path, input_format: str = "auto") -> str:
    """Resolve an explicit or filename-derived input format."""
    if input_format == "auto":
        suffix = path.suffix.lower()
        if suffix == ".asc":
            return "asc"
        elif suffix == ".bin":
            return "bin"
        raise ValueError(
            f"Cannot detect input format from {path.name!r}; "
            "use --input-format asc or --input-format bin."
        )
    return input_format


def parse_input(
    path: Path,
    input_format: str = "auto",
    abort_on_gap: bool = False,
    continue_on_gap: bool = False,
) -> Iterable[Frame]:
    """Select the ASC or binary streaming parser."""
    input_format = resolve_input_format(path, input_format)
    if input_format == "asc":
        return parse_vector_ascii(path)
    return parse_binary(path, abort_on_gap=abort_on_gap, continue_on_gap=continue_on_gap)


@dataclass(slots=True)
class Gap:
    """Represents a missing-counter (or missing-ID) gap between two consecutive frames."""

    key: Tuple[int, int]  # (channel, can_id); can_id == -1 for ID-sequence gaps
    missing_start: int    # first missing value
    missing_count: int    # number of missing values
    modulo: int           # wrap-around modulus used to generate values
    previous: Frame
    current: Frame


def _missing_preview(gap: Gap, preview_values: int) -> tuple[list[int], int]:
    """Return (values_to_show, extra_count) without building the full missing list."""
    n = min(preview_values, gap.missing_count)
    values = [(gap.missing_start + i) % gap.modulo for i in range(n)]
    return values, gap.missing_count - n


def find_all_gaps(
    frames: Iterable[Frame],
    check_id_seq: bool = False,
    id_modulo: int = 256,
    on_flush: Callable[[List[Gap], List[Gap]], None] | None = None,
) -> Tuple[int, int, int, Dict[Tuple[int, int], int], int, Dict[int, int]]:
    """
    Single-pass check for counter gaps and optionally ID-sequence gaps.

    on_flush(counter_batch, id_batch) is called every ~30 s and once at the end
    with accumulated gaps since the last flush.

    Returns (frame_count, frames_with_counter, n_counter_gaps, missing_by_key, n_id_gaps, missing_by_channel).
    """
    last_counter: Dict[Tuple[int, int], Frame] = {}
    missing_by_key: Dict[Tuple[int, int], int] = defaultdict(int)
    n_counter_gaps = 0
    frames_with_counter = 0

    last_id: Dict[int, Frame] = {}
    missing_by_channel: Dict[int, int] = defaultdict(int)
    n_id_gaps = 0

    pending_counter: List[Gap] = []
    pending_id: List[Gap] = []
    last_flush = time.monotonic()
    frame_count = 0

    for frame in frames:
        frame_count += 1

        # --- counter check ---
        if frame.counter is not None:
            frames_with_counter += 1
            key = (frame.channel, frame.can_id)
            prev = last_counter.get(key)
            if prev is not None:
                delta = (frame.counter - prev.counter) % 256
                if delta not in (0, 1):
                    n_missing = delta - 1
                    pending_counter.append(Gap(
                        key=key,
                        missing_start=(prev.counter + 1) % 256,
                        missing_count=n_missing,
                        modulo=256,
                        previous=prev,
                        current=frame,
                    ))
                    missing_by_key[key] += n_missing
                    n_counter_gaps += 1
            last_counter[key] = frame

        # --- ID-sequence check ---
        if check_id_seq:
            ch = frame.channel
            prev_id = last_id.get(ch)
            if prev_id is not None:
                delta = (frame.can_id - prev_id.can_id) % id_modulo
                if delta not in (0, 1):
                    n_missing = delta - 1
                    pending_id.append(Gap(
                        key=(ch, -1),
                        missing_start=(prev_id.can_id + 1) % id_modulo,
                        missing_count=n_missing,
                        modulo=id_modulo,
                        previous=prev_id,
                        current=frame,
                    ))
                    missing_by_channel[ch] += n_missing
                    n_id_gaps += 1
            last_id[ch] = frame

        # Periodically flush accumulated gaps to bound memory use.
        if on_flush and frame_count % _PROGRESS_CHECK_LINES == 0:
            now = time.monotonic()
            if now - last_flush >= _PROGRESS_INTERVAL:
                on_flush(pending_counter, pending_id)
                pending_counter.clear()
                pending_id.clear()
                last_flush = now

    # flush remainder
    if on_flush and (pending_counter or pending_id):
        on_flush(pending_counter, pending_id)

    return frame_count, frames_with_counter, n_counter_gaps, missing_by_key, n_id_gaps, missing_by_channel


def find_binary_id_gaps_fast(
    path: Path,
    id_modulo: int = 256,
    on_flush: Callable[[List[Gap], List[Gap]], None] | None = None,
    abort_on_gap: bool = False,
    continue_on_gap: bool = False,
) -> Tuple[int, int, Dict[int, int], Dict[Tuple[int, int], int]]:
    """
    Check binary ID continuity directly in each block.

    This path deliberately avoids payload copies, per-frame objects, and
    per-frame generator hand-offs. Frame/Gap objects are created only when a
    discontinuity must be reported.
    """
    import log_parser as binary

    last_id_by_epoch_channel: Dict[Tuple[int, int], int] = {}
    last_timestamp_by_epoch_channel: Dict[Tuple[int, int], int] = {}
    last_frame_by_epoch_channel: Dict[Tuple[int, int], int] = {}
    missing_by_channel: Dict[int, int] = defaultdict(int)
    missing_by_epoch_channel: Dict[Tuple[int, int], int] = defaultdict(int)
    pending_id: List[Gap] = []
    n_id_gaps = 0
    frame_count = 0
    next_flush_check = _PROGRESS_CHECK_LINES
    last_flush = time.monotonic()
    abs_time_high = 0

    entry_header_unpack = binary.ENTRY_HEADER_STRUCT.unpack_from
    entry_fixed_unpack = binary.ENTRY_FIXED_STRUCT.unpack_from
    sync_fixed_unpack = binary.SYNC_FIXED_STRUCT.unpack_from
    legacy_header_unpack = binary.LEGACY_ENTRY_HEADER_STRUCT.unpack_from
    legacy_fixed_unpack = binary.LEGACY_ENTRY_FIXED_STRUCT.unpack_from

    def record_gap(
        channel: int,
        epoch: int,
        can_id: int,
        timestamp_us: int,
        previous_id: int,
        previous_timestamp_us: int,
        previous_frame_no: int,
        missing_count: int,
    ) -> None:
        nonlocal n_id_gaps
        pending_id.append(
            Gap(
                key=(channel, -1),
                missing_start=(previous_id + 1) % id_modulo,
                missing_count=missing_count,
                modulo=id_modulo,
                previous=Frame(
                    timestamp=previous_timestamp_us / 1_000_000,
                    channel=channel,
                    can_id=previous_id,
                    counter=None,
                    line_no=previous_frame_no,
                    position_unit="frame",
                ),
                current=Frame(
                    timestamp=timestamp_us / 1_000_000,
                    channel=channel,
                    can_id=can_id,
                    counter=None,
                    line_no=frame_count,
                    position_unit="frame",
                ),
            )
        )
        missing_by_channel[channel] += missing_count
        missing_by_epoch_channel[(epoch, channel)] += missing_count
        n_id_gaps += 1

    for block, block_index, header in binary.iter_can_blocks(
        path,
        abort_on_gap=abort_on_gap,
        continue_on_gap=continue_on_gap,
        on_progress=make_byte_progress_reporter(path),
    ):
        offset = header["header_size"]
        epoch = header["epoch"]
        end_of_valid_data = min(header["block_fill"], len(block))
        entries_in_block = 0

        if header["legacy"]:
            while offset + binary.LEGACY_ENTRY_HEADER_SIZE <= end_of_valid_data:
                entry_type, _header_len, total_len = legacy_header_unpack(block, offset)
                if (
                    total_len < binary.LEGACY_ENTRY_HEADER_SIZE + binary.LEGACY_ENTRY_FIXED_SIZE
                    or offset + total_len > end_of_valid_data
                    or total_len == 0
                ):
                    break

                if entry_type not in (1, 2):
                    offset += total_len
                    continue

                entries_in_block += 1
                timestamp_us, can_id, channel, _dlc, _flags, _bus_id = legacy_fixed_unpack(
                    block, offset + binary.LEGACY_ENTRY_HEADER_SIZE
                )
                frame_count += 1
                key = (epoch, channel)
                previous_frame_no = last_frame_by_epoch_channel.get(key)
                if previous_frame_no is not None:
                    previous_id = last_id_by_epoch_channel[key]
                    delta = (can_id - previous_id) % id_modulo
                    if delta not in (0, 1):
                        record_gap(
                            channel,
                            epoch,
                            can_id,
                            timestamp_us,
                            previous_id,
                            last_timestamp_by_epoch_channel[key],
                            previous_frame_no,
                            delta - 1,
                        )
                last_id_by_epoch_channel[key] = can_id
                last_timestamp_by_epoch_channel[key] = timestamp_us
                last_frame_by_epoch_channel[key] = frame_count
                offset += total_len
        else:
            while offset + binary.ENTRY_HEADER_SIZE <= end_of_valid_data:
                entry_type, _header_len, total_len = entry_header_unpack(block, offset)
                if total_len == 0 or offset + total_len > end_of_valid_data:
                    break

                fixed_offset = offset + binary.ENTRY_HEADER_SIZE

                if entry_type == binary.CLB_ENTRY_TYPE_SYNC:
                    if total_len < binary.ENTRY_HEADER_SIZE + binary.SYNC_FIXED_SIZE:
                        break
                    entries_in_block += 1
                    _timestamp, abs_time_high = sync_fixed_unpack(block, fixed_offset)
                    offset += total_len
                    continue

                if entry_type != binary.CLB_ENTRY_TYPE_FRAME:
                    if entry_type == binary.CLB_ENTRY_TYPE_MARKER:
                        entries_in_block += 1
                    offset += total_len
                    continue

                if total_len < binary.ENTRY_HEADER_SIZE + binary.ENTRY_FIXED_SIZE:
                    break

                entries_in_block += 1
                timestamp, can_id, channel, _dlc_flags, _data_len = entry_fixed_unpack(block, fixed_offset)
                timestamp_us = (abs_time_high << 32) | timestamp
                frame_count += 1
                key = (header["epoch"], channel)
                previous_frame_no = last_frame_by_epoch_channel.get(key)
                if previous_frame_no is not None:
                    previous_id = last_id_by_epoch_channel[key]
                    delta = (can_id - previous_id) % id_modulo
                    if delta not in (0, 1):
                        record_gap(
                            channel,
                            header["epoch"],
                            can_id,
                            timestamp_us,
                            previous_id,
                            last_timestamp_by_epoch_channel[key],
                            previous_frame_no,
                            delta - 1,
                        )
                last_id_by_epoch_channel[key] = can_id
                last_timestamp_by_epoch_channel[key] = timestamp_us
                last_frame_by_epoch_channel[key] = frame_count
                offset += total_len

        if entries_in_block != header["entries_count"]:
            print(
                f"Warning: block {block_index} header entries_count={header['entries_count']} "
                f"parsed={entries_in_block} (cnt={header['cnt']}, "
                f"ingress_frames={header['ingress_frames']})",
                flush=True,
            )

        if on_flush and frame_count >= next_flush_check:
            while next_flush_check <= frame_count:
                next_flush_check += _PROGRESS_CHECK_LINES
            now = time.monotonic()
            if now - last_flush >= _PROGRESS_INTERVAL:
                on_flush([], pending_id)
                pending_id.clear()
                last_flush = now

    if on_flush and pending_id:
        on_flush([], pending_id)

    return frame_count, n_id_gaps, missing_by_channel, missing_by_epoch_channel


def format_id_gap(gap: Gap, preview_values: int = 16) -> str:
    """Pretty-print an ID-sequence gap."""
    channel, _ = gap.key
    values, extra = _missing_preview(gap, preview_values)
    missing = " ".join(f"0x{v:X}" for v in values)
    if extra:
        missing += f" ... (+{extra} more)"
    return (
        f"Ch{channel} ID 0x{gap.previous.can_id:X}->0x{gap.current.can_id:X} "
        f"at {gap.previous.timestamp:.6f}s ({gap.previous.position_unit} {gap.previous.line_no}) -> "
        f"{gap.current.timestamp:.6f}s ({gap.current.position_unit} {gap.current.line_no}): "
        f"missing {gap.missing_count} ID(s): {missing}"
    )


def format_gap(gap: Gap, preview_values: int = 16) -> str:
    """Pretty-print a gap with a compact list of missing counter values."""
    channel, can_id = gap.key
    values, extra = _missing_preview(gap, preview_values)
    missing = " ".join(f"{v:02X}" for v in values)
    if extra:
        missing += f" ... (+{extra} more)"
    return (
        f"Ch{channel} ID=0x{can_id:08X} "
        f"{gap.previous.counter:02X}->{gap.current.counter:02X} "
        f"at {gap.previous.timestamp:.6f}s ({gap.previous.position_unit} {gap.previous.line_no}) -> "
        f"{gap.current.timestamp:.6f}s ({gap.current.position_unit} {gap.current.line_no}): "
        f"missing {gap.missing_count} frame(s): {missing}"
    )


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Detect missing frames in binary or Vector ASCII CAN logs."
    )
    parser.add_argument("path", type=Path, help="Path to a .bin or .asc CAN log")
    parser.add_argument(
        "--input-format",
        choices=("auto", "asc", "bin"),
        default="auto",
        help="Input format (default: auto-detect from .asc/.bin extension).",
    )
    parser.add_argument(
        "--abort-on-gap",
        action="store_true",
        help="For binary input, abort when a block gap, epoch change, or empty block is detected.",
    )
    parser.add_argument(
        "--continue-on-gap",
        action="store_true",
        help="For binary input, continue after block gaps/epoch changes (default: stop parsing).",
    )
    parser.add_argument(
        "--preview",
        type=int,
        default=16,
        help="How many missing counter values to print per gap (default: 16)",
    )
    parser.add_argument(
        "--max-gaps",
        type=int,
        default=20,
        help="Maximum number of gaps to print (default: 20, set to 0 to skip).",
    )
    parser.add_argument(
        "--show-all",
        action="store_true",
        help="Print every detected gap (overrides --max-gaps).",
    )
    parser.add_argument(
        "--check-id-sequence",
        action="store_true",
        help="Also verify that the CAN ID increments by 1 (mod --id-modulo) per channel.",
    )
    parser.add_argument(
        "--id-sequence-only",
        action="store_true",
        help="For BIN input, use the allocation-free fast path and check only the CAN ID sequence.",
    )
    parser.add_argument(
        "--id-modulo",
        type=lambda s: int(s, 0),
        default=256,
        metavar="N",
        help="Wrap-around modulus for the CAN ID sequence check (default: 256). Accepts hex (0x...).",
    )
    args = parser.parse_args()

    try:
        input_format = resolve_input_format(args.path, args.input_format)
    except ValueError as exc:
        parser.error(str(exc))
    if args.id_sequence_only and input_format != "bin":
        parser.error("--id-sequence-only requires BIN input")

    print(f"Checked file: {args.path}")
    print(f"Git version: {describe_git_version()}")

    check_id_sequence = args.check_id_sequence or args.id_sequence_only
    max_counter = None if args.show_all else (args.max_gaps if args.max_gaps > 0 else 0)
    max_id      = None if args.show_all else (args.max_gaps if args.max_gaps > 0 else 0)
    counter_shown = 0
    id_shown = 0

    def on_flush(counter_batch: List[Gap], id_batch: List[Gap]) -> None:
        nonlocal counter_shown, id_shown

        for gap in counter_batch:
            if max_counter is None or counter_shown < max_counter:
                print(format_gap(gap, preview_values=args.preview), flush=True)
                counter_shown += 1

        if check_id_sequence:
            for gap in id_batch:
                if max_id is None or id_shown < max_id:
                    print(format_id_gap(gap, preview_values=args.preview), flush=True)
                    id_shown += 1

    if args.id_sequence_only:
        frame_count, n_id_gaps, missing_by_channel, missing_by_epoch_channel = find_binary_id_gaps_fast(
            args.path,
            id_modulo=args.id_modulo,
            on_flush=on_flush,
            abort_on_gap=args.abort_on_gap,
            continue_on_gap=args.continue_on_gap,
        )
        frames_with_counter = 0
        n_counter_gaps = 0
        missing_by_key: Dict[Tuple[int, int], int] = {}
    else:
        missing_by_epoch_channel: Dict[Tuple[int, int], int] = {}
        frame_count, frames_with_counter, n_counter_gaps, missing_by_key, n_id_gaps, missing_by_channel = find_all_gaps(
            parse_input(
                args.path,
                input_format=input_format,
                abort_on_gap=args.abort_on_gap,
                continue_on_gap=args.continue_on_gap,
            ),
            check_id_seq=check_id_sequence,
            id_modulo=args.id_modulo,
            on_flush=on_flush,
        )

    if frame_count == 0:
        print(f"No frames found in {args.path}")
        return

    # --- counter check summary ---
    if args.id_sequence_only:
        print("Counter check: skipped (--id-sequence-only).")
    elif frames_with_counter == 0:
        print(f"Counter check: NOT APPLICABLE — all {frame_count} frames have DLC=0 (no counter payload).")
    elif n_counter_gaps == 0:
        print(f"Counter check: no missing frames in {args.path} (checked {frames_with_counter} frames with payload).")
    else:
        skipped = n_counter_gaps - counter_shown
        print(f"\nCounter check: {n_counter_gaps} gap(s) in {frame_count} frames ({counter_shown} shown"
              + (f", {skipped} skipped — use --show-all to print everything" if skipped > 0 else "") + ").")
        print("\nMissing frame counts by (channel, CAN ID):")
        for key, count in sorted(missing_by_key.items()):
            channel, can_id = key
            print(f"  Ch{channel} ID=0x{can_id:08X}: {count} missing frame(s)")

    # --- ID-sequence check summary ---
    if check_id_sequence:
        print()
        if n_id_gaps == 0:
            print(f"ID-sequence check (mod {args.id_modulo}): no gaps in {frame_count} frames.")
        else:
            skipped_id = n_id_gaps - id_shown
            print(f"ID-sequence check (mod {args.id_modulo}): {n_id_gaps} gap(s) in {frame_count} frames "
                  f"({id_shown} shown"
                  + (f", {skipped_id} skipped — use --show-all to print everything" if skipped_id > 0 else "") + ").")
            print("\nMissing ID counts by channel:")
            for ch, count in sorted(missing_by_channel.items()):
                print(f"  Ch{ch}: {count} missing ID(s)")
            if missing_by_epoch_channel:
                print("\nMissing ID counts by epoch and channel:")
                for (epoch, ch), count in sorted(missing_by_epoch_channel.items()):
                    print(f"  Epoch {epoch} Ch{ch}: {count} missing ID(s)")


if __name__ == "__main__":
    main()
