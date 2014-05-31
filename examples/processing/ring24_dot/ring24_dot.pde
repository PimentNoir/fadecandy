OPC opc;
PImage dot;

void setup()
{
  size(500, 500);

  // Load a sample image
  dot = loadImage("dot.png");

  // Connect to the local instance of fcserver
  opc = new OPC(this, "127.0.0.1", 7890);

  // Map one 24-LED ring to the center of the window
  opc.ledRing(0, 24, width/2, height/2, width*0.18, 0);
}

void draw()
{
  background(0);

  // Draw the image, centered at the mouse location
  float dotSize = width * 0.6;
  image(dot, mouseX - dotSize/2, mouseY - dotSize/2, dotSize, dotSize);
}

