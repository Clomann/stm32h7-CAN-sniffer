"""
Parse CAN logger blocks produced by CanLogBuffer_ReadNextBlock.

The binary layout is generated from app/services/logging/CanLogBuffer.h at runtime to keep this
tool in sync with the firmware source.
"""

from __future__ import annotations

import argparse
import importlib
import struct
from datetime import datetime
from dataclasses import dataclass
from collections import deque
from pathlib import Path
from typing import Deque, Iterator, List, Sequence

from gen_canlog_layout import generate_layout, needs_regeneration

if needs_regeneration():
    generate_layout()

layout = importlib.import_module("generated_canlog_layout")

BLOCK_HEADER_STRUCT = layout.BLOCK_HEADER_STRUCT
BLOCK_HEADER_SIZE = layout.BLOCK_HEADER_SIZE
ENTRY_HEADER_STRUCT = layout.ENTRY_HEADER_STRUCT
ENTRY_HEADER_SIZE = layout.ENTRY_HEADER_SIZE
ENTRY_FIXED_STRUCT = layout.ENTRY_FIXED_STRUCT
ENTRY_FIXED_SIZE = layout.ENTRY_FIXED_SIZE
SYNC_FIXED_STRUCT = layout.SYNC_FIXED_STRUCT
SYNC_FIXED_SIZE = layout.SYNC_FIXED_SIZE

CAN_DLC_MASK = getattr(layout, "CAN_DLC_MASK", 0x0F)
CAN_FLAG_IDE = getattr(layout, "CAN_FLAG_IDE", 1 << getattr(layout, "CAN_FLAG_IDE_Pos", 4))
CAN_FLAG_RTR_FDF = getattr(layout, "CAN_FLAG_RTR_FDF", 1 << getattr(layout, "CAN_FLAG_RTR_FDF_Pos", 5))
CAN_FLAG_BRS = getattr(layout, "CAN_FLAG_BRS", 1 << getattr(layout, "CAN_FLAG_BRS_Pos", 6))
CAN_FLAG_ESI = getattr(layout, "CAN_FLAG_ESI", 1 << getattr(layout, "CAN_FLAG_ESI_Pos", 7))
CLB_ENTRY_TYPE_FRAME = layout.CLB_ENTRY_TYPE_FRAME
CLB_ENTRY_TYPE_SYNC = layout.CLB_ENTRY_TYPE_SYNC
CLB_ENTRY_TYPE_MARKER = layout.CLB_ENTRY_TYPE_MARKER

CLASSIC_MAX_LEN = 8
FDCAN_MAX_LEN = layout.CANLOG_ENTRY_MAX_DATA_LENGTH

LEGACY_BLOCK_HEADER_STRUCT = struct.Struct("<BBHHBBII")
LEGACY_BLOCK_HEADER_SIZE = LEGACY_BLOCK_HEADER_STRUCT.size
LEGACY_ENTRY_HEADER_STRUCT = struct.Struct("<BBH")
LEGACY_ENTRY_HEADER_SIZE = LEGACY_ENTRY_HEADER_STRUCT.size
LEGACY_ENTRY_FIXED_STRUCT = struct.Struct("<QIBBBB")
LEGACY_ENTRY_FIXED_SIZE = LEGACY_ENTRY_FIXED_STRUCT.size
LEGACY_CLASSIC_MAX_LEN = 8
LEGACY_FD_MAX_LEN = 64

DEFAULT_TRACE = Path(__file__).with_name("CAN-TRACE-1_MBitPs_85_percent_1_byte_dlc.BIN")
DEFAULT_LOG_DIR = Path(__file__).with_name("logs")
DEFAULT_TRACE_OUT = DEFAULT_TRACE


@dataclass
class CanLogFrame:
    """Container for one decoded CAN log entry."""

    frame_type: str  # "Classic" or "FD"
    epoch: int
    timestamp_us: int  # reconstructed absolute timestamp if sync was seen
    can_id: int
    channel: int
    dlc: int
    flags: int  # raw dlc_flags upper bits
    data: bytes
    block_index: int
    block_offset: int

    def as_dict(self) -> dict:
        return {
            "type": self.frame_type,
            "timestamp_us": self.timestamp_us,
            "timestamp_s": self.timestamp_us / 1_000_000,
            "epoch": self.epoch,
            "can_id": self.can_id,
            "channel": self.channel,
            "dlc": self.dlc,
            "flags": self.flags,
            "data": list(self.data),
            "block_index": self.block_index,
            "block_offset": self.block_offset,
        }


def _read_block_header(fh) -> dict | None:
    """Read a block header, supporting both legacy (16-byte) and current (20-byte) layouts."""
    prefix = fh.read(2)
    if not prefix or len(prefix) < 2:
        return None

    header_size = prefix[1]
    if header_size < 2:
        return None

    rest = fh.read(header_size - 2)
    if len(rest) < header_size - 2:
        return None

    header_bytes = prefix + rest

    if header_size == BLOCK_HEADER_SIZE:
        (
            version,
            _header_size,
            epoch,
            cnt,
            block_size,
            block_fill,
            ingress_frames,
            frame_count,
        ) = BLOCK_HEADER_STRUCT.unpack(header_bytes)
        return {
            "legacy": False,
            "version": version,
            "header_size": header_size,
            "epoch": epoch,
            "cnt": cnt,
            "block_size": block_size,
            "block_fill": block_fill,
            "ingress_frames": ingress_frames,
            "frame_count": frame_count,
            "header_bytes": header_bytes,
        }
    if header_size == LEGACY_BLOCK_HEADER_SIZE:
        (
            version,
            _header_size,
            block_size,
            block_fill,
            epoch,
            cnt,
            ingress_frames,
            frame_count,
        ) = LEGACY_BLOCK_HEADER_STRUCT.unpack(header_bytes)
        return {
            "legacy": True,
            "version": version,
            "header_size": header_size,
            "epoch": epoch,
            "cnt": cnt,
            "block_size": block_size,
            "block_fill": block_fill,
            "ingress_frames": ingress_frames,
            "frame_count": frame_count,
            "header_bytes": header_bytes,
        }

    return None


def _iter_block_frames(block: memoryview, block_index: int, abs_time_state: dict, header_meta: dict) -> Iterator[CanLogFrame]:
    """
    Yield decoded frames within a single block.

    block_fill in the header marks the end of valid data; padding is 0xFF.
    """
    header_size = header_meta["header_size"]
    block_size = header_meta["block_size"]
    block_fill = header_meta["block_fill"]
    header_frame_count = header_meta["frame_count"]
    header_cnt = header_meta["cnt"]
    ingress_frames = header_meta["ingress_frames"]
    legacy = header_meta["legacy"]
    epoch = header_meta["epoch"]

    if len(block) < header_size:
        return
    if block_size == 0 or block_fill < header_size:
        return

    end_of_valid_data = min(block_fill, len(block))
    offset = header_size
    frames_in_block = 0
    entries_in_block = 0

    while True:
        entry_struct = LEGACY_ENTRY_HEADER_STRUCT if legacy else ENTRY_HEADER_STRUCT
        if offset + entry_struct.size > end_of_valid_data:
            break

        entry_type, header_len, total_len = entry_struct.unpack_from(block, offset)
        if legacy:
            min_len = entry_struct.size + LEGACY_ENTRY_FIXED_SIZE
        else:
            if entry_type == CLB_ENTRY_TYPE_SYNC:
                min_len = entry_struct.size + SYNC_FIXED_SIZE
            else:
                min_len = entry_struct.size + ENTRY_FIXED_SIZE

        if total_len < min_len or offset + total_len > end_of_valid_data or total_len == 0:
            break

        fixed_offset = offset + entry_struct.size

        if legacy:
            timestamp, can_id, channel, dlc, flags, _bus_id = LEGACY_ENTRY_FIXED_STRUCT.unpack_from(
                block, fixed_offset
            )
            payload_offset = fixed_offset + LEGACY_ENTRY_FIXED_SIZE
            payload_len = total_len - (LEGACY_ENTRY_HEADER_SIZE + LEGACY_ENTRY_FIXED_SIZE)

            if entry_type == 1:
                data_len = min(LEGACY_CLASSIC_MAX_LEN, payload_len, dlc)
                frame_type = "Classic"
            elif entry_type == 2:
                data_len = min(LEGACY_FD_MAX_LEN, payload_len, dlc)
                frame_type = "FD"
            else:
                offset += total_len
                continue

            timestamp_us = timestamp
            dlc_val = dlc
        else:
            if entry_type == CLB_ENTRY_TYPE_SYNC and total_len >= ENTRY_HEADER_SIZE + SYNC_FIXED_SIZE:
                timestamp, abs_high = SYNC_FIXED_STRUCT.unpack_from(block, fixed_offset)
                abs_time_state["high"] = abs_high
                entries_in_block += 1
                offset += total_len
                continue

            if entry_type != CLB_ENTRY_TYPE_FRAME:
                offset += total_len
                continue

            timestamp, can_id, channel, dlc_flags, data_len = ENTRY_FIXED_STRUCT.unpack_from(
                block, fixed_offset
            )
            payload_offset = fixed_offset + ENTRY_FIXED_SIZE
            payload_len = total_len - (ENTRY_HEADER_SIZE + ENTRY_FIXED_SIZE)
            data_len = min(data_len, payload_len, FDCAN_MAX_LEN)

            dlc_val = dlc_flags & CAN_DLC_MASK
            flags = dlc_flags & ~CAN_DLC_MASK
            is_fd = bool(flags & CAN_FLAG_RTR_FDF) or dlc_val > CLASSIC_MAX_LEN
            frame_type = "FD" if is_fd else "Classic"

            abs_high = abs_time_state.get("high", 0)
            timestamp_us = (abs_high << 32) | timestamp

        data = block[payload_offset : payload_offset + data_len].tobytes()

        yield CanLogFrame(
            frame_type=frame_type,
            epoch=epoch,
            timestamp_us=timestamp_us,
            can_id=can_id,
            channel=channel,
            dlc=dlc_val,
            flags=flags,
            data=data,
            block_index=block_index,
            block_offset=offset,
        )
        frames_in_block += 1
        entries_in_block += 1
        offset += total_len

    if entries_in_block != header_frame_count:
        print(
            f"Warning: block {block_index} header frame_count={header_frame_count} "
            f"parsed={entries_in_block} (cnt={header_cnt}, ingress_frames={ingress_frames})"
        )


def iter_can_frames(path: Path = DEFAULT_TRACE, abort_on_gap: bool = False, continue_on_gap: bool = False) -> Iterator[CanLogFrame]:
    """
    Stream frames from a CAN trace without holding the entire file in memory.
    """
    block_index = 0
    buf = bytearray(max(BLOCK_HEADER_SIZE, LEGACY_BLOCK_HEADER_SIZE, 64 * 1024))
    prev_epoch = None
    prev_cnt = None
    abs_time_state = {"high": 0}
    with path.open("rb") as fh:
        print("Parsing data ...")

        while True:
            header_meta = _read_block_header(fh)
            if not header_meta:
                break

            header_size = header_meta["header_size"]
            epoch = header_meta["epoch"]
            block_cnt = header_meta["cnt"]
            block_size = header_meta["block_size"]
            block_fill = header_meta["block_fill"]
            frame_count = header_meta["frame_count"]
            ingress_frames = header_meta["ingress_frames"]
            legacy = header_meta["legacy"]

            min_header_size = LEGACY_BLOCK_HEADER_SIZE if legacy else BLOCK_HEADER_SIZE
            if block_size == 0:
                break
            if block_size < header_size or block_size < min_header_size:
                break

            gap_reason = None
            if prev_epoch is not None:
                if epoch != prev_epoch:
                    gap_reason = f"epoch change {prev_epoch}->{epoch}"
                expected_cnt = (prev_cnt + 1) & 0xFF
                if gap_reason is None and block_cnt != expected_cnt:
                    gap_reason = f"block counter gap {prev_cnt}->{block_cnt}"

            if block_fill <= header_size:
                gap_reason = gap_reason or "empty block (block_fill <= header_size)"

            if gap_reason:
                msg = f"Block validation failed at index {block_index}: {gap_reason}"
                if abort_on_gap:
                    raise RuntimeError(msg)
                else:
                    print(msg)
                    if not continue_on_gap:
                        break

            if len(buf) < block_size:
                buf = bytearray(block_size)

            # Fill buffer with header + payload to avoid allocations per block.
            buf_view = memoryview(buf)[:block_size]
            buf_view[:header_size] = header_meta["header_bytes"]

            remaining = block_size - header_size
            if remaining:
                read_n = fh.readinto(buf_view[header_size:])
                if read_n is None or read_n < remaining:
                    break  # Truncated block at EOF

            for frame in _iter_block_frames(buf_view, block_index, abs_time_state, header_meta):
                yield frame

            prev_epoch = epoch
            prev_cnt = block_cnt
            block_index += 1


def parse_can_trace(path: Path = DEFAULT_TRACE, abort_on_gap: bool = False) -> List[CanLogFrame]:
    """
    Return a list of all frames in the trace file.

    Note: The trace file can be large; prefer iter_can_frames for streaming use.
    """
    return list(iter_can_frames(path, abort_on_gap=abort_on_gap))


def concat_logs_to_trace(log_dir: Path, out_path: Path) -> int:
    """
    Concatenate individual log chunks (CAN.LOG*) into a single CAN-TRACE.BIN file.

    Returns number of files concatenated.
    """
    if not log_dir.exists() or not log_dir.is_dir():
        raise FileNotFoundError(f"Log directory not found: {log_dir}")

    def log_key(p: Path) -> int:
        # Extract numeric suffix after "CAN.LOG"
        try:
            return int(p.name.split("CAN.LOG", 1)[1])
        except Exception:
            return -1

    log_files = sorted([p for p in log_dir.iterdir() if p.name.startswith("CAN.LOG")], key=log_key)
    log_files = [p for p in log_files if log_key(p) >= 0]

    if not log_files:
        raise FileNotFoundError(f"No CAN.LOG* files found in {log_dir}")

    out_path.parent.mkdir(parents=True, exist_ok=True)

    with out_path.open("wb") as out_f:
        for p in log_files:
            with p.open("rb") as in_f:
                while True:
                    chunk = in_f.read(1024 * 1024)
                    if not chunk:
                        break
                    out_f.write(chunk)

    return len(log_files)


def _format_frame(frame: CanLogFrame) -> str:
    data_hex = " ".join(f"{byte:02X}" for byte in frame.data)
    return (
        f"{frame.timestamp_us:012d}us "
        f"ID=0x{frame.can_id:08X} "
        f"Ch{frame.channel} DLC={frame.dlc} "
        f"Flags=0x{frame.flags:02X} "
        f"{frame.frame_type} [{data_hex}]"
    )


def _format_vector_ascii_frame(base_timestamp_us: int | None, frame: CanLogFrame, ts_width: int) -> str:
    """Render a single frame in Vector ASCII (.asc) format."""
    if base_timestamp_us is None:
        rel_time_s = frame.timestamp_us / 1_000_000
    else:
        rel_time_s = (frame.timestamp_us - base_timestamp_us) / 1_000_000
    data_len = min(frame.dlc, len(frame.data))
    data_hex = " ".join(f"{byte:02X}" for byte in frame.data[:data_len])

    # Currently we treat every frame as extended-ID. Flags are kept for future decoding (IDE/RTR/BRS/ESI).
    is_extended = True
    can_id_str = f"{frame.can_id:X}{'x' if is_extended else ''}"

    direction = "Rx"
    frame_type = "fd" if frame.frame_type == "FD" else "d"

    return f"{rel_time_s:>{ts_width}.6f} {frame.channel} {can_id_str} {direction} {frame_type} {data_len} {data_hex}".rstrip()


class VectorAsciiWriter:
    """Stream writer that emits frames in Vector ASCII (.asc) format."""

    def __init__(self, path: Path, use_relative_ts: bool):
        self.path = path
        self.file = path.open("w", encoding="ascii", newline="\n")
        now = datetime.now()
        now_str = now.strftime("%a %b %d %H:%M:%S %Y")
        self.file.write(f"date {now_str}\n")
        ts_mode = "relative" if use_relative_ts else "absolute"
        self.file.write(f"base hex  timestamps {ts_mode}\n")
        self.file.write("no internal events logged\n")
        self.file.write(f"Begin Triggerblock {now_str}\n")
        self.file.write("   0.000000 Start of measurement\n")
        self.base_timestamp_us: int | None = None if use_relative_ts else 0
        self.ts_width: int = 14  # minimum width; grows if integer part grows

    def write_frame(self, frame: CanLogFrame) -> None:
        if self.base_timestamp_us is None:
            self.base_timestamp_us = frame.timestamp_us
        needed_width = len(str(int(frame.timestamp_us / 1_000_000))) + 1 + 6  # digits + dot + 6 decimals
        if needed_width > self.ts_width:
            self.ts_width = needed_width

        line = _format_vector_ascii_frame(self.base_timestamp_us, frame, self.ts_width)
        self.file.write(line + "\n")

    def close(self) -> None:
        if self.file:
            self.file.write("End Triggerblock\n")
            self.file.close()
            self.file = None


def main(argv: Sequence[str] | None = None) -> None:
    parser = argparse.ArgumentParser(description="Parse CAN-TRACE.BIN produced by the logger.")
    parser.add_argument(
        "path",
        nargs="?",
        type=Path,
        help=f"Path to CAN trace (default: {DEFAULT_TRACE.name})",
    )
    parser.add_argument(
        "--build-trace",
        action="store_true",
        help=f"Concatenate CAN.LOG* files into {DEFAULT_TRACE_OUT.name} and exit.",
    )
    parser.add_argument(
        "--logs-dir",
        type=Path,
        help=f"Directory containing CAN.LOG* parts (default: {DEFAULT_LOG_DIR})",
    )
    parser.add_argument(
        "--trace-out",
        type=Path,
        help=f"Output path when using --build-trace (default: {DEFAULT_TRACE_OUT})",
    )
    parser.add_argument(
        "--head",
        type=int,
        default=10,
        help="Print the first N frames while streaming (default: 10).",
    )
    parser.add_argument(
        "--count-only",
        action="store_true",
        help="Only count frames instead of collecting them into memory.",
    )
    parser.add_argument(
        "--collect-all",
        action="store_true",
        help="Also materialize all frames into a list (can use a lot of RAM).",
    )
    parser.add_argument(
        "--tail",
        type=int,
        default=10,
        help="Also print the last N frames (default: 10).",
    )
    parser.add_argument(
        "--write-asc",
        type=Path,
        help="Write frames to a Vector ASCII (.asc) file at the given path.",
    )
    parser.add_argument(
        "--split-epochs",
        action="store_true",
        help="When writing .asc, split output by epoch sequences into separate files.",
    )
    parser.add_argument(
        "--merge-scattered-epochs",
        action="store_true",
        help="When combined with --split-epochs, reuse the same output file per epoch even if that epoch appears after gaps.",
    )
    parser.add_argument(
        "--relative-ts",
        action="store_true",
        help="When writing .asc, make timestamps relative to the first frame instead of absolute.",
    )
    parser.add_argument(
        "--abort-on-gap",
        action="store_true",
        help="Abort parsing when a block gap/epoch change/empty block is detected.",
    )
    parser.add_argument(
        "--continue-on-gap",
        action="store_true",
        help="Keep parsing after gap/epoch changes instead of stopping (default stops but does not raise).",
    )
    args = parser.parse_args(argv)

    # Merging scattered epochs only makes sense when we keep parsing through gaps.
    if args.merge_scattered_epochs and not args.continue_on_gap:
        print("Enabling --continue-on-gap because --merge-scattered-epochs was requested.")
        args.continue_on_gap = True

    trace_path = args.path or DEFAULT_TRACE
    logs_dir = args.logs_dir or DEFAULT_LOG_DIR
    trace_out = args.trace_out or DEFAULT_TRACE_OUT

    if args.build_trace:
        count = concat_logs_to_trace(logs_dir, trace_out)
        print(f"Concatenated {count} log file(s) into {trace_out}")
        return

    head = 0 if args.count_only else args.head
    tail = 0 if args.count_only else max(0, args.tail)
    preview: List[CanLogFrame] = []
    tail_buf: Deque[CanLogFrame] = deque(maxlen=tail) if tail else deque()
    asc_writer = VectorAsciiWriter(args.write_asc, args.relative_ts) if args.write_asc and not args.split_epochs else None
    current_epoch: int | None = None
    epoch_seq = 0

    max_time = 0
    # When merging scattered epochs, keep a writer per epoch so segments that reappear are appended.
    epoch_writers: dict[int, VectorAsciiWriter] = {}

    if args.collect_all:
        frames = list(iter_can_frames(trace_path, abort_on_gap=args.abort_on_gap, continue_on_gap=args.continue_on_gap))
        preview = frames[:head]
        if tail:
            tail_buf.extend(frames[-tail:])
        count = len(frames)
        if args.write_asc:
            if args.split_epochs:
                base = args.write_asc
                suffix = base.suffix or ".asc"
                stem = base.stem
                if args.merge_scattered_epochs:
                    for frame in frames:
                        writer = epoch_writers.get(frame.epoch)
                        if not writer:
                            out_path = base.with_name(f"{stem}_epoch{frame.epoch:03d}{suffix}")
                            writer = VectorAsciiWriter(out_path, args.relative_ts)
                            epoch_writers[frame.epoch] = writer
                        writer.write_frame(frame)
                else:
                    current_epoch = None
                    epoch_seq = 0
                    writer: VectorAsciiWriter | None = None
                    for frame in frames:
                        if current_epoch is None or frame.epoch != current_epoch:
                            if writer:
                                writer.close()
                            epoch_seq += 1
                            out_path = base.with_name(f"{stem}_epoch{frame.epoch:03d}_seq{epoch_seq:03d}{suffix}")
                            writer = VectorAsciiWriter(out_path, args.relative_ts)
                            current_epoch = frame.epoch
                        writer.write_frame(frame)
                    if writer:
                        writer.close()
            else:
                asc_writer = VectorAsciiWriter(args.write_asc, args.relative_ts)
                for frame in frames:
                    asc_writer.write_frame(frame)
                asc_writer.close()
        max_time = frames[-1].timestamp_us
    else:
        count = 0
        for frame in iter_can_frames(trace_path, abort_on_gap=args.abort_on_gap, continue_on_gap=args.continue_on_gap):
            if count < head:
                preview.append(frame)
            if tail:
                tail_buf.append(frame)
            if args.write_asc:
                if args.split_epochs:
                    base = args.write_asc
                    suffix = base.suffix or ".asc"
                    stem = base.stem
                    if args.merge_scattered_epochs:
                        writer = epoch_writers.get(frame.epoch)
                        if not writer:
                            out_path = base.with_name(f"{stem}_epoch{frame.epoch:03d}{suffix}")
                            writer = VectorAsciiWriter(out_path, args.relative_ts)
                            epoch_writers[frame.epoch] = writer
                        writer.write_frame(frame)
                    else:
                        if current_epoch is None or frame.epoch != current_epoch:
                            if asc_writer:
                                asc_writer.close()
                            epoch_seq += 1
                            out_path = base.with_name(f"{stem}_epoch{frame.epoch:03d}_seq{epoch_seq:03d}{suffix}")
                            asc_writer = VectorAsciiWriter(out_path, args.relative_ts)
                            current_epoch = frame.epoch
                        if asc_writer:
                            asc_writer.write_frame(frame)
                else:
                    if asc_writer:
                        asc_writer.write_frame(frame)
            if frame.timestamp_us > max_time:
                max_time = frame.timestamp_us
            count += 1

    if preview:
        for frame in preview:
            print(_format_frame(frame))

    if tail and tail_buf:
        if preview:
            print("\n--- tail ---")
        for frame in tail_buf:
            print(_format_frame(frame))

    print(f"\nParsed {count} frame(s) from {trace_path}")

    # Close any open writers created during streaming.
    if asc_writer:
        asc_writer.close()
    for writer in epoch_writers.values():
        writer.close()

    if args.write_asc:
        if args.split_epochs:
            if args.merge_scattered_epochs:
                print(f"Wrote Vector ASCII logs merged by epoch using base {args.write_asc}")
            else:
                print(f"Wrote Vector ASCII logs split by epoch using base {args.write_asc}")
        else:
            print(f"Wrote Vector ASCII log to {args.write_asc}")

    if args.collect_all:
        print(f"Collected {count} frame(s) into memory.")


if __name__ == "__main__":
    main()
