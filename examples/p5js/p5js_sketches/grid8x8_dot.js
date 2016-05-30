// Connect to the local instance of fcserver
var WebSocketAddress = "ws://127.0.0.1:7890"; 
//Show LED pixel locations.
var showPixelLocations = true; 
//Change the HTML Id of the canvas.
var canvasId = "grid8x8_dot";


//Canvas
function setup() {
	var canvas = createCanvas(640, 360);
	canvas.id(canvasId);
	socketSetup(WebSocketAddress);
	
  // Load a sample image
  dot = loadImage("images/dot.png");

  // Map an 8x8 grid of LEDs to the center of the window
  ledGrid8x8(0, width/2, height/2, height/12, 0, false);
}

function draw() {
  background(0);

  // Draw the image, centered at the mouse location
  var dotSize = height * 0.7;
  image(dot, mouseX - dotSize/2, mouseY - dotSize/2, dotSize, dotSize);
  drawFrame();
}