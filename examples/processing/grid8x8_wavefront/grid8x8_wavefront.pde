class Ring
{
  float x, y, size, intensity, hue;

  void respawn(float x1, float y1, float x2, float y2) {
    x = x2;
    y = y2;
    intensity = dist(x1, y1, x2, y2);
    hue = map(atan2(y2 - y1, x2 - x1), -PI, PI, 0, 100);
    size = height * 0.1;
  }

  void draw() {
    intensity *= 0.95;
    size += height * intensity * 0.01;

    if (intensity >= 1) {
      blendMode(ADD);
      tint(hue, 50, intensity);
      image(texture, x - size/2, y - size/2, size, size);
    }
  }
};

OPC opc;
PImage texture;
Ring rings[];
float smoothX, smoothY;

void setup()
{
  size(500, 500, P3D);
  colorMode(HSB, 100);
  texture = loadImage("ring.png");

  opc = new OPC(this, "127.0.0.1", 7890);
  opc.ledGrid8x8(0, width/2, height/2, height / 16.0, 0, false);

  rings = new Ring[100];
  for (int i = 0; i < rings.length; i++) {
    rings[i] = new Ring();
  }
}

void draw() {
  background(0);

  float prevX = smoothX;
  float prevY = smoothY;
  smoothX += (mouseX - smoothX) * 0.1;
  smoothY += (mouseY - smoothY) * 0.1;

  rings[int(random(rings.length))].respawn(prevX, prevY, smoothX, smoothY);

  for (int i = 0; i < rings.length; i++) {
    rings[i].draw();
  }
}

