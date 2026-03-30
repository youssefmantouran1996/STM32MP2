/*
 * Linker script for STM32MP257FAK3 — Cortex-M33
 *
 * Memory map (STM32MP257 Reference Manual RM0457, Section 2.5):
 *   SRAM1  : 0x0A000000, 256 KB  — M33 SRAM (tightly coupled)
 *   SRAM2  : 0x0A040000, 64 KB   — additional M33 SRAM
 *   DDR    : loaded by remoteproc from A35 side
 *
 * Adjust sizes and addresses to match your linker script from STM32CubeMX.
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x0A000000, LENGTH = 256K   /* M33 SRAM1 (used as flash for remoteproc) */
    RAM   (rwx) : ORIGIN = 0x0A040000, LENGTH = 64K    /* M33 SRAM2 */
}

SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector))
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .data :
    {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    _sidata = LOADADDR(.data);

    .bss :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    .heap_stack (NOLOAD) :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        . = . + 0x1000;  /* 4 KB heap */
        . = . + 0x1000;  /* 4 KB stack */
        . = ALIGN(8);
    } > RAM
}
