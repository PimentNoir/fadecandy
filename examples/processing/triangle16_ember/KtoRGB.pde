class KtoRGB
{
  PImage ramp;
  float min = 1000;
  float max = 10000;
  
  KtoRGB(){
    ramp = loadImage("bb-rampcomp.png");
    ramp.loadPixels();
  }
  
  color convert(float Kelvins) {
    return ramp.pixels[int(constrain(map(Kelvins,min,max,0,ramp.width),0,ramp.width-1))];
  }
};
