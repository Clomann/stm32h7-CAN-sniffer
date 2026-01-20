| Field | Notes |
| - | - |
| Date / firmware commit | e.g., `2024-03-21 / 1a2b3c4` |
| Test duration | Target 3–6 h, note actual time (`t1 - t0`) |
| Traffic profile | CANoe scenario name, per-channel bitrate/load, frame payload (sequence ID location) |
| Environment | Board rev, SD card model/capacity, ambient temp if relevant |
| Frames sent (CANoe) | `first_seq`, `last_seq`, total frames |
| Frames logged (metadata) | same fields + `drop_count` |
| Host CRC result | command + checksum over SD logs |
| Observations | any anomalies (write warnings, thermal throttling, etc.) |

Store the populated table (and links to the CANoe report, metadata file, and CRC log) under `doc/tests/long-run/<date>` so reviewers can confirm the README claim.