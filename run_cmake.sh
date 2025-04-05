rm -rf ./build
mkdir -p ./build

if [ -d build ]; then
    cmake --preset "Debug" -B ./build/
fi