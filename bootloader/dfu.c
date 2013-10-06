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

#include "usb_dev.h"
#include "serial.h"

typedef enum {
	appIDLE = 0,
	appDETACH,
	dfuIDLE,
	dfuDNLOAD_SYNC,
	dfuDNBUSY,
	dfuDNLOAD_IDLE,
	dfuMANIFEST_SYNC,
	dfuMANIFEST,
	dfuMANIFEST_WAIT_RESET,
	dfuUPLOAD_IDLE,
	dfuERROR
} dfu_state_t;

typedef enum {
	OK = 0,
	errTARGET,
	errFILE,
	errWRITE,
	errERASE,
	errCHECK_ERASED,
	errPROG,
	errVERIFY,
	errADDRESS,
	errNOTDONE,
	errFIRMWARE,
	errVENDOR,
	errUSBR,
	errPOR,
	errUNKNOWN,
	errSTALLEDPKT,
} dfu_status_t;


static dfu_state_t dfu_state = dfuIDLE;
static dfu_status_t dfu_status = OK;
static unsigned dfu_poll_timeout = 1;


void dfu_download(unsigned blockNum, unsigned length, const uint8_t *data)
{
	serial_phex32(blockNum);
	serial_putchar(' ');
	serial_phex32(length);
	serial_putchar('\n');
}

void dfu_getstatus(uint8_t *status)
{
	status[0] = dfu_status;
	status[1] = dfu_poll_timeout >> 16;
	status[2] = dfu_poll_timeout >> 8;
	status[3] = dfu_poll_timeout;
	status[4] = dfu_state;
	status[5] = 0;  // iString
}

void dfu_clrstatus()
{
	if (dfu_state == dfuERROR) {
		dfu_state = dfuIDLE;
	}
}

uint8_t dfu_getstate()
{
	return dfu_state;
}

void dfu_abort()
{
	dfu_state = dfuIDLE;
}
