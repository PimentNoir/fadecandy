Fadecandy
=========

Fadecandy is firmware for the [Teensy 3.0](http://www.pjrc.com/store/teensy3.html), a tiny and inexpensive ARM microcontroller board.

Fadecandy drives addressable LED strips with the WS2811 and WS2812 controllers. These LED strips are common and inexpensive, available from [many suppliers](http://www.aliexpress.com/item/5M-WS2811-LED-digital-strip-60leds-m-with-60pcs-WS2811-built-in-tthe-5050-smd-rgb/635563383.html?tracelog=back_to_detail_a) for around $0.25 per pixel.

This firmware is based on Stoffregen's excellent [OctoWS2811](http://www.pjrc.com/teensy/td_libs_OctoWS2811.html) library, which pumps out serial data for these LED strips entirely using DMA. This firmware builds on Paul's work by adding:

* A high performance USB protocol
* Zero copy architecture with triple-buffering
* Interpolation between keyframes
* Gamma and color correction with per-channel lookup tables
* Temporal dithering

These features add up to give *very smooth* fades and high dynamic range. Ever notice that annoying stair-stepping effect when fading LEDs from off to dim? Fadecandy avoids that using a form of [delta-sigma modulation](http://en.wikipedia.org/wiki/Delta-sigma_modulation). It rapidly wiggles each pixel's value up or down by one 8-bit step, in order to achieve 16-bit resolution for fades.

Example videos:

* [Smooth interpolation](http://youtu.be/JilFl-xTdJ4)
* [Multiple Fadecandy boards running in unison](http://youtu.be/OXvY6aQAGcs)

Vitals
------

* 512 pixels supported per Teensy board (8 strings, 64 pixels per string)
* Very high hardware frame rate (395 FPS) to support temporal dithering
* Full-speed (12 Mbps) USB
* 257x3-entry 16-bit color lookup table, for gamma correction and color balance

Color Processing
----------------

Fadecandy internally represents colors with 16 bits of precision per channel, or 48 bits per pixel. Why 48-bit color? In combination with our dithering algorithm, this gives a lot more color resolution. It's especially helpful near the low end of the brightness range, where stair-stepping and color *popping* artifacts can be most apparent.

Each pixel goes through the following processing steps in Fadecandy:

* 8 bit per channel framebuffer values are expanded to 16 bits per channel
* We interpolate smoothly from the old framebuffer values to the new framebuffer values
* This interpolated 16-bit value goes through the color LUT, which itself is linearly interpolated
* The final 16-bit value is fed into our temporal dithering algorithm, which results in an 8-bit color
* These 8-bit colors are converted to the format needed by OctoWS2811's DMA engine
* In hardware, the converted colors are streamed out to eight LED strings in parallel

The color lookup tables can be used to implement gamma correction, brightness and contrast, and white point correction. Each channel (RGB) has a 257 entry table. Each entry is a 16-bit intensity. Entry 0 corresponds to the 16-bit color 0x0000, entry 1 corresponds to 0x0100, etc. The 257th entry corresponds to 0x10000, which is just past the end of the 16-bit intensity space.

Keyframe Interpolation
----------------------

By default, Fadecandy interprets each frame it receives as a keyframe. In-between these keyframes, Fadecandy will generate smooth intermediate frames using linear interpolation. The interpolation duration is determined by the elapsed time between when the final packet of one frame is received and when the final packet of the next frame is received.

This scheme works well when frames are arriving at a nearly constant rate. If frames suddenly arrive slower than they had been arriving, interpolation will proceed faster than it optimally should, and one keyframe will hold steady until the next keyframe arrives. If frames suddenly arrive faster than they had been arriving, Fadecandy will need to jump ahead in order to avoid falling behind.

This keyframe interpolation is not intended as a substitute for other forms of animation control. It is intended to generate high-framerate video from a source that operates at typical video framerates.

Open Pixel Control Server
-------------------------

The Fadecandy project includes an [Open Pixel Control](http://openpixelcontrol.org/) server which can drive multiple Fadecandy boards and DMX adaptors. USB devices may be hotplugged while the server is up, and the server uses a JSON configuration file to map OPC messages to individual Fadecandy boards and DMX devices.

Why use Open Pixel Control?

* You can keep your effects code portable among different lighting controllers.
* OPC includes an OpenGL-based simulator, allowing you to develop effects before the hardware is done.
* The OPC server manages USB hotplug and multi-device synchronization, so you don't have to.
* The OPC server loads color-correction data into each Fadecandy board.

For more information, see the [Server README](https://github.com/scanlime/fadecandy/blob/master/server/README.md).

Prerequisites
-------------

* To install a firmware image, you'll need the Teensy Loader: <http://www.pjrc.com/teensy/loader.html>
* To recompile the firmware, please use the recommended ARM toolchain from <https://code.launchpad.net/gcc-arm-embedded>

Pin Assignment
--------------

The pin assignment is the same as the original [OctoWS2811](http://www.pjrc.com/teensy/td_libs_OctoWS2811.html) pinout:

![Wiring Diagram](https://raw.github.com/scanlime/fadecandy/master/firmware/wiring-diagram.png)

Teensy 3.0 Pin | Function
-------------- | --------------
2              | Led strip #1
14             | Led strip #2
7              | Led strip #3
8              | Led strip #4
6              | Led strip #5
20             | Led strip #6
21             | Led strip #7
5              | Led strip #8
15 & 16        | Connect together
13 (LED)       | Built-in LED blinks when data is received over USB

Remember that each strip may be up to 64 LEDs long. It's fine to have shorter strips or to leave some outputs unused. These outputs are 3.3V logic signals at 800 kilobits per second. It usually works to connect them directly to the 5V inputs of your WS2811 LED strips, but for the best signal integrity you should really use a level-shifting buffer to convert the 3.3V logic to 5V.

USB Protocol
------------

To achieve the best CPU efficiency, Fadecandy uses a custom packet-oriented USB protocol rather than emulating a USB serial device. This simple USB protocol is easy to speak using cross-platform libraries like [libusb](http://www.libusb.org) and [PyUSB](http://pyusb.sourceforge.net/). Examples are included. If you use the included [Open Pixel Control](http://openpixelcontrol.org/) bridge, you need not worry about the USB protocol at all.

Attribute       | Value
--------------- | -----
Vendor ID       | 0x1d50
Product ID      | 0x607a
Manufacturer    | "scanlime"
Product         | "Fadecandy"
Serial          | Unique 128-bit ID, as a hexadecimal string
Device Class    | Vendor-specific
Configurations  | 1
Endpoints       | 1
Endpoint 1      | Bulk OUT (Host to Device), 64-byte packets

The device has a single Bulk OUT endpoint which expects packets of up to 64 bytes. Multiple packets may be transmitted in one LibUSB "write" operation, as long as the buffer you provide is a multiple of 64 bytes in length.

Each packet begins with an 8-bit control byte, which is divided into three bit-fields:

Bits 7..6  | Bit 5       | Bits 4..0
---------- | ----------- | ------------
Type code  | 'Final' bit | Packet index

* The 'type' code indicates what kind of packet this is.
* The 'final' bit, if set, causes the most recent group of packets to take effect
* The packet index is used to sequence packets within a particular type code

The following packet types are recognized:

Type code | Meaning of 'final' bit          | Index range | Packet contents
--------- | ------------------------------- | ----------- | -------------------------------------
0         | Interpolate to new video frame  | 0 … 24      | Up to 21 pixels, 24-bit RGB
1         | Instantly apply new color LUT   | 0 … 24      | Up to 31 16-bit lookup table entries
2         | (reserved)                      | 0           | Set configuration data
3         |                                 |             | (reserved)

In a type 0 packet, the USB packet contains up to 21 pixels of 24-bit RGB color data. The last packet (index 24) only needs to contain 8 valid pixels. Pixels 9-20 in these packets are ignored.

Byte Offset   | Description
------------- | ------------
0             | Control byte
1             | Pixel 0, Red
2             | Pixel 0, Green
3             | Pixel 0, Blue
4             | Pixel 1, Red
5             | Pixel 1, Green
6             | Pixel 1, Blue
…             | …
61            | Pixel 20, Red
62            | Pixel 20, Green
63            | Pixel 20, Blue

In a type 1 packet, the USB packet contains up to 31 lookup-table entries. The lookup table is structured as three arrays of 257 entries, starting with the entire red-channel LUT, then the green-channel LUT, then the blue-channel LUT. Each packet is structured as follows:

Byte Offset   | Description
------------- | ------------
0             | Control byte
1             | Reserved (0)
2             | LUT entry #0, low byte
3             | LUT entry #0, high byte
4             | LUT entry #1, low byte
5             | LUT entry #1, high byte
…             | …
62            | LUT entry #30, low byte
63            | LUT entry #30, high byte

A type 2 packet sets optional device-wide configuration settings:

Byte Offset | Bits   | Description
----------- | ------ | ------------
0           | 7 … 0  | Control byte
1           | 7 … 2  | (reserved)
1           | 1      | Disable keyframe interpolation
1           | 0      | Disable dithering
2 … 63      | 7 … 0  | (reserved)


Contact
-------

Micah Elizabeth Scott <<micah@scanlime.org>>

