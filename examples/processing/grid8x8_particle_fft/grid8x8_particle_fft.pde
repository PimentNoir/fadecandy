// Some real-time FFT! This visualizes music in the frequency domain using a
// polar-coordinate particle system. Particle size and radial distance are modulated
// using a filtered FFT. Color is sampled from an image.

import ddf.minim.analysis.*;
import ddf.minim.*;

OPC opc;
PImage dot;
PImage colors;
Minim minim;
//AudioPlayer[] sound;
AudioInput in;
AudioOutput out;
AudioRecorder recorder;
FFT fftout,fftin;
//FFT[] fft;
float[] fftFilter;

//String[] filename = {"083_trippy-ringysnarebeat-3bars.mp3"};
String[] filename = {"17 - Disco Boy.mp3", "Bobby McFerrin - Don't Worry, Be Happy.mp3", "06. Get up Stand Up.mp3", "01-amon_tobin--journeyman-oma.mp3", "02 - Plastic People.mp3" }; 
AudioPlayer[] sound = new AudioPlayer[filename.length];
FFT[] fft = new FFT[filename.length];
boolean isPlaying = true;
float spin = 0.0001;
float radiansPerBucket = radians(4);
float decay = 0.97;
float opacity = 40;
float minSize = 0.15;
float sizeScale = 1;

float zoom = 4;

int song = 0;

void setup()
{
  size((int)zoom*100, (int)zoom*100, P3D);
  colorMode(RGB,255);
  //TODO: make framerate depend on beat detection
  int framerate = 60;
  frameRate(framerate);
  
  minim = new Minim(this); 
   
  in =  minim.getLineIn(Minim.STEREO, 2048);  
  fftin = new FFT(in.bufferSize(), in.sampleRate());
     
  out = minim.getLineOut(Minim.STEREO, 2048);   
  fftout = new FFT(out.bufferSize(), out.sampleRate());   
    
  // Small buffer size!
  sound[song] = minim.loadFile(filename[song], 512);
  sound[song].play();
  fft[song] = new FFT(sound[song].bufferSize(), sound[song].sampleRate());
  fftFilter = new float[fft[song].specSize()];
          
  dot = loadImage("dot.png");
  colors = loadImage("colors.png");

  // Connect to the local instance of fcserver
  opc = new OPC(this, "127.0.0.1", 7890);
  
  opc.ledGrid8x8(0 * 64, width * 1/2, height * 1/2, height/16, 0, false);
  
  // Make the status LED quiet
  opc.setStatusLed(false);
  
  // Hide or show leds location
  opc.showLocations(true);
  noiseSeed(0);
  
  smooth();
}

void keyPressed() {
  if (key == 'd') opc.setDithering(false);
  if (key == ' ' && sound[song].isPlaying()) { 
    sound[song].pause();
    isPlaying = false;
  }
  //if (key == 'n') song++;
  if (key == 'f') sound[song].skip(100);
  if (key == 'r') sound[song].skip(-100);
  if (key == ']') zoom *= 1.1;
  if (key == '[') zoom *= 0.9;
}

void keyReleased() {
  if (key == 'd') opc.setDithering(true);
  if (key == 'p' && !sound[song].isPlaying()) {
    sound[song].play();
    isPlaying = true;
  }
}

void mousePressed()
{
  // choose a position to cue to based on where the user clicked.
  // the length() method returns the length of recording in milliseconds.
  int position = int(map(mouseX, 0, width, 0, sound[song].length()));
  sound[song].cue(position);
}

void draw()
{
  background(0);
  //FIXME: recenter after zooming
  //scale(width/2 * zoom, height/2 * zoom);
   
  if (sound[song].position() >= sound[song].length()-4*512 && song < filename.length-1) {
     song++;
     sound[song-1].close();
     sound[song] = minim.loadFile(filename[song], 512);
     sound[song].play();
     fft[song] = new FFT(sound[song].bufferSize(), sound[song].sampleRate());
     fftFilter = new float[fft[song].specSize()];
     noiseSeed(0);    
  }
  
  fft[song].forward(sound[song].mix);
    
  for (int i = 0; i < fftFilter.length; i++) {
    fftFilter[i] = max(fftFilter[i] * decay, log(1 + fft[song].getBand(i)));
  }
  
  for (int i = 0; i < fftFilter.length; i++) { 
    float pulse = (sin(fftFilter[i] - 0.3) * 0.3); 
    color rgb = colors.get(int(map(i, 0, fftFilter.length-1, 0, colors.width-1)), colors.height/2);
    tint(rgb, fftFilter[i] * opacity);
    blendMode(ADD);
    
    float size_pulse = abs(fftFilter[i] * pulse); 
    float size = height * (minSize + sizeScale * size_pulse);
    float noiseScalex=width/(2*zoom) - size/(2*zoom);
    float noiseScaley=height/(2*zoom) - size/(2*zoom);
    float perlin_noise_2d = noise(millis() * fftFilter[i] * noiseScalex, millis() * fftFilter[i] * noiseScaley); 
    float centerx = width * fftFilter[i] *  perlin_noise_2d * 1.125; 
    float centery = height * fftFilter[i] * perlin_noise_2d * 1.125;
    PVector center = new PVector(centerx * 1/2, centery * 1/2);
    center.rotate(millis() * spin + i * radiansPerBucket);
    center.add(new PVector(width * 1/2, height * 1/2));
        
    image(dot, center.x - size/2, center.y - size/2, size, size);
  }
  stroke(255, 255, 255);
  float position = map(sound[song].position(), 0, sound[song].length(), 0, width);
  line(position, 1, position, height/32);
}

