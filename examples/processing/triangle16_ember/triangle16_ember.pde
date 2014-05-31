// April Arcus, 2014

int numParticles = 10;

OPC opc;
PImage dot;
PImage planck;
KtoRGB KtoRGB;
TriangleGrid triangle;
Particle[] particles;
float heat;

void setup()
{
  size(300, 300, P3D);
  //frameRate(10);
  heat = 8000;

  dot = loadImage("dot.png");
  KtoRGB = new KtoRGB();

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
  
  particles = new Particle[numParticles];
  
  for (int i=0; i < particles.length; i++) {
     particles[i] = new Particle(random(height),heat); 
  }

}

void draw()
{
  background(0);

  for (int i = 0; i < particles.length; i++) {
    if (particles[i].center.x < 0 || particles[i].center.x > width || 
        particles[i].center.y < 0 || particles[i].center.y > height ||
        particles[i].temperature < 800) {
      particles[i] = new Particle(height-50,heat);
    }
    particles[i].draw();
  }
}

