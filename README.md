fadecandy
=========

Fadecandy is firmware for the [Teensy 3.0](http://www.pjrc.com/store/teensy3.html), a tiny and inexpensive ARM microcontroller board.

Fadecandy drives addressable LED strips with the WS2811 and WS2812 controllers. These LED strips are common and inexpensive, available from [many suppliers](http://www.aliexpress.com/item/5M-WS2811-LED-digital-strip-60leds-m-with-60pcs-WS2811-built-in-tthe-5050-smd-rgb/635563383.html?tracelog=back_to_detail_a) for around $0.25 per pixel.

This firmware is based on Stoffregen's excellent [OctoWS2811](http://www.pjrc.com/teensy/td_libs_OctoWS2811.html) library, which pumps out serial data for these LED strips entirely using DMA. This firmware builds on Paul's work by adding:

* A high performance USB protocol
* Double-buffering
* Interpolation between keyframes
* Gamma correction
* Temporal dithering

These features add up to give *very smooth* fades and high dynamic range. Ever notice that annoying stair-stepping effect when fading LEDs from off to dim? Fadecandy avoids that using a form of [delta-sigma modulation](http://en.wikipedia.org/wiki/Delta-sigma_modulation). It rapidly wiggles each pixel's value up or down by one 8-bit step, in order to achieve 16-bit resolution for fades.

TBD
---

This is a work in progress! Things I don't know yet:

* How many LEDs will be supported per Teensy board
* Maximum frame rates
* Specific documentation for the USB protocol

Prerequisites
-------------

* The recommended ARM toolchain, from <https://code.launchpad.net/gcc-arm-embedded>
* The Teensy Loader: <http://www.pjrc.com/teensy/loader.html>
