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


int ARMDebug::begin(unsigned clockPin, unsigned dataPin, libswd_loglevel_t logLevel)
{
	int *idcode;
	int ret;

	end();

	this->clockPin = clockPin;
	this->dataPin = dataPin;
	pinMode(clockPin, OUTPUT);
	pinMode(dataPin, INPUT_PULLUP);

	libswdctx = libswd_init();
	libswdctx->driver->device = this;
	libswdctx->config.autofixerrors = false;
	libswd_log_level_set(libswdctx, logLevel);

 	ret = libswd_dap_detect(libswdctx, LIBSWD_OPERATION_EXECUTE, &idcode);
	if (ret >= 0) {
		libswd_log(libswdctx, LIBSWD_LOGLEVEL_NORMAL,
			"Found ARM processor. IDCODE: 0x%X (%s)\n",
			*idcode, libswd_bin32_string(idcode));
	}

	libswd_cmdq_free_head(libswdctx->cmdq);
	return ret;
}

void ARMDebug::end()
{
	if (libswdctx) {
		free(libswdctx->driver);
		libswdctx->driver = 0;
		libswd_deinit(libswdctx);
		libswdctx = 0;
	}
}

int ARMDebug::memStore(uint32_t addr, uint32_t data, uint8_t accessPort)
{
	return 0;
}

int ARMDebug::memLoad(uint32_t addr, uint32_t &data, uint8_t accessPort)
{
	int ret = 0;

	ret = libswd_ap_select(libswdctx, LIBSWD_OPERATION_EXECUTE, accessPort);
	if (ret >= 0) {

		int *result = 0;
		ret = libswd_ap_read(libswdctx, LIBSWD_OPERATION_EXECUTE, 0xFC, &result);
		if (ret >= 0) {
			libswd_log(libswdctx, LIBSWD_LOGLEVEL_NORMAL,
				"FC -> %p %08x\n", result, *result);
		}
	}

	libswd_cmdq_free_head(libswdctx->cmdq);
	return ret;
}

void ARMDebug::mosi_transfer(uint32_t data, unsigned nBits)
{
	while (nBits--) {
		digitalWrite(clockPin, LOW);
		digitalWrite(dataPin, data & 1);
		data >>= 1;
		digitalWrite(clockPin, HIGH);
	}
}

uint32_t ARMDebug::miso_transfer(unsigned nBits)
{
	uint32_t result = 0;
	uint32_t mask = 1;
	while (nBits--) {
		digitalWrite(clockPin, LOW);
		if (digitalRead(dataPin)) {
			result |= mask;
		}
		mask <<= 1;
		digitalWrite(clockPin, HIGH);
	}
	return result;
}

void ARMDebug::mosi_trn(unsigned nClocks)
{
	pinMode(dataPin, INPUT_PULLUP);
	while (nClocks--) {
		digitalWrite(clockPin, LOW);
		digitalWrite(clockPin, HIGH);
	}
	pinMode(dataPin, OUTPUT);
}

void ARMDebug::miso_trn(unsigned nClocks)
{
	digitalWrite(dataPin, HIGH);
	pinMode(dataPin, INPUT_PULLUP);
	while (nClocks--) {
		digitalWrite(clockPin, LOW);
		digitalWrite(clockPin, HIGH);
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
