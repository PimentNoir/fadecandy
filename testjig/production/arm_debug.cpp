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

#include <Arduino.h>
#include "arm_debug.h"
extern "C" {
	#include "libswd.h"
}

void ARMDebug::begin(unsigned clockPin, unsigned dataPin)
{
	libswd_ctx_t *libswdctx;
	libswdctx = libswd_init();
	context = libswdctx;

	this->clockPin = clockPin;
	this->dataPin = dataPin;
	libswdctx->driver->device = this;

	pinMode(this->clockPin, OUTPUT);
	pinMode(this->dataPin, INPUT_PULLUP);
}

void ARMDebug::end()
{
	libswd_ctx_t *libswdctx = (libswd_ctx_t*) this->context;
	libswd_deinit(libswdctx);
	this->context = 0;
}

bool ARMDebug::identify()
{
	libswd_ctx_t *libswdctx = (libswd_ctx_t*) this->context;
	int *idcode;

	if (libswd_dap_detect(libswdctx, LIBSWD_OPERATION_EXECUTE, &idcode) < 0)
		return false;

	libswd_log(libswdctx, LIBSWD_LOGLEVEL_NORMAL,
		"Found ARM processor. IDCODE: 0x%X (%s)\n",
		*idcode, libswd_bin32_string(idcode));
	return true;
}

void ARMDebug::bit_delay()
{
	delayMicroseconds(1);
}

void ARMDebug::mosi_transfer(uint32_t data, unsigned nBits)
{
	while (nBits--) {
		digitalWrite(this->clockPin, LOW);
		digitalWrite(this->dataPin, data & 1);
		data >>= 1;
		bit_delay();
		digitalWrite(this->clockPin, HIGH);
		bit_delay();
	}
}

uint32_t ARMDebug::miso_transfer(unsigned nBits)
{
	uint32_t result = 0;
	uint32_t mask = 1;
	while (nBits--) {
		digitalWrite(this->clockPin, LOW);
		bit_delay();
		if (digitalRead(this->dataPin)) {
			result |= mask;
		}
		mask <<= 1;
		digitalWrite(this->clockPin, HIGH);
		bit_delay();
	}
	return result;
}

void ARMDebug::mosi_trn(unsigned nClocks)
{
	pinMode(this->dataPin, INPUT_PULLUP);
	while (nClocks--) {
		digitalWrite(this->clockPin, LOW);
		bit_delay();
		digitalWrite(this->clockPin, HIGH);
		bit_delay();
	}
	pinMode(this->dataPin, OUTPUT);
}

void ARMDebug::miso_trn(unsigned nClocks)
{
	digitalWrite(this->dataPin, HIGH);
	pinMode(this->dataPin, INPUT_PULLUP);
	while (nClocks--) {
		digitalWrite(this->clockPin, LOW);
		bit_delay();
		digitalWrite(this->clockPin, HIGH);
		bit_delay();
	}
}

extern "C" int
libswd_log(libswd_ctx_t *libswdctx, libswd_loglevel_t loglevel, char *msg, ...)
{
	if (loglevel < LIBSWD_LOGLEVEL_MIN && loglevel > LIBSWD_LOGLEVEL_MAX)
		return LIBSWD_ERROR_LOGLEVEL;

	if (loglevel > libswdctx->config.loglevel)
		return LIBSWD_OK;

	if (!Serial)
		return LIBSWD_OK;

	va_list ap;
	char buffer[256];

	va_start(ap, msg);
	int ret = vsnprintf(buffer, sizeof buffer, msg, ap);
	va_end(ap);

	Serial.print(buffer);
	return ret;
}

extern "C" int
libswd_drv_mosi_8(libswd_ctx_t *libswdctx, libswd_cmd_t *cmd, char *data, int bits, int nLSBfirst)
{
	ARMDebug *self = (ARMDebug*) libswdctx->driver->device;
	if (nLSBfirst == LIBSWD_DIR_MSBFIRST) {
		unsigned char lData = *data;
		libswd_bin8_bitswap(&lData, bits);
		self->mosi_transfer(lData, bits);
	} else {
		self->mosi_transfer(*data, bits);
	}
	return bits;
}

extern "C" int
libswd_drv_mosi_32(libswd_ctx_t *libswdctx, libswd_cmd_t *cmd, int *data, int bits, int nLSBfirst)
{
	ARMDebug *self = (ARMDebug*) libswdctx->driver->device;
	if (nLSBfirst == LIBSWD_DIR_MSBFIRST) {
		unsigned int lData = *data;
		libswd_bin32_bitswap(&lData, bits);
		self->mosi_transfer(lData, bits);
	} else {
		self->mosi_transfer(*data, bits);
	}
	return bits;
}

extern "C" int
libswd_drv_miso_8(libswd_ctx_t *libswdctx, libswd_cmd_t *cmd, char *data, int bits, int nLSBfirst)
{
	ARMDebug *self = (ARMDebug*) libswdctx->driver->device;
	*data = self->miso_transfer(bits);
	if (nLSBfirst == LIBSWD_DIR_MSBFIRST)
		libswd_bin8_bitswap((unsigned char*) data, bits);
	return bits;
}

extern "C" int
libswd_drv_miso_32(libswd_ctx_t *libswdctx, libswd_cmd_t *cmd, int *data, int bits, int nLSBfirst)
{
	ARMDebug *self = (ARMDebug*) libswdctx->driver->device;
	*data = self->miso_transfer(bits);
	if (nLSBfirst == LIBSWD_DIR_MSBFIRST)
		libswd_bin32_bitswap((unsigned int*) data, bits);
	return bits;
}

extern "C" int
libswd_drv_mosi_trn(libswd_ctx_t *libswdctx, int clks)
{
	ARMDebug *self = (ARMDebug*) libswdctx->driver->device;
	self->mosi_trn(clks);
	return clks;
}

extern "C" int
libswd_drv_miso_trn(libswd_ctx_t *libswdctx, int clks)
{
	ARMDebug *self = (ARMDebug*) libswdctx->driver->device;
	self->miso_trn(clks);
	return clks;
}
