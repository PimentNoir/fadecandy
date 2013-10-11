/*
 * Simple ARM debug interface for Arduino, using libswd
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

#pragma once
#include <stdint.h>
#include <stdbool.h>

class ARMDebug
{
public:
	// Init or de-init the debug interface itself
	void begin(unsigned clockPin, unsigned dataPin);
	void end();

	/*
	 * Identify the connected chip. Tries to reset it, put it in SWD mode,
	 * and log its identity. If successful, this returns 'true'. If something is wrong
	 * (the chip isn't responding) returns 'false', and logs an error.
	 */
	bool identify();

	// Low-level interface (LSB-first)
	void mosi_transfer(uint32_t data, unsigned nBits);
	uint32_t miso_transfer(unsigned nBits);
	void mosi_trn(unsigned nClocks);
	void miso_trn(unsigned nClocks);

private:
	uint8_t clockPin, dataPin;
	void *context;	 // libswd context (opaque type)

	void bit_delay();
};
