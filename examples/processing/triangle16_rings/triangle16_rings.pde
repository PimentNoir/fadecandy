OPC opc;
TriangleGrid triangle;
float dx, dy, dz;

void setup()
{
  int zoom = 4;
  size(20*zoom, 20*zoom);

  // Slower more regular frame rate; Since we animate slowly, this can make the colors a little
  // smoother since the Fadecandy board interpolates frames with 16-bit precision whereas our
  // processing sketch only has 8-bit percision per channel.
  frameRate(12);

  // Connect to the local instance of fcserver. You can change this line to connect to another computer's fcserver
  opc = new OPC(this, "127.0.0.1", 7890);

  // Map our triangle grid to the center of the window
  triangle = new TriangleGrid();
  triangle.grid16();
  triangle.mirror();
  triangle.rotate(radians(60));
  triangle.scale(height * 0.2);
  triangle.translate(width * 0.5, height * 0.57);
  triangle.leds(opc, 0);

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
  float speed = 0.0005;
  float zspeed = 0.01;
  float angle = sin(now * 0.001);
  float z = now * 0.00008;
  float hue = now * 0.01;
  float scale = 0.005;

  float saturation = 100 * constrain(pow(1.15 * noise(now * 0.000122), 2.5), 0, 1);
  float spacing = noise(now * 0.000124) * 0.1;

  dx += cos(angle) * speed;
  dy += sin(angle) * speed;
  dz += (noise(now * 0.000014) - 0.5) * zspeed;

  float centerx = noise(now *  0.00001) * 1.25 * width;
  float centery = noise(now * -0.00001) * 1.25 * height;

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
         100 * constrain(pow(3.0 * n, 1.5), 0, 0.9)
         );
      
      pixels[x + width*y] = c;
    }
  }
  updatePixels();
}

