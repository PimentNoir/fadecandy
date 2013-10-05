/*
 * Fadecandy DFU Bootloader
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

#include <stdbool.h>
#include "usb_dev.h"
#include "serial.h"
#include "mk20dx128.h"


void led_on()
{
	// Set the status LED on PC5, as an indication that we're in bootloading mode.
	PORTC_PCR5 = PORT_PCR_MUX(1) | PORT_PCR_DSE | PORT_PCR_SRE;
	GPIOC_PDDR = 1 << 5;
	GPIOC_PDOR = 1 << 5;
}


bool test_banner_echo()
{
	/*
	 * At startup we print this banner out to the serial port.
	 * If we see it echo back to us, we enter bootloader mode no matter what.
	 * This is intended to be a foolproof way to enter recovery mode, even if other
	 * circuitry has been connected to the serial port.
	 */

	static char banner[] = "FC-Boot";
	const unsigned bannerLength = sizeof banner - 1;
	unsigned matched = 0;

	// Write banner
	serial_begin(BAUD2DIV(9600));
	serial_write(banner, sizeof banner - 1);

	// Newline is not technically part of the banner, so we can do the RX check
	// at a time when we're sure the other characters have arrived in the RX fifo.
	serial_putchar('\n');
	serial_flush();

	while (matched < bannerLength) {
		if (serial_available() && serial_getchar() == banner[matched]) {
			matched++;
		} else {
			break;
		}
	}

	serial_end();
	return matched == bannerLength;
}


int main()
{
	// Say hello!

	if (test_banner_echo()) {
		led_on();
	}

    usb_init();

    while (1) {
    }
}
