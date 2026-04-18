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
    sudo apt-get update >/dev/null &&
    sudo apt-get install -y clang-format-18 >/dev/null &&
    find adapters app bootloader dev middleware platform tests \
      -path "dev/scripts/env" -prune -o \
      -path "*/_deps" -prune -o \
      -path "*/build" -prune -o \
      -path "*/CMakeFiles" -prune -o \
      -type f \( -name "*.c" -o -name "*.h" \) -print \
      | awk '"'"'NR==1{printf "%s",$0; next} {printf "\n%s",$0}'"'"' \
      > tools/clang/clang_files.txt &&
    clang-format-18 -style=file -i --files=./tools/clang/clang_files.txt'
```

To pre-check the GitHub action locally, run:

```sh
act -v -W .github/workflows/unity-tests.yml -j build-and-test
```

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
