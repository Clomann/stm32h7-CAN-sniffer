include(CMakeParseArguments)

# Optional global override for all MCUboot signing calls.
set(MCUBOOT_PYTHON ""
    CACHE FILEPATH
    "Python interpreter used for MCUboot imgtool signing"
)

function(mcuboot_add_imgtool_sign_command)
    set(options REQUIRE_CLICK OVERWRITE_ONLY PAD_HEADER)
    set(oneValueArgs
        TARGET
        OUTPUT
        INPUT
        PYTHON
        IMGTOOL
        HEADER_SIZE
        ALIGN
        SLOT_SIZE
        VERSION
        KEY
        ENCRYPT_PUBKEY
        COMMENT
    )
    set(multiValueArgs DEPENDS)
    cmake_parse_arguments(MCBS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT MCBS_INPUT OR NOT MCBS_OUTPUT OR NOT MCBS_IMGTOOL)
        message(FATAL_ERROR
            "mcuboot_add_imgtool_sign_command requires INPUT, OUTPUT and IMGTOOL."
        )
    endif()

    if(MCBS_TARGET AND MCBS_DEPENDS)
        message(FATAL_ERROR
            "mcuboot_add_imgtool_sign_command: DEPENDS cannot be used with TARGET mode."
        )
    endif()

    if(MCBS_TARGET AND NOT TARGET ${MCBS_TARGET})
        message(FATAL_ERROR
            "mcuboot_add_imgtool_sign_command: TARGET '${MCBS_TARGET}' does not exist."
        )
    endif()

    set(_python "${MCBS_PYTHON}")
    if(NOT _python AND MCUBOOT_PYTHON)
        set(_python "${MCUBOOT_PYTHON}")
    endif()

    # Prefer active virtualenv when available.
    if(NOT _python AND DEFINED ENV{VIRTUAL_ENV})
        if(EXISTS "$ENV{VIRTUAL_ENV}/bin/python3")
            set(_python "$ENV{VIRTUAL_ENV}/bin/python3")
        elseif(EXISTS "$ENV{VIRTUAL_ENV}/bin/python")
            set(_python "$ENV{VIRTUAL_ENV}/bin/python")
        endif()
    endif()

    if(NOT _python)
        set(Python3_FIND_VIRTUALENV FIRST)
        find_package(Python3 QUIET COMPONENTS Interpreter)
        if(Python3_Interpreter_FOUND)
            set(_python "${Python3_EXECUTABLE}")
        endif()
    endif()

    if(NOT _python)
        unset(_mcbs_python CACHE)
        find_program(_mcbs_python NAMES python3 python py
            HINTS /usr/local/bin /opt/homebrew/bin /usr/bin
        )
        if(_mcbs_python)
            set(_python "${_mcbs_python}")
        endif()
    endif()
    if(NOT _python)
        message(FATAL_ERROR
            "Python interpreter not found (required for MCUboot imgtool signing)."
        )
    endif()

    if(MCBS_REQUIRE_CLICK)
        execute_process(
            COMMAND ${_python} -c "import click"
            RESULT_VARIABLE _click_rc
            OUTPUT_QUIET
            ERROR_QUIET
        )
        if(NOT _click_rc EQUAL 0)
            message(FATAL_ERROR
                "Python module 'click' is required for imgtool signing. "
                "Install it for ${_python} (for example: ${_python} -m pip install click)."
            )
        endif()
    endif()

    set(_cmd
        ${_python}
        ${MCBS_IMGTOOL}
        sign
        --header-size ${MCBS_HEADER_SIZE}
        --align ${MCBS_ALIGN}
        --slot-size ${MCBS_SLOT_SIZE}
    )

    if(MCBS_OVERWRITE_ONLY)
        list(APPEND _cmd --overwrite-only)
    endif()
    if(MCBS_PAD_HEADER)
        list(APPEND _cmd --pad-header)
    endif()
    if(MCBS_KEY)
        list(APPEND _cmd -k ${MCBS_KEY})
    endif()
    if(MCBS_ENCRYPT_PUBKEY)
        list(APPEND _cmd -E ${MCBS_ENCRYPT_PUBKEY})
    endif()

    list(APPEND _cmd
        --version ${MCBS_VERSION}
        ${MCBS_INPUT}
        ${MCBS_OUTPUT}
    )

    if(MCBS_COMMENT)
        set(_comment "${MCBS_COMMENT}")
    else()
        set(_comment "Signing image ${MCBS_INPUT}")
    endif()

    if(MCBS_TARGET)
        add_custom_command(
            TARGET ${MCBS_TARGET}
            POST_BUILD
            COMMAND ${_cmd}
            COMMENT "${_comment}"
            VERBATIM
        )
    else()
        set(_deps ${MCBS_DEPENDS})
        if(NOT _deps)
            set(_deps ${MCBS_INPUT} ${MCBS_IMGTOOL})
        endif()

        add_custom_command(
            OUTPUT ${MCBS_OUTPUT}
            COMMAND ${_cmd}
            DEPENDS ${_deps}
            COMMENT "${_comment}"
            VERBATIM
        )
    endif()
endfunction()
