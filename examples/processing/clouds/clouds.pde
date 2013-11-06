OPC opc;

void setup()
{
  size(640, 360);
  frameRate(15);

  // Connect to the local instance of fcserver. You can change this line to connect to another computer's fcserver
  opc = new OPC(this, "127.0.0.1", 7890);

  // Map an 8x8 grid of LEDs to the center of the window, scaled to take up most of the space
  float spacing = height / 16.0;
  opc.ledGrid8x8(0, width/2, height/2, spacing, 0);

  // Put two more 8x8 grids to the left and to the right of that one.
  opc.ledGrid8x8(64, width/2 - spacing * 8, height/2, spacing, 0);
  opc.ledGrid8x8(128, width/2 + spacing * 8, height/2, spacing, 0);

  // Make the LED grid visible on-screen. By default, the LED sampling locations
  // are hidden and don't affect Processing's output.
  opc.showLocations(true);
  
  colorMode(HSB, 100);
}

float noiseScale=0.02;

void draw() {
  loadPixels();
  for (int x=0; x < width; x++) {
    for (int y=0; y < height; y++) {
      float j = 0.01;
      pixels[x + width*y] = color((20+millis()*0.001) % 100, 50,
        -80 + 200 * noise(x*j, y*j, millis()*0.0001));
    }
  }
  updatePixels();
}

