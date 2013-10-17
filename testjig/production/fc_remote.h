/*
 * Remote control for the Fadecandy firmware, using the ARM debug interface.
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
#include "arm_kinetis_debug.h"

class FcRemote
{
public:
    FcRemote(ARMKinetisDebug &target) : target(target) {}

    bool installFirmware();
    bool boot();

    // Set remote LED
    bool setLED(bool on);

    // Set control flags
    bool setFlags(uint8_t cflag);

    // Measure actual frame rate of Fadecandy firmware
    float measureFrameRate(float minDuration = 1.0);
    bool testFrameRate();

    // Direct framebuffer access
    bool initLUT();
    bool setLUT(unsigned channel, unsigned index, int value);
    bool setPixel(unsigned index, int red, int green, int blue);

private:
    ARMKinetisDebug &target;
};

#define CFLAG_NO_DITHERING      (1 << 0)
#define CFLAG_NO_INTERPOLATION  (1 << 1)
#define CFLAG_NO_ACTIVITY_LED   (1 << 2)
#define CFLAG_LED_CONTROL       (1 << 3)
#define LUT_CH_SIZE             257
#define LUT_TOTAL_SIZE          (LUT_CH_SIZE * 3)
#define PIXELS_PER_PACKET       21
#define LUTENTRIES_PER_PACKET   31
#define PACKETS_PER_FRAME       25
#define PACKETS_PER_LUT         25
