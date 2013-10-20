Fadecandy: Pre-compiled Binaries
================================

This directory contains convenient precompiled firmware and server binaries for Fadecandy. This file explains the conventions for the various types of files here.

Fadecandy Server
----------------

* fcserver-osx
  * Ready-to-run fcserver for Mac OS

Firmware Images
---------------

* `fc-boot-v###.hex`
  * Bootloader image only, in Intel Hex format.
  * This can be combined with other images to create a bootloader plus firmware image, or you can burn it alone and flash firmware separately using dfu-util.
  * For use with Teensy Loader, JTAG debuggers, or the Testjig.
* `fc-firmware-v###.hex`
  * Firmware image, including bootloader, in Intel Hex format.
  * For use with Teensy Loader, JTAG debuggers, or the Testjig.
  * Since this includes the bootloader, it can be updated with dfu-util if necessary.
* `fc-firmware-v###.elf`
  * ELF binary for the firmware. Includes debug symbols. Does NOT include the bootloader.
  * For use with GDB. Also used to generate Testjig firmware images.
  * Mainly used as a source file for generating other types of firmware images. You probably won't use it directly.
* `fc-firmware-v###.dfu`
  * Firmware image for DFU firmware updates.
  * For use with dfu-util to update firmware without any special debug hardware.
  
