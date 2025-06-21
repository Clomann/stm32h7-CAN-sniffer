# rm -rf ./build
mkdir -p ./build

git -C libs/STM32_HAL/STM32H7 submodule update --recursive --init Drivers/BSP/STM32H7xx_Nucleo
git -C libs/STM32_HAL/STM32H7 submodule update --recursive --init Drivers/BSP/Components/lan8742
git -C libs/STM32_HAL/STM32H7 submodule update --recursive --init Drivers/CMSIS/Device/ST/STM32H7xx
git -C libs/STM32_HAL/STM32H7 submodule update --recursive --init Drivers/BSP/Components/Common

if [ -d build ]; then
    cmake --preset "Debug" -B ./build/
fi