// Some real-time FFT! This visualizes music in the frequency domain using a
// polar-coordinate particle system. Particle size and radial distance are modulated
// using a filtered FFT. Color is sampled from an image.

import ddf.minim.analysis.*;
import ddf.minim.*;
import javax.sound.sampled.*;

OPC opc;
PImage dot;
PImage colors;
Minim minim,minimin;
AudioInput in;
AudioOutput out;
AudioRecorder recorder;
FFT fftout,fftin,fftsong;
float[] fftFilter;
int AudioBufferSize = 512;

//String[] filename = {"083_trippy-ringysnarebeat-3bars.mp3"};
String[] filename = {"11. Redemption Song.mp3", "King Crimson - 1969 - In the Court of the Crimson King - 01 - 21st Century Schizoid Man.mp3", "02. No Woman No Cry.mp3", 
"05. Buffalo Soldier.mp3", "17 - Disco Boy.mp3", "Bobby McFerrin - Don't Worry, Be Happy.mp3", "06. Get up Stand Up.mp3", "01-amon_tobin--journeyman-oma.mp3", 
"02 - Plastic People.mp3" }; 
AudioPlayer[] sound = new AudioPlayer[filename.length];
boolean isPlaying;
boolean isPlayer = true;
float spin = 0.0001;
float radiansPerBucket = radians(4);
float decay = 0.97;
float opacity = 40;
float minSize = 0.15;
float sizeScale = 1;

float zoom = 4;

int song = 0;
int oldsong;

float centerx,centery;

void setup()
{
  size((int)zoom*100, (int)zoom*100, P3D);
  colorMode(RGB,255);
  //TODO: make framerate depend on beat detection
  int framerate = 60;
  frameRate(framerate);
  
  centerx = width/2 - minSize/2;
  centery = height/2 - minSize/2;
  
  minim = new Minim(this);
  minimin = new Minim(this);
  minim.debugOn();
  minimin.debugOn();
         
  out = minim.getLineOut(Minim.STEREO, AudioBufferSize);   
  fftout = new FFT(out.bufferSize(), out.sampleRate());   
    
  // Small buffer size!
  if (isPlayer) {
    sound[song] = minim.loadFile(filename[song], AudioBufferSize);
    init_sound_fft_noise();
  } else {
     Mixer.Info[] mixerInfo;
     mixerInfo = AudioSystem.getMixerInfo(); 
    
    for(int i = 0; i < mixerInfo.length; i++) {
      println(i + ": " +mixerInfo[i].getName());
    } 
    // 0 is pulseaudio mixer on GNU/Linux
    Mixer mixer = AudioSystem.getMixer(mixerInfo[0]); 
    minim.setInputMixer(mixer); 
    in =  minim.getLineIn(Minim.STEREO, AudioBufferSize);  
    fftin = new FFT(in.bufferSize(), in.sampleRate());
    fftFilter = new float[fftin.specSize()];
    //fftFilter = new float[fftout.specSize()];
  }
          
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
  if (key == ' ' && isPlayer) { 
    sound[song].pause();
    isPlaying = false;
  }
  if (key == 'p' && isPlayer) {
    sound[song].play();
    isPlaying = true;
  }
  if (key == 'n' && isPlayer && sound[song].position() <= sound[song].length()-4*AudioBufferSize && song < filename.length-1) {
    oldsong = song;
    song++;
    sound[song] = minim.loadFile(filename[song], AudioBufferSize);
    reinit_sound_fft_noise();
  }
  if (key == 'b' && isPlayer && sound[song].position() <= sound[song].length()-4*AudioBufferSize && song > 0) {
    oldsong = song;
    song--;
    sound[song] = minim.loadFile(filename[song], AudioBufferSize);
    reinit_sound_fft_noise();
  }
  if (key == 'f' && isPlayer) sound[song].skip(100);
  if (key == 'r' && isPlayer) sound[song].skip(-100);
  if (key == ']') zoom *= 1.1;
  if (key == '[') zoom *= 0.9;
}

void keyReleased() {
  if (key == 'd') opc.setDithering(true);
}

void mousePressed()
{
  // choose a position to cue to based on where the user clicked.
  // the length() method returns the length of recording in milliseconds.
  if (isPlayer) {
    int position = int(map(mouseX, 0, width, 0, sound[song].length()));
    sound[song].cue(position);
  }
    
}

//TODO: pass an FFT type argument to init differently the FFT filter
void init_fft() {
     fftsong = new FFT(sound[song].bufferSize(), sound[song].sampleRate());
     fftFilter = new float[fftsong.specSize()];   
}  

void init_sound_fft_noise() {
     sound[song].play();
     isPlaying = true;
     init_fft();
     noiseSeed(0);
}

void reinit_sound_fft_noise() {
     sound[oldsong].close();
     init_sound_fft_noise();
}

void draw()
{
  background(0);
  //FIXME: recenter after zooming
  //scale(width/2 * zoom, height/2 * zoom);
   
  if (isPlayer && sound[song].position() > sound[song].length()-4*AudioBufferSize && song < filename.length-1 && song >= 0) {
     oldsong = song; 
     song++;
     sound[song] = minim.loadFile(filename[song], AudioBufferSize);
     reinit_sound_fft_noise();    
  }
  
  if (isPlayer) {
    fftsong.forward(sound[song].mix);
  } else {
    fftin.forward(in.mix);
    //fftout.forward(out.mix);
  }
  
    
  for (int i = 0; i < fftFilter.length; i++) {
    if (isPlayer) { 
      fftFilter[i] = max(fftFilter[i] * decay, log(1 + fftsong.getBand(i)));
    } else { 
      fftFilter[i] = max(fftFilter[i] * decay, log(1 + fftin.getBand(i)));
      //fftFilter[i] = max(fftFilter[i] * decay, log(1 + fftout.getBand(i)));
    }
  }
  
  for (int i = 0; i < fftFilter.length; i++) { 
    float pulse = (sin(fftFilter[i] - 0.3) * 0.3); 
    color rgb = colors.get(int(map(i, 0, fftFilter.length-1, 0, colors.width-1)), colors.height/2);
    tint(rgb, fftFilter[i] * opacity);
    blendMode(ADD);
    
    float size_pulse = abs(fftFilter[i] * pulse); 
    float size = height * (minSize + sizeScale * size_pulse);
    //TODO: noiseScale has something to learn from decibels in the audio played
    float noiseScalex=width/(2*zoom) - size/(2*zoom);
    float noiseScaley=height/(2*zoom) - size/(2*zoom);
    float prev_centerx = centerx;
    float prev_centery = centery;
    float perlin_noise_2d = noise(millis() * noiseScalex * prev_centerx, millis() * noiseScaley * prev_centery); 
    centerx = width * fftFilter[i] * perlin_noise_2d * 1.125; 
    centery = height * fftFilter[i] * perlin_noise_2d * 1.125;
    PVector center = new PVector(centerx * 1/2, centery * 1/2);
    center.rotate(millis() * spin + i * radiansPerBucket);
    center.add(new PVector(width * 1/2, height * 1/2));
        
    image(dot, center.x - size/2, center.y - size/2, size, size);
  }
  if (isPlayer) {
    stroke(255, 255, 255);
    float position = map(sound[song].position(), 0, sound[song].length(), 0, width);
    line(position, 0, position, height/32);
  }
}

void stop()
{
  // always close Minim audio classes when you are done with them
  out.close();
  if (isPlayer) {
    sound[song].close();
  } else {
    in.close();
  }
  
  minim.stop();
  minimin.stop();

  super.stop();
}

