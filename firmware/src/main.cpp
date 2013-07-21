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

uint16_t sinelut[1024];

void setup()
{
    leds.begin();
    Serial.begin(115200);

    for (unsigned i = 0; i < ledsTotal; ++i) {
        pixbuf.pixels[i].color = HColor16(0,0,0);
    }

    // Quick and dirty...
    for (unsigned i = 0; i < 1024; ++i) {
    	sinelut[i] = pow(sin(i * (M_PI * 2 / 1024)) * 0.5 + 0.5, 2.2) * 0x1000;
    }
}

void loop()
{
	// XXX: Proof of concept

	for (int i = 0; i < 16; i++) {
		int t = (millis() >> 2) + (i << 4);
		unsigned c = sinelut[t & 1023];
	    pixbuf.pixels[i].color = HColor16(c>>2, c, c>>1);
   	}
	
	pixbuf.show(leds);
}
