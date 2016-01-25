Open Pixel Control for Processing
=================================

The Processing examples included with Fadecandy use a very simple Open Pixel Control library, `OPC.pde`. A copy of this library is included with each example, and it's easy to get it into your own sketches by saving a copy of an example or copying the `template` sketch.

LED Mapping Example
-------------------

This OPC object makes it easy to map LEDs to specific locations on the screen. When you draw to the screen, the LEDs automatically update. The `strip64_dot` example sketch illustrates this:

```
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

Direct Control Example
----------------------

Alternatively, you can control the LEDs directly. Instead of automatically updating after every draw(), the LED colors are set explicitly and a packet is explicitly sent for every frame. This is illustrated in the `strip64_unmapped` example:

```
OPC opc;

void setup()
{
  opc = new OPC(this, "127.0.0.1", 7890);
  frameRate(10);
  colorMode(HSB, 100);
}

void draw()
{
  // RAINBOW FADE!!!!!
  for (int i = 0; i < 64; i++) {
    float hue = (millis() * 0.01 + i * 2.0) % 100; 
    opc.setPixel(i, color(hue, 20, 80));
  }

  // When you haven't assigned any LEDs to pixels, you have to explicitly
  // write them to the server. Otherwise, this happens automatically after draw().
  opc.writePixels();
}
```

Creating an OPC Object
----------------------

The examples above create an OPC object by first declaring a variable:

```
OPC opc;
```

Then, in **setup()**, they use *new* to create a new OPC instance:

```
opc = new OPC(this, "127.0.0.1", 7890);
```

The first parameter is the parent object. For a normal sketch, pass *this* as the first parameter. The next two parameters are the server name and port to connect to. This connects to fcserver on the same computer. If you've configured fcserver to allow connections from other computers, you can run your Processing sketch and fcserver on different machines.

Function Reference
------------------

This section lists all public methods available on the OPC object:

----

* **opc.led**(*index*, *x*, *y*)
  * Set the location of a single LED
  * *index*: LED number, starting with zero
  * *x*, *y*: Location on the screen, in pixels

----

* **opc.ledStrip**(*index*, *count*, *x*, *y*, *spacing*, *angle*, *reversed*)
  * Place a rigid strip of LEDs on the screen
  * *index*: Number for the first LED in the strip, starting with zero
  * *count*: How many LEDs are in the strip?
  * *x*, *y*: Center location, in pixels
  * *spacing*: Spacing between LEDs, in pixels
  * *angle*: Angle, in radians. Positive is clockwise, 0 is to the right.
  * *reversed*: true = Right to left, false = Left to right

----

* **opc.ledGrid**(*index*, *stripLength*, *numStrips*, *x*, *y*, *ledSpacing*, *stripSpacing*, *angle*, *zigzag*, *flip*)
  * Place a rigid grid of LEDs on the screen
  * *index*: Number for the first LED in the grid, starting with zero
  * *stripLength*: How long is each strip in the grid?
  * *numStrips*: How many strips of LEDs make up the grid?
  * *x*, *y*: Center location, in pixels
  * *ledSpacing*: Spacing between LEDs, in pixels
  * *stripSpacing*: Spacing between strips, in pixels
  * *angle*: Angle, in radians. Positive is clockwise. 0 has pixels in a strip going left-to-right and strips going top-to-bottom.
  * *zigzag*: true = Every other strip is reversed, false = All strips are non-reversed
  * *flip*: true = All strips are reversed, false = All strips are non-reversed
 
----

* **opc.ledGrid8x8**(*index*, *x*, *y*, *spacing*, *angle*, *zigzag*, *flip*)
  * Convenience method for placing an 8x8 grid of LEDs on the screen
  * *index*: Number for the first LED in the grid, starting with zero
  * *x*, *y*: Center location, in pixels
  * *spacing*: Spacing between LEDs and strips, in pixels
  * *angle*: Angle, in radians. Positive is clockwise. 0 has pixels in a strip going left-to-right and strips going top-to-bottom.
  * *zigzag*: true = Every other strip is reversed, false = All strips are non-reversed
  * *flip*: true = All strips are reversed, false = All strips are non-reversed
  
----

* **opc.showLocations**(*enabled*)
  * By default, the OPC client indicates pixels we're sampling for LED values by drawing small dots.
  * *enabled*: true = Draw sampling locations as small dots, false = Do not draw sampling locations
  
----

* **opc.setPixel**(*index*, *color*)
  * Sets a single LED pixel to the indicated color
  * Only useful for LEDs that were never mapped to on-screen pixels
  * **index**: The index number for this LED, starting with zero
  * **color**: A Processing color object (24-bit)

----

* *color* = **opc.getPixel**(*index*)
  * Retrieves the color stored for a single LED pixel.
  * For LEDs mapped to on-screen pixels, this returns the color we sampled on the previous frame.
  * **index**: The index number for this LED, starting with zero
  * **color**: A Processing color object (24-bit)

----

* **opc.writePixels**()
  * Send all buffered pixels to the OPC server
  * If any pixels have been mapped on-screen, this happens automatically after every draw()
  * This is only necessary for sketches that have no pixel mapping at all.

----

* **opc.setDithering**(*enabled*)
  * By default, Fadecandy controller boards use a temporal dithering algorithm to reduce "popping" artifacts and retain color purity even when brightness changes.
  * *enabled*: true = All attached Fadecandy controller boards use dithering, false = Dithering is disabled.
  
----

* **opc.setInterpolation**(*enabled*)
  * By default, Fadecandy interpolates smoothly in hardware between the frames it receives from a sketch.
  * *enabled*: true = Frame interpolation is enabled, false = No frame interpolation, new frames take effect immediately.

----

* **opc.setStatusLed**(*on*)
  * Each Fadecandy Controller has a built-in status LED. This function takes manual control over the LEDs on each attached board, setting the LED on or off.
  * By default, LEDs blink when the board receives a new data. This default functionality can be restored with **opc.statusLedAuto()**.
  * *on*: true = Status LED turns on, false = Status LED turns off

----

* **opc.statusLedAuto**()
  * Each connected Fadecandy board's status LED will go back to its default behavior, blinking any time new data is received over USB.

----

* **opc.setColorCorrection**(*gamma*, *red*, *green*, *blue*)
* **opc.setColorCorrection**(*json*)
  * Sends new color correction parameters to all attached Fadecandy controllers.
  * These parameters override the defaults from fcserver's configuration file.
  * **gamma**: Exponent for the nonlinear brightness curve. Default is 2.5
  * **red**, **green**, **blue**: Floating point brightness values for red, green, and blue channels. These can adjust both the overall brightness and the whitepoint. Defaults are 1.0. 
  * **json**: Alternatively, a raw JSON string can be supplied. This string must be in the same format as the fcserver's *color* configuration parameter.
