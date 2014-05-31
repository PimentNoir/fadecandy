class Particle
{
  PVector center;
  float temperature;
  float swerve = 1.5;
  float dieoff = 1.005;
  
  Particle(float y, float temperature)
  {
    center = new PVector(random(width), y);
    this.temperature = temperature;
  }

  void draw() 
  {
    temperature /= random(dieoff-.001,dieoff+.001);
    center.y -= random(2*swerve*(temperature/heat));
    //center.x += random(-swerve,swerve);
    color rgb = KtoRGB.convert(temperature);
    int opacity = 255;
    float size = height * 0.4;
    tint(rgb, opacity);
    blendMode(ADD);
    image(dot, center.x - size/2, center.y - size/2, size, size);
  }

}

