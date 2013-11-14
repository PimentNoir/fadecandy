Fadecandy Example Code
======================

This directory contains sample projects and configuration files for Fadecandy.

* `processing`
  * Examples for [Processing 2](http://processing.org/), a popular tool for creative coding
* `html`
  * Browser-based examples, written in JavaScript with HTML5
* `node`
  * JavaScript examples for Node.js
* `python`
  * Some examples written in [Python](http://python.org/)
* `config`
  * Sample JSON configuration files for `fcserver`

LED Layouts
-----------

Some examples are written for a specific LED layout or art project, some examples are general-purpose. For examples that use a specific LED layout, we indicate this with a naming convention. For example, if a project "potato" were written for a 48x24 grid, it would be named "grid48x24_potato".

Layout names we use so far:

* grid8x8 - A simple left-to-right top-to-bottom 8x8 pixel grid
  * For example, the [AdaFruit NeoMatrix](http://www.adafruit.com/products/1487)
* grid24x8z - Three 8x8 zig-zag grids side by side
  * Channel 0 is the center, and may be used alone.
  * For example, three of the 8x8 grids from [RGB-123](http://www.kickstarter.com/projects/311408456/rgb-123-led-matrices)

More Examples
-------------

There are other places to look for example code too!

* The [Open Pixel Control](https://github.com/zestyping/openpixelcontrol) project
  * Everything here will work with Fadecandy, including their [Python examples](https://github.com/zestyping/openpixelcontrol/tree/master/python_clients)
* The [Ardent Mobile Cloud Platform source code](https://github.com/ArdentHeavyIndustries/amcp-rpi)
  * The [AMCP](http://scanlime.org/2013/09/the-ardent-mobile-cloud-platform/) is the first art project to use Fadecandy.
  * It controls 2500 LEDs with five Fadecandy boards and a Raspberry Pi.
  * The software is written in Python, with a C extension to accelerate the LED effects.
  * It's controlled over Wi-fi, by a [TouchOSC](http://hexler.net/software/touchosc) remote control.
