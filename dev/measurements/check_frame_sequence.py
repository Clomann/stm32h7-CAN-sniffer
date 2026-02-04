"""
Check 8-bit counter continuity in Vector ASCII (.asc) CAN logs.

The script parses the first data byte of every frame and verifies that the
counter increases by 1 (modulo 256) for each CAN ID/channel combination.
Missing frames are reported with timestamps and the missing counter values.
"""

from __future__ import annotations

import argparse
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Tuple


@dataclass
class Frame:
    """Minimal frame info extracted from the ASCII log."""

    timestamp: float
    channel: int
    can_id: int
    counter: int
    line_no: int


def parse_vector_ascii(path: Path) -> Iterable[Frame]:
    """
    Yield frames from a Vector ASCII (.asc) CAN log.

    Only lines that look like a frame are parsed; headers and comments are skipped.
    """
    with path.open(encoding="ascii", errors="ignore") as fh:
        for line_no, raw in enumerate(fh, start=1):
            parts = raw.strip().split()
            if len(parts) < 7:
                continue

            try:
                timestamp = float(parts[0])
                channel = int(parts[1])
                can_id = int(parts[2].rstrip("xX"), 16)
                dlc = int(parts[5])
            except ValueError:
                # Header or malformed line
                continue

            if dlc < 1:
                continue

            data_tokens = parts[6 : 6 + dlc]
            if not data_tokens:
                continue

            try:
                counter = int(data_tokens[0], 16)
            except ValueError:
                continue

            yield Frame(timestamp=timestamp, channel=channel, can_id=can_id, counter=counter, line_no=line_no)


@dataclass
class Gap:
    """Represents a missing-counter gap between two consecutive frames."""

    key: Tuple[int, int]  # (channel, can_id)
    missing_values: List[int]
    previous: Frame
    current: Frame

    @property
    def missing_count(self) -> int:
        return len(self.missing_values)


def find_gaps(frames: Iterable[Frame]) -> Tuple[int, List[Gap], Dict[Tuple[int, int], int]]:
    """
    Check all frames for missing counter values.

    Returns a tuple: (frame count, list of gaps, total missing count per (channel, can_id)).
    """
    last_seen: Dict[Tuple[int, int], Frame] = {}
    missing_by_key: Dict[Tuple[int, int], int] = defaultdict(int)
    gaps: List[Gap] = []
    frame_count = 0

    for frame in frames:
        frame_count += 1
        key = (frame.channel, frame.can_id)
        prev = last_seen.get(key)

        if prev is None:
            last_seen[key] = frame
            continue

        delta = (frame.counter - prev.counter) % 256

        if delta == 0:
            # Repeated counter; update baseline but do not report a gap.
            last_seen[key] = frame
            continue

        if delta != 1:
            missing_values = [(prev.counter + i + 1) % 256 for i in range(delta - 1)]
            gaps.append(Gap(key=key, missing_values=missing_values, previous=prev, current=frame))
            missing_by_key[key] += len(missing_values)

        last_seen[key] = frame

    return frame_count, gaps, missing_by_key


def format_gap(gap: Gap, preview_values: int = 16) -> str:
    """Pretty-print a gap with a compact list of missing counter values."""
    channel, can_id = gap.key
    missing = " ".join(f"{v:02X}" for v in gap.missing_values[:preview_values])
    if len(gap.missing_values) > preview_values:
        missing += f" ... (+{len(gap.missing_values) - preview_values} more)"

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
    args = parser.parse_args()

    frame_count, gaps, missing_by_key = find_gaps(parse_vector_ascii(args.path))

    if frame_count == 0:
        print(f"No frames found in {args.path}")
        return

    if not gaps:
        print(f"No missing frames detected in {args.path} (checked {frame_count} frames).")
        return

    print(f"Found {len(gaps)} gap(s) in {args.path} (checked {frame_count} frames):\n")

    to_show: List[Gap]
    if args.show_all:
        to_show = gaps
    elif args.max_gaps > 0:
        to_show = gaps[: args.max_gaps]
    else:
        to_show = []

    for gap in to_show:
        print(format_gap(gap, preview_values=args.preview))

    if not args.show_all and len(gaps) > len(to_show):
        remaining = len(gaps) - len(to_show)
        print(f"... skipped {remaining} additional gap(s). Use --show-all to print everything.")

    print("\nMissing frame counts by (channel, CAN ID):")
    for key, count in sorted(missing_by_key.items()):
        channel, can_id = key
        print(f"  Ch{channel} ID=0x{can_id:08X}: {count} missing frame(s)")


if __name__ == "__main__":
    main()
