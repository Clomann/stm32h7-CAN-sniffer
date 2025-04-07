cmake_minimum_required(VERSION 3.22)

# Setup compiler settings
set(CMAKE_C_STANDARD                11)
set(CMAKE_C_STANDARD_REQUIRED       ON)
set(CMAKE_C_EXTENSIONS              ON)
set(CMAKE_CXX_STANDARD              20)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)
set(CMAKE_CXX_EXTENSIONS            ON)
set(PROJ_PATH                       ${CMAKE_CURRENT_SOURCE_DIR})
set(STM32_HAL_PATH                  ${CMAKE_CURRENT_SOURCE_DIR}/libs/STM32_HAL/STM32H7)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJ_PATH}/_bin)

message("Build type: "              ${CMAKE_BUILD_TYPE})

#
# Core project settings
#
project(SPI_FullDuplex_ComDMA_CM7)          # Modified
enable_language(C CXX ASM)

#
# Core MCU flags, CPU, instruction set and FPU setup
# Needs to be set properly for your MCU
#
set(CPU_PARAMETERS
    -mthumb

    # This needs attention to properly set for used MCU
    -mcpu=cortex-m7                 # Modified
    -mfpu=fpv5-d16                  # Modified
    -mfloat-abi=hard                # Modified
)

# Set linker script
set(linker_script_SRC               ${PROJ_PATH}/STM32CubeIDE/CM7/STM32H745ZITX_FLASH.ld) # Modified
set(EXECUTABLE                      ${CMAKE_PROJECT_NAME})

#
# List of source files to compile
#
set(sources_SRCS                    # Modified
    ${PROJ_PATH}/CM7/Src/main.c
    ${PROJ_PATH}/CM7/Src/stm32h7xx_hal_msp.c
    ${PROJ_PATH}/CM7/Src/stm32h7xx_it.c
    ${PROJ_PATH}/CM7/Src/httpd_cgi_ssi.c

    ${PROJ_PATH}/app/FileHandler.c
    ${PROJ_PATH}/app/ethernetif.c
    ${PROJ_PATH}/app/lwip.c

    ${PROJ_PATH}/app/tcp_echoserver.c
    ${PROJ_PATH}/app/app_ethernet.c

    ${PROJ_PATH}/drivers/spi/Spi_Cmds.c

    ${PROJ_PATH}/adapters/fatfs_sd/DiskIoFacade.c
    ${PROJ_PATH}/adapters/fatfs_sd/MmcAdapter.c

    ${PROJ_PATH}/middleware/Sd/Sd.c

    ${PROJ_PATH}/Common/Src/system_stm32h7xx.c

    ${PROJ_PATH}/STM32CubeIDE/CM7/Drivers/BSP/STM32H7xx_Nucleo/stm32h7xx_nucleo.c
    
    ${PROJ_PATH}/STM32CubeIDE/CM7/Example/User/CM7/syscalls.c
    ${PROJ_PATH}/STM32CubeIDE/CM7/Example/User/CM7/sysmem.c
    ${PROJ_PATH}/STM32CubeIDE/CM7/Example/User/startup/startup_stm32h745zitx.s

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
    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2s_ex.c
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

#
# Include directories
#
set(include_path_DIRS               # Modified
    ${PROJ_PATH}/CM7/Inc

    ${PROJ_PATH}/app

    ${PROJ_PATH}/drivers/spi

    ${PROJ_PATH}/adapters/fatfs_sd

    ${PROJ_PATH}/middleware/Sd

    ${PROJ_PATH}/Common/Inc

    ${STM32_HAL_PATH}//Drivers/BSP/Components/lan8742

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
)

#
# Symbols definition
#
set(symbols_SYMB                    # Modified
    "STM32H745xx"
    "USE_HAL_DRIVER"
    "CORE_CM7"
    "NO_SYS=1"
    "USE_STM32H7XX_NUCLEO_144_MB1363"
)

# Executable files
add_executable(${EXECUTABLE} ${sources_SRCS})

# Include paths
target_include_directories(${EXECUTABLE} PRIVATE ${include_path_DIRS})

# Project symbols
target_compile_definitions(${EXECUTABLE} PRIVATE ${symbols_SYMB})

# Compiler options
target_compile_options(${EXECUTABLE} PRIVATE
    ${CPU_PARAMETERS}
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    -Werror=implicit-function-declaration
    # Full debug configuration
    -Og -g3 -ggdb
)

# Linker options
target_link_options(${EXECUTABLE} PRIVATE
    -T${linker_script_SRC}
    ${CPU_PARAMETERS}
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    --specs=nosys.specs
    -u _printf_float                # STDIO float formatting support
    -Wl,--start-group
    -lc
    -lm
    -lstdc++
    -lsupc++
    -Wl,--end-group
    -Wl,--print-memory-usage
    -Wl,--warn-unresolved-symbols
    -Wl,--trace
)



# Execute post-build to print size
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${EXECUTABLE}>
)

# Convert output to hex and binary
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${EXECUTABLE}> ${EXECUTABLE}.hex
)

# Convert to bin file -> add conditional check?
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${EXECUTABLE}> ${EXECUTABLE}.bin
)