class Particle
{
  PVector center;
  PVector velocity;
  color rgb;

  Particle(float x, float y, color rgb)
  {
    center = new PVector(x, y);
    velocity = new PVector(0, 0); 
    this.rgb = rgb;
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
    float size = height * 0.6;
    tint(rgb, opacity);
    blendMode(ADD);
    image(dot, center.x - size/2, center.y - size/2, size, size);
  }  
  
  void attract(PVector v, float coefficient)
  {
    PVector d = PVector.sub(v, center);
    d.mult(coefficient / max(1, d.magSq()));
    velocity.add(d);
  }

  float energy()
  {
    return velocity.magSq();
  }
}

