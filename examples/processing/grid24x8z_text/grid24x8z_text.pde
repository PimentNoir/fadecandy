OPC opc;
PFont f;
PShader blur;

void setup()
{
  size(640, 360, P2D);

  // Horizontal blur, from the SepBlur Processing example
  blur = loadShader("blur.glsl");
  blur.set("blurSize", 50);
  blur.set("sigma", 8.0f);
  blur.set("horizontalPass", 1);

  // Connect to the local instance of fcserver. You can change this line to connect to another computer's fcserver
  opc = new OPC(this, "127.0.0.1", 7890);

  // Map an 8x8 grid of LEDs to the center of the window, scaled to take up most of the space
  float spacing = height / 16.0;
  opc.ledGrid8x8(0, width/2, height/2, spacing, 0, true, false);

  // Put two more 8x8 grids to the left and to the right of that one.
  opc.ledGrid8x8(64, width/2 - spacing * 8, height/2, spacing, 0, true, false);
  opc.ledGrid8x8(128, width/2 + spacing * 8, height/2, spacing, 0, true, false);

  // Create the font
  f = createFont("Futura", 200);
  textFont(f);
}

void scrollMessage(String s, float speed)
{
  int x = int( width + (millis() * -speed) % (textWidth(s) + width) );
  text(s, x, 250);  
}

void draw()
{
  background(0);
  
  fill(190, 50, 255);
  scrollMessage("Om nom Fadecandy", 0.05);
  
  filter(blur);
}

