Checklist

- [ ] Replace every placeholder chart/screenshot in the README measurements and UI sections with real captures so reviewers see actual output (README.md (lines 55-88)).
    - [X] Capture Saleae/oscilloscope data for CAN ISR latency and export static image to replace Mermaid placeholder.
    - [X] Capture SD multi-block write timing plot (from measurement script) and embed exported image.
    - [X] Capture block flush window Saleae screenshot and embed.
    - [X] Take a screenshot of the web GUI (status + config view) and swap in for the UI placeholder.

- [ ] Flesh out “Run unit tests” and “Verify logging” with the exact commands, expected artifacts, and sample output/screenshot to prove the tooling works (README.md (lines 107-114)).
    - [ ] Write exact command sequence for configuring/building/running `ctest` for unit tests, including expected summary.
    - [ ] Describe the CANoe (or loopback) scenario used for “Verify logging,” including commands to replay and where to find the produced SD file.
    - [ ] Add screenshot or console capture showing successful test/log verification.

- [ ] Publish the host-side CRC/metadata verification script plus a captured run log, and link it from the lossless validation section so the proof is reproducible (README.md (lines 90-95)).
    - [ ] Add a script (e.g., `tools/verify_log_crc.py`) that streams SD log files, reports first/last sequence IDs, and computes CRC.
    - [ ] Run the script on an actual capture, save the console output (or log file), and link both script + output from README + arc42.
    - [ ] Describe how to collect the metadata file from the device and where to store it for comparisons.

- [ ] Add a short “how to replay/inspect logs” section or link for evaluators who want to check a sample capture without parsing 9 GB manually.
    - [ ] Prepare a trimmed sample log (few MB) and store it under `doc/tests/samples`.
    - [ ] Document the parser/inspection steps (CLI or script) and reference the sample log in README.

- [ ] Document the long-run test scenario (traffic mix, duration, environment) inside doc/tests or arc42 so the result isn’t just a README claim.
    - [ ] Use the new template in `doc/arc42-template-EN.md` to capture the test details.
    - [ ] Store CANoe scenario/config, metadata file, and host CRC output under `doc/tests/long-run/<date>`.
    - [ ] Link the specific long-run document from README (measurements section).

- [X] Include at least one photo of the assembled hard ware and wiring in the README hardware section for quick visual context.

- [ ] Verify that all diagrams referenced in arc42 (business/technical context, measurement blocks) render in the repo and regenerate any stale .svg files, especially the new lossless-proof flow.
    - [ ] Run the Mermaid conversion script for every `.md` under `doc/images` and check for errors.
    - [ ] Confirm that `measurement_lossless_proof.md.svg` exists and matches the latest diagram.
    - [ ] Update README/arc42 image links if paths change.

- [ ] Tag or describe a “recommended commit”/release in the repo so application reviewers can clone a known-good version without sifting through WIP branches.
    - [ ] Choose the commit after completing the above tasks, create a lightweight tag (e.g., `application-demo`), and push it.
    - [ ] Mention the tag (or GitHub release) in README’s introduction or project recap.
