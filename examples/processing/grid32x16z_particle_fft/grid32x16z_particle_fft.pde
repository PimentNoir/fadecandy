// Some real-time FFT! This visualizes music in the frequency domain using a
// polar-coordinate particle system. Particle size and radial distance are modulated
// using a filtered FFT. Color is sampled from an image.

import ddf.minim.analysis.*;
import ddf.minim.*;

OPC opc;
PImage dot;
PImage colors;
Minim minim;
AudioPlayer sound;
FFT fft;
float[] fftFilter;

String filename = "/Users/micah/Dropbox/music/Mark Farina - Mushroom Jazz Vol 5.mp3";

float spin = 0.001;
float radiansPerBucket = radians(2);
float decay = 0.97;
float opacity = 50;
float minSize = 0.1;
float sizeScale = 0.6;

void setup()
{
  size(600, 300, P3D);

  minim = new Minim(this); 

  // Small buffer size!
  sound = minim.loadFile(filename, 512);
  sound.loop();
  fft = new FFT(sound.bufferSize(), sound.sampleRate());
  fftFilter = new float[fft.specSize()];

  dot = loadImage("dot.png");
  colors = loadImage("colors.png");

  // Connect to the local instance of fcserver
  opc = new OPC(this, "127.0.0.1", 7890);

  opc.ledGrid8x8(0 * 64, width * 1/8, height * 1/4, height/16, 0, true);
  opc.ledGrid8x8(1 * 64, width * 3/8, height * 1/4, height/16, 0, true);
  opc.ledGrid8x8(2 * 64, width * 5/8, height * 1/4, height/16, 0, true);
  opc.ledGrid8x8(3 * 64, width * 7/8, height * 1/4, height/16, 0, true);
  opc.ledGrid8x8(4 * 64, width * 1/8, height * 3/4, height/16, 0, true);
  opc.ledGrid8x8(5 * 64, width * 3/8, height * 3/4, height/16, 0, true);
  opc.ledGrid8x8(6 * 64, width * 5/8, height * 3/4, height/16, 0, true);
  opc.ledGrid8x8(7 * 64, width * 7/8, height * 3/4, height/16, 0, true);
}

void draw()
{
  background(0);

  fft.forward(sound.mix);
  for (int i = 0; i < fftFilter.length; i++) {
    fftFilter[i] = max(fftFilter[i] * decay, log(1 + fft.getBand(i)));
  }
  
  for (int i = 0; i < fftFilter.length; i += 3) {   
    color rgb = colors.get(int(map(i, 0, fftFilter.length-1, 0, colors.width-1)), colors.height/2);
    tint(rgb, fftFilter[i] * opacity);
    blendMode(ADD);
 
    float size = height * (minSize + sizeScale * fftFilter[i]);
    PVector center = new PVector(width * (fftFilter[i] * 0.2), 0);
    center.rotate(millis() * spin + i * radiansPerBucket);
    center.add(new PVector(width * 0.5, height * 0.5));
 
    image(dot, center.x - size/2, center.y - size/2, size, size);
  }
}

