/*
 * Fadecandy Firmware
 * For Arduino with Teensyduino and OctoWS2811.
 *
 * Copyright <c> 2013 Micah Elizabeth Scott. <micah@scanlime.org>
 */

#include <OctoWS2811.h>
#include <math.h>
#include "hcolor.h"

static const int ledsPerStrip = 64;
static const int ledsTotal = ledsPerStrip * 8;

DMAMEM int displayMemory[ledsPerStrip * 6];
int drawingMemory[ledsPerStrip * 6];
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, WS2811_GRB | WS2811_800kHz);
HPixelBuffer<ledsTotal> pixbuf;

void setup()
{
    leds.begin();
    Serial.begin(115200);

    for (unsigned i = 0; i < ledsTotal; ++i) {
        pixbuf.pixels[i].color = HColor16(0,0,0);
    }
}

void loop()
{
	// XXX: Proof of concept

	for (int i = 0; i < 16; i++) {
		unsigned c = pow(sin(i * 0.2 + millis() * 0.0005) * 0.5 + 0.5, 2.2) * 0x1000;
	    pixbuf.pixels[i].color = HColor16(c>>2, c, c>>1);
   	}
	
	for (int i = 0; i < 100; i++) {
		pixbuf.show(leds);
	}
}
