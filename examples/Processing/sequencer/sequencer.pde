/*
 * A simple grid sequencer, launching several effects in rhythm.
 */

float BPM = 120;
int[][] pattern = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 1},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0}
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
int ledBottom = int(ledY + 3.5 * ledSpacing);

// Images
PImage imgGreenDot;
PImage imgOrangeDot;
PImage imgPinkDot;
PImage imgPurpleDot;
PImage imgCheckers;


void setup()
{
  size(640, 360);

  imgGreenDot = loadImage("greenDot.png");
  imgOrangeDot = loadImage("orangeDot.png");
  imgPinkDot = loadImage("pinkDot.png");
  imgPurpleDot = loadImage("purpleDot.png");
  imgCheckers = loadImage("checkers.png");

  // Connect to the local instance of fcserver. You can change this line to connect to another computer's fcserver
  opc = new OPC(this, "127.0.0.1", 7890);

  // Three 8x8 grids side by side
  opc.ledGrid8x8(0, ledX, ledY, ledSpacing, 0);
  opc.ledGrid8x8(64, ledX - ledSpacing * 8, ledY, ledSpacing, 0);
  opc.ledGrid8x8(128, ledX + ledSpacing * 8, ledY, ledSpacing, 0);
  opc.showLocations(true);

  /*
   * Set up custom color management settings. We want all of the dynamic range we can
   * get, and it's okay if the very darkest parts of the display flicker a little due to
   * the dithering. We can get this effect by setting linearCutoff to zero, to disable
   * the linear portion of the gamma curve. This makes our "flash" effect roll off very
   * smoothly, instead of stuttering right before it completely shuts off.
   *
   * For more information about these settings, see the fcserver README. This is a JSON
   * blob in the same format as used by fcserver's config file.
   */
  opc.setColorCorrection("{ \"gamma\": 2.5, \"linearCutoff\": 0.0 }");

  // If the Fadecandy board's status LED is bothersome, it's easy to turn off or repurpose.
  opc.setStatusLed(false);
}

void draw()
{
  background(0);

  float now = millis() * 1e-3;
  drawEffects(now);
  drawGrid(now);
}

void mousePressed()
{
  int gx = (mouseX - gridX) / gridSquareSpacing;
  int gy = (mouseY - gridY) / gridSquareSpacing;
  if (gx >= 0 && gx < pattern[0].length && gy >= 0 && gy < pattern.length) {
    pattern[gy][gx] ^= 1;
  }
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
          float firingTime = patternStartTime + patternDuration * whichPattern + rowDuration * row;
          drawSingleEffect(col, now - firingTime);
        }
      }
    }
  }
}

void drawSingleEffect(int column, float time)
{
  switch (column) {

    // First four tracks: Colored dots rising from below
    case 0: drawDotEffect(column, time, imgOrangeDot); break;
    case 1: drawDotEffect(column, time, imgPinkDot); break;
    case 2: drawDotEffect(column, time, imgPurpleDot); break;
    case 3: drawDotEffect(column, time, imgGreenDot); break;

    // Spinner overlay
    case 6: drawSpinnerEffect(time, imgCheckers); break;

    // Full-frame flash effect
    case 7: drawFlashEffect(time); break;
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
  float topY = ledBottom - time * motionSpeed;
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
  float t = time / (rowDuration * 8.0);
  if (t < 0 || t > 1) return;

  float angle = t * 2.5;
  float size = 400;
  int alpha = int(100 * sin(t * PI));

  pushMatrix();
  translate(ledX, ledY);
  rotate(angle);
  blendMode(ADD);
  tint(alpha);
  image(im, -size/2, -size/2, size, size);
  popMatrix();
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

