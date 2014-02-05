// Some real-time FFT! This visualizes music in the frequency domain using a
// polar-coordinate particle system. Particle size and radial distance are modulated
// using a filtered FFT. Color is sampled from an image.

import ddf.minim.analysis.*;
import ddf.minim.*;

OPC opc;
PImage dot;
PImage colors;
TriangleGrid triangle;
Minim minim;
AudioPlayer sound;
FFT fft;
float[] fftFilter;

String filename = "workshop-lowfi.mp3";
float spin = 0.001;
float decay = 0.98;
float opacity = 4;
float meanBrightness = 200;

void setup()
{
  size(300, 300, P3D);

  minim = new Minim(this); 

  // Small buffer size!
  sound = minim.loadFile(filename, 256);
  sound.loop();
  fft = new FFT(sound.bufferSize(), sound.sampleRate());
  fftFilter = new float[fft.specSize()];

  dot = loadImage("dot.png");
  colors = loadImage("colors.png");

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
}

void draw()
{
  background(0);

  fft.forward(sound.mix);
  for (int i = 0; i < fftFilter.length; i++) {
    fftFilter[i] = max(fftFilter[i] * decay, log(1 + fft.getBand(i)));
  }
 
  // How bright is our brightest pixel?
  float brightness = 0;
  for (int i = 0; i < opc.pixelLocations.length; i++) {
    color rgb = opc.getPixel(i);
    brightness = max(brightness, max(red(rgb), max(blue(rgb), green(rgb))));
  }

  // Automatic brightness control
  opacity += (meanBrightness - brightness) * 0.001;
 
  for (int i = 0; i < fftFilter.length; i += 3) {
    
    color rgb = colors.get(int(map(i, 0, fftFilter.length-1, 0, colors.width-1)), colors.height/2);
    tint(rgb, fftFilter[i] * opacity);
    blendMode(ADD);
 
    float size = height * (0.1 + 0.4 * fftFilter[i]);
    PVector center = new PVector(width * (fftFilter[i] * 0.2), 0);
    center.rotate(millis() * spin + map(i, 0, fftFilter.length, 0, 4*PI));
    center.add(new PVector(width * 0.5, height * 0.5));
 
    image(dot, center.x - size/2, center.y - size/2, size, size);
  }
}

