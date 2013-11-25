import de.voidplus.leapmotion.*;

LeapMotion leap;
int lastId = -1;
float lastX, lastY;

class Ring
{
  float x, y, size, intensity, hue;

  void respawn(float x1, float y1, float x2, float y2)
  {
    // Start at the newer mouse position
    x = x2;
    y = y2;
    
    // Intensity is just the distance between mouse points
    intensity = dist(x1, y1, x2, y2);
    
    // Hue is the angle of mouse movement, scaled from -PI..PI to 0..100
    hue = map(atan2(y2 - y1, x2 - x1), -PI, PI, 0, 100);

  // Default size is based on the screen size
    size = height * 0.1;
  }

  void draw()
  {
    // Particles fade each frame
    intensity *= 0.95;
    
    // They grow at a rate based on their intensity
    size += height * intensity * 0.01;

    // If the particle is still alive, draw it
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
boolean f = false;

void setup()
{
  size(640, 320, P3D);
  colorMode(HSB, 100);
  texture = loadImage("ring.png");

  leap = new LeapMotion(this);

  opc = new OPC(this, "127.0.0.1", 7890);
  opc.ledGrid8x8(0 * 64, width * 1/8, height * 1/4, height/16, 0, true);
  opc.ledGrid8x8(1 * 64, width * 3/8, height * 1/4, height/16, 0, true);
  opc.ledGrid8x8(2 * 64, width * 5/8, height * 1/4, height/16, 0, true);
  opc.ledGrid8x8(3 * 64, width * 7/8, height * 1/4, height/16, 0, true);
  opc.ledGrid8x8(4 * 64, width * 1/8, height * 3/4, height/16, 0, true);
  opc.ledGrid8x8(5 * 64, width * 3/8, height * 3/4, height/16, 0, true);
  opc.ledGrid8x8(6 * 64, width * 5/8, height * 3/4, height/16, 0, true);
  opc.ledGrid8x8(7 * 64, width * 7/8, height * 3/4, height/16, 0, true);

  // We can have up to 100 rings. They all start out invisible.
  rings = new Ring[100];
  for (int i = 0; i < rings.length; i++) {
    rings[i] = new Ring();
  }
}

void draw()
{
  background(0);

  Finger f = leap.getFrontFinger();

  if (f != null) {
    PVector position = f.getPosition();

    float x = (position.x - 200) * width / 300;
    float y = (position.y - 100) * width / 100;

    float smoothX = lastX + (x - lastX) * 0.2;
    float smoothY = lastY + (y - lastY) * 0.2;

    if (f.getId() == lastId && lastId >= 0) {
      rings[int(random(rings.length))].respawn(lastX, lastY, smoothX, smoothY);
    }
     
    lastX = lastId < 0 ? x : smoothX;
    lastY = lastId < 0 ? y : smoothY;
    lastId = f.getId();
  } else {
    lastId = -1;
  }

  // Give each ring a chance to redraw and update
  for (int i = 0; i < rings.length; i++) {
    rings[i].draw();
  }
}

