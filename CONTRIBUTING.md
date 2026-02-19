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

The docker-compose helper only provides a local Kroki + Mermaid renderer; the actual export still requires running `convert_mermaid.py` from inside `doc/scripts`.

> NOTE: Usage of the Mermaid CLI can be found here:
> https://doctoolchain.org/docToolchain/v2.0.x/020_tutorial/170_kroki-configuration.html

### Formatting tooling

To create the file list passed to clang-format, run the following command from the project root:

```sh
find adapters app dev middleware platform tests -path 'dev/scripts/env' -prune -o -path '*/_deps' -prune -o -path '*/build' -prune -o -path '*/CMakeFiles' -prune -o -type f \( -name '*.c' -o -name '*.h' \) -print | awk 'NR==1{printf "%s",$0; next} {printf "\n%s",$0}' > tools/clang/clang_files.txt
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
    find adapters app dev middleware platform tests \
      -path "dev/scripts/env" -prune -o \
      -path "*/_deps" -prune -o \
      -path "*/build" -prune -o \
      -path "*/CMakeFiles" -prune -o \
      -type f \( -name "*.c" -o -name "*.h" \) -print \
      | awk '"'"'NR==1{printf "%s",$0; next} {printf "\n%s",$0}'"'"' \
      > tools/clang/clang_files.txt &&
    clang-format-18 -style=file -i --files=./tools/clang/clang_files.txt
  '
```

To pre-check the GitHub action locally, run:

```sh
act -v -W .github/workflows/unity-tests.yml -j build-and-test
```
