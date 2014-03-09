// Some real-time FFT! This visualizes music in the frequency domain using a
// polar-coordinate particle system. Particle size and radial distance are modulated
// using a filtered FFT. Color is sampled from an image.

import ddf.minim.analysis.*;
import ddf.minim.*;
import javax.sound.sampled.*;

OPC opc;
SimplexNoise simplexnoise = new SimplexNoise();
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
String[] filename = {"http://www.ledjamradio.com/sound"};
//String[] filename = {"10-amon_tobin--bedtime_stories-oma.mp3", "07-amon_tobin--mass_and_spring-oma.mp3", "01-amon_tobin--journeyman-oma.mp3", "11. Redemption Song.mp3", "King Crimson - 1969 - In the Court of the Crimson King - 01 - 21st Century Schizoid Man.mp3", "02. No Woman No Cry.mp3", 
//"05. Buffalo Soldier.mp3", "17 - Disco Boy.mp3", "Bobby McFerrin - Don't Worry, Be Happy.mp3", "06. Get up Stand Up.mp3", "01-amon_tobin--journeyman-oma.mp3", 
//"02 - Plastic People.mp3" }; 
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

// Random sound array index startup
int song = (int)random(0, filename.length);
int oldsong;

float centerx = width/2, centery = height/2;

void setup()
{
  size((int)zoom*100, (int)zoom*100, P3D);
  colorMode(RGB,255);
  //TODO: make framerate depend on beat detection
  int framerate = 60;
  frameRate(framerate);
     
  minim = new Minim(this);
  minimin = new Minim(this);
  //minim.debugOn();
  //minimin.debugOn();
         
  out = minim.getLineOut(Minim.STEREO, AudioBufferSize);   
  fftout = new FFT(out.bufferSize(), out.sampleRate());   
    
  // Small buffer size!
  if (isPlayer) {
    sound[song] = minim.loadFile(filename[song], AudioBufferSize);
    init_sound_fft();
  } else {
     Mixer.Info[] mixerInfo;
     mixerInfo = AudioSystem.getMixerInfo(); 
    
    for(int i = 0; i < mixerInfo.length; i++) {
      println(i + ": " +mixerInfo[i].getName());
    } 
    // 0 is pulseaudio mixer on GNU/Linux
    Mixer mixer = AudioSystem.getMixer(mixerInfo[1]); 
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
    reinit_sound_fft();
  }
  if (key == 'b' && isPlayer && sound[song].position() <= sound[song].length()-4*AudioBufferSize && song > 0) {
    oldsong = song;
    song--;
    sound[song] = minim.loadFile(filename[song], AudioBufferSize);
    reinit_sound_fft();
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
  //choose a position to cue to based on where the user clicked.
  //the length() method returns the length of recording in milliseconds.
  if (isPlayer && mouseY <=  height*0.03125) {
    float mousex = mouseX;
    int position = int(map(mousex, 0, width, 0, sound[song].length()));
    sound[song].cue(position);
  }
}

//It's a standard noise FBM with persistence = 1/octave, lacunarity = 1/2 and # of octaves = 4.
float simplexnoise(float x, float y) {
  int octave = 4;
  float persistence = 0.25;
  float lacunarity = 0.5;
  //One might argue that the initial frequency should always be 1.0 for the very first octave.
  float frequency = 1.0;
  
  float r = 0;
  float amp = 1.0;
  for (int l = 0; l < octave; l++) {
    //Keep the same behaviour as the processing perlin noise() function, return values in the [0,1] float range.
    r += (((float)simplexnoise.noise((double)(frequency * x), (double)(frequency * y)) + 1) / 2.0f) * amp;
    amp *= persistence;
    frequency *= lacunarity;
  }
  return r * (1 - persistence)/(1 - amp);
} 

//TODO: pass an FFT type argument to init differently the FFT filter.
void init_fft() {
     fftsong = new FFT(sound[song].bufferSize(), sound[song].sampleRate());
     fftFilter = new float[fftsong.specSize()];   
}  

void init_sound_fft() {
     sound[song].play();
     isPlaying = true;
     init_fft();
}

void reinit_sound_fft() {
     sound[oldsong].close();
     init_sound_fft();
}

float pulse = 0;
float noise_scalex = 0, noise_scaley = 0; 

void draw()
{
  background(0);
  //FIXME: recenter after zooming
  //translate(width/2, height/2);
   
  if (isPlayer && sound[song].position() > sound[song].length()-4*AudioBufferSize && song < filename.length-1 && song >= 0) {
     oldsong = song; 
     song++;
     sound[song] = minim.loadFile(filename[song], AudioBufferSize);
     reinit_sound_fft();    
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
  
  float fftFiltermax = max(fftFilter);
        
  for (int i = 0; i < fftFilter.length; i++) { 
    if (fftFiltermax != 0) pulse = (sin(1 - (fftFilter[i]/fftFiltermax)) * decay * 0.5125); 
    color rgb = colors.get(int(map(i, 0, fftFilter.length-1, 0, colors.width-1)), colors.height/2);
    tint(rgb, fftFilter[i] * opacity);
    blendMode(ADD);
    
    float size_pulse = abs(fftFilter[i] * pulse); 
    float size = height * (minSize + sizeScale * size_pulse);
    float prev_centerx = centerx;
    float prev_centery = centery;
    float now = millis();
    noise_scalex = 0.5125;
    noise_scaley = 0.5125;
    float smooth_noisey = now * spin + prev_centery * noise_scaley * abs(centery - prev_centery);
    float smooth_noisey_pulse = now * spin + prev_centery * noise_scaley * abs(centery - prev_centery) + pulse; 
    float simplex_noise_2d = simplexnoise(now * spin + prev_centerx * noise_scalex * abs(centerx - prev_centerx), smooth_noisey_pulse);
    float noise_type = simplex_noise_2d; 
    centerx = width * fftFilter[i] * noise_type; 
    centery = height * fftFilter[i] * -noise_type;
    PVector center = new PVector(centerx * 1/2, centery * 1/2);
    center.rotate(now * spin + i * radiansPerBucket);
    center.add(new PVector(width * 1/2, height * 1/2));
            
    image(dot, center.x - size/2, center.y - size/2, size, size);
  }
  if (isPlayer) {
    stroke(212, 212, 212);
    float position = map(sound[song].position(), 0, sound[song].length(), 0, width);
    line(position, 0, position, height*0.03125);
    line(0, height*0.03125, width, height*0.03125);
  }
}

void stop()
{
  //always close Minim audio classes when you are done with them
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

