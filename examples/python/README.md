Fadecandy Python Examples
========================

This directory contains examples for Fadecandy written in Python.

The Open Pixel Control project includes its own [Python examples](https://github.com/zestyping/openpixelcontrol/tree/master/python). Those will all work with Fadecandy too.


Simple LED Patterns
-------------------

* `chase.py`
  * Light each LED in sequence
* `crosstalk-test.py`
  * Each LED strip glows dimly, and one strip at a time will briefly brighten.
  * The first three pixels of each strip indicate its position on the controller.
  * This is intended as a hardware test pattern to isolate any electrical interference issues between strips.
* `every-other-white.py`
  * Alternate between full-brightness and dark every other LED.
* `measuring-stick.py`
  * A tool for measuring LED strips.
  * The first LED in each group of 10 glows green.
  * All other LEDs in the strip are dark.
  * LEDs past the first 64 in the strip will be dark.
* `solid-white.py`
  * Fade all LEDs in to 100% brightness
* `strobe.py`
  * Rapidly fade all LEDs on and off

Other Samples
-------------

* `color-correction-ui.py`
  * Tk user interface for adjusting color correction settings.
  * Connects directly to `fcserver`
* `firmware-config-ui.py`
  * Tk user interface for firmware configuration settings.
  * Connects directly to `fcserver`
* `usb-lowlevel.py`
  * Demonstrates low-level USB control of a Fadecandy board, without `fcserver`.
  * Uses PyUSB

Sample Libraries
----------------

* `opc.py`
  * Original Open Pixel Control client
* `fastopc.py`
  * Higher-performance OPC client, using NumPy
