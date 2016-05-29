// Connect to the local instance of fcserver
var WebSocketAddress = "ws://127.0.0.1:7890"; 
//Show LED pixel locations.
var showPixelLocations = true; 
//Change the HTML Id of the canvas.
var canvasId = "grid8x8_simple_noise";


//A simple example of using Processing's noise() function to draw LED clouds
var clouds;
//
function setup(){
createCanvas(128, 128);
socketSetup(WebSocketAddress);
ledGrid8x8(0, width/2, height/2, height / 8.0, 0, false);
colorMode(HSB, 100);
noiseDetail(5, 0.4);
// Render the noise to a smaller image, it's faster than updating the entire window.
clouds = new p5.Image(128, 128, RGB);
}

function draw(){
	var colorHue = (noise(millis() * 0.0001) * 200) % 100;
	var z = millis() * 0.0001;
	var dx = millis() * 0.0001;
	clouds.loadPixels();
	for (var x=0; x < clouds.width*4; x+=2) {
		for (var y=0; y < clouds.height*4; y+=4) {
			var n = 500 * (noise(dx + x * 0.01, y * 0.01, z) - 0.4);
			//var c = color(hue, 80 - n, n);
			clouds.pixels[((x + clouds.width*y))] = colorHue;
			clouds.pixels[((x + clouds.width*y))+1] = 80 - n;
			clouds.pixels[((x + clouds.width*y))+2] = n;
		}
	}
	clouds.updatePixels();
	image(clouds, 0, 0, clouds.width, clouds.height,0,0,width,height);
	
	
	drawFrame();
}