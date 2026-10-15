# Contributing

## Tooling

### Documentation tooling

Mermaid diagrams live under `doc/images/*.md`.

To regenerate the `.svg` outputs:

1. Create/activate a virtual environment in `dev/scripts`:
    > python -m venv .venv && source .venv/bin/activate
2. Install the converter’s dependencies (inside `dev/scripts`):
    > pip install -r requirements.txt
3. Start Docker from within `dev/scripts`:
    > docker-compose -f docker-compose.yaml up -d
4. Run:
    > python convert_mermaid.py --root ../doc/images
    
    The script scans for `.md` sources and emits `.md.svg` files with the same name.

The docker-compose helper only provides a local Kroki + Mermaid renderer; the actual export still requires running `convert_mermaid.py` from inside `dev/scripts`.

> NOTE: Usage of the Mermaid CLI can be found here:
> https://doctoolchain.org/docToolchain/v2.0.x/020_tutorial/170_kroki-configuration.html

### Formatting tooling

To create the file list passed to clang-format, run the following command from the project root:

```sh
find adapters app bootloader dev middleware platform tests -path 'dev/scripts/env' -prune -o -path '*/_deps' -prune -o -path '*/build' -prune -o -path '*/CMakeFiles' -prune -o -type f \( -name '*.c' -o -name '*.h' \) -print | awk 'NR==1{printf "%s",$0; next} {printf "\n%s",$0}' > tools/clang/clang_files.txt
```

After that, you can apply formatting using:

```sh
clang-format -i --files=tools/clang/clang_files.txt
```

### GitHub actions

CI runs in an Ubuntu container and uses a specific `clang-format` version.
To apply the same formatting to the code base, run this Docker command:

```sh
docker run --rm -v "$PWD:/repo" -w /repo catthehacker/ubuntu:act-latest \
  bash -lc '
    set -o pipefail &&
    sudo apt-get update >/dev/null &&
    sudo apt-get install -y clang-format-18 >/dev/null &&
    find . -type f \( -name "*.c" -o -name "*.h" \) -print \
      | sed "s#^\./##" \
      | grep -vFf tools/clang/format_exclude.txt \
      | LC_ALL=C sort \
      | awk '\''NR==1 { printf "%s", $0; next } { printf "\n%s", $0 }'\'' \
      > tools/clang/clang_files.txt &&
    clang-format-18 -style=file -i --files=./tools/clang/clang_files.txt
  '
```

To pre-check the GitHub action locally, run:

```sh
act -v -W .github/workflows/unity-tests.yml -j build-and-test \
  --env-file /dev/null \
  -P ubuntu-24.04=catthehacker/ubuntu:act-latest
```

You can also pre-build an image and use it for a local container:

```sh
docker compose -f dev/scripts/docker-compose.act-unity.yaml build
```

and then run act using this image:

```sh
act -W .github/workflows/unity-tests.yml -j build-and-test \
  -P ubuntu-24.04=local/act-unity:ubuntu-24.04 \
  --pull=false --action-offline-mode --env-file /dev/null
```

### Build presets

All configure presets in `CMakePresets.json` use the build directory `build/<preset>` and write binaries to `_bin/<build type>/`. Each configure preset has a matching build preset and a workflow preset (`cmake --workflow --preset <workflow>`).

| Configure preset | Workflow | Target | Notes |
|-|-|-|-|
| `Debug` | `debug-all` | CM7 app, single-core | |
| `Debug_CM7` | `debug-cm7` | CM7 app | |
| `Debug_CM7_Boot` | `debug-cm7-boot` | CM7 bootloader | |
| `Debug_CM7_Both` | `debug-cm7-both` | CM7 bootloader + app | |
| `Debug_CM4` | `debug-cm4` | CM4 app | |
| `Release_CM7` | `release-cm7` | CM7 app | image encryption, GCC static analyzer |
| `Release_CM7_Boot` | `release-cm7-boot` | CM7 bootloader | image encryption, GCC static analyzer |
| `Release_CM7_Both` | `release-cm7-both` | CM7 bootloader + app | image encryption, GCC static analyzer |
| `Release_CM4` | `release-cm4` | CM4 app | GCC static analyzer |
| `Release_CM7_Test` | – | CM7 app | `Release_CM7` + CAN ID check |
| `Pre_Release_CM7` | `pre-release-test` | CM7 app | soak test build, see below |

`Pre_Release_CM7` inherits `Release_CM7` and is used for the soak test before merging into `main`:

- the GCC static analyzer is disabled
- both FDCAN channels replay static frames in external loopback mode (`CAN_STATIC_TX_REPLAY_*`), so no external CAN bus or CANoe is needed
- the CAN ID check is enabled (`DEBUG_CHECK_CAN_FRAME_ID_SEQUENCE=1U`). With static replay it only checks that each ID is in the replay ID range; frame loss is detected by the sequence and drop counters
- file handler tracing is enabled (`COMPONENT_TRACE=COMPONENT_TRACE_FILEHANDLER`)

The build options of a running device are reported by `/api/version`.

### Flashing and debugging in VS Code

`.vscode/tasks.json` provides tasks to flash the images via `STM32_Programmer_CLI` and to start OpenOCD:

| Task | Description |
|-|-|
| `Flash CM7 Boot`, `Flash CM7 App`, `Flash CM4` | flash a single debug image |
| `Flash CM7 Boot & App`, `Flash CM7 & CM4` | flash several debug images in sequence |
| `Flash CM7 & CM4 and start OpenOCD` | flash all debug images, then start OpenOCD |
| `... (Release)` | same for the images in `_bin/Release/` |
| `Start OpenOCD` | start OpenOCD in the background (ST-LINK, dual-bank H7) |
| `cppcheck` | run cppcheck on `build/compile_commands.json` |

| Image | Address |
|-|-|
| `SPI_FullDuplex_ComDMA_CM7_boot.elf` | `0x08000000` |
| `SPI_FullDuplex_ComDMA_CM7_app-signed.bin` | `0x08020000` |
| `SPI_FullDuplex_ComDMA_CM4_app.elf` | `0x08100000` |

`.vscode/launch.json` contains attach configurations for the target (`Debug/Release CM7 (Attach)`, `Debug/Release CM4 (Attach)`, using cortex-debug with an external GDB server). Start OpenOCD first, e.g. with the task `Flash CM7 & CM4 and start OpenOCD`, then attach. The CM7 configurations load the symbols of both the bootloader and the application.

The LLDB configurations (`Debug fdcan`, `Debug app`, `Debug bootloader`) debug the host unit test binaries (see [Testing](#testing)).

### Soak test

Before merging into `main`, run the `Pre_Release_CM7` build for at least 6 h without frame loss and record the device stats:

```sh
cmake --workflow --preset pre-release-test
# flash the images, start logging via the web GUI, then:
tools/poll_rb1_usage.sh -u http://can-sniffer.local/logger/status
```

The script polls the logger status every 10 s into `logs/<timestamp>_rb1_usage.log` and records `/api/version`, `/api/card`, `/logger/debug` and `/logger/errors` at the start and when it is stopped (Ctrl+C). Use `-t SECONDS` to stop after a fixed duration and `--stop-on-frames-lost` to stop on the first lost frame (see `tools/poll_rb1_usage.sh -h`). Plot the results with:

```sh
python3 tools/plot_rb1_usage.py logs/<timestamp>_rb1_usage.log --skip-invalid --buffers --smooth 30 --out buffers_utilization.svg
```

Store the results under `doc/measurements/losslessnes/<date>/` (see [2026-10-14](doc/measurements/losslessnes/2026-10-14/README.md) as an example).

### Sign and encrypt image using MCUboot

Use `imgtool.py` from the vendored MCUboot repository.

Install imgtool dependencies once:

```sh
python3 -m pip install -r ./libs/mcuboot/scripts/requirements.txt
```

Keep key material in `.keys/` (local only, not committed). Two key pairs are needed:

1. Signing key (ECDSA P-256, image signature).
2. Encryption key (ECIES P-256, image payload encryption/decryption).

Generate keys:

```sh
mkdir -p .keys

# Signing key (use your existing key if already created)
python3 ./libs/mcuboot/scripts/imgtool.py keygen -k .keys/sign-ecdsa-p256.priv.pem -t ecdsa-p256

# Encryption key (the "other" key for MCUBOOT_ENCRYPT_EC256)
python3 ./libs/mcuboot/scripts/imgtool.py keygen -k .keys/enc-ecies-p256.priv.pem -t ecdsa-p256
```

Derive the encryption public key (PEM) from the encryption private key, then
use that public key for image encryption:

```sh
python3 ./libs/mcuboot/scripts/imgtool.py getpub \
  -k .keys/enc-ecies-p256.priv.pem \
  -e pem \
  -o ./.keys/enc-ecies-p256.pub.pem
```

Configure build files with/without encryption:

```sh
# Encryption OFF (default)
cmake --preset Debug_CM7_Boot -DMCUBOOT_ENABLE_ENCRYPTION=OFF

# Encryption ON
cmake --preset Release_CM7_Boot -DMCUBOOT_ENABLE_ENCRYPTION=ON
```

Build commands (from project root):

```sh
# Application image (required by bootloader unit tests)
cmake --preset Release_CM7 -B ./build/application
cmake --build ./build/application -j

# Bootloader image
cmake --preset Release_CM7_Boot -B ./build/bootloader
cmake --build ./build/bootloader -j
```

During CMake configure, key C sources are generated automatically:

- `build/<preset>/generated/keys/autogen_sign_pub.c` from `MCUBOOT_SIGN_KEY_PEM`
- `build/<preset>/generated/keys/autogen_enc_priv.c` from `MCUBOOT_ENC_KEY_PEM` (only when `MCUBOOT_ENABLE_ENCRYPTION=ON`)

Manual generation (for troubleshooting only):

```sh
mkdir -p ./build/Release_CM7_Boot/generated/keys

python3 ./libs/mcuboot/scripts/imgtool.py getpub \
  -k .keys/sign-ecdsa-p256.priv.pem \
  -e lang-c \
  -o ./build/Release_CM7_Boot/generated/keys/autogen_sign_pub.c

# getpriv writes to stdout (no -o option)
python3 ./libs/mcuboot/scripts/imgtool.py getpriv \
  -k .keys/enc-ecies-p256.priv.pem \
  -f pkcs8 \
  > ./build/Release_CM7_Boot/generated/keys/autogen_enc_priv.c
```

Sign and encrypt app image:

```sh
python3 ./libs/mcuboot/scripts/imgtool.py sign \
  -k .keys/sign-ecdsa-p256.priv.pem \
  -E ./.keys/enc-ecies-p256.pub.pem \
  --header-size 0x400 \
  --align 4 \
  --slot-size 0x60000 \
  --overwrite-only \
  --version 0.4.0 \
  ./_bin/Release/SPI_FullDuplex_ComDMA_CM7_app.bin \
  ./_bin/Release/SPI_FullDuplex_ComDMA_CM7_app-signed-encrypted.bin
```

### Testing

```sh
# Prerequisite: application ELF used for generating test images
cmake --preset Release_CM7 -B ./build/application
cmake --build ./build/application -j

# Unit tests with encryption OFF
preset=tests-default
cmake -S tests --preset "$preset"
cmake --build "tests/build/$preset" -j
ctest --test-dir "tests/build/$preset" --output-on-failure

# Unit tests with encryption ON
preset=tests-encryption
cmake -S tests --preset "$preset"
cmake --build "tests/build/$preset" -j
ctest --test-dir "tests/build/$preset" --output-on-failure
```

The workflow presets `tests-default-workflow` and `tests-encryption-workflow` configure and build the tests in one step (with a clean build). They have no test step, so run `ctest` afterwards:

```sh
(cd tests && cmake --workflow --preset tests-default-workflow)
ctest --test-dir tests/build/tests-default --output-on-failure

(cd tests && cmake --workflow --preset tests-encryption-workflow)
ctest --test-dir tests/build/tests-encryption --output-on-failure
```
