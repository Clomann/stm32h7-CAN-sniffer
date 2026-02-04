import matplotlib.pyplot as plt
import numpy as np
import pathlib

# Parameters
bus_rate = 1_000_000  # 1 Mbit/s
bus_load = 1.0
base_bytes_wo_data = 14  # bytes without the payload data[] (packed header)
available_bits = bus_rate * bus_load  # bits/sec

# DLC range
dlc = np.arange(0, 9)

# Frame size in bits (approximate: 47 base + 8*DLC + stuffing overhead)
frame_size_bits = 55 + 8 * dlc

# Frame rate per channel
frame_rate = available_bits / frame_size_bits

# Total frame rate (2 channels)
frame_rate_total = 2 * frame_rate

# Storage rate (2 channels, dynamic storage: 20 + DLC bytes per frame)
storage_rate_dynamic = 2 * frame_rate * (base_bytes_wo_data + dlc) / (1024 * 1024)  # MiB/s

# Storage rate (2 channels, fixed 8 byte payload)
storage_rate_fixed = 2 * frame_rate * (base_bytes_wo_data + 8) / (1024 * 1024)  # MiB/s

# Payload rate (2 channels)
payload_rate = 2 * frame_rate * dlc / (1024 * 1024)  # MiB/s

# SD card capacity
sd_capacity = 0.91  # MiB/s

# Create the plot with two y-axes
fig, ax1 = plt.subplots(figsize=(10, 6))
ax2 = ax1.twinx()

# Plot byte rates on left axis
line1, = ax1.plot(dlc, storage_rate_dynamic, 'o-', color='#e74c3c', linewidth=2.5, markersize=8, label='Storage dynamic (14+DLC)')
line2, = ax1.plot(dlc, storage_rate_fixed, 'x--', color='#e67e22', linewidth=2.5, markersize=8, label='Storage fixed (22)')
line3, = ax1.plot(dlc, payload_rate, 's-', color='#3498db', linewidth=2.5, markersize=8, label='Payload byte rate')

# SD capacity reference line
line4 = ax1.axhline(y=sd_capacity, color='#27ae60', linestyle='--', linewidth=2, label=f'SD capacity ({sd_capacity} MiB/s)')

# Plot frame rate on right axis
line5, = ax2.plot(dlc, frame_rate_total / 1000, '^-', color='#9b59b6', linewidth=2.5, markersize=8, label='Frame rate (2 ch)')

# Labels and title
ax1.set_xlabel('DLC (bytes)', fontsize=12)
ax1.set_ylabel('Byte Rate (MiB/s)', fontsize=12, color='black')
ax2.set_ylabel('Frame Rate (kFrames/s, 2 channels)', fontsize=12, color='#9b59b6')

ax1.set_title(f'CAN Logger Rates vs DLC\n2 channels @ {round(bus_load * 100)}% bus load, {round(bus_rate / 1000)} kbit/s, dynamic storage (14 + DLC bytes/frame)', 
             fontsize=12, fontweight='bold')

# Grid and ticks
ax1.set_xticks(dlc)
ax1.set_ylim(0, 1.25)
ax2.set_ylim(0, 40)
ax1.set_xlim(-0.2, 8.2)
ax1.grid(True, linestyle='--', alpha=0.7)

# Color the right y-axis
ax2.tick_params(axis='y', labelcolor='#9b59b6')

# Combined legend
lines = [line1, line2, line3, line4, line5]
labels = [l.get_label() for l in lines]
ax1.legend(lines, labels, loc='upper right', fontsize=9)

# Add annotations for key points
for i in [1, 8]:
    ax1.annotate(f'{storage_rate_dynamic[i]:.3f}', 
                xy=(i, storage_rate_dynamic[i]), 
                xytext=(i + 0.3, storage_rate_dynamic[i] + 0.03),
                fontsize=9, color='#e74c3c')
    ax1.annotate(f'{storage_rate_fixed[i]:.3f}', 
                xy=(i, storage_rate_fixed[i]), 
                xytext=(i + 0.3, storage_rate_fixed[i] + 0.03),
                fontsize=9, color='#e67e22')
    ax2.annotate(f'{frame_rate_total[i]/1000:.1f}k', 
                xy=(i, frame_rate_total[i]/1000), 
                xytext=(i - 0.7, frame_rate_total[i]/1000 + 1),
                fontsize=9, color='#9b59b6')

plt.tight_layout()
print(pathlib.Path(__file__))
plt.savefig(str(pathlib.Path(__file__)) + '.svg', dpi=150, bbox_inches='tight')
print("Chart saved successfully!")
