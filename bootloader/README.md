Fadecandy Bootloader
====================

This is a simple open source bootloader for the Freescale Kinetis MK20DX128 microcontroller. It uses the USB Device Firmware Upgrade (DFU) standard. This bootloader was developed for use with Fadecandy, but it should be portable with to other projects using chips in the Kinetis microcontroller family.

Installing
----------

On a Teensy 3.0, you can use the Teensy Loader application to install `fc-boot.hex`. Afterwards, your Teensy should appear to the system as a "Fadecandy Bootloader" USB device capable of loading an application firmware image.

On a Fadecandy fc64x8 board, the bootloader can be installed via JTAG using OpenOCD. If you have an Olimex ARM-USB-OCD adapter, everything's already set up to install the bootloader with "make install". Other JTAG adapters will require changing `openocd.cfg`.

Application Interface
---------------------

This section describes the programming interface that exists between the bootloader and the application firmware.

The bootloader uses the smallest possible protected region on the MK20DX128's flash, a 4KB block. It uses a tiny amount of RAM for a "token" which is used to force entry into the bootloader on system reset.

When entering the application firmware, the system clocks will already be configured, and the watchdog timer is already enabled with a 10ms timeout. The application may disable the watchdog timer if desired.

The bootloader normally transfers control to the application early in boot, before setting up the USB controller. It will skip this step and run the DFU implementation if any of the following conditions are true:

* A banner ("FC-Boot\n") printed to the hardware UART at 9600 baud is echoed back within one character-period. (Manual entry by shorting TX and RX)
* A 32-bit entry token (0x74624346, or 'FCbt') is found at 0x2000_1FFC. (Programmatic entry)
* The application ResetVector does not reside within application flash. (No application is installed)

Memory address range       | Description
-------------------------- | ----------------------------
0x0000_0000 - 0x0000_0FFF  | Bootloader protected flash
0x0000_1000 - 0x0000_10F7  | Application IVT in flash
0x0000_1000 - 0x0001_FFFF  | Remainder of application flash
0x1FFF_E000 - 0x2000_1FFC  | Application SRAM
0x2000_1FFC - 0x2000_1FFF  | Entry token in SRAM

External Hardware
-----------------

No external hardware is required for the bootloader to operate. The following pins are used by the bootloader for optional features:

* PC5 is a status LED, active (high) while the bootloader is in DFU mode.
* PCR16/17 are set up as UART0 briefly during boot, for broadcasting the banner message and testing for manual bootloader entry.

File Format
-----------

The DFU file consists of raw 1 kilobyte blocks to be programmed into flash starting at address 0x0000_1000. The file may contain up to 127 blocks. No additional headers or checksums are included. On disk, the standard DFU suffix and CRC are used. During transit, the standard USB CRC is used.

Contact
-------

Micah Elizabeth Scott <<micah@scanlime.org>>
