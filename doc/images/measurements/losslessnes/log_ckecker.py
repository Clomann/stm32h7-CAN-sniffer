"""
Parse CAN logger blocks produced by CanLogBuffer_ReadNextBlock.

Supports variable-length payloads (total_len includes header + fixed fields + dlc bytes).
The binary format is described in app/CanLogBuffer.h and mirrored here.
"""

from __future__ import annotations

import argparse
import struct
from datetime import datetime
from dataclasses import dataclass
from collections import deque
from pathlib import Path
from typing import Deque, Iterator, List, Sequence

# Frame types
CANLOG_UNDEFINED_TYPE = 0
CANLOG_CLASSIC_TYPE = 1
CANLOG_FD_TYPE = 2
CANLOG_MARKER_TYPE = 3
CANLOG_CUSTOM_TYPE = 4

# Binary layouts (all little-endian, packed)
# Block header matches CanLogBlockHeaderType (version, header_size, block_size, block_fill, epoch, cnt, ingress_frames, frame_count)
BLOCK_HEADER_STRUCT = struct.Struct("<BBHHBBII")
ENTRY_HEADER_STRUCT = struct.Struct("<BBH")  # type, header_len, total_len
ENTRY_HEADER_SIZE = ENTRY_HEADER_STRUCT.size

# Fixed fields following the entry header
FIXED_FIELDS_STRUCT = struct.Struct("<QIBBBB")  # timestamp, can_id, channel, dlc, flags, bus_id
FIXED_FIELDS_SIZE = FIXED_FIELDS_STRUCT.size

# Payload caps
CLASSIC_MAX_LEN = 8
FDCAN_MAX_LEN = 64

BLOCK_HEADER_SIZE = BLOCK_HEADER_STRUCT.size

DEFAULT_TRACE = Path(__file__).with_name("CAN-TRACE.BIN")
DEFAULT_LOG_DIR = Path(__file__).with_name("logs")
DEFAULT_TRACE_OUT = DEFAULT_TRACE


@dataclass
class CanLogFrame:
    """Container for one decoded CAN log entry."""

    frame_type: str
    timestamp_us: int
    can_id: int
    channel: int
    dlc: int
    flags: int
    bus_id: int
    data: bytes
    block_index: int
    block_offset: int

    def as_dict(self) -> dict:
        return {
            "type": self.frame_type,
            "timestamp_us": self.timestamp_us,
            "timestamp_s": self.timestamp_us / 1_000_000,
            "can_id": self.can_id,
            "channel": self.channel,
            "dlc": self.dlc,
            "flags": self.flags,
            "bus_id": self.bus_id,
            "data": list(self.data),
            "block_index": self.block_index,
            "block_offset": self.block_offset,
        }


def _iter_block_frames(block: memoryview, block_index: int) -> Iterator[CanLogFrame]:
    """
    Yield decoded frames within a single block.

    block_fill in the header marks the end of valid data; padding is 0xFF.
    """
    if len(block) < BLOCK_HEADER_SIZE:
        return

    _version, header_size, block_size, block_fill, _epoch, _cnt, _ingress_frames, _frame_count = (
        BLOCK_HEADER_STRUCT.unpack_from(block)
    )
    if block_size == 0 or block_fill < header_size:
        return

    end_of_valid_data = min(block_fill, len(block))
    offset = header_size

    while offset + ENTRY_HEADER_SIZE <= end_of_valid_data:
        entry_type, header_len, total_len = ENTRY_HEADER_STRUCT.unpack_from(block, offset)

        min_len = ENTRY_HEADER_SIZE + FIXED_FIELDS_SIZE
        if total_len < min_len or offset + total_len > end_of_valid_data or total_len == 0:
            break

        timestamp, can_id, channel, dlc, flags, bus_id = FIXED_FIELDS_STRUCT.unpack_from(
            block, offset + ENTRY_HEADER_SIZE
        )

        payload_offset = offset + ENTRY_HEADER_SIZE + FIXED_FIELDS_SIZE
        payload_len = total_len - (ENTRY_HEADER_SIZE + FIXED_FIELDS_SIZE)

        if entry_type == CANLOG_CLASSIC_TYPE:
            data_len = min(CLASSIC_MAX_LEN, payload_len, dlc)
            data = block[payload_offset : payload_offset + data_len].tobytes()
            yield CanLogFrame(
                frame_type="Classic",
                timestamp_us=timestamp,
                can_id=can_id,
                channel=channel,
                dlc=dlc,
                flags=flags,
                bus_id=bus_id,
                data=data,
                block_index=block_index,
                block_offset=offset,
            )
        elif entry_type == CANLOG_FD_TYPE:
            data_len = min(FDCAN_MAX_LEN, payload_len, dlc)
            data = block[payload_offset : payload_offset + data_len].tobytes()
            yield CanLogFrame(
                frame_type="FD",
                timestamp_us=timestamp,
                can_id=can_id,
                channel=channel,
                dlc=dlc,
                flags=flags,
                bus_id=bus_id,
                data=data,
                block_index=block_index,
                block_offset=offset,
            )
        else:
            # Unknown entry type; skip over the declared length to stay aligned.
            pass

        offset += total_len


def iter_can_frames(path: Path = DEFAULT_TRACE, abort_on_gap: bool = False) -> Iterator[CanLogFrame]:
    """
    Stream frames from a CAN trace without holding the entire file in memory.
    """
    block_index = 0
    buf = bytearray(max(BLOCK_HEADER_SIZE, 64 * 1024))
    prev_epoch = None
    prev_cnt = None
    with path.open("rb") as fh:
        while True:
            header = fh.read(BLOCK_HEADER_SIZE)
            if not header:
                break
            if len(header) < BLOCK_HEADER_SIZE:
                break  # Truncated header at EOF

            (
                _version,
                header_size,
                block_size,
                block_fill,
                epoch,
                block_cnt,
                _ingress_frames,
                _frame_count,
            ) = BLOCK_HEADER_STRUCT.unpack(header)
            if block_size == 0:
                break
            if block_size < header_size or block_size < BLOCK_HEADER_SIZE:
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
                    break

            if len(buf) < block_size:
                buf = bytearray(block_size)

            # Fill buffer with header + payload to avoid allocations per block.
            buf_view = memoryview(buf)[:block_size]
            buf_view[:BLOCK_HEADER_SIZE] = header

            remaining = block_size - BLOCK_HEADER_SIZE
            if remaining:
                read_n = fh.readinto(buf_view[BLOCK_HEADER_SIZE:])
                if read_n is None or read_n < remaining:
                    break  # Truncated block at EOF

            for frame in _iter_block_frames(buf_view, block_index):
                yield frame

            prev_epoch = epoch
            prev_cnt = block_cnt
            block_index += 1


def parse_can_trace(
    path: Path = DEFAULT_TRACE,
    abort_on_gap: bool = False,
    sort_by_ts: bool = False,
) -> List[CanLogFrame]:
    """
    Return a list of all frames in the trace file.

    Note: The trace file can be large; prefer iter_can_frames for streaming use.
    """
    frames = list(iter_can_frames(path, abort_on_gap=abort_on_gap))
    if sort_by_ts:
        frames.sort(key=lambda f: f.timestamp_us)
    return frames


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
        f"Flags=0x{frame.flags:02X} Bus={frame.bus_id} "
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
        default=DEFAULT_TRACE,
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
        default=DEFAULT_LOG_DIR,
        help=f"Directory containing CAN.LOG* parts (default: {DEFAULT_LOG_DIR})",
    )
    parser.add_argument(
        "--trace-out",
        type=Path,
        default=DEFAULT_TRACE_OUT,
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
        "--sort-by-ts",
        action="store_true",
        help="Sort frames by timestamp before output (requires collecting them).",
    )
    args = parser.parse_args(argv)

    if args.build_trace:
        count = concat_logs_to_trace(args.logs_dir, args.trace_out)
        print(f"Concatenated {count} log file(s) into {args.trace_out}")
        return

    head = 0 if args.count_only else args.head
    tail = 0 if args.count_only else max(0, args.tail)
    preview: List[CanLogFrame] = []
    tail_buf: Deque[CanLogFrame] = deque(maxlen=tail) if tail else deque()
    asc_writer = VectorAsciiWriter(args.write_asc, args.relative_ts) if args.write_asc else None

    max_time = 0
    if args.collect_all:
        frames = parse_can_trace(args.path, abort_on_gap=args.abort_on_gap, sort_by_ts=args.sort_by_ts)
        preview = frames[:head]
        if tail:
            tail_buf.extend(frames[-tail:])
        count = len(frames)
        if asc_writer:
            for frame in frames:
                asc_writer.write_frame(frame)
        max_time = frames[-1].timestamp_us
    else:
        count = 0
        for frame in iter_can_frames(args.path, abort_on_gap=args.abort_on_gap):
            if count < head:
                preview.append(frame)
            if tail:
                tail_buf.append(frame)
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

    print(f"\nParsed {count} frame(s) from {args.path}")

    if asc_writer:
        asc_writer.close()
        print(f"Wrote Vector ASCII log to {args.write_asc}")

    if args.collect_all:
        print(f"Collected {count} frame(s) into memory.")


if __name__ == "__main__":
    main()
