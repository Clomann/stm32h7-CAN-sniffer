"""
Cross-reference two ASC CAN logs to find frames present in the ground-truth
(CANoe/transmitter) trace but absent in the logger (receiver) trace.

Both files must use sequential CAN IDs as frame counters (DLC=0, ID cycles
0x00-0xFF per channel).  CANoe is assumed gap-free; any gap in the logger
is a real capture loss.

Usage:
    python3 compare_traces.py <canoe.asc> <logger.asc> [options]

Output:
    - Total lost frames per channel
    - Burst list (consecutive lost IDs) with CANoe timestamps
    - Loss histogram bucketed by CANoe time
"""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, Iterator, List, Tuple


# ---------------------------------------------------------------------------
# ASC parser (handles both CANoe verbose format and compact logger format)
# ---------------------------------------------------------------------------

@dataclass(slots=True)
class Frame:
    timestamp: float
    channel: int
    can_id: int
    line_no: int


def parse_asc(path: Path) -> Iterator[Frame]:
    with path.open(encoding="ascii", errors="ignore") as fh:
        for line_no, raw in enumerate(fh, start=1):
            parts = raw.split()
            if len(parts) < 6:
                continue
            try:
                timestamp = float(parts[0])
                channel = int(parts[1])
                can_id = int(parts[2].rstrip("xX"), 16)
                # parts[3]=direction, parts[4]="d", parts[5]=DLC — we only need ID
            except ValueError:
                continue
            yield Frame(timestamp=timestamp, channel=channel, can_id=can_id, line_no=line_no)


# ---------------------------------------------------------------------------
# Build absolute-position sequence per channel
# abs_pos = cycle * modulo + can_id  (cycle increments on ID wrap-around)
# ---------------------------------------------------------------------------

def extract_channel(
    frames: Iterable[Frame], channel: int, modulo: int
) -> List[Tuple[int, float]]:
    """Return list of (abs_pos, timestamp) in arrival order for one channel."""
    result: List[Tuple[int, float]] = []
    prev_id: int | None = None
    cycle = 0
    for f in frames:
        if f.channel != channel:
            continue
        if prev_id is not None and f.can_id < prev_id:
            cycle += 1
        result.append((cycle * modulo + f.can_id, f.timestamp))
        prev_id = f.can_id
    return result


# ---------------------------------------------------------------------------
# Merge-join: find positions present in canoe but absent in logger
# Both lists are sorted by abs_pos (they arrive in ID order).
# ---------------------------------------------------------------------------

@dataclass(slots=True)
class LostBurst:
    abs_start: int
    abs_end: int          # inclusive
    canoe_ts_start: float
    canoe_ts_end: float
    count: int


def find_lost(
    canoe: List[Tuple[int, float]],
    logger: List[Tuple[int, float]],
) -> List[LostBurst]:
    """Merge-join canoe and logger abs_pos lists; return lost-frame bursts."""
    bursts: List[LostBurst] = []
    current_burst: LostBurst | None = None

    ci = 0
    li = 0
    cn = len(canoe)
    ln = len(logger)

    while ci < cn:
        abs_c, ts_c = canoe[ci]
        if li < ln and logger[li][0] < abs_c:
            # logger has a frame CANoe didn't see — spurious, skip
            li += 1
            continue

        if li < ln and logger[li][0] == abs_c:
            # frame present in both — flush any open burst
            if current_burst is not None:
                bursts.append(current_burst)
                current_burst = None
            ci += 1
            li += 1
        else:
            # abs_c not in logger — lost frame
            if current_burst is not None and abs_c == current_burst.abs_end + 1:
                current_burst.abs_end = abs_c
                current_burst.canoe_ts_end = ts_c
                current_burst.count += 1
            else:
                if current_burst is not None:
                    bursts.append(current_burst)
                current_burst = LostBurst(
                    abs_start=abs_c, abs_end=abs_c,
                    canoe_ts_start=ts_c, canoe_ts_end=ts_c,
                    count=1,
                )
            ci += 1

    if current_burst is not None:
        bursts.append(current_burst)

    return bursts


# ---------------------------------------------------------------------------
# Histogram: loss count bucketed by CANoe timestamp
# ---------------------------------------------------------------------------

def build_histogram(
    bursts: List[LostBurst], bucket_s: float, total_duration: float
) -> Dict[int, int]:
    hist: Dict[int, int] = {}
    for b in bursts:
        bucket = int(b.canoe_ts_start / bucket_s)
        hist[bucket] = hist.get(bucket, 0) + b.count
    return hist


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("canoe", type=Path, help="Ground-truth ASC (CANoe / transmitter)")
    ap.add_argument("logger", type=Path, help="Logger ASC (receiver under test)")
    ap.add_argument("--channels", type=int, nargs="+", default=[1, 2], metavar="N",
                    help="Channels to compare (default: 1 2)")
    ap.add_argument("--modulo", type=lambda s: int(s, 0), default=256,
                    help="ID wrap-around modulus (default: 256)")
    ap.add_argument("--max-bursts", type=int, default=30,
                    help="Max burst entries to print per channel (default: 30, 0=all)")
    ap.add_argument("--bucket", type=float, default=1.0,
                    help="Histogram bucket width in seconds (default: 1.0)")
    args = ap.parse_args()

    print(f"Parsing CANoe trace:  {args.canoe}")
    canoe_frames = list(parse_asc(args.canoe))
    print(f"  {len(canoe_frames):,} frames parsed")

    print(f"Parsing logger trace: {args.logger}")
    logger_frames = list(parse_asc(args.logger))
    print(f"  {len(logger_frames):,} frames parsed\n")

    canoe_duration = canoe_frames[-1].timestamp - canoe_frames[0].timestamp if canoe_frames else 0.0

    for ch in args.channels:
        print(f"{'='*60}")
        print(f"Channel {ch}")
        print(f"{'='*60}")

        canoe_ch = extract_channel(iter(canoe_frames), ch, args.modulo)
        logger_ch = extract_channel(iter(logger_frames), ch, args.modulo)

        canoe_n = len(canoe_ch)
        logger_n = len(logger_ch)
        print(f"  CANoe frames:  {canoe_n:>10,}")
        print(f"  Logger frames: {logger_n:>10,}")
        print(f"  Difference:    {canoe_n - logger_n:>+10,}")

        if canoe_n == 0:
            print("  No CANoe frames — skipping.\n")
            continue

        bursts = find_lost(canoe_ch, logger_ch)
        total_lost = sum(b.count for b in bursts)
        print(f"  Lost frames:   {total_lost:>10,}  ({total_lost/canoe_n*100:.2f}%)")
        print(f"  Loss bursts:   {len(bursts):>10,}\n")

        if not bursts:
            print("  No losses.\n")
            continue

        # Print burst table
        show = bursts if args.max_bursts == 0 else bursts[:args.max_bursts]
        skipped = len(bursts) - len(show)
        print(f"  {'Start-ID':>10}  {'End-ID':>8}  {'Count':>6}  {'CANoe start':>13}  {'CANoe end':>13}")
        print(f"  {'-'*10}  {'-'*8}  {'-'*6}  {'-'*13}  {'-'*13}")
        for b in show:
            sid = b.abs_start % args.modulo
            eid = b.abs_end % args.modulo
            print(f"  0x{sid:02X} (abs {b.abs_start:7d})  "
                  f"0x{eid:02X} (abs {b.abs_end:7d})  "
                  f"{b.count:6d}  "
                  f"{b.canoe_ts_start:13.6f}s  "
                  f"{b.canoe_ts_end:13.6f}s")
        if skipped:
            print(f"  ... ({skipped} more bursts — use --max-bursts 0 to show all)")

        # Histogram
        print(f"\n  Loss histogram (bucket = {args.bucket}s):")
        hist = build_histogram(bursts, args.bucket, canoe_duration)
        if hist:
            max_count = max(hist.values())
            bar_width = 40
            print(f"  {'Time (s)':>12}  {'Lost':>8}  {'Bar'}")
            for bucket in sorted(hist):
                t_start = bucket * args.bucket
                count = hist[bucket]
                bar = "#" * int(count / max_count * bar_width)
                print(f"  {t_start:>8.1f}s    {count:>8,}  {bar}")
        print()


if __name__ == "__main__":
    main()
