/*
 * Fadecandy Firmware
 * For Arduino with Teensyduino and OctoWS2811.
 *
 * Copyright <c> 2013 Micah Elizabeth Scott. <micah@scanlime.org>
 */

#include <OctoWS2811.h>
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

    for (unsigned i = 0; i < ledsTotal; ++i) {
        pixbuf.pixels[i].color = HColor16(0x0080, 0x0080, 0x0080);
    }
}

void loop()
{
    pixbuf.pixels[0].color.b = millis() & 0xffff;

    pixbuf.show(leds);
}
