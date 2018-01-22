import processing.video.*;

//String filename = "/Users/micah/Dropbox/video/Sixteen Dots - An interactive music video for Amon Tobins Lost and Found-720p.mp4";
//String filename = "/Users/micah/Dropbox/video/Nobody Beats the Drum - Grindin-720p.mp4";
//String filename = "/Users/micah/Dropbox/video/amon_tobin_sordid.mp4";
//String filename = "/Users/micah/Dropbox/video/La Roux - Bulletproof-360p.mp4";
//String filename = "/Users/micah/Dropbox/video/will.i.am - Scream & Shout ft. Britney Spears-360p.mp4";
String filename = "/Users/micah/Dropbox/video/The Glitch Mob - We Can Make The World Stop (Official Video)-720p.mp4";

float zoom = 2;

OPC opc;
Movie movie;
PGraphics[] pyramid;

void setup()
{
  size(480, 240, P3D);

  // Connect to the local instance of fcserver. You can change this line to connect to another computer's fcserver
  opc = new OPC(this, "127.0.0.1", 7890);

  opc.ledGrid8x8(0 * 64, width * 1/8, height * 1/4, height/16, 0, true, false);
  opc.ledGrid8x8(1 * 64, width * 3/8, height * 1/4, height/16, 0, true, false);
  opc.ledGrid8x8(2 * 64, width * 5/8, height * 1/4, height/16, 0, true, false);
  opc.ledGrid8x8(3 * 64, width * 7/8, height * 1/4, height/16, 0, true, false);
  opc.ledGrid8x8(4 * 64, width * 1/8, height * 3/4, height/16, 0, true, false);
  opc.ledGrid8x8(5 * 64, width * 3/8, height * 3/4, height/16, 0, true, false);
  opc.ledGrid8x8(6 * 64, width * 5/8, height * 3/4, height/16, 0, true, false);
  opc.ledGrid8x8(7 * 64, width * 7/8, height * 3/4, height/16, 0, true, false);

  movie = new Movie(this, filename);
  movie.loop();

  pyramid = new PGraphics[4];
  for (int i = 0; i < pyramid.length; i++) {
    pyramid[i] = createGraphics(width / (1 << i), height / (1 << i), P3D);
  }
}

void keyPressed() {
  if (key == 'd') opc.setDithering(false);
  if (key == ' ') movie.pause();
  if (key == ']') zoom *= 1.1;
  if (key == '[') zoom *= 0.9;
}

void keyReleased() {
  if (key == 'd') opc.setDithering(true);
  if (key == ' ') movie.play();
}  

void movieEvent(Movie m)
{
  m.read();
}

void draw()
{
  // Scale to width, center height
  int mWidth = int(pyramid[0].width * zoom);
  int mHeight = mWidth * movie.height / movie.width;

  // Center location
  float x, y;

  if (mousePressed) {
    // Pan horizontally and vertically with the mouse
    x = -mouseX * (mWidth - pyramid[0].width) / width;
    y = -mouseY * (mHeight - pyramid[0].height) / height;
  } else {
    // Centered
    x = -(mWidth - pyramid[0].width) / 2;
    y = -(mHeight - pyramid[0].height) / 2;
  }

  pyramid[0].beginDraw();
  pyramid[0].background(0);
  pyramid[0].image(movie, x, y, mWidth, mHeight);
  pyramid[0].endDraw();

  for (int i = 1; i < pyramid.length; i++) {
    pyramid[i].beginDraw();
    pyramid[i].image(pyramid[i-1], 0, 0, pyramid[i].width, pyramid[i].height);
    pyramid[i].endDraw();
  }

  image(pyramid[pyramid.length - 1], 0, 0, width, height);
}

