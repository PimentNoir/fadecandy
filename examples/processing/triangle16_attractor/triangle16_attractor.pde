// Particle system with attraction to each corner of the triangle.
// Spawns centered around a random point, lives out a cycle and dies; the cycle repeats.

int numParticles = 20;
float cornerCoefficient = 0.2;
int integrationSteps = 20;
float maxOpacity = 100;
float epochStep = 0.002;

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
  float s = 0.3;
  float cx = width * (0.5 + random(-s, s));
  float cy = height * (0.5 + random(-s, s));
 
  // Half-width of particle bundle
  float w = width * 0.02;
  
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
  epoch += epochStep;
  
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

