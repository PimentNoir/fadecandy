// A modified version of the "rings" example that's more suitable
// as a light source. It changes more slowly, and never goes
// completely dark.

OPC opc;
float dx, dy, dz;

void setup()
{
  int zoom = 4;
  size(32*zoom, 16*zoom);

  // Connect to the local instance of fcserver. You can change this line to connect to another computer's fcserver
  opc = new OPC(this, "127.0.0.1", 7890);

  opc.ledGrid8x8(0 * 64, width * 1/8, height * 1/4, height/16, 0, true, false);
  opc.ledGrid8x8(1 * 64, width * 3/8, height * 1/4, height/16, 0, true, false);
  opc.ledGrid8x8(2 * 64, width * 5/8, height * 1/4, height/16, 0, true, false);
  opc.ledGrid8x8(3 * 64, width * 7/8, height * 1/4, height/16, 0, true, false);
  opc.ledGrid8x8(4 * 64, width * 1/8, height * 3/4, height/16, 0, true, false);
  opc.ledGrid8x8(5 * 64, width * 3/8, height * 3/4, height/16, 0, true, false);
  opc.ledGrid8x8(6 * 64, width * 5/8, height * 3/4, height/16, 0, true, false);
  opc.ledGrid8x8(7 * 64, width * 7/8, height * 3/4, height/16, 0, true, false);

  // Make the status LED quiet
  opc.setStatusLed(false);
  
  colorMode(HSB, 100);
}

float noiseScale=0.02;

float fractalNoise(float x, float y, float z) {
  float r = 0;
  float amp = 1.0;
  for (int octave = 0; octave < 4; octave++) {
    r += noise(x, y, z) * amp;
    amp /= 2;
    x *= 2;
    y *= 2;
    z *= 2;
  }
  return r;
}

void draw() {
  long now = millis();
  float speed = 0.002;
  float zspeed = 0.1;
  float angle = sin(now * 0.001);
  float z = now * 0.00008;
  float hue = now * 0.001;
  float scale = 0.005;

  float saturation = 100 * constrain(pow(1.05 * noise(now * 0.000122), 2.5), 0, 1);
  float spacing = noise(now * 0.000124) * 0.1;

  dx += cos(angle) * speed;
  dy += sin(angle) * speed;
  dz += (noise(now * 0.000014) - 0.5) * zspeed;

  float centerx = noise(now *  0.000125) * 1.25 * width;
  float centery = noise(now * -0.000125) * 1.25 * height;

  loadPixels();
  for (int x=0; x < width; x++) {
    for (int y=0; y < height; y++) {
     
      float dist = sqrt(pow(x - centerx, 2) + pow(y - centery, 2));
      float pulse = (sin(dz + dist * spacing) - 0.3) * 0.3;
      
      float n = fractalNoise(dx + x*scale + pulse, dy + y*scale, z) - 0.75;
      float m = fractalNoise(dx + x*scale, dy + y*scale, z + 10.0) - 0.75;

      color c = color(
         (hue + 40.0 * m) % 100.0,
         saturation,
         100 * constrain(pow(1.0 * max(0, n) + 0.3, 1.5), 0, 0.9)
         );
      
      pixels[x + width*y] = c;
    }
  }
  updatePixels();
}

