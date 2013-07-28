/*
 * Measuring Stick - a quick tool for measuring out 64-LED strips.
 *
 * This is a TeensyDuino sketch which lights the first 64 LEDs
 * on a strip. LEDs 0, 10, 20, 30, 40, 50, and 60 are green,
 * the others are dim white. The entire strip is kept dim enough
 * that this can be used as a battery-powered device that you can
 * plug into LED strips while you work with them.
 */

 #include <OctoWS2811.h>

const int ledsPerStrip = 128;
DMAMEM int displayMemory[ledsPerStrip*6];
int drawingMemory[ledsPerStrip*6];

OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, WS2811_GRB | WS2811_800kHz);

void setup() {
	leds.begin();

	for (int i = 0; i < ledsPerStrip; ++i)
		leds.setPixel(i, 0x000000);

	for (int i = 0; i < 64; ++i)
		leds.setPixel(i, 0x101010);

	for (int i = 0; i < 64; i += 10)
		leds.setPixel(i, 0x104010);
}

void loop() {
	leds.show();
}

