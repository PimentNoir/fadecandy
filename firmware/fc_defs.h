/*
 * Fadecandy Firmware
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

/*
 * Definitions for Fadecandy, based on the number of supported LEDs
 */

#pragma once

#define LEDS_PER_STRIP          64
#define LEDS_TOTAL              (LEDS_PER_STRIP * 8)
#define CHANNELS_TOTAL          (LEDS_TOTAL * 3)

#define LUT_CH_SIZE             257
#define LUT_TOTAL_SIZE          (LUT_CH_SIZE * 3)

// USB packet layout
#define PIXELS_PER_PACKET       21
#define LUTENTRIES_PER_PACKET   31
#define PACKETS_PER_FRAME       25
#define PACKETS_PER_LUT         25

#define NUM_USB_BUFFERS         104       // Three full frames (3*25), one LUT buffer (25), a little extra (4)

#define VENDOR_ID               0x1d50    // OpenMoko
#define PRODUCT_ID              0x607a    // Assigned to Fadecandy project
#define DEVICE_VER              0x0108	  // BCD device version
#define DEVICE_VER_STRING		"1.08"
