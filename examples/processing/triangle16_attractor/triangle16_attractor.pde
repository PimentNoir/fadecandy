// Particle system with attraction to each corner of the triangle.
// Spawns centered around a random point, lives out a cycle and dies; the cycle repeats.

int numParticles = 20;
float cornerCoefficient = 0.2;
int integrationSteps = 20;
float maxOpacity = 30;
float epochStep = 0.005;

OPC opc;
PImage dot;
TriangleGrid triangle;
Particle[] particles;
PVector[] corners;
float epoch = 0;

void setup()
{
  size(300, 300, P3D);
  frameRate(30);

  // Load a sample image
  dot = loadImage("dot.png");

  // Connect to the local instance of fcserver
  opc = new OPC(this, "127.0.0.1", 7890);

  // Map our triangle grid to the center of the window
  triangle = new TriangleGrid();
  triangle.grid16();
  triangle.mirror();
  triangle.rotate(radians(60));
  triangle.scale(height * 0.2);
  triangle.translate(width * 0.5, height * 0.6);
  triangle.leds(opc, 0);
  
  colorMode(HSB, 255);

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
  float cx = width * random(-0.2, 1.2);
  float cy = width * random(-0.2, 1.2);
 
  // Half-width of particle bundle
  float w = width * 0.1;
  
  particles = new Particle[numParticles];
  for (int i = 0; i < particles.length; i++) {
    particles[i] = new Particle(
      cx + random(-w, w),
      cy + random(-w, w));
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

