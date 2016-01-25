/*
 * A simple grid sequencer, launching several effects in rhythm.
 */

float BPM = 120;
int[][] pattern = {
  {0, 0, 0, 0, 0, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0},
  {0, 0, 1, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 1, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 1},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 1},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 1},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 1}
};

OPC opc;

// Grid coordinates
int gridX = 20;
int gridY = 20;
int gridSquareSize = 15;
int gridSquareSpacing = 20;

// Timing info
float rowsPerSecond = 2 * BPM / 60.0;
float rowDuration = 1.0 / rowsPerSecond;
float patternDuration = pattern.length / rowsPerSecond;

// LED array coordinates
int ledX = 400;
int ledY = 360/2;
int ledSpacing = 15;
int ledWidth = ledSpacing * 23;
int ledHeight = ledSpacing * 7;

// Images
PImage imgGreenDot;
PImage imgOrangeDot;
PImage imgPinkDot;
PImage imgPurpleDot;
PImage imgCheckers;
PImage imgGlass;
PImage[] dots;

// Timekeeping
long startTime, pauseTime;

void setup()
{
  size(640, 360);

  imgGreenDot = loadImage("greenDot.png");
  imgOrangeDot = loadImage("orangeDot.png");
  imgPinkDot = loadImage("pinkDot.png");
  imgPurpleDot = loadImage("purpleDot.png");
  imgCheckers = loadImage("checkers.png");
  imgGlass = loadImage("glass.jpeg");

  // Keep our multicolored dots in an array for easy access later
  dots = new PImage[] { imgOrangeDot, imgPurpleDot, imgPinkDot, imgGreenDot };

  // Connect to the local instance of fcserver. You can change this line to connect to another computer's fcserver
  opc = new OPC(this, "127.0.0.1", 7890);

  // Three 8x8 grids side by side
  opc.ledGrid8x8(0, ledX, ledY, ledSpacing, 0, true, false);
  opc.ledGrid8x8(64, ledX - ledSpacing * 8, ledY, ledSpacing, 0, true, false);
  opc.ledGrid8x8(128, ledX + ledSpacing * 8, ledY, ledSpacing, 0, true, false);
  
  // Init timekeeping, start the pattern from the beginning
  startPattern();
}

void draw()
{
  background(0);

  long m = millis();
  if (pauseTime != 0) {
    // Advance startTime forward while paused, so we don't make any progress
    long delta = m - pauseTime;
    startTime += delta;
    pauseTime += delta;
  }

  float now = (m - startTime) * 1e-3;
  drawEffects(now);
  drawGrid(now);
  drawInstructions();
}

void clearPattern()
{
  for (int row = 0; row < pattern.length; row++) {
    for (int col = 0; col < pattern[0].length; col++) {
      pattern[row][col] = 0;
    }
  }
}

void startPattern()
{
  startTime = millis();
  pauseTime = 0;
}

void pausePattern()
{
  if (pauseTime == 0) {
    // Pause by stopping the clock and remembering when to unpause at
    pauseTime = millis();
  } else {
    pauseTime = 0;
  }
}   

void mousePressed()
{
  int gx = (mouseX - gridX) / gridSquareSpacing;
  int gy = (mouseY - gridY) / gridSquareSpacing;
  if (gx >= 0 && gx < pattern[0].length && gy >= 0 && gy < pattern.length) {
    pattern[gy][gx] ^= 1;
  }
}

void keyPressed()
{
  if (keyCode == DELETE) clearPattern();
  if (keyCode == BACKSPACE) clearPattern();
  if (keyCode == UP) startPattern();
  if (key == ' ') pausePattern();
}

void drawGrid(float now)
{
  int currentRow = int(rowsPerSecond * now) % pattern.length;
  blendMode(BLEND);

  for (int row = 0; row < pattern.length; row++) {
    for (int col = 0; col < pattern[0].length; col++) {
      fill(pattern[row][col] != 0 ? 190 : 64);
      rect(gridX + gridSquareSpacing * col, gridY + gridSquareSpacing * row, gridSquareSize, gridSquareSize);
    }
    
    if (row == currentRow) {
      // Highlight the current row
      fill(255, 255, 0, 32);
      rect(gridX, gridY + gridSquareSpacing * row,
        gridSquareSpacing * (pattern[0].length - 1) + gridSquareSize, gridSquareSize);
    }
  }
}

void drawInstructions()
{
  int size = 12;
  int x = gridX + gridSquareSpacing * pattern[0].length + 5;
  int y = gridY + size;

  fill(255);
  textSize(size);

  text("<- Click squares to create an effect pattern\n[Delete] to clear, [Space] to pause, [Up] to restart pattern.\n", x, y);
}

void drawEffects(float now)
{
  // To keep this simple and flexible, we'll calculate every possible effect that
  // could be in progress: every grid square, and the previous/next loop of the pattern.
  // Each effect is rendered according to the time difference between the present and
  // when that grid square would fire.

  // When did the current loop of the pattern begin?
  float patternStartTime = now - (now % patternDuration);

  for (int whichPattern = -1; whichPattern <= 1; whichPattern++) {
    for (int row = 0; row < pattern.length; row++) {
      for (int col = 0; col < pattern[0].length; col++) {
        if (pattern[row][col] != 0) {
          float patternTime = patternStartTime + patternDuration * whichPattern;
          float firingTime = patternTime + rowDuration * row;
          drawSingleEffect(col, firingTime, now);
        }
      }
    }
  }
}

void drawSingleEffect(int column, float firingTime, float now)
{
  /*
   * Map sequencer columns to effects. Edit this to add your own effects!
   *
   * Parameters:
   *   column -- Number of the column in the sequencer that we're drawing an effect for.
   *   firingTime -- Time at which the effect in question should fire. May be in the
   *                 past or the future. This number also uniquely identifies a particular
   *                 instance of an effect.
   *   now -- The current time, in seconds.
   */

  float timeDelta = now - firingTime;
 
  switch (column) {

    // First four tracks: Colored dots rising from below
    case 0:
    case 1:
    case 2:
    case 3:
      drawDotEffect(column, timeDelta, dots[column]);
      break;
   
    // Stripes moving from left to right. Each stripe particle is unique based on firingTime.
    case 4: drawStripeEffect(firingTime, timeDelta); break;

    // Image spinner overlays
    case 5: drawSpinnerEffect(timeDelta, imgCheckers); break;
    case 6: drawSpinnerEffect(timeDelta, imgGlass); break;

    // Full-frame flash effect
    case 7: drawFlashEffect(timeDelta); break;
  }
}

void drawDotEffect(int column, float time, PImage im)
{
  // Draw an image dot that hits the bottom of the array at the beat,
  // then quickly shrinks and fades.

  float motionSpeed = rowsPerSecond * 90.0;
  float fadeSpeed = motionSpeed * 1.0;
  float shrinkSpeed = motionSpeed * 1.2;
  float size = 200 - max(0, time * shrinkSpeed);
  float centerX = ledX + (column - 1.5) * 75.0;
  float topY = ledY + ledHeight/2 - time * motionSpeed;
  int brightness = int(255 - max(0, fadeSpeed * time));

  // Adjust the 'top' position so the dot seems to appear on-time
  topY -= size * 0.4;
 
  if (brightness > 0) {
    blendMode(ADD);
    tint(brightness);
    image(im, centerX - size/2, topY, size, size);
  }
}

void drawSpinnerEffect(float time, PImage im)
{
  float t = time / (rowDuration * 1.5);
  if (t < -1 || t > 1) return;

  float angle = time * 5.0;
  float size = 400;
  int alpha = int(128 * (1.0 + cos(t * PI)));

  if (alpha > 0) {
    pushMatrix();
    translate(ledX, ledY);
    rotate(angle);
    blendMode(ADD);
    tint(alpha);
    image(im, -size/2, -size/2, size, size);
    popMatrix();
  }
}

void drawFlashEffect(float time)
{
  float t = time / (rowDuration * 2.0);
  if (t < 0 || t > 1) return;

  // Not super-bright, and with a pleasing falloff
  blendMode(ADD);
  fill(64 * pow(1.0 - t, 1.5));
  rect(0, 0, width, height);
}

void drawStripeEffect(float identity, float time)
{
  // Pick a pseudorandom dot and Y position
  randomSeed(int(identity * 1e3));
  PImage im = dots[int(random(dots.length))];
  float y = ledY - ledHeight/2 + random(ledHeight);

  // Animation
  float motionSpeed = rowsPerSecond * 400.0;
  float x = ledX - ledWidth/2 + time * motionSpeed;
  float sizeX = 300;
  float sizeY = 30;
  
  blendMode(ADD);
  tint(255);
  image(im, x - sizeX/2, y - sizeY/2, sizeX, sizeY);   
}

