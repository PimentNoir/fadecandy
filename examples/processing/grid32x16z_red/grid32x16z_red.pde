OPC opc;
PShader blur;
PGraphics src;
PGraphics pass1, pass2;

void setup()
{
  size(640, 320, P2D);

  blur = loadShader("blur.glsl");
  blur.set("blurSize", height / 8);
  blur.set("sigma", 20.0f);  
  
  src = createGraphics(width, height, P3D); 
  
  pass1 = createGraphics(width, height, P2D);
  pass1.noSmooth();  
  
  pass2 = createGraphics(width, height, P2D);
  pass2.noSmooth();

  // Connect to the local instance of fcserver. You can change this line to connect to another computer's fcserver
  opc = new OPC(this, "127.0.0.1", 7890);

  opc.ledGrid8x8(0 * 64, width * 1/8, height * 1/4, height/16, 0, true);
  opc.ledGrid8x8(1 * 64, width * 3/8, height * 1/4, height/16, 0, true);
  opc.ledGrid8x8(2 * 64, width * 5/8, height * 1/4, height/16, 0, true);
  opc.ledGrid8x8(3 * 64, width * 7/8, height * 1/4, height/16, 0, true);
  opc.ledGrid8x8(4 * 64, width * 1/8, height * 3/4, height/16, 0, true);
  opc.ledGrid8x8(5 * 64, width * 3/8, height * 3/4, height/16, 0, true);
  opc.ledGrid8x8(6 * 64, width * 5/8, height * 3/4, height/16, 0, true);
  opc.ledGrid8x8(7 * 64, width * 7/8, height * 3/4, height/16, 0, true);

  // Make the status LED quiet
  opc.setStatusLed(false);
}

void draw()
{
  float t = millis() * 0.001;
  randomSeed(0);
  
  src.beginDraw();
  src.noStroke();
  src.background(0);
  src.fill(255);

  src.directionalLight(255, 255, 255, -1, 0, 0.4);
  src.directionalLight(255, 255, 255, -1, 0, 0.2);
  src.directionalLight(255, 255, 255, -1, 0, 0);

  // Lots of rotating cubes
  for (int i = 0; i < 50; i++) {
    src.pushMatrix();
    src.translate(random(0, width), random(0, height), 0);
    src.rotateY(t * 0.5 + randomGaussian());
    src.rotateX(randomGaussian());
    src.box(height * abs(0.3 + 0.1 * randomGaussian()));
    src.popMatrix();
  }

  // Separable blur filter
  src.endDraw();

  blur.set("horizontalPass", 0);
  pass1.beginDraw();            
  pass1.shader(blur);  
  pass1.image(src, 0, 0);
  pass1.endDraw();
  
  blur.set("horizontalPass", 1);
  pass2.beginDraw();            
  pass2.shader(blur);  
  pass2.image(pass1, 0, 0);
  pass2.endDraw();    
        
  blendMode(NORMAL);
  image(pass2, 0, 0);

  // Bright red wash
  blendMode(ADD);
  fill(255, 20, 20, 190);
  noStroke();
  rect(0, 0, width, height);
}

