// Demonstration that pokes colors into the LEDs directly instead
// of mapping them to pixels on the screen. You may want to do this
// if you have your own mapping scheme that doesn't fit well with
// a 2D display.

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

