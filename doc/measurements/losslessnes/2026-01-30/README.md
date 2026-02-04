# Losslessnes test 2026-01-30 

*tested on: v0.2.0-rc-6-ga5107ff*

Change from repo root to execute commands in this doc:

```sh
cd doc/measurements/losslessnes
```

All test result files from the SD card are stored here in a separate cloud and can be retrieved using:

```sh
curl -L -o ./2026-01-30/SD/logs_2026-01-30.tar.zst "https://f003.backblazeb2.com/file/STM32H7-CAN-sniffer/2026-01-30/logs_2026-01-30.tar.zst"
``` 

The file [rb1_usage.log](./2026-01-30/rb1_usage.log) shows the stats polled from the REST API (at the start and at the end of the continuous run). The start and end time of the relevant run were copied into [start_and_end_times_and_frames.csv](./2026-01-30/start_and_end_times_and_frames.csv):

```sh
>>> python3 compute_time_and_frame_deltas.py ./2026-01-30/start_and_end_times_and_frames.csv

from 2026-01-30T10:41:09+00:00 -> 2026-01-30T16:58:30+00:00
time_delta   = 6:17:21 (~22641 s| ~377 min| ~6 hrs)
frame_delta  = 679194310
fps          = 29998.423656
```

The value *fps* shows ~30000 frmes/s (~15000 frames/s per channel), which fits the stats recorded by CANoe and is expected for ~86 % bus load for classic CAN frames of length DLC=1 and standards IDs at 1 Mbit/s.

Followig commands were used to create the ASC file from the individual log files from the SD card:

```sh
>>> source dev/measurements/dirs.env
```

```sh
>>> python3 dev/measurements/log_parser.py --build-trace --logs-dir ./path/to/separated/logs --trace-out CAN-LOG.BIN
``` 

```sh
>>> python3 dev/measurements/log_parser.py $BIN_DIR --split-epochs --merge-scattered-epochs --write-asc $ASC_DIR

Enabling --continue-on-gap because --merge-scattered-epochs was requested.
Parsing data ...
Warning: block 155537 header frame_count=1374 parsed=1187 (cnt=145, ingress_frames=679231279)
000892020329us ID=0x00000008 Ch1 DLC=1 Flags=0x00 Classic [00]
000892020388us ID=0x00000001 Ch1 DLC=1 Flags=0x00 Classic [00]
000892020450us ID=0x00000005 Ch1 DLC=1 Flags=0x00 Classic [00]
000892020508us ID=0x0000000E Ch1 DLC=1 Flags=0x00 Classic [00]
000892020566us ID=0x00000006 Ch1 DLC=1 Flags=0x00 Classic [00]
000892020627us ID=0x00000003 Ch1 DLC=1 Flags=0x00 Classic [00]
000892020687us ID=0x00000007 Ch1 DLC=1 Flags=0x00 Classic [00]
000892020745us ID=0x00000009 Ch1 DLC=1 Flags=0x00 Classic [00]
000892020804us ID=0x00000000 Ch1 DLC=1 Flags=0x00 Classic [00]
000892020863us ID=0x00000002 Ch1 DLC=1 Flags=0x00 Classic [00]

--- tail ---
023642633558us ID=0x00000006 Ch2 DLC=1 Flags=0x00 Classic [91]
023642633615us ID=0x00000007 Ch2 DLC=1 Flags=0x00 Classic [91]
023642633673us ID=0x00000013 Ch2 DLC=1 Flags=0x00 Classic [91]
023642633730us ID=0x00000010 Ch2 DLC=1 Flags=0x00 Classic [91]
023642633788us ID=0x00000004 Ch2 DLC=1 Flags=0x00 Classic [91]
023642633845us ID=0x00000009 Ch2 DLC=1 Flags=0x00 Classic [91]
023642633903us ID=0x00000009 Ch2 DLC=1 Flags=0x00 Classic [92]
023642633961us ID=0x00000003 Ch2 DLC=1 Flags=0x00 Classic [92]
023642634019us ID=0x00000015 Ch2 DLC=1 Flags=0x00 Classic [92]
023642634077us ID=0x00000001 Ch2 DLC=1 Flags=0x00 Classic [92]

Parsed 679231266 frame(s) from doc/measurements/losslessnes/2026-01-30/trace/bin/CAN-LOG.BIN
Wrote Vector ASCII logs merged by epoch using base doc/measurements/losslessnes/2026-01-30/trace/asc
``` 

The SD card capture shows no frame drops:

```sh
>>> python3 dev/measurements/check_frame_sequence.py $ASC_DIR/*.asc --show-all  

No missing frames detected in doc/measurements/losslessnes/2026-01-30/trace/asc/epoch001.asc (checked 679231266 frames).
```

> **NOTE**: The cluster size was 32 KiB instead of 64 KiB (see ./2026-01-30/SD/CONF.TXT)
