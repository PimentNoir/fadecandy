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


void drawDot(float angle, float distance, float size)
{
  image(dot, width/2 - distance * sin(angle) - size/2,
    height/2 - distance * cos(angle) - size/2, size, size);
}

void draw()
{
  background(0);

  float a = millis();

  blendMode(ADD);
  tint(40, 100, 40);
  drawDot(a * -0.002, width*0.1, width*0.6);
  tint(155, 155, 155);
  drawDot(a * -0.003, width*0.1, width*0.6);
  tint(90, 90, 155);
  drawDot(a *  0.001, width*0.1, width*0.6);
}

