/*
 * Fadecandy Firmware
 * 
 * Copyright (c) 2013 Micah Elizabeth Scott
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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

extern "C" int main()
{
    leds.begin();

    for (int i = 0; i < ledsTotal; ++i) {
        pixbuf.pixels[i].color = HColor16(0,0,0);
    }

    // Quick and dirty...
    for (unsigned i = 0; i < 1024; ++i) {
    	sinelut[i] = pow(sin(i * (M_PI * 2 / 1024)) * 0.5 + 0.5, 2.2) * 0x1000;
    }

    while (1) {
    	// XXX: Proof of concept

    	for (int i = 0; i < 16; i++) {
    		int t = (millis() >> 2) + (i << 4);
    		unsigned c = sinelut[t & 1023];
    	    pixbuf.pixels[i].color = HColor16(c>>2, c, c>>1);
       	}
    	
    	pixbuf.show(leds);
    }
}
