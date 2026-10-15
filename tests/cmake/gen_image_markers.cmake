if(NOT DEFINED ELF OR NOT DEFINED OUT)
    message(FATAL_ERROR "gen_image_markers.cmake requires -DELF and -DOUT")
endif()

execute_process(
    COMMAND arm-none-eabi-nm -n ${ELF}
    OUTPUT_VARIABLE NM_OUT
    RESULT_VARIABLE NM_RC
)

if(NM_RC)
    message(FATAL_ERROR "arm-none-eabi-nm failed with code ${NM_RC}")
endif()

string(REGEX MATCH "([0-9A-Fa-f]+)[ \t]+[A-Za-z][ \t]+Reset_Handler" _match "${NM_OUT}")
if(NOT CMAKE_MATCH_1)
    message(FATAL_ERROR "Reset_Handler not found in ${ELF}")
endif()

file(WRITE ${OUT}
"#pragma once\n\n#define IMAGE_TEST_RESET_HANDLER_ADDR 0x${CMAKE_MATCH_1}\n")
