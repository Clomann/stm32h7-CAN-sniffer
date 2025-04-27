cmake_minimum_required(VERSION 3.16)
project(UnitTests C)

# Fetch Unity only if we are building tests
include(FetchContent)
FetchContent_Declare(
    Unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(Unity)

enable_testing()
set(CTEST_OUTPUT_ON_FAILURE ON)

# Build the test executable
add_executable(tests
    fdcan/test_fdcan.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../drivers/fdcan/fdcan_utils.c
)

# Link against main library and Unity
target_link_libraries(tests PRIVATE
    unity
)

# Include paths
target_include_directories(tests PRIVATE
    ${unity_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../drivers/fdcan/
)

# Add test to CTest
add_test(NAME tests COMMAND tests)
