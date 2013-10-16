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

Pin      | Description
-------- | ----------------------------------
Gnd      | Shared ground
Vin      | +5V power for testjig itself
0        | Teensy RX, Fadecandy TX
1        | Teensy TX, Fadecandy RX
2        | To ground via green button
3        | Fadecandy TCLK (SWCLK)
4        | Fadecandy TMS (SWDIO)
14 (A0)  | Fadecandy output 0 (via resistor divider)
15 (A1)  | Fadecandy output 1 (via resistor divider)
16 (A2)  | Fadecandy output 2 (via resistor divider)
17 (A3)  | Fadecandy output 3 (via resistor divider)
18 (A4)  | Fadecandy output 4 (via resistor divider)
19 (A5)  | Fadecandy output 5 (via resistor divider)
20 (A6)  | Fadecandy output 6 (via resistor divider)
21 (A7)  | Fadecandy output 7 (via resistor divider)
22 (A8)  | Fadecandy 3.3v     (via resistor divider)
23 (A9)  | Fadecandy VUSB     (via resistor divider)

All analog inputs connect via the same resistor divider:
* 1K between analog input and ground
* 6.8K between analog input and Fadecandy signal
* Use 5% tolerance resistors or better

Testjig Firmwares
-----------------

* `production`
	* Programs bootloader and initial firmware image
	* Runs electrical tests
	* Communicates with the DUT processor using Serial Wire Debug
* `serial_passthrough`
	* Teensyduino sketch
    * Appears as a USB serial port device
	* Passes through access to the DUT serial port
	* When the green button is held, acts as a loopback for the DUT serial port, as one way to enter FC-Boot.

Contact
-------

Micah Elizabeth Scott <<micah@scanlime.org>>