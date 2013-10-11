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

extern "C" {
	#include "libswd.h"
}


class ARMDebug
{
public:
	/**
	 * (Re)initialize the debug interface, and identify the connected chip.
	 * This resets the target chip, putting it in SWD mode and logging its
	 * identity.
	 *
	 * All functions here return >= 0 on success, or a negative libswd error 
	 * code on failure. Errors are also logged, so generally you don't need
	 * to do that yourself.
	 */
	int begin(unsigned clockPin, unsigned dataPin, libswd_loglevel_t logLevel = LIBSWD_LOGLEVEL_NORMAL);

	/// Deinitialize the debug interface, if it's been initialized.
	void end();

	/// Memory store
	int memStore(uint32_t addr, uint32_t data);
	int memStore(uint32_t addr, uint32_t *data, unsigned count);

	/// Memory load
	int memLoad(uint32_t addr, uint32_t &data);
	int memLoad(uint32_t addr, uint32_t *data, unsigned count);

	// Low-level interface (LSB-first)
	void mosi_transfer(uint32_t data, unsigned nBits);
	uint32_t miso_transfer(unsigned nBits);
	void mosi_trn(unsigned nClocks);
	void miso_trn(unsigned nClocks);

	// Low-level access to libswd
	libswd_ctx_t *getContext() { return libswdctx; }

private:
	uint8_t clockPin, dataPin;
	libswd_ctx_t *libswdctx;
};
