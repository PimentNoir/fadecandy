/*
 * FC-Boot linker script.
 *
 *   - Early startup code runs out of flash
 *   - Everything else runs out of RAM
 *   - All flash after the first 4K page is reserved for application use
 *   - The last 4 bytes of RAM are used as our boot token
 */

MEMORY
{
    BOOT_FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 4K
    APP_FLASH (rx) : ORIGIN = 0x00001000, LENGTH = 124K
    RAM (rwx) : ORIGIN = 0x1FFFE000, LENGTH = 16K
    FLEXRAM (rwx) : ORIGIN = 0x14000000, LENGTH = 2K
}

SECTIONS
{
    .flash : {
        . = 0;
        KEEP(*(.vectors))
        *(.startup*)
        . = 0x400;
        KEEP(*(.flashconfig*))
    } > BOOT_FLASH = 0xFF
    _eflash = .;

    .apptext (NOLOAD) : {
        . = ORIGIN(APP_FLASH);
        *(.appvectors)
    } > APP_FLASH = 0xFF

    .usbdescriptortable (NOLOAD) : {
        . = ALIGN(512);
        *(.usbdescriptortable*)
    } > RAM

    .dmabuffers (NOLOAD) : {
        . = ALIGN(4);
        *(.dmabuffers*)
    } > RAM

    .usbbuffers (NOLOAD) : {
        . = ALIGN(4);
        *(.usbbuffers*)
    } > RAM

    .flexram (NOLOAD) : {
        . = ALIGN(4);
        *(.flexram*)
    } > FLEXRAM

    /* Combined data and text, after relocation */
    .dtext : AT (_eflash) {
        . = ALIGN(4);
        _sdtext = .; 
        *(.text*)
        *(.rodata*)
        *(.data*)
        . = ALIGN(4);
        _edtext = .; 
    } > RAM

    .noinit (NOLOAD) : {
        *(.noinit*)
    } > RAM

    .bss : {
        . = ALIGN(4);
        _sbss = .; 
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end = .;
    } > RAM

    _estack = ORIGIN(RAM) + LENGTH(RAM) - 4;
    boot_token = _estack;
}


