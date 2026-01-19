# This script computes the worst-case bandwidth needed for 
# logging classic CAN frames on the selected number of 
# CAN  channels for 100 % bus-load 

# pip install -y tabulate
from tabulate import tabulate

# baudrate in bit/s
baudrate = 1000000

# number of CAN channels
channel_no = 2

# max. bits per frame
max_bits_per_frame = 137

# bytes per frame in CAN log
bytes_per_frame = 28

# SPI clock freq. in Hz
spi_clock_freq = 80000096 / 4

# Hypothetical contin. measurement duration in seconds
measurement_duration = 3600

fac_kbytes = 1 / 8 / 1024
fac_Mbytes = 1 / 1024 * fac_kbytes

frames_per_second = channel_no * baudrate / max_bits_per_frame
frames_datarate = frames_per_second * (8 * bytes_per_frame)

spi_raw_max_throughout = spi_clock_freq * fac_Mbytes
sd_card_max_write_datarate = frames_datarate * fac_Mbytes

sd_card_write_overhead = 0.10
sd_card_max_write_w_overhead_datarate = (1 + sd_card_write_overhead) * sd_card_max_write_datarate

bandwidth_margin = spi_raw_max_throughout / sd_card_max_write_w_overhead_datarate

accumulateed_data = measurement_duration * sd_card_max_write_w_overhead_datarate

content = [
    [f"Arriving frames ({baudrate/1000} kbit/s; 100 % bus-load; {channel_no} channel):", f"{ round(frames_per_second,2)} frames/s"],
    [f"Maximum CAN frame datarate to log ({bytes_per_frame} bytes/frame):", f"{round(sd_card_max_write_datarate,2)} Mb/s"],
    [f"Maximum datarate to SD card (+ {sd_card_write_overhead * 100} % for CMDs/FAT32):", f"{round(sd_card_max_write_w_overhead_datarate,2)} Mb/s"],
    [f"Maximum raw SPI throughput  ({round(spi_clock_freq/1e6,2)} MHz):", f"{round(spi_raw_max_throughout,2)} Mb/s"],
    [f"Margin:", f"{round(bandwidth_margin,2)}"],
    [f"Accumulated data after {measurement_duration} seconds", f"{round(accumulateed_data,2)} Mb"]
]

print(tabulate(content))
