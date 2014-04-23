Fadecandy Controller Boards
===========================

This directory contains information and design files for the various circuit boards that you can run Fadecandy firmware on.

Teensy 3.0
----------

The [Teensy 3.0](http://www.pjrc.com/teensy/) microcontroller board from PJRC was the original development platform for Fadecandy. You can still run the Fadecandy firmware, including the bootloader, on this platform.

* Use the [Teensy Loader](http://www.pjrc.com/teensy/loader.html) to install a [Firmware Binary](https://raw.github.com/scanlime/fadecandy/master/bin/fc-firmware-v106.hex) on your Teensy 3.0
* Bridge pins 15 and 16
* Wire your LEDs according to the diagram:

**Warning:** The Teensy's output signals are 3.3V, whereas the WS2811 LEDs expect a 5V signal. It will often work anyway, but this tends to be unreliable if your LED power supply voltage isn't consistent or if your wires aren't very short.

![Teensy 3.0 wiring diagram](https://raw.github.com/scanlime/fadecandy/master/pcb/teensy3/wiring-diagram.png)


Fc64x8 Revision A
-----------------
 
This board uses the same microcontroller as the Teensy 3.0 board, a Freescale Kinetis K20. There are several differences from the Teensy 3.0, however, that make it easier to use and more reliable for LED applications:

* It comes pre-programmed with the Fadecandy firmware.
* The Fadecandy board has a more convenient pinout, with more ground pads and with the LED strings organized sensibly.
* The Teensy uses 3.3v IO which doesn't work reliably with the WS2811 LEDs. Fadecandy includes a level shifter and series termination resistors.
* Fadecandy also includes a boost converter power supply to generate a stable 5V for the level shifter even when the USB power is noisy or sagging due to long cables.
* The Teensy uses a proprietary bootloader implemented on a separate microcontroller. The Fadecandy board has an open source bootloader that runs on the same microcontroller as the firmware, and it speaks the standard DFU protocol.

The PCB design for this revision is tagged as **fc64x8-rev-a** in Git. This board was manufactured in a limited run of 100, and availability has been very limited.

* [PCB Design Files](https://github.com/scanlime/fadecandy/tree/fc64x8-rev-a/pcb/fc64x8)
* Design software is [Eagle CAD](http://www.cadsoftusa.com/)

![Fc64x8-a Photo](http://farm6.staticflickr.com/5513/10877817543_2f654c8699.jpg)


Fc64x8 Revision B
-----------------

Electrically the same as Revision A, this is the version produced by Adafruit. It is slightly larger, at 0.8" by 1.25". It uses a mini-USB instead of a micro-USB connector, and includes two extra small holes at the top corners for mounting onto larger boards.


RGB-123 Fadecandy Controller
----------------------------

This is a new board layout for Fadecandy which uses different component packages. It is produced and sold by Ryan O'Hara at RGB-123. The current version is Revision G.


Test Jig
--------

This board, with the help of a Teensy 3.0 and the testjig firmware, performs automated factory tests and firmware installation on Fadecandy boards.

The design was contributed by Ryan O'Hara of RGB-123.

* [Order PCBs via OSH Park](http://oshpark.com/shared_projects/0Hx8kSYR)
* Design software is [LabCenter Proteus](http://www.labcenter.co.uk/)

