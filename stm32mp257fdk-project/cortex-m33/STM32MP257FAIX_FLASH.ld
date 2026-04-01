/*
 * Linker script for STM32MP257FAK3 — Cortex-M33
 *
 * Memory map (STM32MP257 Reference Manual RM0457, Section 2.5):
 *   SRAM1  : 0x0A000000, 256 KB  — M33 SRAM (tightly coupled)
 *   SRAM2  : 0x0A040000, 64 KB   — additional M33 SRAM
 *   DDR    : loaded by remoteproc from A35 side
 *
 * When loaded via remoteproc, the entire ELF (including "FLASH" regions)
 * is copied into DDR. SRAM addresses are only used for bare-metal JTAG load.
 */

ENTRY(Reset_Handler)

_estack = ORIGIN(RAM) + LENGTH(RAM);  /* Initial stack pointer = top of RAM */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x0A000000, LENGTH = 256K   /* M33 SRAM1 */
    RAM   (rw)  : ORIGIN = 0x0A040000, LENGTH = 64K    /* M33 SRAM2 */
}

SECTIONS
{
    /* Interrupt vector table — must be first */
    .isr_vectors :
    {
        KEEP(*(.isr_vectors))
        . = ALIGN(4);
    } > FLASH

    /*
     * OpenAMP resource table — remoteproc on the A35 scans the firmware ELF
     * for a section named ".resource_table" to find the vring descriptors
     * and shared memory layout.
     */
    .resource_table :
    {
        KEEP(*(.resource_table))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* C++ static constructors / destructors */
    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    /* Initialised data — stored in FLASH, copied to RAM at startup */
    .data :
    {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    _sidata = LOADADDR(.data);

    /* Zero-initialised data */
    .bss :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* Heap and stack */
    .heap_stack (NOLOAD) :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_heap_start = .);
        . = . + 0x1000;  /* 4 KB heap */
        PROVIDE(_heap_end = .);
        . = . + 0x1000;  /* 4 KB stack */
        . = ALIGN(8);
    } > RAM
}
