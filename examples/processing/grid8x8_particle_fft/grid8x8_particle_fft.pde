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
AudioInput in;
AudioRecorder recorder;
FFT fft;
float[] fftFilter;

//String filename = "083_trippy-ringysnarebeat-3bars.mp3";
String filename = "12-amon_tobin--dropped_from_the_sky-oma.mp3";
float spin = 0.0001;
float radiansPerBucket = radians(4);
float decay = 0.97;
float opacity = 50;
float minSize = 0.125;
float sizeScale = 1;

float zoom = 4;

void setup()
{
  size((int)zoom*100, (int)zoom*100, P3D);
  colorMode(HSB,100);
  //TODO: make framerate depend on beat detection
  int framerate = 60;
  frameRate(framerate);
  
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
  
  opc.ledGrid8x8(0 * 64, width * 1/2, height * 1/2, height/16, 0, false);
  
  // Make the status LED quiet
  opc.setStatusLed(false);
  
  // Hide or show leds location
  opc.showLocations(true);
  
  smooth();
}

void keyPressed() {
  if (key == 'd') opc.setDithering(false);
  if (key == ']') zoom *= 1.1;
  if (key == '[') zoom *= 0.9;
}

void keyReleased() {
  if (key == 'd') opc.setDithering(true);
}

void draw()
{
  background(0);
  //FIXME: recenter after zooming
  //scale(width/2 * zoom, height/2 * zoom);
  
  fft.forward(sound.mix);
  for (int i = 0; i < fftFilter.length; i++) {
    fftFilter[i] = max(fftFilter[i] * decay, log(1 + fft.getBand(i)));
  }
  
  for (int i = 0; i < fftFilter.length; i += 1) { 
    float pulse = (sin(fftFilter[i] - 0.3) * 0.3); 
    color rgb = colors.get(int(map(i, 0, fftFilter.length-1, 0, colors.width-1)), colors.height/2);
    tint(rgb, fftFilter[i] * opacity);
    blendMode(ADD);
    
    float size_pulse = abs(fftFilter[i] * pulse); 
    float size = height * (minSize + sizeScale * size_pulse);
    float centerx = width * fftFilter[i] * noise(millis() * fftFilter[i] * pulse * 0.000125) * 1.125; 
    float centery = height * fftFilter[i] * noise(millis() * fftFilter[i] * pulse * 0.000125) * 1.125;
    PVector center = new PVector(centerx * 1/2, centery * 1/2);
    center.rotate(millis() * spin + i * radiansPerBucket);
    center.add(new PVector(width * 1/2, height * 1/2));
        
    image(dot, center.x - size/2, center.y - size/2, size, size);
  }
}

