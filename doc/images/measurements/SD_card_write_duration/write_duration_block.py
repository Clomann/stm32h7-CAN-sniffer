import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

file = f'write_duration_2_channel_19920_fps_per_channel.csv'
BYTES = 64 * 1024

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
    # Get current axis limits
    xlim = ax.get_xlim()
    ylim = ax.get_ylim()
    
    # X-axis arrow at the RIGHT end, at BOTTOM of plot (ylim[0], not 0!)
    ax.annotate('', xy=(xlim[1], ylim[0]), xytext=(xlim[0], ylim[0]),
                xycoords='data',
                arrowprops=dict(arrowstyle='->', lw=2, color='black'))
    
    # Y-axis arrow at the TOP, at LEFT of plot (xlim[0], not 0!)
    ax.annotate('', xy=(xlim[0], ylim[1]), xytext=(xlim[0], ylim[0]),
                xycoords='data',
                arrowprops=dict(arrowstyle='->', lw=2, color='black'))

df = pd.read_csv(file, 
                 sep=';', 
                 names=['t','dT'],
                 skiprows=2,
                 converters={'t': ParseCsvValue, 'dT': ParseCsvValue})

tt = df['t'] / 1000 / 1000
yy = df['dT']

sort_indices = np.argsort(tt)

tt = tt.iloc[sort_indices]  # Use .iloc for pandas Series
yy = yy.iloc[sort_indices]

# drop last entry (last entry is cause by persisting the log)
tt = tt[:-1]
yy = yy[:-1]

print("Median write duration", np.median(yy))
print(f"Median raw data throughput: {round(BYTES / np.median(yy),2)} Mbytes/s")

COARSE_CADENCE_LIMIT = 38000
FINE_CADENCE_LIMIT   = 34000

# compute circular forward differences, including wrap-around at the end
idx = yy[yy > COARSE_CADENCE_LIMIT].index
diffs = (np.roll(idx.values, -1) - idx.values) % len(yy)

# print("Index differences of coarse cadence for values > " + str(COARSE_CADENCE_LIMIT) + ": ", diffs)

# compute circular forward differences, including wrap-around at the end
idx = yy[yy < FINE_CADENCE_LIMIT].index
diffs = (np.roll(idx.values, -1) - idx.values) % len(yy)

# print("Index differences of fine cadence for values < " + str(FINE_CADENCE_LIMIT) + ": ", diffs)

grid_style = {
    'linestyle': ':', 
    'linewidth': 1, 
    'alpha': 0.7
}

fig = plt.figure(constrained_layout=True)
axs = fig.subplot_mosaic([['Left', 'TopRight'],['Left', 'BottomRight']],
                          gridspec_kw={'width_ratios':[2, 2]})

axs['Left'].set_title(f'Multi-block write duration ({round(BYTES)/1024} kB data)')
axs['Left'].grid(True, **grid_style)

x_min = min(tt)
x_max = max(tt)
y_min = min(yy[:-1]) - 500
y_max = max(yy) + 500

axs['Left'].plot(tt, yy)
axs['Left'].axis([x_min, x_max, y_min, y_max])

axs['Left'].annotate(f" Median raw rate: {round(BYTES / np.median(yy), 2)} Mbytes/s", 
                xy=(x_min, y_max - 250), xytext=(x_min, y_max - 250),
                xycoords='data')

# histograms 
BIN_DURATION_1 = 50

hist = yy.hist(bins=round((y_max - y_min) / BIN_DURATION_1), ax=axs['TopRight'])

BIN_DURATION_2 = 100

hist = yy.hist(bins=round((y_max - y_min) / BIN_DURATION_2), ax=axs['BottomRight'])

for el in axs:
    ConfigureAxis(axs[el])

axs['TopRight'].set_xlabel('dT in us')
axs['TopRight'].set_xlabel('dT in us (bin width: ' + str(BIN_DURATION_1) + ' us)')

axs['BottomRight'].set_xlabel('dT in us (bin width: ' + str(BIN_DURATION_2) + ' us)')
axs['BottomRight'].set_ylabel('samples')
axs['BottomRight'].axis([y_min, y_max, 0, 10])

plt.savefig(file + '.svg', format='svg', bbox_inches='tight')
plt.show()
