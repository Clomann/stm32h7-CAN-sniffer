/*
******************************************************************************
**

**  File        : LinkerScript_boot.ld
**
**
**  Abstract    : Linker script for STM32H7 series (MCUboot bootloader)
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Distribution: The file is distributed as is, without any warranty
**                of any kind.
**
*****************************************************************************
** @attention
**
** Copyright (c) 2019 STMicroelectronics.
** All rights reserved.
**
** This software is licensed under terms that can be found in the LICENSE file
** in the root directory of this software component.
** If no LICENSE file comes with this software, it is provided AS-IS.
**
******************************************************************************
*/

/* Entry Point */
ENTRY(Boot_Reset_Handler)

/* Highest address of the user mode stack */
PROVIDE(_estack = ORIGIN(RAM_D1) + LENGTH(RAM_D1));

_Boot_Flash_Size     = 0x20000 ; /* required amount of flash for bootloader */
_App_Flash_Size      = 0x60000 ; /* required amount of flash for app */
_Scratch_Flash_Size  = 0x0 ; /* overwrite-only: no scratch */
_Flash_Sector_Size   = 0x20000 ; /* STM32H7 flash sector size (128KB) */
__flash_base__ = ORIGIN(FLASH);
PROVIDE(__flash_base = __flash_base__);

/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x200 ;      /* required amount of heap  */
_Min_Stack_Size = 0x400 ; /* required amount of stack */

_Lwip_Heap_Size = LWIP_HEAP_SIZE ; /* required amount of heap for LwIP stack */

/* Specify the memory areas */
MEMORY
{
  FLASH (rx)        : ORIGIN = 0x08000000, LENGTH = 1024K
  FLASH_BOOT (rx)   : ORIGIN = ORIGIN(FLASH), LENGTH = _Boot_Flash_Size
  FLASH_PRIMARY (rx): ORIGIN = ORIGIN(FLASH_BOOT) + LENGTH(FLASH_BOOT), LENGTH = _App_Flash_Size
  FLASH_SECONDARY (rx): ORIGIN = ORIGIN(FLASH_PRIMARY) + LENGTH(FLASH_PRIMARY), LENGTH = _App_Flash_Size
  FLASH_SCRATCH (rx): ORIGIN = ORIGIN(FLASH_SECONDARY) + LENGTH(FLASH_SECONDARY), LENGTH = _Scratch_Flash_Size
  DTCMRAM (rw)     : ORIGIN = 0x20000000, LENGTH = 128K
  RAM_D1 (rw)      : ORIGIN = 0x24000000, LENGTH = 512K
  RAM_D2 (rw)      : ORIGIN = 0x30000000, LENGTH = 288K
  RAM_D3 (rw)      : ORIGIN = 0x38000000, LENGTH = 64K
  ITCMRAM (xrw)     : ORIGIN = 0x00000000, LENGTH = 64K
}

/* Define output sections */
SECTIONS
{
  __boot_start__ = ORIGIN(FLASH_BOOT);
  __boot_end__   = ORIGIN(FLASH_BOOT) + LENGTH(FLASH_BOOT);
  __boot_size__ = LENGTH(FLASH_BOOT);
  __boot_off__ = __boot_start__ - __flash_base__;

  __app_primary_start__ = ORIGIN(FLASH_PRIMARY);
  __app_primary_end__ = ORIGIN(FLASH_PRIMARY) + LENGTH(FLASH_PRIMARY);
  __app_primary_size__ = LENGTH(FLASH_PRIMARY);
  __app_primary_off__ = __app_primary_start__ - __flash_base__;

  __app_secondary_start__ = ORIGIN(FLASH_SECONDARY);
  __app_secondary_end__ = ORIGIN(FLASH_SECONDARY) + LENGTH(FLASH_SECONDARY);
  __app_secondary_size__ = LENGTH(FLASH_SECONDARY);
  __app_secondary_off__ = __app_secondary_start__ - __flash_base__;

  __scratch_start__ = ORIGIN(FLASH_SCRATCH);
  __scratch_end__ = ORIGIN(FLASH_SCRATCH) + LENGTH(FLASH_SCRATCH);
  __scratch_size__ = LENGTH(FLASH_SCRATCH);
  __scratch_off__ = __scratch_start__ - __flash_base__;

  .boot __boot_start__ :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))

    *(.text .text.*)
    *(.rodata .rodata.*)

    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    *(.ARM.extab* .gnu.linkonce.armextab.*)

    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;

    KEEP (*(.init))
    KEEP (*(.fini))
  } >FLASH_BOOT
  
  .preinit_array (READONLY) :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH_BOOT
  
  .init_array (READONLY) :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH_BOOT

  .fini_array (READONLY) :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);

    . = ALIGN(4);
    __boot_flash_lma_end__ = .;
  } >FLASH_BOOT

  .data : AT ( __boot_flash_lma_end__ )
  {
    *(.data .data.*)
  } >RAM_D1

  __boot_sidata = LOADADDR(.data);
  __boot_sdata  = ADDR(.data);
  __boot_edata  = ADDR(.data) + SIZEOF(.data);

  __boot_flash_lma_end_with_data__ = __boot_flash_lma_end__ + SIZEOF(.data);
  ASSERT(__boot_flash_lma_end_with_data__ <= __boot_end__, "Bootloader overflow (text+data)")

  .boot_bss (NOLOAD) :
  {
    *(.bss .bss.* COMMON)
  } >RAM_D1

  __boot_sbss   = ADDR(.boot_bss);
  __boot_ebss   = ADDR(.boot_bss) + SIZEOF(.boot_bss);

  /* Provide heap start symbol for newlib syscalls (e.g. _sbrk) */
  PROVIDE(_end = .);
  PROVIDE(end = .);

  PROVIDE(__boot_start = __boot_start__);
  PROVIDE(__boot_end = __boot_end__);
  PROVIDE(__boot_size = __boot_size__);
  PROVIDE(__boot_off = __boot_off__);

  PROVIDE(__app_primary_start = __app_primary_start__);
  PROVIDE(__app_primary_end__ = __app_primary_end__);
  PROVIDE(__app_primary_size = __app_primary_size__);
  PROVIDE(__app_primary_off = __app_primary_off__);
  PROVIDE(__app_secondary_start = __app_secondary_start__);
  PROVIDE(__app_secondary_end__ = __app_secondary_end__);
  PROVIDE(__app_secondary_size = __app_secondary_size__);
  PROVIDE(__app_secondary_off = __app_secondary_off__);
  PROVIDE(__scratch_start = __scratch_start__);
  PROVIDE(__scratch_end = __scratch_end__);
  PROVIDE(__scratch_size = __scratch_size__);
  PROVIDE(__scratch_off = __scratch_off__);

  ASSERT(__boot_start__ % _Flash_Sector_Size == 0, "Boot start not sector aligned")
  ASSERT(__boot_size__ % _Flash_Sector_Size == 0, "Boot size not sector aligned")
}
