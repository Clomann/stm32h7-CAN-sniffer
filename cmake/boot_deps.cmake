# Bootloader dependency aggregation

set(sources_SRCS ${common_sources_SRCS})
set(include_path_DIRS ${common_include_path_DIRS})

set(MCUBOOT_GEN_KEYS_DIR "${CMAKE_BINARY_DIR}/generated/keys")
set(MCUBOOT_SIGN_KEY_C "${MCUBOOT_GEN_KEYS_DIR}/autogen_sign_pub.c")
set(MCUBOOT_ENC_KEY_C "${MCUBOOT_GEN_KEYS_DIR}/autogen_enc_priv.c")

if(NOT PYTHON_EXE)
    message(FATAL_ERROR
        "Python interpreter not found. It is required to generate MCUboot key sources."
    )
endif()

if(NOT EXISTS "${MCUBOOT_SIGN_KEY_PEM}")
    message(FATAL_ERROR
        "Signing key PEM not found: ${MCUBOOT_SIGN_KEY_PEM}. "
        "Generate it with imgtool keygen (ecdsa-p256)."
    )
endif()

if(MCUBOOT_ENABLE_ENCRYPTION AND NOT EXISTS "${MCUBOOT_ENC_KEY_PEM}")
    message(FATAL_ERROR
        "MCUBOOT_ENABLE_ENCRYPTION=ON but encryption key PEM not found: ${MCUBOOT_ENC_KEY_PEM}. "
        "Generate it with imgtool keygen (ecdsa-p256)."
    )
endif()

if(NOT EXISTS "${IMGTOOL}")
    message(FATAL_ERROR "imgtool not found: ${IMGTOOL}")
endif()

file(MAKE_DIRECTORY "${MCUBOOT_GEN_KEYS_DIR}")

execute_process(
    COMMAND ${PYTHON_EXE} ${IMGTOOL} getpub -k ${MCUBOOT_SIGN_KEY_PEM} -e lang-c -o ${MCUBOOT_SIGN_KEY_C}
    RESULT_VARIABLE _mcuboot_getpub_rc
    OUTPUT_VARIABLE _mcuboot_getpub_out
    ERROR_VARIABLE _mcuboot_getpub_err
)
if(NOT _mcuboot_getpub_rc EQUAL 0)
    message(FATAL_ERROR
        "Failed to generate signing key source '${MCUBOOT_SIGN_KEY_C}'.\n"
        "stdout:\n${_mcuboot_getpub_out}\n"
        "stderr:\n${_mcuboot_getpub_err}"
    )
endif()

if(MCUBOOT_ENABLE_ENCRYPTION)
    execute_process(
        COMMAND ${PYTHON_EXE} ${IMGTOOL} getpriv -k ${MCUBOOT_ENC_KEY_PEM} -f pkcs8
        RESULT_VARIABLE _mcuboot_getpriv_rc
        OUTPUT_VARIABLE _mcuboot_getpriv_out
        ERROR_VARIABLE _mcuboot_getpriv_err
    )
    if(NOT _mcuboot_getpriv_rc EQUAL 0)
        message(FATAL_ERROR
            "Failed to generate encryption key source '${MCUBOOT_ENC_KEY_C}'.\n"
            "stdout:\n${_mcuboot_getpriv_out}\n"
            "stderr:\n${_mcuboot_getpriv_err}"
        )
    endif()
    file(WRITE "${MCUBOOT_ENC_KEY_C}" "${_mcuboot_getpriv_out}")
endif()

list(APPEND sources_SRCS
    ${PROJ_PATH}/platform/board/stm32h745/cm7/Src/boot_platform.c
    ${PROJ_PATH}/platform/Common/Src/FwUpdateHandoff.c
    ${PROJ_PATH}/platform/drivers/flash/flash.c
    ${PROJ_PATH}/bootloader/irq_handlers.c
    ${PROJ_PATH}/bootloader/syscalls.c
    ${PROJ_PATH}/bootloader/mcuboot_config/keys.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c

    # MbedTLS deps
    # ${PROJ_PATH}/libs/mbedtls/library/x509_create.c
    # ${PROJ_PATH}/libs/mbedtls/library/x509_crt.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_client.c
    ${PROJ_PATH}/libs/mbedtls/library/aes.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_slot_management.c
    # ${PROJ_PATH}/libs/mbedtls/library/bignum_mod_raw.c
    # ${PROJ_PATH}/libs/mbedtls/library/block_cipher.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_driver_wrappers_no_static.c
    # ${PROJ_PATH}/libs/mbedtls/library/camellia.c
    ${PROJ_PATH}/libs/mbedtls/library/constant_time.c
    # ${PROJ_PATH}/libs/mbedtls/library/pk_wrap.c
    # ${PROJ_PATH}/libs/mbedtls/library/pk.c
    # ${PROJ_PATH}/libs/mbedtls/library/pkcs7.c
    # ${PROJ_PATH}/libs/mbedtls/library/aesce.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_tls13_client.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_tls12_client.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_util.c
    ${PROJ_PATH}/libs/mbedtls/library/ecdh.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_tls.c
    # ${PROJ_PATH}/libs/mbedtls/library/x509_crl.c
    # ${PROJ_PATH}/libs/mbedtls/library/cipher_wrap.c
    # ${PROJ_PATH}/libs/mbedtls/library/chacha20.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_rsa.c
    # ${PROJ_PATH}/libs/mbedtls/library/des.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_cookie.c
    # ${PROJ_PATH}/libs/mbedtls/library/ctr_drbg.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_mac.c
    # ${PROJ_PATH}/libs/mbedtls/library/aesni.c
    # ${PROJ_PATH}/libs/mbedtls/library/dhm.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_cache.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_ciphersuites.c
    # ${PROJ_PATH}/libs/mbedtls/library/ecp_curves_new.c
    # ${PROJ_PATH}/libs/mbedtls/library/hmac_drbg.c
    # ${PROJ_PATH}/libs/mbedtls/library/rsa.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_ticket.c
    ${PROJ_PATH}/libs/mbedtls/library/asn1parse.c
    # ${PROJ_PATH}/libs/mbedtls/library/mps_trace.c
    # ${PROJ_PATH}/libs/mbedtls/library/pkwrite.c
    # ${PROJ_PATH}/libs/mbedtls/library/gcm.c
    ${PROJ_PATH}/libs/mbedtls/library/sha1.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_client.c
    # ${PROJ_PATH}/libs/mbedtls/library/asn1write.c
    # ${PROJ_PATH}/libs/mbedtls/library/ccm.c
    # ${PROJ_PATH}/libs/mbedtls/library/version_features.c
    # ${PROJ_PATH}/libs/mbedtls/library/aria.c
    # ${PROJ_PATH}/libs/mbedtls/library/lms.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_cipher.c
    # ${PROJ_PATH}/libs/mbedtls/library/pk_ecc.c
    # ${PROJ_PATH}/libs/mbedtls/library/entropy_poll.c
    # ${PROJ_PATH}/libs/mbedtls/library/x509write_csr.c
    ${PROJ_PATH}/libs/mbedtls/library/platform.c
    # ${PROJ_PATH}/libs/mbedtls/library/cmac.c
    ${PROJ_PATH}/libs/mbedtls/library/bignum.c
    # ${PROJ_PATH}/libs/mbedtls/library/pkparse.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_ffdh.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_msg.c
    # ${PROJ_PATH}/libs/mbedtls/library/debug.c
    ${PROJ_PATH}/libs/mbedtls/library/ripemd160.c
    # ${PROJ_PATH}/libs/mbedtls/library/pkcs5.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_tls13_generic.c
    # ${PROJ_PATH}/libs/mbedtls/library/x509write.c
    # ${PROJ_PATH}/libs/mbedtls/library/bignum_mod.c
    # ${PROJ_PATH}/libs/mbedtls/library/pem.c
    # ${PROJ_PATH}/libs/mbedtls/library/oid.c
    # ${PROJ_PATH}/libs/mbedtls/library/error.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_pake.c
    # ${PROJ_PATH}/libs/mbedtls/library/x509_csr.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_its_file.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto.c
    # ${PROJ_PATH}/libs/mbedtls/library/rsa_alt_helpers.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_debug_helpers_generated.c
    ${PROJ_PATH}/libs/mbedtls/library/platform_util.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_se.c
    # ${PROJ_PATH}/libs/mbedtls/library/base64.c
    ${PROJ_PATH}/libs/mbedtls/library/memory_buffer_alloc.c
    # ${PROJ_PATH}/libs/mbedtls/library/mps_reader.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_aead.c
    ${PROJ_PATH}/libs/mbedtls/library/ecp.c
    # ${PROJ_PATH}/libs/mbedtls/library/lmots.c
    # ${PROJ_PATH}/libs/mbedtls/library/version.c
    # ${PROJ_PATH}/libs/mbedtls/library/x509.c
    ${PROJ_PATH}/libs/mbedtls/library/bignum_core.c
    # ${PROJ_PATH}/libs/mbedtls/library/chachapoly.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_tls13_keys.c
    ${PROJ_PATH}/libs/mbedtls/library/sha256.c
    ${PROJ_PATH}/libs/mbedtls/library/ecp_curves.c
    # ${PROJ_PATH}/libs/mbedtls/library/md5.c
    # ${PROJ_PATH}/libs/mbedtls/library/timing.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_ecp.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_storage.c
    # ${PROJ_PATH}/libs/mbedtls/library/poly1305.c
    # ${PROJ_PATH}/libs/mbedtls/library/x509write_crt.c
    # ${PROJ_PATH}/libs/mbedtls/library/hkdf.c
    ${PROJ_PATH}/libs/mbedtls/library/sha3.c
    # ${PROJ_PATH}/libs/mbedtls/library/threading.c
    # ${PROJ_PATH}/libs/mbedtls/library/padlock.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_hash.c
    # ${PROJ_PATH}/libs/mbedtls/library/pkcs12.c
    # ${PROJ_PATH}/libs/mbedtls/library/entropy.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_tls13_server.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_tls12_server.c
    # ${PROJ_PATH}/libs/mbedtls/library/net_sockets.c
    ${PROJ_PATH}/libs/mbedtls/library/sha512.c
    ${PROJ_PATH}/libs/mbedtls/library/md.c
    # ${PROJ_PATH}/libs/mbedtls/library/ecjpake.c
    # ${PROJ_PATH}/libs/mbedtls/library/cipher.c
    ${PROJ_PATH}/libs/mbedtls/library/ecdsa.c
    ${PROJ_PATH}/libs/mbedtls/library/nist_kw.c
)

mcuboot_keys_append_generated_sources(
    sources_SRCS
    ${MCUBOOT_SIGN_KEY_C}
    ${MCUBOOT_ENC_KEY_C}
)

if(FW_USE_MALLOC)
    list(APPEND sources_SRCS
        ${PROJ_PATH}/bootloader/sysmem.c
    )
endif()

list(APPEND include_path_DIRS
    ${PROJ_PATH}/platform/board/stm32h745/cm7/Inc
    ${PROJ_PATH}/platform/board/stm32h745/cm7/Src
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Inc
    ${PROJ_PATH}/platform/drivers/flash
    ${STM32_HAL_PATH}/Drivers/CMSIS/Device/ST/STM32H7xx/Include
    ${STM32_HAL_PATH}/Drivers/CMSIS/Include
    ${PROJ_PATH}/libs/mbedtls/include
)

add_subdirectory(bootloader)
list(APPEND sources_SRCS ${boot_sources_SRCS})
list(APPEND include_path_DIRS ${boot_include_path_DIRS})

set(boot_target_sources_SRCS "${sources_SRCS}")
set(boot_target_include_path_DIRS "${include_path_DIRS}")
set(boot_target_compile_defs
    MBEDTLS_CONFIG_FILE=\"${PROJ_PATH}/bootloader/mbedtls_config.h\"
)
mcuboot_keys_append_compile_defs(boot_target_compile_defs)
set(boot_link_libs "")
