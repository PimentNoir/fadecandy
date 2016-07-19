// Connect to the local instance of fcserver
var WebSocketAddress = "ws://127.0.0.1:7890"; 
//Show LED pixel locations.
var showPixelLocations = true; 
//Change the HTML Id of the canvas.
var canvasId = "grid24x8_clouds"


var dx = 0;
var dy = 0;
var im;
var d = 0;
function setup(){
	var canvas = createCanvas(52, 24);
	canvas.id(canvasId);
	socketSetup(WebSocketAddress); // Connect to the local instance of fcserver via websocket.
	
	
	loadPixels();
	pixelDensity(1);
	d = pixelDensity();

	// Map an 8x8 grid of LEDs to the center of the window, scaled to take up most of the space
	var spacing = (height / 8.0)-1;
	ledGrid8x8(0, width/2, height/2, spacing, 0, true);

	//Put two more 8x8 grids to the left and to the right of that one.
	ledGrid8x8(64, width/2 - spacing * 8, height/2, spacing, 0, true);
	ledGrid8x8(128, width/2 + spacing * 8, height/2, spacing, 0, true);
	colorMode(HSB, 100);
	background(0);
}

var noiseScale=0.02;

function fractalNoise(x, y, z) {
	var r = 0;
	var amp = 1.0;
	for (var octave = 0; octave < 4; octave++) {
		r += noise(x, y, z) * amp;
		amp /= 2;
		x *= 2;
		y *= 2;
		z *= 2;
	}
	return r;
}

function draw() {
	var now = millis();
	var speed = 0.002;
	var angle = sin(now * 0.001);
	var z = now * 0.00008;
	var Hue = now * 0.01;
	var Scale = 0.005;
	dx += cos(angle) * speed;
	dy += sin(angle) * speed;
	loadPixels();
	for (var x=0; x < width*d*4; x+=4) {
		for (var y=0; y < height*d*4; y+=4) {
			var m = fractalNoise(dx + x*Scale, dy + y*Scale, z+10) - 0.75;
			var n = fractalNoise(dx + x*Scale, dy + y*Scale, z)*3 - 0.75*3;
			var c = color(
			(Hue + 80.0 * m) % 100.0,
			100 - 100 * constrain(pow(n, 3), 0, .9),
			100 * constrain(pow(n, 1.5), 0, .9)
			);
			var idx = width*d*y+x;
			pixels[idx] = c._getRed();
			pixels[idx+1] = c._getGreen();
			pixels[idx+2] = c._getBlue();
			//pixels[idx+3] = c._getAlpha();
		}
	}
	updatePixels();
	drawFrame();
}