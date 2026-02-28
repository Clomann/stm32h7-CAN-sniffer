/*
******************************************************************************
**

**  File        : LinkerScript.ld
**
**
**  Abstract    : Linker script for STM32H7 series
**                1024Kbytes FLASH and 192Kbytes RAM_D1
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
  DTCMRAM (xrw)     : ORIGIN = 0x20000000, LENGTH = 128K
  RAM_D1 (xrw)      : ORIGIN = 0x24000000, LENGTH = 512K
  RAM_D2 (xrw)      : ORIGIN = 0x30000000, LENGTH = 288K
  RAM_D3 (xrw)      : ORIGIN = 0x38000000, LENGTH = 64K
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
    /* bootloader sources */
    KEEP(*bootloader*(.isr_vector))
    *bootloader*(.text .text.* .rodata .rodata.*)

    /* mcuboot sources */
    *mcuboot*(.text .text.* .rodata .rodata.*)

    . = ALIGN(4);

    __boot_flash_lma_end__ = .;
  } >FLASH_BOOT

  .boot_data : AT ( __boot_flash_lma_end__ )
  {
    *bootloader*(.data .data.*)
    *mcuboot*(.data .data.*)
  } >RAM_D1

  __boot_sidata = LOADADDR(.boot_data);
  __boot_sdata  = ADDR(.boot_data);
  __boot_edata  = ADDR(.boot_data) + SIZEOF(.boot_data);

  __boot_flash_lma_end_with_data__ = __boot_flash_lma_end__ + SIZEOF(.boot_data);
  ASSERT(__boot_flash_lma_end_with_data__ <= __boot_end__, "Bootloader overflow (text+data)")

  . = __boot_end__;

  PROVIDE(__boot_start = __boot_start__);
  PROVIDE(__boot_end = __boot_end__);
  PROVIDE(__boot_size = __boot_size__);
  PROVIDE(__boot_off = __boot_off__);
  
  .boot_bss (NOLOAD) :
  {
    *bootloader*(.bss .bss.* COMMON)
    *mcuboot*(.bss .bss.* COMMON)
  } >RAM_D1

  __boot_sbss   = ADDR(.boot_bss);
  __boot_ebss   = ADDR(.boot_bss) + SIZEOF(.boot_bss);


  /* The startup code goes first into FLASH */
  .isr_vector __app_primary_start__ :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH_PRIMARY

  ASSERT(__app_primary_start__ == ADDR(.isr_vector), "ISR vector not at app start")

  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);

    *(EXCLUDE_FILE (*bootloader* *mcuboot*) .text .text.*)

    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >FLASH_PRIMARY

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    
    __start_comm_factory = .;
    KEEP(*(.comm_factory))
    __stop_comm_factory = .;

    . = ALIGN(4);
    *(EXCLUDE_FILE (*bootloader* *mcuboot*) .rodata .rodata.*)
    . = ALIGN(4);
  } >FLASH_PRIMARY

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH_PRIMARY
  .ARM (READONLY) : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH_PRIMARY

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH_PRIMARY
  .init_array (READONLY) :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH_PRIMARY
  .fini_array (READONLY) :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH_PRIMARY

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

  .secondary_reserve __app_secondary_start__ (NOLOAD) :
  {
    . = . + _App_Flash_Size;
  } >FLASH_SECONDARY

  .scratch_reserve __scratch_start__ (NOLOAD) :
  {
    . = ALIGN(4);
    . = . + _Scratch_Flash_Size;
  } >FLASH_SCRATCH

  ASSERT(__boot_start__ % _Flash_Sector_Size == 0, "Boot start not sector aligned")
  ASSERT(__boot_size__ % _Flash_Sector_Size == 0, "Boot size not sector aligned")
  ASSERT(__app_primary_start__ % _Flash_Sector_Size == 0, "Primary start not sector aligned")
  ASSERT(__app_primary_size__ % _Flash_Sector_Size == 0, "Primary size not sector aligned")
  ASSERT(__app_secondary_start__ % _Flash_Sector_Size == 0, "Secondary start not sector aligned")
  ASSERT(__app_secondary_size__ % _Flash_Sector_Size == 0, "Secondary size not sector aligned")
  ASSERT(__scratch_start__ % _Flash_Sector_Size == 0, "Scratch start not sector aligned")
  ASSERT(__scratch_size__ % _Flash_Sector_Size == 0, "Scratch size not sector aligned")
  ASSERT(__app_primary_end__ == __app_secondary_start__, "Primary and secondary overlap/gap")
  ASSERT(__app_secondary_end__ == __scratch_start__, "Secondary and scratch overlap/gap")
  ASSERT(__app_secondary_end__ <= (ORIGIN(FLASH) + LENGTH(FLASH)), "Secondary exceeds FLASH size")
  ASSERT(__scratch_end__ <= (ORIGIN(FLASH) + LENGTH(FLASH)), "Flash layout exceeds FLASH size")

  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  .dtcram (NOLOAD): {
    . = ALIGN(4);
    *(.dtcram)
    . = ALIGN(4);
  } >DTCMRAM

  /* Initialized data sections goes into RAM_D1, load LMA copy after code */
  .data : 
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */

    *(EXCLUDE_FILE (*bootloader* *mcuboot*) .data .data.*)

    . = ALIGN(32);
    __dma_ro_start = .;
    KEEP(*(.dma_buffer.ro))
    __dma_ro_end = .;
    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM_D1 AT> FLASH_PRIMARY

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss section */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    
    *(EXCLUDE_FILE (*bootloader* *mcuboot*) .bss .bss.* COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM_D1

  .ram_d1 (NOLOAD): {
    . = ALIGN(4);
    *(.ram_d1)
    . = ALIGN(4);
  } >RAM_D1

  /* User_heap_stack section, used to check that there is enough RAM_D1 left */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM_D1

  .lwip_sec (NOLOAD): {
    . = ABSOLUTE(0x30000000);
    __lwip_sec_start__ = .;

    *(.RxDecripSection) 
    
    . = ABSOLUTE(0x30000200);
    *(.TxDecripSection)
    
    . = ABSOLUTE(0x30000400);
    *(.Rx_PoolSection) 
    
    . = ALIGN(4);
    __lwip_heap_start__ = .;
    . = . + _Lwip_Heap_Size;
    __lwip_heap_end__ = .;
    
    __lwip_sec_end__ = .;
  } >RAM_D2

  PROVIDE(__lwip_heap_start = __lwip_heap_start__);
  PROVIDE(__lwip_heap_end   = __lwip_heap_end__);

  .ram_d2 (NOLOAD): {
    . = ALIGN(4);
    *(.ram_d2)
    . = ALIGN(4);
  } >RAM_D2

  .dma_buffer (NOLOAD) : ALIGN(32) {
    __dma_buffers_start__ = .;
    *(.dma_buffer)
    __dma_buffers_end__   = .;
  } >RAM_D2

  PROVIDE(__dma_buffers_start = __dma_buffers_start__);
  PROVIDE(__dma_buffers_end   = __dma_buffers_end__);

  PROVIDE(__lwip_sec_start = __lwip_sec_start__);
  PROVIDE(__lwip_sec_end   = __lwip_sec_end__);

  .ram_d3 (NOLOAD): {
    . = ALIGN(4);
    *(.ram_d3)
    . = ALIGN(4);
  } >RAM_D3

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
