/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2013 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows 
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

MEMORY
{
    /* Boot and application partitions in flash */
    BOOT_FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 4K
    APP_FLASH (rx) : ORIGIN = 0x00001000, LENGTH = 124K

    /* Boot and application partitions in RAM */
    BOOT_RAM (rwx) : ORIGIN = 0x1FFFE000, LENGTH = 256
    APP_RAM  (rwx) : ORIGIN = 0x1FFFE100, LENGTH = 16128

    /* Special RAM used for programming flash */
    FLEXRAM (rwx) : ORIGIN = 0x14000000, LENGTH = 2K
}

SECTIONS
{
    .text : {
        . = 0;
        KEEP(*(.vectors))
        *(.startup*)
        . = 0x400;
        KEEP(*(.flashconfig*))
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        KEEP(*(.init))
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;
        __init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        __init_array_end = .;
    } > BOOT_FLASH = 0xFF
    _etext = .;

    .apptext : {
        . = 0;
        *(.appvectors)
    } > APP_FLASH = 0xFF

    .bootram (NOLOAD) : {
        . = 0;
        *(.bootram)
    } > BOOT_RAM

    .usbdescriptortable (NOLOAD) : {
        /* . = ORIGIN(RAM); */
        . = ALIGN(512);
        *(.usbdescriptortable*)
    } > APP_RAM

    .dmabuffers (NOLOAD) : {
        . = ALIGN(4);
        *(.dmabuffers*)
    } > APP_RAM

    .usbbuffers (NOLOAD) : {
        . = ALIGN(4);
        *(.usbbuffers*)
    } > APP_RAM

    .flexram (NOLOAD) : {
        . = ALIGN(4);
        *(.flexram*)
    } > FLEXRAM

    .data : AT (_etext) {
        . = ALIGN(4);
        _sdata = .; 
        *(.data*)
        . = ALIGN(4);
        _edata = .; 
    } > APP_RAM

    .noinit (NOLOAD) : {
        *(.noinit*)
    } > APP_RAM

    .bss : {
        . = ALIGN(4);
        _sbss = .; 
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end = .;
    } > APP_RAM

    _estack = ORIGIN(APP_RAM) + LENGTH(APP_RAM);
}


