Fadecandy Bootloader
====================

This is a simple open source bootloader for the Freescale Kinetis K20 microcontroller. It uses the USB Device Firmware Upgrade (DFU) standard. This bootloader was developed for use with Fadecandy, but it should be portable to other projects using the Kinetis microcontroller family.

* This bootloader is a work in progress. Doesn't actually work yet!

Quick facts:

* Enter the bootloader via serial loopback detect, DFU command from runtime firmware, or if runtime firmware is missing.
* Tested with the open source `dfu-util` package.

Installing
----------

The bootloader can be installed via JTAG using OpenOCD. If you have an Olimex ARM-USB-OCD adapter, everything's already set up to install the bootloader with "make install". Other JTAG adapters will require changing `openocd.cfg`.

Contact
-------

Micah Elizabeth Scott <<micah@scanlime.org>>