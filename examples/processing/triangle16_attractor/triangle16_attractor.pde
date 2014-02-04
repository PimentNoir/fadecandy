// Particle system with attraction to each corner of the triangle.
// Spawns centered around a random point, lives out a cycle and dies; the cycle repeats.

int numParticles = 10;
float cornerCoefficient = 0.2;
int integrationSteps = 20;
float maxOpacity = 100;
float stepFast = 1.0 / 40;
float stepSlow = 1.0 / 1000;

OPC opc;
PImage dot;
PImage colors;
TriangleGrid triangle;
Particle[] particles;
PVector[] corners;
float epoch = 0;

void setup()
{
  size(300, 300, P3D);
  frameRate(30);

  dot = loadImage("dot.png");
  colors = loadImage("colors.png");
  colors.loadPixels();

  // Connect to the local instance of fcserver
  opc = new OPC(this, "127.0.0.1", 7890);

  // Map our triangle grid to the center of the window
  triangle = new TriangleGrid();
  triangle.grid16();
  triangle.mirror();
  triangle.rotate(radians(60));
  triangle.scale(height * 0.2);
  triangle.translate(width * 0.5, height * 0.5);
  triangle.leds(opc, 0);

  corners = new PVector[3];
  corners[0] = triangle.cells[0].center;
  corners[1] = triangle.cells[6].center;
  corners[2] = triangle.cells[15].center;

  beginEpoch();
}

void beginEpoch()
{
  epoch = 0;
 
  // Center of bundle
  float s = 0.5;
  float cx = width * (0.5 + random(-s, s));
  float cy = height * (0.5 + random(-s, s));
 
  // Half-width of particle bundle
  float w = width * 0.2;
 
  particles = new Particle[numParticles];
  for (int i = 0; i < particles.length; i++) {
    color rgb = colors.pixels[int(random(0, colors.width * colors.height))];
    particles[i] = new Particle(
      cx + random(-w, w),
      cy + random(-w, w), rgb);
  }
}

void draw()
{
  background(0);
  
  // How much energy is still left?
  float energy = 0;
  for (int i = 0; i < particles.length; i++) {
    energy += particles[i].energy();
  }
  
  // How bright is our brightest pixel?
  float brightness = 0;
  for (int i = 0; i < opc.pixelLocations.length; i++) {
    color rgb = opc.getPixel(i);
    brightness = max(brightness, max(red(rgb), max(blue(rgb), green(rgb))));
  }
  brightness /= 255.0;
  
  text("Energy: " + energy, 2, 12);
  text("Brightness: " + brightness, 2, 25);

  // What's interesting? Can we maintain high brightness and high energy?
  // These are normally conflicting goals. If we've managed to balance the two,
  // keep going to see how it turns out.
  if (energy > 1.5 && brightness > 0.8) {
 
    // Time moves slower when we're interested
    epoch += stepSlow;
    text("+", 2, 40);
  } else {
    epoch += stepFast;
  }
  
  if (epoch > 1) {
    beginEpoch();
  }
    
  for (int step = 0; step < integrationSteps; step++) {
    for (int i = 0; i < particles.length; i++) {
      particles[i].integrate();

      // Each particle is attracted by the corners
      for (int j = 0; j < corners.length; j++) {
        particles[i].attract(corners[j], cornerCoefficient);
      }
    }
  }

  for (int i = 0; i < particles.length; i++) {
    particles[i].draw(sin(epoch * PI) * maxOpacity);
  }
}

