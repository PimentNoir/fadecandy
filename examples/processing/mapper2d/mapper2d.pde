/*
 * Using a camera, approximate the 2-D location of the area illuminated by each LED.
 * Instead of illuminating each LED in sequence, this uses a binary pattern to quickly
 * identify large numbers of LEDs with few test patterns.
 *
 * This is still really experimental! Currently it's just a minimal proof-of-concept :)
 *
 * Micah Elizabeth Scott, 2013
 */

import processing.video.*;

//int MAX_PIXELS = 0xFFFF / 3;    // Maximum number of pixels
int MAX_PIXELS = 16;
int BRIGHTNESS = 130;           // How bright should our test pattern be?
float MSE_THRESHOLD = 30;       // Mean squared error below which frames are "identical"

int S_WAITING    = 0;
int S_START_BIT  = 1;
int S_SETTLE_BIT = 2;
int S_FINALIZE = 3;

int state;
int bit;
int numBitsInUse;
OPCLowLevel opc;
Capture video;
int[][] bitImages;

void setup()
{
  size(640, 480);
  video = new Capture(this, width, height);
  video.start();

  bitImages = new int[16][width * height];

  opc = new OPCLowLevel("127.0.0.1", 7890, MAX_PIXELS);
  opc.firmwareConfig = 0x07;  // Disable dithering, interpolation, and status LED
}

void allPixelsOff()
{
  for (int i = 0; i < MAX_PIXELS; i++) {
    opc.setPixel(i, 0, 0, 0);
  }
  opc.sendPixels();
}  

void sendBitPattern()
{
  // Send an LED pattern in which red/blue correspond to 1/0 for the indicated bit position.
  for (int i = 0; i < MAX_PIXELS; i++) {
    boolean b = 0 != ((i >> bit) & 1);
    opc.setPixel(i, b ? BRIGHTNESS : 0, 0, b ? 0 : BRIGHTNESS);
  }
  opc.sendPixels();
}

void keyPressed()
{
  if (key == ' ' && state == S_WAITING) {
    // Start!
    bit = 0;
    state = S_START_BIT;
  }
  if (keyCode == DELETE || keyCode == BACKSPACE) {
    // Start over
    state = S_WAITING;
  }
}

void sText(String s, int x, int y)
{
  // Text with a shadow
  fill(0);
  text(s, x+2, y+2);
  fill(255);
  text(s, x, y);
}

float frameDifference(int[] a, int[] b)
{
  // Mean squared distance between frames
  float total = 0;
  for (int i = 0; i < a.length; i++) {
    int aPixel = a[i];
    int bPixel = b[i];
    int diffR = ((aPixel >> 16) & 0xFF) - ((bPixel >> 16) & 0xFF);
    int diffG = ((aPixel >> 8) & 0xFF) - ((bPixel >> 8) & 0xFF);
    int diffB = (aPixel & 0xFF) - (bPixel & 0xFF);
    total += diffR*diffR + diffG*diffG + diffB*diffB;
  }
  return total / (width * height);
}

PVector bitSimilarityImage(int[] output, int pattern)
{
  /*
   * Generate an image which maps how similar each pixel is, across all bits, to
   * the indicated bit pattern. One way to think of this: we're calculating a
   * probability that each pixel in question was in fact pointed at the pattern
   * we're looking for. Each bit gives us a single probability, and those probabilities
   * are multiplied together to give a pattern probability.
   *
   * Returns the weighted centroid of the image.
   */

  float xTotal = 0, xWeight = 0;
  float yTotal = 0, yWeight = 0;

  for (int i = 0; i < output.length; i++) {
    int x = i % width;
    int y = i / width;
    float probability = 1.0;
    
    for (int b = 0; b < numBitsInUse; b++) {
      int pixel = bitImages[b][i];
      boolean patternBit = 1 == ((pattern >> b) & 1);
      
      // We might want to do something smarter here, like histogram backprojection
      // based on actual colors sampled from the device. This is pretty stupid.
      
      int diffR = (patternBit ? 0xFF : 0x00) - ((pixel >> 16) & 0xFF);
      int diffB = (patternBit ? 0x00 : 0xFF) - (pixel & 0xFF);  
      probability /= 1 + (diffR*diffR + diffB*diffB) * 1e-4;
    }

    // Update weighted centroid
    float w = probability * probability;
    xTotal += x * w;
    xWeight += w;
    yTotal += y * w;
    yWeight += w;

    int gray = constrain(int(probability * 1e3), 0, 255);
    output[i] = gray | (gray << 8) | (gray << 16);
  }
  
  return new PVector(xTotal / xWeight, yTotal / yWeight);
}

void showFrameSequence()
{
  // Show our captured frames in sequence
  int i = (millis() / 500) % numBitsInUse;
  loadPixels();
  arrayCopy(bitImages[i], pixels);
  updatePixels();
  textSize(25);
  sText("Bit " + i, 10, 30);
}

void showPatternSequence()
{
  // Show pattern similarity measurements in sequence
  int i = (millis() / 500) % MAX_PIXELS;
  loadPixels();
  PVector peak = bitSimilarityImage(pixels, i);
  updatePixels();
  textSize(25);
  sText("Similarity to pattern " + i, 10, 30);

  // Draw crosshairs at the peak probability location
  stroke(128, 255, 128);
  int size = 100;
  line(peak.x, peak.y - size, peak.x, peak.y + size);
  line(peak.x - size, peak.y, peak.x + size, peak.y);
}

void draw()
{
  if (state == S_FINALIZE) {
    if (mousePressed)
      showFrameSequence();
    else
      showPatternSequence();
      
    allPixelsOff();
    return;
  }

  if (!video.available())
    return;
  video.read();
  image(video, 0, 0, width, height);
  video.loadPixels();

  textSize(25);
  if (state == S_WAITING) {
    sText("Press [SPACE] to start mapping", 10, 30);
    allPixelsOff();
  } else {
    sText("Mapping bit " + bit + "...", 10, 30);
  }
  textSize(16);

  if (state == S_START_BIT) {
    // Update the LEDs, and discard this video frame.
    sendBitPattern();
    state = S_SETTLE_BIT;
  } else if (state == S_SETTLE_BIT) {
    // Keep capturing frames until the frames are similar enough.
    // This gives the camera's brightness control time to settle.
    float d = frameDifference(video.pixels, bitImages[bit]);
    sText("Settling... MSE: " + d, 10, 60);
    arrayCopy(video.pixels, bitImages[bit]);

    if (d < MSE_THRESHOLD) {
      // Settled enough! Ready to move on.

      if (((MAX_PIXELS - 1) >> (bit + 1) == 0)) {
        // No more pixels possible beyond this bit. We're done.
        numBitsInUse = bit + 1;
        state = S_FINALIZE;

      } else {
        // More bits
        bit++;
        state = S_START_BIT;
      }
    }     
  }
}
