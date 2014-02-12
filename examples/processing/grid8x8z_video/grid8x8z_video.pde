import processing.video.*;

//String filename = "/Users/micah/Dropbox/video/Sixteen Dots - An interactive music video for Amon Tobins Lost and Found-720p.mp4";
//String filename = "/Users/micah/Dropbox/video/Nobody Beats the Drum - Grindin-720p.mp4";
//String filename = "/Users/micah/Dropbox/video/amon_tobin_sordid.mp4";
//String filename = "/Users/micah/Dropbox/video/La Roux - Bulletproof-360p.mp4";
//String filename = "/Users/micah/Dropbox/video/will.i.am - Scream & Shout ft. Britney Spears-360p.mp4";
//String filename = "/Users/micah/Dropbox/video/The Glitch Mob - We Can Make The World Stop (Official Video)-720p.mp4";
//String filename = "/home/elecdev/Téléchargements/Homeland.S03E07.FRENCH.LD.HDTV.XviD-MiND.avi";
String filename = "/home/elecdev/Téléchargements/Ahmad Jamal invite Yusef Lateef à l'Olympia_France Ô_2013_08_17_00_15.avi";
//String filename = "/home/frag/Videos/Ahmad Jamal invite Yusef Lateef à l'Olympia_France Ô_2013_08_17_00_15.avi";

int zoom = 2;

float movie_speed = 1.0;
boolean isPlaying;
boolean isLooping;

int framerate = 1;

OPC opc;
Movie movie;
PGraphics[] pyramid;

void setup()
{
  size(zoom*480, zoom*240, P3D);
  colorMode(HSB,100,100,100);
  
  // Connect to the local instance of fcserver. You can change this line to connect to another computer's fcserver
  opc = new OPC(this, "127.0.0.1", 7890);


  opc.ledGrid8x8(0 * 64, width/2, height/2, height/8, 0, false);
  
  // Make the status LED quiet
  opc.setStatusLed(false);
  
  movie = new Movie(this, filename);
  movie.loop();
  isPlaying = true;
  isLooping = true;

  pyramid = new PGraphics[4];
  for (int i = 0; i < pyramid.length; i++) {
    pyramid[i] = createGraphics(width / (1 << i), height / (1 << i), P3D);
  }
  smooth();
}

void keyPressed() {
  if (key == 'd') opc.setDithering(false);
  if (key == ' ') movie.pause();
  if (key == ']') zoom *= 1.1;
  if (key == '[') zoom *= 0.9;
  if (key == 'j') movie.jump(random(movie.duration()));
  if (key == '1') movie_speed = 1.0;
  if (key == '2') movie_speed = 2.0;
  if (key == '3') movie_speed = 3.0;
  if (key == '4') movie_speed = 4.0;
  if (key == '5') movie_speed = 5.0;
  if (key == '6') movie_speed = 6.0;
  if (key == '7') movie_speed = 7.0;
  if (key == '8') movie_speed = 8.0;
  if (key == '9') movie_speed = 9.0;
  if (key == '/' && key == '2') movie_speed = 0.5;
  if (key == '/' && key == '3') movie_speed = 0.33;
  if (key == '/' && key == '4') movie_speed = 0.25;
  if (key == '/' && key == '5') movie_speed = 0.2;
  if (key == 'r') movie_speed = -1;
  if (key == '+') framerate += 1;
  if (key == '-' && framerate > 1) framerate -= 1;
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
  frameRate(framerate);
  movie.speed(movie_speed);
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

