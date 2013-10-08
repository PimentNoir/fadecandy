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
#include "mk20dx128.h"
#include "usb_dev.h"
#include "dfu.h"

extern uint32_t debug;

static dfu_state_t dfu_state = dfuIDLE;
static dfu_status_t dfu_status = OK;
static unsigned dfu_poll_timeout = 1;

// Programming buffer in MK20DX128 FlexRAM, where the flash controller can quickly access it.
static __attribute__ ((section(".flexram"))) uint8_t dfu_buffer[DFU_TRANSFER_SIZE];

static void *memcpy(void *dst, const void *src, size_t cnt) {
	uint8_t *dst8 = dst;
	const uint8_t *src8 = src;
	while (cnt > 0) {
		cnt--;
		*(dst8++) = *(src8++);
	}
	return dst;
}

static bool ftfl_busy()
{
	// Is the flash memory controller busy?
	return 0 == (FTFL_FSTAT_CCIF & FTFL_FSTAT);
}

static void ftfl_busy_wait()
{
	// Wait for the flash memory controller to finish any pending operation.
	while (ftfl_busy());
}

static void ftfl_launch_command()
{
	// Begin a flash memory controller command
	FTFL_FSTAT = FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL | FTFL_FSTAT_RDCOLERR;
	FTFL_FSTAT = FTFL_FSTAT_CCIF;
}

static void ftfl_set_flexram_function(uint8_t control_code)
{
	// Issue a Set FlexRAM Function command
	
	ftfl_busy_wait();
	FTFL_FCCOB0 = 0x81;
	FTFL_FCCOB1 = control_code;
	ftfl_launch_command();
	ftfl_busy_wait();
}

void dfu_init()
{
	// Use FlexRAM (dfu_buffer) as normal RAM.
	ftfl_set_flexram_function(0xFF);
}

uint8_t dfu_getstate()
{
	return dfu_state;
}

bool dfu_download(unsigned blockNum, unsigned blockLength,
	unsigned packetOffset, unsigned packetLength, const uint8_t *data)
{
	if (packetOffset + packetLength > DFU_TRANSFER_SIZE ||
		packetOffset + packetLength > blockLength) {

		// Overflow!
		dfu_state = dfuERROR;
		dfu_status = errADDRESS;
		return false;
	}

	// Store more data...
	memcpy(dfu_buffer + packetOffset, data, packetLength);

	if (packetOffset + packetLength != blockLength) {
		// Still waiting for more data.
		return true;
	}

	// Full block received.

	switch (dfu_state) {

		case dfuIDLE:
		case dfuDNLOAD_IDLE:

			if (blockLength) {
				// Full block received
				dfu_state = dfuDNLOAD_SYNC;
				dfu_status = OK;
				return true;

			} else {
				// End of download
				dfu_state = dfuMANIFEST_SYNC;
				dfu_status = OK;
				return true;
			}

		default:
			dfu_state = dfuERROR;
			dfu_status = errSTALLEDPKT;
			return false;
	}

	return true;
}

bool dfu_getstatus(uint8_t *status)
{
	switch (dfu_state) {

		case dfuDNLOAD_SYNC:
		case dfuDNBUSY:
			if (ftfl_busy()) {
				dfu_state = dfuDNBUSY;
			} else {
				dfu_state = dfuDNLOAD_IDLE;
			}
			break;

		case dfuMANIFEST_SYNC:
			dfu_state = dfuMANIFEST;
			break;

		default:
			break;
	}

	status[0] = dfu_status;
	status[1] = dfu_poll_timeout;
	status[2] = dfu_poll_timeout >> 8;
	status[3] = dfu_poll_timeout >> 16;
	status[4] = dfu_state;
	status[5] = 0;  // iString

	return true;
}

bool dfu_clrstatus()
{
	switch (dfu_state) {

		case dfuERROR:
			// Clear an error
			dfu_state = dfuIDLE;
			dfu_status = OK;
			return true;

		default:
			// Unexpected request
			dfu_state = dfuERROR;
			dfu_status = errSTALLEDPKT;
			return false;
	}
}

bool dfu_abort()
{
	dfu_state = dfuIDLE;
	dfu_status = OK;
	return true;
}

void dfu_usb_reset()
{
	debug++;
}
