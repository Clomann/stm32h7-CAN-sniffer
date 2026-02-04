import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import argparse
import sys

# Parse command-line arguments
parser = argparse.ArgumentParser(
    description='Analyze and visualize multi-block write duration performance metrics.',
    formatter_class=argparse.RawDescriptionHelpFormatter,
    epilog='''
Examples:
  %(prog)s data.csv
  %(prog)s write_duration_2_channel_19920_fps_per_channel.csv --bytes 65536

Notes:
  - The CSV file should contain two columns: timestamp (t) and duration (dT)
  - Output includes statistics printed to console and an SVG plot
  - The plot shows write duration over time and distribution histograms
    ''')

parser.add_argument('file', 
                    help='Path to the CSV file containing write duration data')
parser.add_argument('--bytes', 
                    type=int, 
                    default=64 * 1024,
                    help='Number of bytes per flush (default: 65536 = 64 KiB)')

args = parser.parse_args()

file = args.file
BYTES = args.bytes


def ParseCsvValue(value):
    if pd.isna(value):
        return np.nan
    text = str(value).strip()
    if not text:
        return np.nan
    lower = text.lower()
    has_decimal = ('.' in text) or (',' in text)
    has_exponent = ('e' in lower) and not lower.startswith('0x')
    has_hex_letters = any(ch in lower for ch in 'abcdef')

    if lower.startswith('0x') or (has_hex_letters and not has_decimal and not has_exponent):
        return int(lower, 16)

    return float(text.replace(',', '.'))


def ConfigureAxis(ax):
    ax.set_xlabel('t in s')
    ax.set_ylabel('dT in us')
    ax.spines['top'].set_visible(False)
    ax.spines['right'].set_visible(False)

    xlim = ax.get_xlim()
    ylim = ax.get_ylim()

    ax.annotate('', xy=(xlim[1], ylim[0]), xytext=(xlim[0], ylim[0]),
                xycoords='data',
                arrowprops=dict(arrowstyle='->', lw=2, color='black'))

    ax.annotate('', xy=(xlim[0], ylim[1]), xytext=(xlim[0], ylim[0]),
                xycoords='data',
                arrowprops=dict(arrowstyle='->', lw=2, color='black'))


def pct(x: np.ndarray, q: float) -> float:
    """Percentile helper; ignores NaNs."""
    x = np.asarray(x)
    x = x[np.isfinite(x)]
    if x.size == 0:
        return np.nan
    return float(np.percentile(x, q))


# Read CSV file
try:
    df = pd.read_csv(
        file,
        sep=';',
        names=['t', 'dT'],
        skiprows=2,
        converters={'t': ParseCsvValue, 'dT': ParseCsvValue}
    )
except FileNotFoundError:
    print(f"Error: File '{file}' not found.", file=sys.stderr)
    sys.exit(1)
except Exception as e:
    print(f"Error reading file: {e}", file=sys.stderr)
    sys.exit(1)

t_us = df['t'].to_numpy(dtype=np.float64)
dT_us = df['dT'].to_numpy(dtype=np.float64)

mask = np.isfinite(t_us) & np.isfinite(dT_us)
t_us = t_us[mask]
dT_us = dT_us[mask]

# ---- Fix circular buffer dump order (head/tail meet) ----
dt = np.diff(t_us)
wrap_candidates = np.where(dt < 0)[0]
if wrap_candidates.size > 0:
    k = wrap_candidates[np.argmin(dt[wrap_candidates])]
    t_us = np.concatenate([t_us[k + 1:], t_us[:k + 1]])
    dT_us = np.concatenate([dT_us[k + 1:], dT_us[:k + 1]])
else:
    order = np.argsort(t_us)
    t_us = t_us[order]
    dT_us = dT_us[order]

# ---- Drop the last sample w.r.t. time (persist-log entry) ----
if len(t_us) > 1:
    dT_us = dT_us[:-2]
    t_us = t_us[:-2]

print(len(t_us))

tt = t_us / 1e6  # seconds
yy = dT_us       # microseconds

# ---- Derived metrics: cadence + end-to-end throughput ----
P_us_full = np.diff(t_us)
valid = P_us_full > 0
P_us = P_us_full[valid]

cadence_hz = 1e6 / P_us
e2e_MBps = (BYTES / (P_us * 1e-6)) / (1024 * 1024)

dT_aligned = dT_us[:-1]
util = dT_aligned[valid] / P_us_full[valid]

raw_MBps = (BYTES / (yy * 1e-6)) / (1024 * 1024)

# ---- Print key stats to console ----
print("=== f_write duration dT (us) ===")
print("p50  :", round(pct(yy, 50), 2))
print("p99  :", round(pct(yy, 99), 2))
print("p999 :", round(pct(yy, 99.9), 2))
print("max  :", round(np.nanmax(yy), 2))

print("\n=== Raw in-flush rate (MiB/s), computed from dT ===")
print("p50  :", round(pct(raw_MBps, 50), 3))
print("p99  :", round(pct(raw_MBps, 99), 3))
print("p999 :", round(pct(raw_MBps, 99.9), 3))
print("min  :", round(np.nanmin(raw_MBps), 3))

print("\n=== Flush cadence (Hz), computed from start timestamps ===")
print("p50  :", round(pct(cadence_hz, 50), 2))
print("p99  :", round(pct(cadence_hz, 99), 2))
print("p999 :", round(pct(cadence_hz, 99.9), 2))
print("min  :", round(np.nanmin(cadence_hz), 2))

print("\n=== End-to-end throughput (MiB/s), includes waiting (start-to-start) ===")
print("p50  :", round(pct(e2e_MBps, 50), 3))
print("p99  :", round(pct(e2e_MBps, 99), 3))
print("p999 :", round(pct(e2e_MBps, 99.9), 3))
print("min  :", round(np.nanmin(e2e_MBps), 3))

print("\n=== Writer utilization U = dT/P (unitless) ===")
print("p50  :", round(pct(util, 50), 3))
print("p99  :", round(pct(util, 99), 3))
print("p999 :", round(pct(util, 99.9), 3))
print("min  :", round(np.nanmin(util), 3))
print("max  :", round(np.nanmax(util), 3))

# ---- Plotting ----
grid_style = {'linestyle': ':', 'linewidth': 1, 'alpha': 0.7}

fig = plt.figure(constrained_layout=False)
axs = fig.subplot_mosaic([['Left', 'TopRight'], ['Left', 'BottomRight']],
                         gridspec_kw={'width_ratios': [2, 2]})

# Grid on ALL axes (same style)
for key in axs:
    axs[key].grid(True, **grid_style)

axs['Left'].set_title(f'Multi-block write duration ({BYTES/1024:.1f} kB data; {len(tt)} samples)')

x_min = float(np.nanmin(tt))
x_max = float(np.nanmax(tt))
y_min = float(np.nanmin(yy)) - 500
y_max = float(np.nanmax(yy)) + 500

axs['Left'].plot(tt, yy)
axs['Left'].axis([x_min, x_max, y_min, y_max])

# ---- Histograms ----
BIN_DURATION_1 = 50
BIN_DURATION_2 = 100

yy_series = pd.Series(yy)

# Same x-range on both histograms to align x axes
hist_bins_1 = max(1, round((y_max - y_min) / BIN_DURATION_1))
hist_bins_2 = max(1, round((y_max - y_min) / BIN_DURATION_2))

yy_series.hist(bins=hist_bins_1, range=(y_min, y_max), ax=axs['TopRight'])
yy_series.hist(bins=hist_bins_2, range=(y_min, y_max), ax=axs['BottomRight'])

# Force identical x-limits and identical ticks
axs['TopRight'].set_xlim(y_min, y_max)
axs['BottomRight'].set_xlim(y_min, y_max)
axs['BottomRight'].set_xticks(axs['TopRight'].get_xticks())

# Axis cosmetics (arrows)
for el in axs:
    ConfigureAxis(axs[el])

# Labels for histogram axes
axs['TopRight'].set_xlabel(f'dT in us')
axs['TopRight'].set_ylabel('samples')
fig.text(0.90, 0.85, f'bin width:\n{BIN_DURATION_1} us', ha='right', va='top', fontsize=8)
axs['BottomRight'].set_xlabel(f'dT in us')
axs['BottomRight'].set_ylabel('samples')
axs['BottomRight'].axis([y_min, y_max, 0, 10])
fig.text(0.90, 0.5, f'bin width:\n{BIN_DURATION_2} us', ha='right', va='top', fontsize=8)
fig.tight_layout()

# ---- Put stats BELOW the plot (line breaks, fully printed) ----
median_dT = pct(yy, 50)
p99_dT = pct(yy, 99)
p999_dT = pct(yy, 99.9)
max_dT = float(np.nanmax(yy))

median_raw = (BYTES / (median_dT * 1e-6)) / (1024 * 1024)  # MiB/s
median_cad = pct(cadence_hz, 50) if cadence_hz.size else np.nan
median_e2e = pct(e2e_MBps, 50) if e2e_MBps.size else np.nan

stats_line_1 = (
f"dT(us):\n\
  p50={median_dT:.0f}\n\
  p99={p99_dT:.0f}\n\
  p999={p999_dT:.0f}\n\
  max={max_dT:.0f}"
)
stats_line_2 = (
f"Raw(in-flush): p50={median_raw:.2f} MiB/s\n\
Cadence: p50={median_cad:.2f} Hz\n\
E2E: p50={median_e2e:.2f} MiB/s\n"
)

# Reserve space for 2 lines of stats text
plt.subplots_adjust(bottom=0.26)
fig.text(0.01, 0.02, stats_line_1, ha='left', va='bottom', fontsize=10)
fig.text(0.51, 0.02, stats_line_2, ha='left', va='bottom', fontsize=10)

output_file = file + '.svg'
plt.savefig(output_file, format='svg', bbox_inches='tight')
print(f"\nPlot saved to: {output_file}")
plt.show()