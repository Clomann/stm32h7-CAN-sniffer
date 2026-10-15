#!/usr/bin/env python3
"""Plot trends from rb1_usage.log (timestamp + JSON payload CSV)."""

from __future__ import annotations

import argparse
import json
import sys
from collections import deque
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List, Tuple


def parse_timestamp(value: str) -> datetime:
    value = value.strip()
    if value.endswith("Z"):
        value = value[:-1] + "+00:00"
    return datetime.fromisoformat(value)


def coerce_float(value: Any) -> float | None:
    if isinstance(value, bool):
        return None
    if isinstance(value, (int, float)):
        return float(value)
    return None


def parse_payload(response_raw: str) -> Dict[str, Any]:
    response_raw = response_raw.strip()
    try:
        payload = json.loads(response_raw)
    except json.JSONDecodeError:
        # Handle CSV-escaped JSON strings if present.
        if response_raw.startswith('"') and response_raw.endswith('"'):
            response_raw = response_raw[1:-1].replace('""', '"')
            payload = json.loads(response_raw)
        else:
            raise

    if not isinstance(payload, dict):
        raise ValueError("Response payload is not a JSON object")
    return payload


def parse_rows(path: Path, skip_invalid: bool = False) -> Tuple[List[datetime], List[Dict[str, Any]]]:
    times: List[datetime] = []
    rows: List[Dict[str, Any]] = []
    skipped = 0

    with path.open() as handle:
        for line_number, line in enumerate(handle, start=1):
            line = line.strip()
            if not line:
                continue

            try:
                timestamp_raw, response_raw = line.split(",", 1)
            except ValueError:
                raise ValueError(f"Line {line_number}: expected timestamp,response") from None

            if line_number == 1 and timestamp_raw == "timestamp" and response_raw.strip() == "response":
                continue

            try:
                ts = parse_timestamp(timestamp_raw)
                payload = parse_payload(response_raw)
            except ValueError as exc:
                if not skip_invalid:
                    raise ValueError(f"Line {line_number}: {exc} (use --skip-invalid to ignore)") from exc
                skipped += 1
                continue
            times.append(ts)
            rows.append(payload)

    if skipped:
        print(f"Skipped {skipped} non-status line(s).", file=sys.stderr)
    return times, rows


def moving_average(values: List[float | None], window: int) -> List[float | None]:
    if window <= 1:
        return values

    output: List[float | None] = []
    buf: deque[float] = deque()
    running_sum = 0.0

    for value in values:
        if value is None:
            buf.clear()
            running_sum = 0.0
            output.append(None)
            continue

        buf.append(value)
        running_sum += value
        if len(buf) > window:
            running_sum -= buf.popleft()
        output.append(running_sum / len(buf))

    return output


def to_plot(values: List[float | None]) -> List[float]:
    return [v if v is not None else float("nan") for v in values]


def build_series(times: List[datetime], rows: List[Dict[str, Any]], key: str) -> List[float | None]:
    series: List[float | None] = []
    for payload in rows:
        series.append(coerce_float(payload.get(key)))
    return series


BUFFER_PCT_KEYS = [
    "rb1_bytes_highwater_pct",
    "can1_rx_highwater_pct",
    "can2_rx_highwater_pct",
    "can1_hw_rx_fifo_highwater_pct",
    "can2_hw_rx_fifo_highwater_pct",
    "fdcan_msg_port_highwater_pct",
]

RB1_BYTES_KEYS = [
    "rb1_bytes_now",
    "rb1_bytes_max_since_status",
]


def build_rb1_pct_series(rows: List[Dict[str, Any]], key: str) -> List[float | None]:
    series: List[float | None] = []
    for payload in rows:
        value = coerce_float(payload.get(key))
        capacity = coerce_float(payload.get("rb1_bytes_capacity"))
        if value is None or not capacity:
            series.append(None)
        else:
            series.append(100.0 * value / capacity)
    return series


BUFFER_LABELS = {
    "rb1_bytes_now": "rb1 now",
    "rb1_bytes_max_since_status": "rb1 max per interval",
    "can1_rxf0s": "CAN1 HW RX FIFO now",
    "can2_rxf0s": "CAN2 HW RX FIFO now",
    "rb1_bytes_highwater_pct": "rb1 ATH",
    "can1_rx_highwater_pct": "CAN1 RX buffer ATH",
    "can2_rx_highwater_pct": "CAN2 RX buffer ATH",
    "can1_hw_rx_fifo_highwater_pct": "CAN1 HW RX FIFO ATH",
    "can2_hw_rx_fifo_highwater_pct": "CAN2 HW RX FIFO ATH",
    "fdcan_msg_port_highwater_pct": "FDCAN msg port ATH",
}

HW_RX_FIFO_STATUS_KEYS = [
    "can1_rxf0s",
    "can2_rxf0s",
]


def build_rxf0s_fill_pct_series(rows: List[Dict[str, Any]], key: str, fifo_elements: int) -> List[float | None]:
    # FDCAN_RXF0S.F0FL (bits 6:0) is the FIFO 0 fill level at the time of the status request.
    series: List[float | None] = []
    for payload in rows:
        value = payload.get(key)
        if isinstance(value, bool) or not isinstance(value, int):
            series.append(None)
        else:
            series.append(100.0 * (value & 0x7F) / fifo_elements)
    return series


def metric_unit(key: str) -> str:
    if key.endswith("_us"):
        return "µs"
    if key.endswith("_pct"):
        return "%"
    if "_bytes" in key:
        return "bytes"
    if key.startswith("bus_load"):
        return "%"
    return ""


def discover_numeric_keys(rows: List[Dict[str, Any]]) -> List[str]:
    keys: set[str] = set()
    for payload in rows:
        for k, v in payload.items():
            if coerce_float(v) is not None:
                keys.add(k)
    return sorted(keys)


def main() -> None:
    parser = argparse.ArgumentParser(description="Plot rb1_usage.log trends.")
    parser.add_argument(
        "log",
        nargs="?",
        default="doc/measurements/losslessnes/2026-01-30/rb1_usage.log",
        help="Path to rb1_usage.log",
    )
    parser.add_argument(
        "--metric",
        default="rb1_bytes_highwater_pct",
        help="Metric key to plot (ignored with --all-numeric).",
    )
    parser.add_argument(
        "--all-numeric",
        action="store_true",
        help="Plot all numeric metrics in the payload.",
    )
    parser.add_argument(
        "--buffers",
        action="store_true",
        help="Plot utilization of all buffers in %% (rb1 now/max, HW RX FIFO fill from RXF0S, plus *_highwater_pct).",
    )
    parser.add_argument(
        "--hw-fifo-elements",
        type=int,
        default=64,
        help="FDCAN RX FIFO 0 depth used to scale RXF0S fill level (FDCAN_RAM_RX_ELEMENTS).",
    )
    parser.add_argument(
        "--smooth",
        type=int,
        default=1,
        help="Moving average window size in samples.",
    )
    parser.add_argument(
        "--skip-invalid",
        action="store_true",
        help="Skip lines that are not timestamp,{json} status rows (e.g. tagged api_* rows).",
    )
    parser.add_argument(
        "--out",
        default="",
        help="Optional output image path (PNG/SVG/etc).",
    )

    args = parser.parse_args()

    log_path = Path(args.log)
    if not log_path.exists():
        raise SystemExit(f"Log not found: {log_path}")

    times, rows = parse_rows(log_path, args.skip_invalid)
    if not times:
        raise SystemExit("No rows parsed from log.")

    try:
        import matplotlib.pyplot as plt
    except ModuleNotFoundError as exc:
        raise SystemExit("matplotlib is required to plot: python3 -m pip install matplotlib") from exc

    start = times[0]
    times = [(t - start).total_seconds() / 60.0 for t in times]

    plt.figure(figsize=(12, 6))
    plt.title(log_path.name)

    ylabel = "Value"
    if args.buffers:
        for key in RB1_BYTES_KEYS:
            series = moving_average(build_rb1_pct_series(rows, key), args.smooth)
            plt.plot(times, to_plot(series), label=BUFFER_LABELS[key], linewidth=1.2)
        for key in HW_RX_FIFO_STATUS_KEYS:
            series = build_rxf0s_fill_pct_series(rows, key, args.hw_fifo_elements)
            if all(v is None for v in series):
                continue
            series = moving_average(series, args.smooth)
            plt.plot(times, to_plot(series), label=BUFFER_LABELS[key], linewidth=1.2)
        for key in BUFFER_PCT_KEYS:
            series = build_series(times, rows, key)
            if all(v is None for v in series):
                continue
            plt.plot(times, to_plot(series), label=BUFFER_LABELS[key], linewidth=1.5, linestyle="--")
        plt.ylim(0, 100)
        plt.legend(loc="upper right", bbox_to_anchor=(1.0, 0.78), fontsize="small", ncol=2)
        ylabel = "Utilization [%]"
    elif args.all_numeric:
        keys = discover_numeric_keys(rows)
        if not keys:
            raise SystemExit("No numeric metrics found in payload.")

        for key in keys:
            series = build_series(times, rows, key)
            series = moving_average(series, args.smooth)
            plt.plot(times, to_plot(series), label=key, linewidth=1.2)
        plt.legend(loc="upper right", fontsize="small", ncol=2)
    else:
        series = build_series(times, rows, args.metric)
        if all(v is None for v in series):
            raise SystemExit(f"Metric not found or non-numeric: {args.metric}")
        smoothed = moving_average(series, args.smooth)
        plt.plot(times, to_plot(series), label=args.metric, linewidth=1.2, alpha=0.4)
        if args.smooth > 1:
            plt.plot(times, to_plot(smoothed), label=f"{args.metric} (ma{args.smooth})", linewidth=2)
        plt.legend(loc="upper right")
        unit = metric_unit(args.metric)
        ylabel = f"{args.metric} [{unit}]" if unit else args.metric

    plt.xlabel(f"Elapsed time [min] since {start.isoformat()}")
    plt.ylabel(ylabel)
    plt.grid(True, linestyle="--", alpha=0.4)
    plt.tight_layout()

    if args.out:
        plt.savefig(args.out, dpi=150)
    else:
        plt.show()


if __name__ == "__main__":
    main()
