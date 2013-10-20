/*
 * Waves of color, in the HSV color space.
 *
 * This effect illustrates some of the properties of Fadecandy's dithering.
 * Hotkeys turn on/off various Fadecandy features. This file drives the
 * simple UI for doing so, and the actual effect is rendered entirely by a shader.
 */

PShader effect;
OPC opc;

void setup() {
  size(640, 360, P2D);
  
  effect = loadShader("effect.glsl");
  effect.set("resolution", float(width), float(height));

  opc = new OPC(this, "127.0.0.1", 7890);
  float spacing = height / 16.0;
  opc.ledGrid8x8(0, width/2, height/2, spacing, 0);
  opc.ledGrid8x8(64, width/2 - spacing * 8, height/2, spacing, 0);
  opc.ledGrid8x8(128, width/2 + spacing * 8, height/2, spacing, 0);
  opc.showLocations(true);
}

void mousePressed() {
  opc.setStatusLed(true);
}

void mouseReleased() {
  opc.setStatusLed(false);
}

void keyPressed() {
  if (key == 'd') opc.setDithering(false);
  if (key == 'i') opc.setInterpolation(false);
  if (key == 'l') opc.setStatusLed(true);
  if (key == 'g') opc.setColorCorrection(1.0, 1.0, 1.0, 1.0);
}

void keyReleased() {
  if (key == 'd') opc.setDithering(true);
  if (key == 'i') opc.setInterpolation(true);
  if (key == 'l') opc.setStatusLed(false);
  if (key == 'g') opc.setColorCorrection(2.5, 1.0, 1.0, 1.0);
}  

void draw() {
  // The entire effect happens in a pixel shader
  effect.set("time", millis() / 1000.0);
  effect.set("mouse", float(mouseX) / width, float(mouseY) / height);
  shader(effect);
  rect(0, 0, width, height);
  resetShader();

  // Status text
  textSize(20);
  text("Firmware config byte: " + opc.firmwareConfig, 10, 350);
}

