// Parameters!
int stepsPerSecond = 20;
float maxEnergy = 40;
float minEnergy = 8;
float energyChangeRate = 0.002;
float hueShift = (100.0 / 4) + 0.2;

OPC opc;
TriangleGrid triangle;

// Current snake
float currentHue;
int currentCell;

// Per-cell hue and energy
float[] cellHues;
int[] cellEnergies;

// Step counter
int stepNumber = 0;

void setup()
{
  size(200, 200);

  // Pacing
  frameRate(stepsPerSecond);

  // Connect to the local instance of fcserver. You can change this line to connect to another computer's fcserver
  opc = new OPC(this, "127.0.0.1", 7890);

  // Map our triangle grid to the center of the window
  triangle = new TriangleGrid();
  triangle.grid16();
  triangle.mirror();
  triangle.rotate(radians(60));
  triangle.scale(height * 0.2);
  triangle.translate(width * 0.5, height * 0.57);
  triangle.leds(opc, 0);

  // Initialize cells
  cellHues = new float[triangle.cells.length];
  cellEnergies = new int[triangle.cells.length];
  
  currentHue = random(100);
  currentCell = int(random(triangle.cells.length));
  
  colorMode(HSB, 100);
}

int chooseMaximum(float[] scores)
{
  // Return the index of the largest nonzero member of 'scores'.
  // Returns -1 if all members are <= 0.
    
  int result = -1;
  float best = 0;

  for (int i = 0; i < scores.length; i++) {
    if (scores[i] > best) {
      result = i;
      best = scores[i];
    }
  }
  
  return result;
}


int chooseNeighbor(int current)
{
  // Look for a neighboring cell to move to. Each neighbor gets a
  // score, either a random positive number or zero if it's unsuitable.
  
  float[] scores;
  scores = new float[3];
  for (int i = 0; i < scores.length; i++) {
    int neighbor = triangle.cells[current].neighbors[i];
    if (neighbor >= 0 && cellEnergies[neighbor] == 0) {
      scores[i] = random(1, 2);
    }
  }
  
  int neighbor = chooseMaximum(scores);
  if (neighbor < 0) {
    return -1;
  }
  
  return triangle.cells[current].neighbors[neighbor];
}

int chooseEmpty()
{
  // Look for a random empty cell

  float[] scores;
  scores = new float[triangle.cells.length];

  for (int i = 0; i < scores.length; i++) {
    if (cellEnergies[i] == 0) {
      scores[i] = random(1, 2);
    }
  }
  
  return chooseMaximum(scores);
}

void draw()
{
  background(0);
  stepNumber++;

  // Cell energies vary in sinusoidal epochs, causing the shape of the snakes
  // to shift accordingly as the space becomes more or less fragmented.
  int currentEnergy = int(map(cos(stepNumber * energyChangeRate), 1, -1, minEnergy, maxEnergy));

  // Each live cell decays by one step, no cells can live longer than currentEnergy.
  // This creates a cadence to the life of each snake, and periodic waves of extinction.
  for (int i = 0; i < triangle.cells.length; i++) {
    cellEnergies[i] = max(0, min(currentEnergy, cellEnergies[i] - 1));
  }

  // Can we keep going?
  if (currentCell >= 0) {
    currentCell = chooseNeighbor(currentCell);
  }

  if (currentCell < 0) {
    // New snake

    currentCell = chooseEmpty();
    if (currentCell >= 0) {
      // Hues rotate between three complementary colors, shifting slowly around the color wheel.
      currentHue = (currentHue + hueShift) % 100;
    }
  }

  if (currentCell >= 0) {
    // We have somewhere to go.
    
    cellEnergies[currentCell] = currentEnergy;
    cellHues[currentCell] = currentHue;
  }
  
  // Overall saturation shows the current energy level. Extinction periods (low currentEnergy)
  // correspond with flashes of white.
  float saturation = (currentEnergy / maxEnergy) * 100;

  // Draw the current state of each cell
  for (int i = 0; i < triangle.cells.length; i++) {
    float size = height * 0.1;
    fill(cellHues[i], saturation, map(cellEnergies[i], 0, currentEnergy, 0, 100));
    ellipse(triangle.cells[i].center.x, triangle.cells[i].center.y, size, size);
  }
}
