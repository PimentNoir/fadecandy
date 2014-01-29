class Particle
{
  PVector center;
  PVector velocity;
  float size;
  float hue;

  Particle(float x, float y)
  {
    center = new PVector(x, y);
    velocity = new PVector(0, 0);
 
    size = height * 0.7;
    hue = random(255);
  }
  
  void damp(float factor)
  {
    velocity.mult(factor);
  }

  void integrate()
  {
    center.add(velocity);
  }

  void draw(float opacity) 
  {
    tint(hue, 80, opacity);
    blendMode(ADD);
    image(dot, center.x - size/2, center.y - size/2, size, size);
  }  
  
  void attract(PVector v, float coefficient)
  {
    PVector d = PVector.sub(v, center);
    d.mult(coefficient / max(1, d.magSq()));
    velocity.add(d);
  }
}

