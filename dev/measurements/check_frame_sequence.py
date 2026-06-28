"""
Check frame continuity in Vector ASCII (.asc) CAN logs.

Two independent checks are available and can be combined:

Counter check (default):
  Parses the first data byte of every frame and verifies that it increases by
  1 (modulo 256) for each CAN ID/channel combination.

ID-sequence check (--check-id-sequence):
  Verifies that the CAN ID itself increments by 1 (modulo --id-modulo, default
  256) for each channel.  Useful when the firmware encodes a frame counter
  directly in the arbitration ID.
"""

from __future__ import annotations

import argparse
import time
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Dict, Iterable, List, Optional, Tuple


@dataclass(slots=True)
class Frame:
    """Minimal frame info extracted from the ASCII log."""

    timestamp: float
    channel: int
    can_id: int
    counter: Optional[int]  # None when DLC=0 (no payload to read counter from)
    line_no: int


_PROGRESS_INTERVAL = 30.0
_PROGRESS_CHECK_LINES = 10_000


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

            frame_count += 1
            yield Frame(timestamp=timestamp, channel=channel, can_id=can_id, counter=counter, line_no=line_no)


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


def format_id_gap(gap: Gap, preview_values: int = 16) -> str:
    """Pretty-print an ID-sequence gap."""
    channel, _ = gap.key
    values, extra = _missing_preview(gap, preview_values)
    missing = " ".join(f"0x{v:X}" for v in values)
    if extra:
        missing += f" ... (+{extra} more)"
    return (
        f"Ch{channel} ID 0x{gap.previous.can_id:X}->0x{gap.current.can_id:X} "
        f"at {gap.previous.timestamp:.6f}s (line {gap.previous.line_no}) -> "
        f"{gap.current.timestamp:.6f}s (line {gap.current.line_no}): "
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
        f"at {gap.previous.timestamp:.6f}s (line {gap.previous.line_no}) -> "
        f"{gap.current.timestamp:.6f}s (line {gap.current.line_no}): "
        f"missing {gap.missing_count} frame(s): {missing}"
    )


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Detect missing frames in Vector ASCII CAN logs using the first data byte as an 8-bit counter."
    )
    parser.add_argument("path", type=Path, help="Path to the .asc log file")
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
        "--id-modulo",
        type=lambda s: int(s, 0),
        default=256,
        metavar="N",
        help="Wrap-around modulus for the CAN ID sequence check (default: 256). Accepts hex (0x...).",
    )
    args = parser.parse_args()

    max_counter = None if args.show_all else (args.max_gaps if args.max_gaps > 0 else 0)
    max_id      = None if args.show_all else (args.max_gaps if args.max_gaps > 0 else 0)
    counter_shown = 0
    id_shown = 0

    def on_flush(counter_batch: List[Gap], id_batch: List[Gap]) -> None:
        nonlocal counter_shown, id_shown

        for gap in counter_batch:
            if max_counter is None or counter_shown < max_counter:
                print(format_gap(gap, preview_values=args.preview))
                counter_shown += 1

        if args.check_id_sequence:
            for gap in id_batch:
                if max_id is None or id_shown < max_id:
                    print(format_id_gap(gap, preview_values=args.preview))
                    id_shown += 1

    frame_count, frames_with_counter, n_counter_gaps, missing_by_key, n_id_gaps, missing_by_channel = find_all_gaps(
        parse_vector_ascii(args.path),
        check_id_seq=args.check_id_sequence,
        id_modulo=args.id_modulo,
        on_flush=on_flush,
    )

    if frame_count == 0:
        print(f"No frames found in {args.path}")
        return

    # --- counter check summary ---
    if frames_with_counter == 0:
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
    if args.check_id_sequence:
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


if __name__ == "__main__":
    main()
