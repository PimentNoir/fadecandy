Fadecandy HTML5 Examples
========================

This directory contains examples for Fadecandy written in JavaScript with HTML5.

Stable Examples
---------------

* `grid8x8_dots`
  * A springy trail of glowing dots that follows the mouse
  * Uses [Paper.js](http://paperjs.org/)
  * Set up for an 8x8 grid in left-right top-down order
  * Browser support
    * **Firefox**: Works great
    * **Chrome**: Okay, but not as smooth as Firefox
    * **Safari**: Stops running after a couple seconds 

Additional Examples
-------------------

* `grid24x8z_dots` -- Modification of grid8x8 dots for the grid24x8z layout
* `grid24x8z_dots_leapmotion` -- Very experimental example with Leap Motion control

See Also
--------
 
* The fcserver Web UI
   * Uses the JSON interface to `fcserver`
   * Illustrates device hotplug, and bypassing the OPC pixel mapping
   * Try it out at [http://localhost:7890](http://localhost:7890) when fcserver is running
   * Source code at [fadecandy/server/http/js/home.js](https://github.com/scanlime/fadecandy/blob/master/server/http/js/home.js)
