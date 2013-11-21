![Fadecandy Title](https://raw.github.com/scanlime/fadecandy/master/doc/images/fc-title.png)

Fadecandy is a project that makes LED art easier, tastier, and more creative. We're all about creating tools that remove the technical drudgery from making LED art, freeing you to do more interesting, nuanced, and creative things. We think LEDs are more than just trendy display devices, we think of them as programmable light for interactive art.

* [Video Introduction](https://vimeo.com/79935649)
* [Tutorial: LED Art with Fadecandy](http://learn.adafruit.com/led-art-with-fadecandy)

Simple Example
--------------

Here's a simple project, a single LED strip controlled by a Processing sketch running on your laptop:

![Fadecandy system diagram 1](https://raw.github.com/scanlime/fadecandy/master/doc/images/system-diagram-1.png)

```
// Simple Processing sketch for controlling a 64-LED strip.
// A glowing color-blob appears on the strip under mouse control.

OPC opc;
PImage dot;

void setup()
{
  size(800, 200);

  // Load a sample image
  dot = loadImage("color-dot.png");

  // Connect to the local instance of fcserver
  opc = new OPC(this, "127.0.0.1", 7890);

  // Map one 64-LED strip to the center of the window
  opc.ledStrip(0, 64, width/2, height/2, width / 70.0, 0, false);
}

void draw()
{
  background(0);

  // Draw the image, centered at the mouse location
  float dotSize = width * 0.2;
  image(dot, mouseX - dotSize/2, mouseY - dotSize/2, dotSize, dotSize);
}
```

A More Complex Example
----------------------

Fadecandy is also useful for larger projects with many thousands of LEDs, and it's useful for art that runs on embedded computers like the Raspberry Pi:

![Fadecandy system diagram 2](https://raw.github.com/scanlime/fadecandy/master/doc/images/system-diagram-2.png)

Project Scope
-------------

This project is a collection of reusable pieces you can take or leave. The overall goal of making LED art easier, tastier, and more creative is a broad one. To keep this project manageable to start with, there are some rough limitations on what's supported:

* LED strips, grids, and other modules based on the WS2811 or WS2812 chip.
  * Common and inexpensive, available from [many suppliers](http://www.aliexpress.com/item/5M-WS2811-LED-digital-strip-60leds-m-with-60pcs-WS2811-built-in-tthe-5050-smd-rgb/635563383.html?tracelog=back_to_detail_a) for around $0.25 per pixel.
* Something with a USB host port that can run your art.
  * Laptops and Mac Minis work great
  * The Raspberry Pi is also a great platform for this, but it requires more technical skill
* Distances of no more than 60ft or so between your computer and your farthest LEDs
  * USB cables < 50ft
  * WS2811 data cables < 10ft
* No more than about 10000 LED pixels total
  * There's no hard limit, but it gets more difficult after this point.

These are fuzzy limitations based on current software capabilities and rough electrical limits, so you may be able to stretch them. But this gives you an idea about the kind of art we try to support. Projects are generally larger than wearables, but smaller than entire buildings.

For example, the first project to use Fadecandy was the [Ardent Mobile Cloud Platform](http://scanlime.org/2013/09/the-ardent-mobile-cloud-platform/) at Burning Man 2013. This project used one Raspberry Pi, five Fadecandy controller boards, and 2500 LEDs.

Fadecandy makes no assumptions about how you generate control patterns for the LEDs. You can generate a 2D video and sample pixels from the video, you can make a 3D model of your sculpture and sample a 3D *shader* for your pixel values, or you can create a unique system specifically for your art.

Fadecandy Controller
--------------------

The centerpiece of the Fadecandy project is a controller board which can drive up to 512 LEDs (as 8 strings of 64) over USB. Many Fadecandy boards can be attached to the same computer using a USB hub or a chain of hubs.

Fadecandy makes it easy to drive these LEDs from anything with USB, and it includes unique algorithms which eliminate many of the common visual glitches you see when using these LEDs.

The LED drive engine is based on Stoffregen's excellent [OctoWS2811](http://www.pjrc.com/teensy/td_libs_OctoWS2811.html) library, which pumps out serial data for these LED strips entirely using DMA. This firmware builds on Paul's work by adding:

* A high performance USB protocol
* Zero copy architecture with triple-buffering
* Interpolation between keyframes
* Gamma and color correction with per-channel lookup tables
* Temporal dithering
* A custom PCB with line drivers and a 5V boost converter
* A fully open source bootloader

These features add up to give *very smooth* fades and high dynamic range. Ever notice that annoying stair-stepping effect when fading LEDs from off to dim? Fadecandy avoids that using a form of [delta-sigma modulation](http://en.wikipedia.org/wiki/Delta-sigma_modulation). It rapidly wiggles each pixel's value up or down by one 8-bit step, in order to achieve 16-bit resolution for fades.

Vitals
------

* 512 pixels supported per Teensy board (8 strings, 64 pixels per string)
* Very high hardware frame rate (~400 FPS) to support temporal dithering
* Full-speed (12 Mbps) USB
* 257x3-entry 16-bit color lookup table, for gamma correction and color balance

Platform
--------

Fadecandy uses the Freescale MK20DX128 microcontroller, the same one used by the [Teensy 3.0](http://www.pjrc.com/store/teensy3.html) board. Fadecandy includes its own PCB design featuring a robust power supply and level shifters. It also includes an open source bootloader compatible with the USB Device Firmware Update spec.

You can use Fadecandy either as a full hardware platform or as firmware for the Teensy 3.0 board.

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

WebSockets Server
-----------------

In addition to Open Pixel Control, `fcserver` also supports WebSockets, so you can write LED art algorithms and utilities in Javascript using the plethora of libraries and tools available on that platform. LED art can be easily integrated with any input device or library that will run in the browser or Node.

Browser UI
----------

When you run `fcserver`, it also gives you a simple browser-based UI for identifying the attached Fadecandy Controllers and quickly testing your lights. By default, this UI runs on [http://localhost:7890](http://localhost:7890).

![Browser UI Screenshot](https://raw.github.com/scanlime/fadecandy/master/doc/images/web-ui-screenshot.png)

Where to?
---------

* Sample projects are in [examples](https://github.com/scanlime/fadecandy/tree/master/examples)
* Pre-compiled binaries are in [bin](https://github.com/scanlime/fadecandy/tree/master/bin)
* More documentation is included in [doc](https://github.com/scanlime/fadecandy/tree/master/doc)
  * [Open Pixel Control Protocol](https://github.com/scanlime/fadecandy/blob/master/doc/fc_protocol_opc.md)
  * [WebSocket Protocol](https://github.com/scanlime/fadecandy/blob/master/doc/fc_protocol_websocket.md)
  * [USB Protocol](https://github.com/scanlime/fadecandy/blob/master/doc/fc_protocol_usb.md)
  * [Processing OPC Client](https://github.com/scanlime/fadecandy/blob/master/doc/processing_opc_client.md)
  * [Server Configuration](https://github.com/scanlime/fadecandy/blob/master/doc/fc_server_config.md)


Contact
-------

* [Discussion group](https://groups.google.com/forum/#!forum/fadecandy)
* Micah Elizabeth Scott <<micah@scanlime.org>>

