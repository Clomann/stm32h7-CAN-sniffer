# Sources
if (CORE STREQUAL "CM7")

list(APPEND sources_SRCS
    ${PROJ_PATH}/libs/ff15a/source/diskio.c
    ${PROJ_PATH}/libs/ff15a/source/ff.c
    ${PROJ_PATH}/libs/ff15a/source/ffsystem.c
    ${PROJ_PATH}/libs/ff15a/source/ffunicode.c

    ${PROJ_PATH}/libs/coreJSON/source/core_json.c

    # ${PROJ_PATH}/libs/lwip-tcpip/src/netif/bridgeif_fdb.c 
    # ${PROJ_PATH}/libs/lwip-tcpip/src/netif/bridgeif.c 
    # ${PROJ_PATH}/libs/lwip-tcpip/src/netif/ethernet.c 
    # ${PROJ_PATH}/libs/lwip-tcpip/src/netif/FILES 
    # ${PROJ_PATH}/libs/lwip-tcpip/src/netif/lowpan6_ble.c 
    # ${PROJ_PATH}/libs/lwip-tcpip/src/netif/lowpan6_common.c 
    # ${PROJ_PATH}/libs/lwip-tcpip/src/netif/lowpan6.c 
    # ${PROJ_PATH}/libs/lwip-tcpip/src/netif/slipif.c 
    # ${PROJ_PATH}/libs/lwip-tcpip/src/netif/zepif.c

    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/ipv4/autoip.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/ipv4/dhcp.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/ipv4/etharp.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/ipv4/icmp.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/ipv4/igmp.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/ipv4/ip4_addr.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/ipv4/ip4_frag.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/ipv4/ip4.c

    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/apps/lwiperf/lwiperf.c
    # ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/apps/http/fsdata.c
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/apps/mdns/mdns.c
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/apps/http/httpd.c
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/apps/http/fs.c

    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/netif/ethernet.c

    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/altcp_alloc.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/altcp_tcp.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/altcp.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/def.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/dns.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/inet_chksum.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/init.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/ip.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/mem.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/memp.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/netif.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/pbuf.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/raw.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/stats.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/sys.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/tcp_in.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/tcp_out.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/tcp.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/timeouts.c 
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/core/udp.c

    ${STM32_HAL_PATH}/Drivers/BSP/STM32H7xx_Nucleo/stm32h7xx_nucleo.c
    ${STM32_HAL_PATH}/Drivers/BSP/Components/lan8742/lan8742.c

    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_adc.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_adc_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cec.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_comp.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cordic.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_crc.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_crc_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cryp.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cryp_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dac.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dac_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dcmi.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dfsdm.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dfsdm_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma2d.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dsi.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dts.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_eth.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_eth_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_exti.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_fdcan.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_fmac.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gfxmmu.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hash.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hash_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hcd.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hrtim.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hsem.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2s.c
    # ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2s_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_irda.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_iwdg.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_jpeg.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_lptim.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ltdc.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ltdc_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdios.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mmc.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mmc_ex.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_msp_template.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_nand.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_nor.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_opamp.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_opamp_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ospi.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_otfdec.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pssi.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_qspi.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ramecc.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rng.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rng_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rtc.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rtc_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sai.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sai_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sd.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sd_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sdram.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_smartcard.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_smartcard_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_smbus.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_smbus_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spdifrx.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sram.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_swpmi.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_timebase_rtc_alarm_template.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_timebase_rtc_wakeup_template.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_timebase_tim_template.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_usart.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_usart_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_wwdg.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_adc.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_bdma.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_comp.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_cordic.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_crc.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_crs.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_dac.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_delayblock.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_dma.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_dma2d.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_exti.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_fmac.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_fmc.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_gpio.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_hrtim.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_i2c.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_lptim.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_lpuart.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_mdma.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_opamp.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_pwr.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_rcc.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_rng.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_rtc.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_sdmmc.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_spi.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_swpmi.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_tim.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_usart.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_usb.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_utils.c

    #${PROJ_PATH}/Core/Src/system_stm32h7xx.c

    #    ${PROJ_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_exti.c
    #    ${PROJ_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/
    #    ${PROJ_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_pwr.c
    #    ${PROJ_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_rcc.c
    #    ${PROJ_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_utils.c


    # MCUboot deps
    ${PROJ_PATH}/bootloader/bootloader.c
    ${PROJ_PATH}/bootloader/mcu_port.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/boot_record.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_area.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_area.h
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_find_key.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_img_hash.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_img_security_cnt.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_loader.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_loader.h
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_misc.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_misc.h
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_priv.h
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_public.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/caps.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/ed25519_psa.c
    # ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/encrypted_psa.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/encrypted.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/fault_injection_hardening_delay_rng_mbedtls.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/fault_injection_hardening.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/image_ecdsa.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/image_ed25519.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/image_rsa.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/image_validate.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/loader.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/ram_load.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/swap_misc.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/swap_move.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/swap_offset.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/swap_priv.h
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/swap_scratch.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/tlv.c

    # MbedTLS deps
    # ${PROJ_PATH}/libs/mbedtls/library/x509_create.c
    # ${PROJ_PATH}/libs/mbedtls/library/x509_crt.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_client.c
    # ${PROJ_PATH}/libs/mbedtls/library/aes.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_slot_management.c
    # ${PROJ_PATH}/libs/mbedtls/library/bignum_mod_raw.c
    # ${PROJ_PATH}/libs/mbedtls/library/block_cipher.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_driver_wrappers_no_static.c
    # ${PROJ_PATH}/libs/mbedtls/library/camellia.c
    # ${PROJ_PATH}/libs/mbedtls/library/constant_time.c
    # ${PROJ_PATH}/libs/mbedtls/library/pk_wrap.c
    # ${PROJ_PATH}/libs/mbedtls/library/pk.c
    # ${PROJ_PATH}/libs/mbedtls/library/pkcs7.c
    # ${PROJ_PATH}/libs/mbedtls/library/aesce.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_tls13_client.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_tls12_client.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_util.c
    # ${PROJ_PATH}/libs/mbedtls/library/ecdh.c
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
    # ${PROJ_PATH}/libs/mbedtls/library/asn1parse.c
    # ${PROJ_PATH}/libs/mbedtls/library/mps_trace.c
    # ${PROJ_PATH}/libs/mbedtls/library/pkwrite.c
    # ${PROJ_PATH}/libs/mbedtls/library/gcm.c
    # ${PROJ_PATH}/libs/mbedtls/library/sha1.c
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
    # ${PROJ_PATH}/libs/mbedtls/library/platform.c
    # ${PROJ_PATH}/libs/mbedtls/library/cmac.c
    # ${PROJ_PATH}/libs/mbedtls/library/bignum.c
    # ${PROJ_PATH}/libs/mbedtls/library/pkparse.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_ffdh.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_msg.c
    # ${PROJ_PATH}/libs/mbedtls/library/debug.c
    # ${PROJ_PATH}/libs/mbedtls/library/ripemd160.c
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
    # ${PROJ_PATH}/libs/mbedtls/library/memory_buffer_alloc.c
    # ${PROJ_PATH}/libs/mbedtls/library/mps_reader.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_aead.c
    # ${PROJ_PATH}/libs/mbedtls/library/ecp.c
    # ${PROJ_PATH}/libs/mbedtls/library/lmots.c
    # ${PROJ_PATH}/libs/mbedtls/library/version.c
    # ${PROJ_PATH}/libs/mbedtls/library/x509.c
    # ${PROJ_PATH}/libs/mbedtls/library/bignum_core.c
    # ${PROJ_PATH}/libs/mbedtls/library/chachapoly.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_tls13_keys.c
    ${PROJ_PATH}/libs/mbedtls/library/sha256.c
    # ${PROJ_PATH}/libs/mbedtls/library/ecp_curves.c
    # ${PROJ_PATH}/libs/mbedtls/library/md5.c
    # ${PROJ_PATH}/libs/mbedtls/library/timing.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_ecp.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_storage.c
    # ${PROJ_PATH}/libs/mbedtls/library/poly1305.c
    # ${PROJ_PATH}/libs/mbedtls/library/x509write_crt.c
    # ${PROJ_PATH}/libs/mbedtls/library/hkdf.c
    # ${PROJ_PATH}/libs/mbedtls/library/sha3.c
    # ${PROJ_PATH}/libs/mbedtls/library/threading.c
    # ${PROJ_PATH}/libs/mbedtls/library/padlock.c
    # ${PROJ_PATH}/libs/mbedtls/library/psa_crypto_hash.c
    # ${PROJ_PATH}/libs/mbedtls/library/pkcs12.c
    # ${PROJ_PATH}/libs/mbedtls/library/entropy.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_tls13_server.c
    # ${PROJ_PATH}/libs/mbedtls/library/ssl_tls12_server.c
    # ${PROJ_PATH}/libs/mbedtls/library/net_sockets.c
    # ${PROJ_PATH}/libs/mbedtls/library/sha512.c
    # ${PROJ_PATH}/libs/mbedtls/library/md.c
    # ${PROJ_PATH}/libs/mbedtls/library/ecjpake.c
    # ${PROJ_PATH}/libs/mbedtls/library/cipher.c
    # ${PROJ_PATH}/libs/mbedtls/library/ecdsa.c
    # ${PROJ_PATH}/libs/mbedtls/library/nist_kw.c
)

# Include directories
list(APPEND include_path_DIRS
    ${STM32_HAL_PATH}/Drivers/BSP/Components/lan8742

    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Inc
    ${STM32_HAL_PATH}/Drivers/CMSIS/Device/ST/STM32H7xx/Include
    ${STM32_HAL_PATH}/Drivers/CMSIS/Include
    ${STM32_HAL_PATH}/Drivers/BSP/STM32H7xx_Nucleo
    ${STM32_HAL_PATH}/Drivers/BSP/Components/Common
    ${STM32_HAL_PATH}/Middlewares/ST/STM32_Audio/Addons/PDM/Inc
    ${STM32_HAL_PATH}/Utilities/CPU
    ${STM32_HAL_PATH}/Utilities/Fonts

    ${PROJ_PATH}/libs/ff15a/source/

    ${PROJ_PATH}/libs/coreJSON/source/include/

    # LwIP dependencies
    # ${PROJ_PATH}/libs/lwip-tcpip/src/include
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/include
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/system
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/include/lwip/
    ${STM32_HAL_PATH}/Middlewares/Third_Party/LwIP/src/include/lwip/apps
    ${STM32_HAL_PATH}Middlewares/Third_Party/LwIP/src/apps/http/

    # MCUboot deps
    ${PROJ_PATH}/bootloader
    ${PROJ_PATH}/bootloader/flash_map_backend
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/include
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/include/bootutil
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/include/bootutil/crypto

    #mbedtls
    ${PROJ_PATH}/libs/mbedtls/include
)

endif() 

if (CORE STREQUAL "CM4")
list(APPEND sources_SRCS
    ${STM32_HAL_PATH}/Drivers/BSP/STM32H7xx_Nucleo/stm32h7xx_nucleo.c
    ${STM32_HAL_PATH}/Drivers/BSP/Components/lan8742/lan8742.c

    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_adc.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_adc_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cec.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_comp.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cordic.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_crc.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_crc_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cryp.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cryp_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dac.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dac_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dcmi.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dfsdm.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dfsdm_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma2d.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dsi.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dts.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_eth.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_eth_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_exti.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_fdcan.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_fmac.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gfxmmu.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hash.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hash_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hcd.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hrtim.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hsem.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2s.c
    # ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2s_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_irda.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_iwdg.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_jpeg.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_lptim.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ltdc.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ltdc_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdios.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mmc.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mmc_ex.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_msp_template.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_nand.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_nor.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_opamp.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_opamp_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ospi.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_otfdec.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pssi.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_qspi.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ramecc.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rng.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rng_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rtc.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rtc_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sai.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sai_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sd.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sd_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sdram.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_smartcard.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_smartcard_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_smbus.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_smbus_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spdifrx.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sram.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_swpmi.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_timebase_rtc_alarm_template.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_timebase_rtc_wakeup_template.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_timebase_tim_template.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_usart.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_usart_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_wwdg.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_adc.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_bdma.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_comp.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_cordic.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_crc.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_crs.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_dac.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_delayblock.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_dma.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_dma2d.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_exti.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_fmac.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_fmc.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_gpio.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_hrtim.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_i2c.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_lptim.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_lpuart.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_mdma.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_opamp.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_pwr.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_rcc.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_rng.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_rtc.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_sdmmc.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_spi.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_swpmi.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_tim.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_usart.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_usb.c
    #${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_utils.c

    #${PROJ_PATH}/Core/Src/system_stm32h7xx.c

    #    ${PROJ_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_exti.c
    #    ${PROJ_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/
    #    ${PROJ_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_pwr.c
    #    ${PROJ_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_rcc.c
    #    ${PROJ_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_utils.c
)

# Include directories
list(APPEND include_path_DIRS
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Inc
    ${STM32_HAL_PATH}/Drivers/CMSIS/Device/ST/STM32H7xx/Include
    ${STM32_HAL_PATH}/Drivers/CMSIS/Include
    ${STM32_HAL_PATH}/Drivers/BSP/STM32H7xx_Nucleo
    ${STM32_HAL_PATH}/Drivers/BSP/Components/Common
    ${STM32_HAL_PATH}/Middlewares/ST/STM32_Audio/Addons/PDM/Inc
    ${STM32_HAL_PATH}/Utilities/CPU
    ${STM32_HAL_PATH}/Utilities/Fonts
)
endif()

add_library(third_party_interface INTERFACE)

target_sources(third_party_interface INTERFACE ${sources_SRCS})

target_include_directories(third_party_interface INTERFACE ${include_path_DIRS})

target_compile_definitions(third_party_interface INTERFACE ${symbols_SYMB})

target_compile_options(third_party_interface INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-unused-parameter>
    $<$<CXX_COMPILER_ID:MSVC>:/wd4100>
)
