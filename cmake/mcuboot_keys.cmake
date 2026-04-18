include_guard(GLOBAL)
include(CMakeParseArguments)

function(mcuboot_keys_init)
    set(options)
    set(oneValueArgs ROOT_DIR)
    cmake_parse_arguments(MCK "${options}" "${oneValueArgs}" "" ${ARGN})

    if(NOT MCK_ROOT_DIR)
        set(MCK_ROOT_DIR "${CMAKE_SOURCE_DIR}")
    endif()

    option(MCUBOOT_ENABLE_ENCRYPTION
        "Enable MCUboot image encryption flow (ECIES)"
        OFF
    )

    set(MCUBOOT_KEYS_DIR "${MCK_ROOT_DIR}/.keys" CACHE PATH
        "Directory containing MCUboot signing/encryption key files"
    )
    set(MCUBOOT_SIGN_KEY_PEM "${MCUBOOT_KEYS_DIR}/sign-ecdsa-p256.priv.pem" CACHE FILEPATH
        "Path to MCUboot signing private key PEM (ECDSA P-256)"
    )
    set(MCUBOOT_ENC_KEY_PEM "${MCUBOOT_KEYS_DIR}/enc-ecies-p256.priv.pem" CACHE FILEPATH
        "Path to MCUboot encryption private key PEM (ECIES P-256)"
    )
    set(MCUBOOT_ENC_PUBKEY_PEM "${MCUBOOT_KEYS_DIR}/enc-ecies-p256.pub.pem" CACHE FILEPATH
        "Path to MCUboot encryption public key PEM (ECIES P-256)"
    )
endfunction()

function(mcuboot_keys_append_signing_key_arg OUT_VAR)
    if(NOT EXISTS "${MCUBOOT_SIGN_KEY_PEM}")
        message(FATAL_ERROR
            "Signing key PEM not found: ${MCUBOOT_SIGN_KEY_PEM}. "
            "Generate it with imgtool keygen -t ecdsa-p256."
        )
    endif()

    list(APPEND ${OUT_VAR}
        KEY ${MCUBOOT_SIGN_KEY_PEM}
    )
    set(${OUT_VAR} "${${OUT_VAR}}" PARENT_SCOPE)
endfunction()

function(mcuboot_keys_append_encryption_pubkey_arg OUT_VAR)
    if(NOT EXISTS "${MCUBOOT_ENC_PUBKEY_PEM}")
        message(FATAL_ERROR
            "Encryption public key PEM not found: ${MCUBOOT_ENC_PUBKEY_PEM}. "
            "Generate it from MCUBOOT_ENC_KEY_PEM with imgtool getpub -e pem."
        )
    endif()

    list(APPEND ${OUT_VAR}
        ENCRYPT_PUBKEY ${MCUBOOT_ENC_PUBKEY_PEM}
    )
    set(${OUT_VAR} "${${OUT_VAR}}" PARENT_SCOPE)
endfunction()

function(mcuboot_keys_append_sign_args OUT_VAR)
    mcuboot_keys_append_signing_key_arg(${OUT_VAR})
    if(MCUBOOT_ENABLE_ENCRYPTION)
        mcuboot_keys_append_encryption_pubkey_arg(${OUT_VAR})
    endif()
    set(${OUT_VAR} "${${OUT_VAR}}" PARENT_SCOPE)
endfunction()

function(mcuboot_keys_append_compile_defs OUT_VAR)
    if(MCUBOOT_ENABLE_ENCRYPTION)
        list(APPEND ${OUT_VAR} MCUBOOT_ENABLE_ENCRYPTION)
    endif()
    set(${OUT_VAR} "${${OUT_VAR}}" PARENT_SCOPE)
endfunction()

function(mcuboot_keys_append_generated_sources OUT_VAR SIGN_KEY_C ENC_KEY_C)
    list(APPEND ${OUT_VAR} ${SIGN_KEY_C})
    if(MCUBOOT_ENABLE_ENCRYPTION)
        list(APPEND ${OUT_VAR} ${ENC_KEY_C})
    endif()
    set(${OUT_VAR} "${${OUT_VAR}}" PARENT_SCOPE)
endfunction()

function(mcuboot_keys_generate_sources)
    set(options)
    set(oneValueArgs GENERATE_DIR SIGN_OUT ENC_OUT PYTHON IMGTOOL)
    cmake_parse_arguments(MCKG "${options}" "${oneValueArgs}" "" ${ARGN})

    if(NOT MCKG_GENERATE_DIR OR NOT MCKG_SIGN_OUT OR NOT MCKG_IMGTOOL)
        message(FATAL_ERROR
            "mcuboot_keys_generate_sources requires GENERATE_DIR, SIGN_OUT and IMGTOOL."
        )
    endif()

    if(NOT EXISTS "${MCKG_IMGTOOL}")
        message(FATAL_ERROR "imgtool not found: ${MCKG_IMGTOOL}")
    endif()

    set(_python_candidates)
    if(MCKG_PYTHON)
        list(APPEND _python_candidates "${MCKG_PYTHON}")
    endif()
    if(MCUBOOT_PYTHON)
        list(APPEND _python_candidates "${MCUBOOT_PYTHON}")
    endif()
    if(DEFINED ENV{VIRTUAL_ENV})
        if(EXISTS "$ENV{VIRTUAL_ENV}/bin/python3")
            list(APPEND _python_candidates "$ENV{VIRTUAL_ENV}/bin/python3")
        endif()
        if(EXISTS "$ENV{VIRTUAL_ENV}/bin/python")
            list(APPEND _python_candidates "$ENV{VIRTUAL_ENV}/bin/python")
        endif()
    endif()
    set(Python3_FIND_VIRTUALENV FIRST)
    find_package(Python3 QUIET COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        list(APPEND _python_candidates "${Python3_EXECUTABLE}")
    endif()
    find_program(_mcuboot_python_fallback NAMES python3 python py
        HINTS /usr/local/bin /opt/homebrew/bin /usr/bin
    )
    if(_mcuboot_python_fallback)
        list(APPEND _python_candidates "${_mcuboot_python_fallback}")
    endif()

    list(REMOVE_DUPLICATES _python_candidates)

    set(_python "")
    foreach(_candidate IN LISTS _python_candidates)
        execute_process(
            COMMAND ${_candidate} -c "import click"
            RESULT_VARIABLE _click_rc
            OUTPUT_QUIET
            ERROR_QUIET
        )
        if(_click_rc EQUAL 0)
            set(_python "${_candidate}")
            break()
        endif()
    endforeach()

    if(NOT _python)
        message(FATAL_ERROR
            "No Python interpreter with module 'click' found. "
            "Install dependencies first: python3 -m pip install -r ./libs/mcuboot/scripts/requirements.txt"
        )
    endif()

    if(NOT EXISTS "${MCUBOOT_SIGN_KEY_PEM}")
        message(FATAL_ERROR
            "Signing key PEM not found: ${MCUBOOT_SIGN_KEY_PEM}. "
            "Generate it with imgtool keygen -t ecdsa-p256."
        )
    endif()

    if(MCUBOOT_ENABLE_ENCRYPTION AND NOT EXISTS "${MCUBOOT_ENC_KEY_PEM}")
        message(FATAL_ERROR
            "MCUBOOT_ENABLE_ENCRYPTION=ON but encryption key PEM not found: ${MCUBOOT_ENC_KEY_PEM}. "
            "Generate it with imgtool keygen -t ecdsa-p256."
        )
    endif()

    file(MAKE_DIRECTORY "${MCKG_GENERATE_DIR}")

    execute_process(
        COMMAND ${_python} ${MCKG_IMGTOOL} getpub -k ${MCUBOOT_SIGN_KEY_PEM} -e lang-c -o ${MCKG_SIGN_OUT}
        RESULT_VARIABLE _mcuboot_getpub_rc
        OUTPUT_VARIABLE _mcuboot_getpub_out
        ERROR_VARIABLE _mcuboot_getpub_err
    )
    if(NOT _mcuboot_getpub_rc EQUAL 0)
        message(FATAL_ERROR
            "Failed to generate signing key source '${MCKG_SIGN_OUT}'.\n"
            "stdout:\n${_mcuboot_getpub_out}\n"
            "stderr:\n${_mcuboot_getpub_err}"
        )
    endif()

    if(MCUBOOT_ENABLE_ENCRYPTION)
        if(NOT MCKG_ENC_OUT)
            message(FATAL_ERROR
                "mcuboot_keys_generate_sources requires ENC_OUT when MCUBOOT_ENABLE_ENCRYPTION=ON."
            )
        endif()

        execute_process(
            COMMAND ${_python} ${MCKG_IMGTOOL} getpriv -k ${MCUBOOT_ENC_KEY_PEM} -f pkcs8
            RESULT_VARIABLE _mcuboot_getpriv_rc
            OUTPUT_VARIABLE _mcuboot_getpriv_out
            ERROR_VARIABLE _mcuboot_getpriv_err
        )
        if(NOT _mcuboot_getpriv_rc EQUAL 0)
            message(FATAL_ERROR
                "Failed to generate encryption key source '${MCKG_ENC_OUT}'.\n"
                "stdout:\n${_mcuboot_getpriv_out}\n"
                "stderr:\n${_mcuboot_getpriv_err}"
            )
        endif()
        file(WRITE "${MCKG_ENC_OUT}" "${_mcuboot_getpriv_out}")
    endif()
endfunction()
