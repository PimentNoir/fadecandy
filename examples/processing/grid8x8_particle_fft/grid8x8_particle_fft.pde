// Copyright(C) 2014, Jérôme Benoit <jerome.benoit@piment-noir.org> under GPLv3 
// with the following exceptions : No permissions is granted to reuse, modify, distribute 
// or make commercial use to the people listed below :
// 	* Toufik Medjamia <toofik@toofik.com>
//	* Frederic Renet  <renet.f@gmail.com>
// Portion Copyright(C) 2013-2014, Micah Elisabeth Scott <micah@misc.name>
// Some real-time FFT! This visualizes music in the frequency domain using a
// polar-coordinate particle system altered by a coherent noise. Particle size, theta angle 
// and radial distance are modulated using a filtered FFT and a simplex noise FBM. Color is sampled 
// from an image or generated from a simplex noise FBM and FFT filter normalized  
// and inversed values (the tunable is the boolean isColorFile in setup()).
//
// CREDITS: 
//     * Toufik Medjamia for the relative help on the 3D shape of the light diffuser;
//     * Frederic Renet for the suggestion on the brightness smoothing.
//
// TODO: 
//   * Implement OSC support for remote parameters playing; 
//   * Fix log chirp divide by zero bug;
//   * Use chirp to also dress the particles;
//   * Test differents particles type : lighty sphere, FFT bumpy sphere, etc.;   
//   * Implement simplex noise reseeding;
//   * Implement layers : beat, etc. 

import ddf.minim.analysis.*;
import ddf.minim.*;
import ddf.minim.ugens.*;
import javax.sound.sampled.*;

Debug debug;
SimplexNoise simplexnoise;
OPC opc;

PImage dot;
PImage colors;

Minim minim;
//Minim minimin;
AudioInput in;
AudioOutput out;
AudioRecorder recorder;
AudioPlayer[] sound;
AudioMetaData metasound;
AudioSignal signal;
int AudioBufferSize;
int MultiEndBuffer = 4;
FFT fftout,fftin,fftsong;
float[] fftFilter, fftFilterNorm, fftFilterNormInv, fftFilterNormPrev, fftFilterFreq, fftFilterFreqPrev, fftFilterAmpFreq;
WindowFunction fftWindow;

// Non runtime booleans.
boolean isDebug;
boolean isPlayer;

// Runtime booleans.
// Try to inverse frequency weight ... or not.
boolean isInversed;
boolean useEMA;
boolean isColorFile;
boolean isZeroNaN;
boolean isWebPlayer;

int song;
int oldsong;
//String[] filename = {"083_trippy-ringysnarebeat-3bars.mp3"};
String[] filename = {"http://www.ledjamradio.com/sound", "http://live.radiogrenouille.com/live", "http://stream1.addictradio.net/addictlounge.mp3", "http://stream.divergence-fm.org:8000/divergence.mp3", "http://mp3.live.tv-radio.com/lemouv/all/lemouvhautdebit.mp3", "http://mp3.live.tv-radio.com/franceinter/all/franceinterhautdebit.mp3", "http://mp3.live.tv-radio.com/franceinfo/all/franceinfo.mp3", "http://mp3.live.tv-radio.com/francemusique/all/francemusiquehautdebit.mp3"};
//String[] filename = {"02 Careful With That Axe, Eugene.mp3", "01. One Of These Days.mp3", "08 - The Good, The Bad and The Ugly.mp3", "06. Echoes.mp3", "07 - A fistful of Dollars [Main Title].mp3", "10 - Girl, You'll Be A Woman Soon - Urge Overkill.mp3", "01. The Eagles - Hotel California.mp3", "18 - Kill Bill Vol. 1 [Death rides a Horse].mp3", "02 - Once upon a time in America [Deborah's Theme].mp3", "17 - Once upon a time in the West [The man with the Harmonica].mp3", "Johnny Cash - Hurt.mp3", "New Shoes.mp3", "07 - Selah Sue - Explanations.mp3", "10-amon_tobin--bedtime_stories-oma.mp3", "07-amon_tobin--mass_and_spring-oma.mp3", "01-amon_tobin--journeyman-oma.mp3", "11. Redemption Song.mp3", "King Crimson - 1969 - In the Court of the Crimson King - 01 - 21st Century Schizoid Man.mp3", "02. No Woman No Cry.mp3", 
//"05. Buffalo Soldier.mp3", "17 - Disco Boy.mp3", "Bobby McFerrin - Don't Worry, Be Happy.mp3", "06. Get up Stand Up.mp3", "01-amon_tobin--journeyman-oma.mp3", 
//"02 - Plastic People.mp3" }; 

String ColorGradientImage;

int reactivity_type, pulse_type;
int octaves; 
float persistence, lacunarity;
float noise_fft;
float noise_scale_fft;
float pulse;
float smooth_factor;
float decay;
float phi, phideg, sampleRate; 
float[] f0, f1;

float spin = 0.0001;
float radiansPerBucket = radians(2);
float opacity = 40;
float minSize;
float sizeScale;

float beat_ratio = 1.0f;

void setup()
{
  // Switch between audio player or audio line in capture.
  isPlayer = true;
  //isPlayer = false;
  //isWebPlayer = false;
  isWebPlayer = true;
  
  // Debug for now.
  isDebug = true;
  debug = new Debug(isDebug);
    
  // Weighting mode.
  isInversed = true;
  
  // Log decay FFT filter, better on clean sound source such as properly mixed songs in the time domain.
  // In the frequency domain, it's a visual smoother.
  decay = 0.97f;
  // Exponential Moving Average aka EMA FFT filter, better on unclean sound source in the time domain, it's a low pass filter.
  // In the frequency domain, it's also a very simple and efficient visual smoother. 
  // Adjust the default smooth factor for a visual rendering very smooth for the human eyes.
  useEMA = true;
  smooth_factor = 0.97f;
    
  // Zero NaN FFT values to avoid display glitches.
  isZeroNaN = true;
  
  // Particules size.
  minSize = 0.57125f;
  sizeScale = 0.97f;
  
  // Choose between color gradient in file or autogenerated.
  isColorFile = false;
  if (isColorFile) {
    colorMode(RGB, 255);
  } else {
    colorMode(HSB, 100);
  }
      
  int zoom = 2;
  size(zoom * 100, zoom * 100, P3D);
   
  // TODO: make framerate depend on beat detection.
  int framerate = 72;
  frameRate(framerate);
       
  minim = new Minim(this);
  //minim.debugOn();
  //minimin = new Minim(this);
  //minimin.debugOn();
 
  // Small buffer size! 
  AudioBufferSize = 512;
         
  //out = minim.getLineOut(Minim.STEREO, AudioBufferSize);   
  //fftout = new FFT(out.bufferSize(), out.sampleRate());   
  
 if (isPlayer) {
    sound = new AudioPlayer[filename.length];
    // Random sound array index startup.
    song = (int)random(0, filename.length);
    sound[song] = minim.loadFile(filename[song], AudioBufferSize);
    metasound = sound[song].getMetaData();
    init_sound_fft();
  } else {
     Mixer.Info[] mixerInfo;
     mixerInfo = AudioSystem.getMixerInfo(); 
    
    for(int i = 0; i < mixerInfo.length; i++) {
      debug.prStr(i + ": " + mixerInfo[i].getName());
    } 
    // 0 is pulseaudio mixer on GNU/Linux
    Mixer mixer = AudioSystem.getMixer(mixerInfo[3]); 
    minim.setInputMixer(mixer); 
    in =  minim.getLineIn(Minim.STEREO, AudioBufferSize);  
    fftin = new FFT(in.bufferSize(), in.sampleRate());
    fftWindow = FFT.HAMMING;
    fftin.window(fftWindow);
    fftFilter = new float[fftin.specSize()];
    fftFilterFreq = new float[fftin.specSize()];
    fftFilterFreqPrev = new float[fftin.specSize()];
    fftFilterAmpFreq = new float[fftin.specSize()];
    fftFilterNorm = new float[fftin.specSize()];
    fftFilterNormInv = new float[fftin.specSize()];
    fftFilterNormPrev = new float[fftin.specSize()];
    f0 = new float[fftin.specSize()];
    f1 = new float[fftin.specSize()];
  }
  
  // Noise initialisation.
  simplexnoise = new SimplexNoise();
  noise_scale_fft = 0.5125f;
  // High reactivity noise source by default. 0 mean Low, 1 mean Medium, 2 mean High.  
  reactivity_type = 2;
  // It's the default number of FBM octaves.  
  octaves = 4;
  persistence = 0.25f;
  lacunarity = 0.5f;
  
  // Reactive pulse type by default that do not destroy the human eyes.
  pulse_type = 4;
  phideg = 0;
  
  //ColorGradientImage = "Chaud.png"; 
  ColorGradientImage = "colors.png";  
  dot = loadImage("dot.png");
  colors = loadImage(ColorGradientImage);
  // Connect to the local instance of fcserver
  //opc = new OPC(this, "127.0.0.1", 7890);
  opc = new OPC(this, "10.0.120.232", 7890);
  
  opc.ledGrid8x8(0 * 64, width * 1/2, height * 1/2, height/8, 0, false);
  opc.ledRing(1 * 64, 24, width * 1/3, height * 1/2, height * 1/3, 0);
  opc.ledRing(3 * 64, 16, width * 1/2, height * 1/2, height * 1/4, 0);
  opc.ledRing(2 * 64, 12, width * 2/3, height * 1/2, height * 1/6, 0);
  //opc.ledGrid8x8(512, width * 1/2, height * 1/2, height/8, 0, false);
  //opc.led(3 * 64, width * 1/4, height * 1/2);
  //opc.ledGrid(4 * 64, 11, 11, width * 1/2, height * 1/2, height/16, height/16, 0, true);
    
  // Make the status LED quiet
  opc.setStatusLed(false);
  
  // Hide or show leds location
  opc.showLocations(true);
    
  smooth();
}

void keyPressed() {
  float noise_scale_inc = 0.001f;
  if (key == 'k' && noise_scale_fft < 15-noise_scale_inc) {
    noise_scale_fft += noise_scale_inc;
    debug.UndoPrinting();
  }
  if (key == 'j' && noise_scale_fft > noise_scale_inc) {
    noise_scale_fft -= noise_scale_inc;
    debug.UndoPrinting();
  }
  float inc = 0.01f;
  if (keyCode == RIGHT && decay < 1-inc && !useEMA) {
    decay += inc;
    debug.UndoPrinting();
  }
  if (keyCode == LEFT && decay > inc && !useEMA) {
    decay -= inc;
    debug.UndoPrinting();
  }
  if (key == '=') {
    persistence += inc;
    debug.UndoPrinting();
  }
  if (key == ')' && persistence > inc) {
    persistence -= inc;
    debug.UndoPrinting();
  }
  if (key == '$') {
    lacunarity += inc;
    debug.UndoPrinting();
  }
  if (key == '*' && lacunarity > inc) {
    lacunarity -= inc;
    debug.UndoPrinting();
  }
  float beat_inc = 0.5f;
  if (key == ':' && beat_ratio > 0) {
    beat_ratio -= beat_inc;
    debug.UndoPrinting();
  }
  if (key == '!' && beat_ratio < 64) {
    beat_ratio += beat_inc;
    debug.UndoPrinting();
  }
  int phi_inc = 1;
  if (key == '8' && phideg < 360) {
    phideg += phi_inc;
    debug.UndoPrinting();
  }
  if (key == '7' && phideg > 0) {
    phideg -= phi_inc;
    debug.UndoPrinting();
  }
  if (keyCode == UP && smooth_factor < 1-inc && useEMA) {
    smooth_factor += inc;
    debug.UndoPrinting();
  }
  if (keyCode == DOWN && smooth_factor > inc && useEMA) { 
    smooth_factor -= inc;
    debug.UndoPrinting();
  }
  if (key == 'q' && minSize > inc) { 
    minSize -= inc;
    debug.UndoPrinting();
  }
  if (key == 's' && minSize < 1-inc) { 
    minSize += inc;
    debug.UndoPrinting();
  }
  if (key == 'g' && sizeScale > inc) { 
    sizeScale -= inc;
    debug.UndoPrinting();
  }
  if (key == 'h' && sizeScale < 1-inc) { 
    sizeScale += inc;
    debug.UndoPrinting();
  }
  if (key == 'e') {
    useEMA = !useEMA;
    debug.UndoPrinting();
  }
  if (key == 'm' && isPlayer) {
    isWebPlayer = !isWebPlayer;
    debug.UndoPrinting();
  }
  if (key == 'a') {
    debug.UndoPrinting();
  }
  if (key == 'c') {
    isColorFile = !isColorFile;
    debug.UndoPrinting();
  } 
  if (key == 'w') {
    // Toggle key
    isInversed = !isInversed;
    debug.UndoPrinting();
  }
  if (key == 'z') {
    // Toggle key
    isZeroNaN = !isZeroNaN;
    debug.UndoPrinting();
  }
  if (key == 'o') {
    pulse_type = 0;
    debug.UndoPrinting();
  }
  if (key == 'i') {
    pulse_type = 1;
    debug.UndoPrinting();
   }
  if (key == 'u') {
    pulse_type = 2;
    debug.UndoPrinting();
  }
  if (key == 'y') {
    pulse_type = 3;
    debug.UndoPrinting();
  }
  if (key == 't') {
    pulse_type = 4;
    debug.UndoPrinting();
  }
  if (key == '6') {
    MultiEndBuffer--;
    debug.UndoPrinting();
  }
  if (key == '9') {
    MultiEndBuffer++;
    debug.UndoPrinting();
  }
  if (key == '0') {
    reactivity_type = 0;
    debug.UndoPrinting();
  }
  if (key == '1') {
    reactivity_type = 1;
    debug.UndoPrinting();
  }
  if (key == '2') {
    reactivity_type = 2;
    debug.UndoPrinting();  
  }
  // Limit the number of FBM octaves to the int range [1-64].
  if (key == '+' && octaves < 64) {
    octaves++;
    debug.UndoPrinting();
  }
  if (key == '-' && octaves > 1) {
    octaves--;
    debug.UndoPrinting();
  }
  if (key == 'd') opc.setDithering(false);
  if (key == ' ' && isPlayer) { 
    sound[song].pause();
  }
  if (key == 'p' && isPlayer) {
    sound[song].play();
  }
  if (key == 'n' && isPlayer && !isWebPlayer && sound[song].position() <= sound[song].length()-MultiEndBuffer*AudioBufferSize && song < filename.length-1) {
    oldsong = song;
    song++;
    sound[song] = minim.loadFile(filename[song], AudioBufferSize);
    metasound = sound[song].getMetaData();
    reinit_sound_fft();
    debug.UndoPrinting();
  }
  if (key == 'b' && isPlayer && !isWebPlayer && sound[song].position() <= sound[song].length()-MultiEndBuffer*AudioBufferSize && song > 0) {
    oldsong = song;
    song--;
    sound[song] = minim.loadFile(filename[song], AudioBufferSize);
    metasound = sound[song].getMetaData();
    reinit_sound_fft();
    debug.UndoPrinting();
  }
  if (key == 'n' && isPlayer && isWebPlayer && song < filename.length-1) {
    oldsong = song;
    song++;
    sound[song] = minim.loadFile(filename[song], AudioBufferSize);
    metasound = sound[song].getMetaData();
    reinit_sound_fft();
    debug.UndoPrinting();
  }
  if (key == 'b' && isPlayer && isWebPlayer && song > 0) {
    oldsong = song;
    song--;
    sound[song] = minim.loadFile(filename[song], AudioBufferSize);
    metasound = sound[song].getMetaData();
    reinit_sound_fft();
    debug.UndoPrinting();
  }
  if (key == 'f' && isPlayer && !isWebPlayer) sound[song].skip(100);
  if (key == 'r' && isPlayer && !isWebPlayer) sound[song].skip(-100);
}

void keyReleased() {
  if (key == 'd') opc.setDithering(true);
}

void mousePressed()
{
  // Choose a position to cue to based on where the user clicked.
  // the length() method returns the length of recording in milliseconds.
  if (isPlayer && !isWebPlayer && mouseY <=  height*0.03125) {
    float mousex = mouseX;
    int position = int(map(mousex, 0, width, 0, sound[song].length()));
    sound[song].cue(position);
  }
}

// TODO: pass an FFT type argument to init differently the FFT filter.
void init_fft() {
     fftsong = new FFT(sound[song].bufferSize(), sound[song].sampleRate());
     fftWindow = FFT.HAMMING;
     fftsong.window(fftWindow);
     fftFilter = new float[fftsong.specSize()];
     fftFilterFreq = new float[fftsong.specSize()];
     fftFilterFreqPrev = new float[fftsong.specSize()];
     fftFilterAmpFreq = new float[fftsong.specSize()];  
     fftFilterNorm = new float[fftsong.specSize()];
     fftFilterNormInv = new float[fftsong.specSize()];
     fftFilterNormPrev = new float[fftsong.specSize()];
     f0 = new float[fftsong.specSize()];
     f1 = new float[fftsong.specSize()];
}  

void init_sound_fft() {
     sound[song].play();
     init_fft();
}

void reinit_sound_fft() {
     sound[oldsong].close();
     init_sound_fft();
}

float ZeroNaNValue(float Value) {
  if ((Float.isNaN(Value)) && isZeroNaN) { 
      return 0.0f;          
  } else {
      return Value;
  }    
}  

void draw()
{
  // TODO: Generate background gradient colors in function of the FFT values.
  if (isColorFile) {
    background(0);
  } else {
    background(0);
  }
   
  debug.prStrOnce("Key used = " + key);
     
  if (isPlayer && isWebPlayer) {
    debug.prStrOnce("Mode: Web player playing " + metasound.fileName() + "\n Title: " + metasound.title() + "\n Audio buffer size: " + AudioBufferSize);
  } else if (isPlayer && !isWebPlayer) {
    debug.prStrOnce("Mode: File player playing " + metasound.fileName() + "\n Title: " + metasound.title() +"\n Author: " + metasound.author()  + "\n Track: " + metasound.track() + "\n Duration: " + (metasound.length()/1000)/60 + "min\n Encoded: " + metasound.encoded() + "\n Audio buffer size: " + AudioBufferSize);
  } else {
    debug.prStrOnce("Mode: Line in with audio buffer size = " + AudioBufferSize);
  }
  
  debug.prStrOnce("Multiple for end buffer size = " + MultiEndBuffer);
        
  if (isPlayer && !isWebPlayer && sound[song].position() > sound[song].length()-MultiEndBuffer*AudioBufferSize && song < filename.length-1 && song >= 0) {
     oldsong = song; 
     song++;
     sound[song] = minim.loadFile(filename[song], AudioBufferSize);
     metasound = sound[song].getMetaData();
     reinit_sound_fft();
     debug.UndoPrinting();    
  } 
  
  float now = millis(); 
    
  if (isPlayer) {
    fftsong.forward(sound[song].mix);
  } else {
    fftin.forward(in.mix);
    //fftout.forward(out.mix);
  }
  
  debug.prStrOnce("Current FFT Window: " + fftWindow.toString());
  
  // All fftFilters have the same length.
  int fftFilterLength = fftFilter.length;
  
  if (useEMA) { 
    debug.prStrOnce("FFT Filter: EMA with smooth factor = " + smooth_factor + " and length = " + (fftFilterLength-1));
  } else {
    debug.prStrOnce("FFT Filter: Log with decay = " + decay + " and length = " + (fftFilterLength-1));
  }     
   
  for (int i = 0; i < fftFilterLength; i++) {
    fftFilterFreqPrev[i] = ZeroNaNValue(fftFilterFreq[i]);
    if (isPlayer) {
      // EMA for display smoothing. 
      if (useEMA) {
        fftFilter[i] = smooth_factor * fftFilter[i] + (1 - smooth_factor) * fftsong.getBand(i);
        fftFilterFreq[i] = smooth_factor * fftFilterFreq[i] + (1 - smooth_factor) * fftsong.indexToFreq(i);
        fftFilterAmpFreq[i] = smooth_factor * fftFilterAmpFreq[i] + (1 - smooth_factor) * fftsong.getFreq(fftsong.indexToFreq(i));
      } else {  
        fftFilter[i] = max(fftFilter[i] * decay, log(1 + fftsong.getBand(i)));
        fftFilterFreq[i] = max(fftFilterFreq[i] * decay, log(1 + fftsong.indexToFreq(i)));
        fftFilterAmpFreq[i] = max(fftFilterAmpFreq[i] * decay, log(1 + fftsong.getFreq(fftsong.indexToFreq(i))));
      }
    } else {  
      // EMA for display smoothing.
      if (useEMA) {
        fftFilter[i] = smooth_factor * fftFilter[i] + (1 - smooth_factor) * fftin.getBand(i);
        fftFilterFreq[i] = smooth_factor * fftFilterFreq[i] + (1 - smooth_factor) * fftin.indexToFreq(i);
        fftFilterAmpFreq[i] = smooth_factor * fftFilterAmpFreq[i] + (1 - smooth_factor) * fftin.getFreq(fftin.indexToFreq(i));
      } else {
        fftFilter[i] = max(fftFilter[i] * decay, log(1 + fftin.getBand(i)));
        fftFilterFreq[i] = max(fftFilterFreq[i] * decay, log(1 + fftin.indexToFreq(i)));
        fftFilterAmpFreq[i] = max(fftFilterAmpFreq[i] * decay, log(1 + fftin.getFreq(fftin.indexToFreq(i))));
      }
    }
  }
   
  if (isZeroNaN) {
    debug.prStrOnce("Zero NaN FFT values in fftFilters used for displaying");
  } else {
    debug.prStrOnce("Let NaN FFT values go through the fftFilters used for displaying");
  }     
    
  // Normalize fftFilter array values between [0-1] float range for displaying and noise feeding purpose.
  // But bound them on the min and max on a same sound event if possible? 
  float fftFiltermax = max(fftFilter);
  float fftFiltermin = min(fftFilter);
  for (int i = 0; i < fftFilterLength; i++) {
    fftFilterNormPrev[i] = ZeroNaNValue(fftFilterNorm[i]);
    fftFilterNorm[i] = map(fftFilter[i], fftFiltermin, fftFiltermax, 0 , 1);
    // Zero NaN values or not.
    fftFilterNorm[i] = ZeroNaNValue(fftFilterNorm[i]);
    fftFilterNormInv[i] = 1 - fftFilterNorm[i];
  }
    
  if (isInversed) { 
    debug.prStrOnce("Weighting: Basic inversed frequency weighting mode");
  } else {
    debug.prStrOnce("Weighting: No frequency weighting mode");   
  }
   
  for (int i = 0; i < fftFilterLength; i++) {
    if (isInversed) { 
       fftFilterNorm[i] = fftFilterNormInv[i];
    }
    // Variation on each frequency bands. 
    float fftFilterNormVar = abs(fftFilterNorm[i] - fftFilterNormPrev[i]);
    
    debug.prStrOnce("Beat ratio = " + beat_ratio);
    
    phi *= phideg * PI / 180;
    int j = i + 1;
    float phase, beta;
    if (isPlayer) {
      sampleRate = sound[song].sampleRate();
    } else {
      // FIXME: Should not work with Line in capture.
      sampleRate = in.sampleRate();
    }
    
    debug.prStrOnce("Sample Rate: " + sampleRate + "Hz");
    
    fftFilterAmpFreq[i] = ZeroNaNValue(fftFilterAmpFreq[i]);          
    fftFilterFreq[i] = ZeroNaNValue(fftFilterFreq[i]);          
    f0[i] = fftFilterFreqPrev[i];
    f1[i] = fftFilterFreq[i]; 
    switch(pulse_type) {
      case 0:
        float pulse_zero = fftFilterAmpFreq[i] * sin(fftFilterFreq[i] * 2 * PI * ((float)j / sampleRate) + phi);
        pulse = pulse_zero;
        debug.prStrOnce("Pulse: " + pulse_type + " -> Sine wave with amplitude = fftFilterAmpFreq[i], frequency = fftFilterFreq[i] and initial phase = " + phideg);
        break;
      case 1:
        // A very basic exponential chirp pulse with amplitude = fftFilterAmpFreq[i], frequency = fftFilterFreq[i] and initial phase = phideg.
        boolean isUndefined = false;
        if (f0[i] * f1[i] <= 0.0f) {
          isUndefined = true;
        }  
        if (isUndefined) {
          phase = 0.0f;
          // Debug output
          debug.prStr("Chirp log undefined! i = " + i + " f0 = " + f0[i] + " f1 = " + f1[i]);
          continue;
        } else {
          if (f0[i] == f1[i]) { 
            phase =  2 * PI * f0[i] * ((float)j / sampleRate);
          } else {
            beta = ((float)j / (float)(fftFilterLength)) * ((float)j / sampleRate) / log(f1[i] / f0[i]);
            phase = 2 * PI * beta * f0[i] * (pow(f1[i] / f0[i], ((float)j /sampleRate) / (((float)j / (float)(fftFilterLength)) * ((float)j / sampleRate)) - 1.0f));
          }          
        }
        float pulse_one = fftFilterAmpFreq[i] * cos(phase + phi);       
        pulse = pulse_one;
        debug.prStrOnce("Pulse: " + pulse_type + " -> Log chirp with initial phase = " + phideg);  
        break;
      case 2:
        // Very basic linear chirp pulse with amplitude = fftFilterAmpFreq[i], frequency = fftFilterFreq[i] and initial phase = phideg.
        beta = (f1[i] - f0[i]) / (((float)j / (float)(fftFilterLength)) * ((float)j / sampleRate));
        phase = 2 * PI * (f0[i] * ((float)j / sampleRate) + 0.5 * beta * ((float)j / sampleRate) * ((float)j / sampleRate));
        float pulse_two = fftFilterAmpFreq[i] * cos(phase + phi);
        pulse = pulse_two;
        debug.prStrOnce("Pulse: " + pulse_type + " -> Linear chirp with initial phase = " + phideg);        
        break;
      case 3:       
        // Very basic quadratic chirp pulse with amplitude = fftFilterAmpFreq[i], frequency = fftFilterFreq[i] and initial phase = phideg.
        beta = (f1[i] - f0[i]) / pow(((float)j / (float)fftFilterLength) * ((float)j / sampleRate), 2);
        phase = 2 * PI * (f1[i] * ((float)j / sampleRate) + beta * (pow(((float)j / (float)fftFilterLength) * ((float)j / sampleRate) - ((float)j / sampleRate), 3) - pow(((float)j / (float)fftFilterLength) * ((float)j / sampleRate), 3)) / 3);
        float pulse_three = fftFilterAmpFreq[i] * cos(phase + phi);
        pulse = pulse_three;
        debug.prStrOnce("Pulse: " + pulse_type + " -> Quadratic chirp with initial phase = " + phideg);        
        break;
      case 4:       
        // Very basic hyperbolic chirp pulse with amplitude = fftFilterAmpFreq[i], frequency = fftFilterFreq[i] and initial phase = phideg.
        boolean isDivideZero = false;
        if (f0[i] == 0 || f1[i] == 0) isDivideZero = true;
        if (isDivideZero) {
          phase = 0;
        } else {
          if (f0[i] == f1[i]) {
            phase = 2 * PI * f0[i] * ((float)j / sampleRate); 
          } else {
            float sing = -f1[i] * ((float)j / (float)(fftFilterLength)) * ((float)j / sampleRate) / (f0[i] - f1[i]);   
            phase = 2 * PI * (-sing * f0[i]) * log(abs(1.0f - ((float)j / sampleRate) / sing));
          }
        } 
        float pulse_four = fftFilterAmpFreq[i] * cos(phase + phi);
        pulse = pulse_four;
        debug.prStrOnce("Pulse: " + pulse_type + " -> Hyperbolic chirp with initial phase = " + phideg);        
        break;
      default:
        fftFilter[i] = ZeroNaNValue(fftFilter[i]);
        float pulse_default = sin(fftFilter[i]);
        pulse=pulse_default;
        debug.prStrOnce("Pulse: default sin(fftFilter[i])");
    }   
      
    debug.prStrOnce("FBM Properties:\n Number of octaves: " + octaves + "\n Persistence: " + persistence + "\n Lacunarity: " + lacunarity);
    switch(reactivity_type) {
      case 0:
        float low_noise_fft = simplexnoise.fbm(now * spin + noise_scale_fft * fftFilterNorm[i] * noise_fft * fftFilterNormVar * beat_ratio, noise_scale_fft * fftFilterNorm[i] * noise_fft * fftFilterNormVar * beat_ratio + noise_scale_fft * pulse, octaves, persistence, lacunarity);
        noise_fft = low_noise_fft;
        debug.prStrOnce("Reactivity: Low with FFT noise scale = " +  noise_scale_fft);
        break;
      case 1:
        float medium_noise_fft = simplexnoise.fbm(now * spin + noise_scale_fft * fftFilterNorm[i] * noise_fft * fftFilterNormVar * beat_ratio, noise_scale_fft * fftFilterNorm[i] * noise_fft * fftFilterNormVar * beat_ratio + noise_scale_fft * pulse * noise_fft, octaves, persistence, lacunarity);
        noise_fft = medium_noise_fft;
        debug.prStrOnce("Reactivity: Medium with FFT noise scale = " +  noise_scale_fft);
        break;
      case 2:
        float high_noise_fft = simplexnoise.fbm(now * spin + noise_scale_fft * noise_fft + noise_scale_fft * fftFilterNorm[i] * noise_fft * fftFilterNormVar * beat_ratio, noise_scale_fft * fftFilterNorm[i] * noise_fft * fftFilterNormVar * beat_ratio + noise_scale_fft * pulse * noise_fft, octaves, persistence, lacunarity);
        noise_fft = high_noise_fft;
        debug.prStrOnce("Reactivity: High with FFT noise scale = " +  noise_scale_fft);
        break;
      default:
        float noise_default = simplexnoise.fbm(now * spin + noise_scale_fft * fftFilterNorm[i] * noise_fft * fftFilterNormVar * beat_ratio, noise_scale_fft * fftFilterNorm[i] * noise_fft * fftFilterNormVar * beat_ratio + noise_scale_fft * pulse * noise_fft, octaves, persistence, lacunarity);
        noise_fft = noise_default;
        debug.prStrOnce("Reactivity: default (Medium) with FFT noise scale = " +  noise_scale_fft);        
    }
        
    //float size_pulse_blink = fftFilterNorm[i] * pulse;
    float size_pulse_noblink = fftFilterNorm[i] * pulse * noise_fft;
    float size = height * (minSize + sizeScale * size_pulse_noblink);
    debug.prStrOnce("Visualisation properties: \n Particules minimum size = " + minSize + " with size scale = " + sizeScale); 
           
    // Do not loose some fftFilter values, use fftFilter normalized.
    float centerx = fftFilterNorm[i] * width * noise_fft; 
    float centery = fftFilterNorm[i] * height * -noise_fft;
    PVector center = new PVector(centerx * 1/2, centery * 1/2);
    center.rotate(now * spin + i * radiansPerBucket);
    center.add(new PVector(width * 1/2, height * 1/2));
                 
    if (isColorFile) {
      colorMode(RGB, 255);
      color rgb = colors.get(int(map(i, 0, fftFilterLength-1, 0, colors.width-1)), colors.height/2);
      tint(rgb, fftFilterNormInv[i] * noise_fft * opacity);
      debug.prStrOnce("Color: Use color file = " + ColorGradientImage + " with opacity = " + opacity + " modulated by a coherent noise (RGB mode)");
    } else {
      colorMode(HSB, 100);
      // Choose hue values smoothly around the HSB spectrum to begin with.
      float hue = now * spin + fftFilterNormInv[i] * noise_fft * 100;
      // Saturation level is rather high with fftFilter array values normalization smoothed by a simplex noise FBM. 
      float saturation = fftFilterNormInv[i] * noise_fft * 100;
      // Small brightness variation around a minimum value smoothed by a simplex noise FBM.
      float brightness = 9.7125 + fftFilterNormInv[i] * noise_fft * 2.2125;
      color hsb = color( 
         hue % 100,
         saturation,
         brightness 
      );
      tint(hsb, fftFilterNormInv[i] * opacity);
      debug.prStrOnce("Color: Use gradient autogenerated with a coherent noise and opacity = " + opacity + " (HSB mode)");
    }
    // Last call to a debug.prStrOnce() function in the processing runtime.
    debug.DonePrinting();
    blendMode(ADD);
            
    image(dot, center.x - size/2, center.y - size/2, size, size);
  } 
  if (isPlayer) {
    if (isColorFile) {
      stroke(255);
    } else {  
      stroke(100);
    }
  float position = map(sound[song].position(), 0, sound[song].length(), 0, width);
  line(position, 0, position, height*0.03125);
  line(0, height*0.03125, width, height*0.03125);
  }  
}

void stop()
{
  // Always close Minim audio classes when you are done with them.
  out.close();
  if (isPlayer) {
    sound[song].close();
  } else {
    in.close();
  }
  
  minim.stop();
  //minimin.stop();

  super.stop();
}

