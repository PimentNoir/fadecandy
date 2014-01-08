/*
 * Fadecandy firmware
 * Copyright (c) 2013 Micah Elizabeth Scott
 *
 * Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2013 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows 
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _usb_dev_h_
#define _usb_dev_h_

// This header is NOT meant to be included when compiling
// user sketches in Arduino.  The low-level functions
// provided by usb_dev.c are meant to be called only by
// code which provides higher-level interfaces to the user.

#include "usb_mem.h"
#include "usb_desc.h"

#ifdef __cplusplus
extern "C" {
#endif

void usb_init(void);
void usb_init_serialnumber(void);
void usb_isr(void);

// External handler for received USB packets. Called in ISR context or main loop context.
// Returns true if the packet can be handled immediately, or false if it must be deferred.
int usb_rx_handler(usb_packet_t *packet);

// Called to retry packets that have been deferred.
void usb_rx_resume();


extern volatile uint8_t usb_configuration;
extern volatile uint8_t usb_dfu_state;

// DFU states
#define DFU_appIDLE    0
#define DFU_appDETACH  1

// Performance counters
extern volatile uint32_t perf_frameCounter;
extern volatile uint32_t perf_receivedKeyframeCounter;


#ifdef __cplusplus
}
#endif

#endif
