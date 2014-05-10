Fadecandy: Pre-compiled Binaries
================================

This directory contains convenient precompiled firmware and server binaries for Fadecandy. This file explains the conventions for the various types of files here.

Fadecandy Server
----------------

* `fcserver-osx`
  * Ready-to-run fcserver for Mac OS 10.6 and later
* `fcserver.exe`
  * Ready-to-run fcserver for Windows 7 and later
* `fcserver-rpi`
  * Ready-to-run fcserver for Raspberry Pi
  * May also work on other ARM Linux distributions
  * Requires root privileges and/or special usbfs configuration

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
  
Firmware Update Tool
--------------------

Firmware updates use the open source [dfu-util](http://dfu-util.gnumonks.org/) package.

Usage:

`$ dfu-util -D fc-firmware-v###.dfu`

* On Windows, you may need to run this multiple times as Windows will need to install drivers automatically for the Bootloader device.
* First the Fadecandy board will go into bootloader mode. The LED will light.
* Next, firmware will download. During this process, the LED blinks.
* When the firmware download is done, the LED will go dark.

For example:

	$ dfu-util -D fc-firmware-v106.dfu
	dfu-util 0.7

	Copyright 2005-2008 Weston Schmidt, Harald Welte and OpenMoko Inc.
	Copyright 2010-2012 Tormod Volden and Stefan Schmidt
	This program is Free Software and has ABSOLUTELY NO WARRANTY
	Please report bugs to dfu-util@lists.gnumonks.org
	
	Opening DFU capable USB device... ID 1d50:6082
	Run-time device DFU version 0101
	Found DFU: [1d50:6082] devnum=0, cfg=1, intf=0, alt=0, name="Fadecandy Bootloader"
	Claiming USB DFU Interface...
	Setting Alternate Setting #0 ...
	Determining device status: state = dfuIDLE, status = 0
	dfuIDLE, continuing
	DFU mode device DFU version 0101
	Device returned transfer size 1024
	Dfu suffix version 100
	bytes_per_hash=296
	Copying data from PC to DFU device
	Starting download: [##################################################] finished!
	state(7) = dfuMANIFEST, status(0) = No error condition is present
	state(7) = dfuMANIFEST, status(0) = No error condition is present
	...

* Windows: `dfu-util.exe` is included here, for convenience
* Mac OS: Install this with [homebrew](http://brew.sh): `brew install dfu-util`
* Linux: dfu-util is probably in your favorite package manager
