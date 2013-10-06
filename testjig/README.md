Fadecandy Test Jig
==================

This is a simple one-off test fixture that's designed to facilitate in-circuit debugging of the Fadecandy firmware, and doing reliable low-volume production.

My prototype consists of:

* An ugly hand-wired perfboard
* 0.1"-spaced pogo pins which connect to the Fadecandy board's "hacker port" as well as its output header
* A standard ARM JTAG header for firmware development using an external JTAG adapter
* A 2-LED string of WS2811 LEDs, for verifying the timing of the Fadecandy outputs
* A Teensy 3.0 board which coordinates production and testing tasks

The board is intended for two separate use cases:

* During firmware development, it's a simple breakout board for the JTAG port
* During low-volume production, the Teensy installs firmware and performs electrical testing

This board is a work-in-progress. This directory contains Teensyduino sketches which run on the onboard Teensy. More information about the hardware will be included later.

Hardware tidbits
----------------

* JTAG port uses 10K pull-up resistors to Fadecandy's 3.3v rail on all signals.
* Pogo pins are 2x Digi-Key part ED8178-16-ND
* Big green button next to pogo-pins to initiate programming/test

Teensy 3.0 pin assignment
-------------------------

Pin | Description
--- | ----------------------------------
Gnd | Shared ground
Vin | +5V power for testjig itself
0   | Teesny RX, Fadecandy TX
1   | Teensy TX, Fadecandy RX
2   | To ground via green button


Contact
-------

Micah Elizabeth Scott <<micah@scanlime.org>>