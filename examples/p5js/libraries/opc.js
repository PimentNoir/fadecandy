/*
 * Simple Open Pixel Control client for P5js,
 * designed to sample each LED's color from some point on the canvas.
 *
 * Micah Elizabeth Scott, 2013
 * Ported to P5js by Matthew I. Kessler
 * This file is released into the public domain.
 */

/*
 *!!THIS SCRIPT MUST BE LOADED IN HTML BEFORE THE DRAW SCRIPT!!
 *
 *Example for HTML head:
 *<script src="libraries/opc.js" type="text/javascript"></script>
 *<script src="strip64_flames.js" type="text/javascript"></script>
 */

// Arrays for pixels[]'s locations to send rgb values to fcServer.
var pixelLocationsRed = [];
var pixelLocationsGre = [];
var pixelLocationsBlu = [];

// Arrays for to map pixels on screen.
var ledXpoints = [];
var ledYpoints = [];

// Enable locations on screen.
var enableShowLocations;

//New WebSocket.
var socket;
function socketSetup(WebSocketAddress) {
	socket = new WebSocket(WebSocketAddress);
	enableShowLocations = true;
}

// Set the location of a single LED.
function led(index, x, y) {
	loadPixels();
	if (pixelLocationsRed === null) {
		pixelLocationsRed.length = index + 1;
		pixelLocationsGre.length = index + 1;
		pixelLocationsBlu.length = index + 1;
		ledXpoints.length = index + 1;
		ledYpoints.length = index + 1;
	} else if (index >= pixelLocationsRed.length) {
		pixelLocationsRed.length = index + 1;
		pixelLocationsGre.length = index + 1;
		pixelLocationsBlu.length = index + 1;
		ledXpoints.length = index + 1;
		ledYpoints.length = index + 1;
	}
	//Store pixel[] map to color arrays.
	var pixelD = pixelDensity();
	var idx = pixelD*pixelD*4*y*width+x*pixelD*4;
	pixelLocationsRed[index] = (idx);
	pixelLocationsGre[index] = (idx + 1);
	pixelLocationsBlu[index] = (idx + 2);
	//Store x,y to draw points for pixel locations 
	ledXpoints[index] = x;
	ledYpoints[index] = y;
}

// Set the location of several LEDs arranged in a strip.
// Angle is in radians, measured clockwise from +X.
// (x,y) is the center of the strip.
function ledStrip(index, count, x, y, spacing, angle, reversed) {
	var s = sin(angle);
	var c = cos(angle);
	for (var i = 0; i < count; i++) {
		led(
			reversed ? (index + count - 1 - i) * 1 : (index + i) * 1,
			//floor() These must be integers.  round() causes lag
			floor((x + (i - (count - 1) / 2.0) * spacing * c + 0.5) * 1),
			floor((y + (i - (count - 1) / 2.0) * spacing * s + 0.5) * 1));
	}
}

// Set the locations of a ring of LEDs. The center of the ring is at (x, y),
// with "radius" pixels between the center and each LED. The first LED is at
// the indicated angle, in radians, measured clockwise from +X.
function ledRing(index, count, x, y, radius, angle) {
	for (var i = 0; i < count; i++) {
		var a = angle + i * 2 * PI / count;
		led(
			index + i,
			floor((x - radius * cos(a) + 0.5)),
			floor((y - radius * sin(a) + 0.5))
		);
	}
}

// Set the location of several LEDs arranged in a grid. The first strip is
// at 'angle', measured in radians clockwise from +X.
// (x,y) is the center of the grid.
function ledGrid(index, stripLength, numStrips, x, y, ledSpacing, stripSpacing, angle, zigzag) {
	var s = sin(angle + HALF_PI);
	var c = cos(angle + HALF_PI);
	for (var i = 0; i < numStrips; i++) {
		ledStrip(
			index + stripLength * i,
			stripLength,
			x + (i - (numStrips - 1) / 2.0) * stripSpacing * c,
			y + (i - (numStrips - 1) / 2.0) * stripSpacing * s,
			ledSpacing,
			angle,
			zigzag && (i % 2) == 1);
	}
}

// Set the location of 64 LEDs arranged in a uniform 8x8 grid.
// (x,y) is the center of the grid.
function ledGrid8x8(index, x, y, spacing, angle, zigzag) {
	ledGrid(index, 8, 8, x, y, spacing, spacing, angle, zigzag);
}

//Called in function draw(){...} on last line.
function drawFrame() {
	if (pixelLocationsRed === null) {
		// No pixels defined yet
		return;
	}
	var leds = pixelLocationsRed.length;
	var packet = new Uint8ClampedArray(4 + leds * 3);

	if (socket.readyState != 1 /* OPEN */ ) {
		// The server connection isn't open. Nothing to do.
		return;
	}

	if (socket.bufferedAmount > packet.length) {
		// The network is lagging, and we still haven't sent the previous frame.
		// Don't flood the network, it will just make us laggy.
		// If fcserver is running on the same computer, it should always be able
		// to keep up with the frames we send, so we shouldn't reach this point.
		return;
	}

	// Dest position in our packet. Start right after the header.
	var dest = 4;
	loadPixels();

	// Sample and send the center pixel of each LED
	for (var led = 0; led < leds; led++) {
		var i = led;
		packet[dest++] = pixels[pixelLocationsRed[i]];
		packet[dest++] = pixels[pixelLocationsGre[i]];
		packet[dest++] = pixels[pixelLocationsBlu[i]];
	}
	socket.send(packet.buffer);

	//draw pixel locations on screen if enabled
	if (showPixelLocations === true) {
		for (i = 0; i < leds; i++) {
			stroke(127);
			//offset x+1 and y+1 so we don't send the dots to the fc Server
			point(ledXpoints[i]+1, ledYpoints[i]+1);
		}
	}
}