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

#include <math.h>
#include <algorithm>
#include "OctoWS2811z.h"
#include "arm_math.h"
#include "fc_usb.h"
#include "fc_defs.h"

// USB data buffers
static fcBuffers buffers;

// Double-buffered DMA memory for raw bit planes of output
static DMAMEM int ledBuffer[LEDS_PER_STRIP * 12];
static OctoWS2811z leds(LEDS_PER_STRIP, ledBuffer, WS2811_800kHz);

// Residuals for temporal dithering
static int8_t residual[CHANNELS_TOTAL];


ALWAYS_INLINE static inline uint32_t lutInterpolate(const uint16_t *lut, uint32_t arg)
{
    /*
     * Using our color LUT for the indicated channel, convert the
     * 16-bit intensity "arg" in our input colorspace to a corresponding
     * 16-bit intensity in the device colorspace.
     */

    unsigned index = arg >> 8;
    unsigned alpha = arg & 0xFF;
    unsigned invAlpha = 0x100 - alpha;

    return (lut[index] * invAlpha + lut[index + 1] * alpha) >> 8;
}

static inline uint32_t updatePixel(uint32_t icPrev, uint32_t icNext, unsigned n)
{
    /*
     * Update pipeline for one pixel:
     *
     *    1. Interpolate framebuffer
     *    2. Interpolate LUT
     *    3. Dithering
     */

    const uint8_t *pixelPrev = buffers.fbPrev->pixel(n);
    const uint8_t *pixelNext = buffers.fbNext->pixel(n);

    // Per-channel linear interpolation and conversion to 16-bit color.
    int iR = (pixelPrev[0] * icPrev + pixelNext[0] * icNext) >> 16;
    int iG = (pixelPrev[1] * icPrev + pixelNext[1] * icNext) >> 16;
    int iB = (pixelPrev[2] * icPrev + pixelNext[2] * icNext) >> 16;

    // Pass through our color LUT
    iR = lutInterpolate(&buffers.lutCurrent[0 * 256], iR);
    iG = lutInterpolate(&buffers.lutCurrent[1 * 256], iG);
    iB = lutInterpolate(&buffers.lutCurrent[2 * 256], iB);

    // Pointer to the residual buffer for this pixel
    int8_t *pResidual = &residual[n * 3];

    // Incorporate the residual from last frame
    iR += pResidual[0];
    iG += pResidual[1];
    iB += pResidual[2];

    /*
     * Round to the nearest 8-bit value. Clamping is necessary!
     * This value might be as low as -128 prior to adding 0x80
     * for rounding. After this addition, the result is guaranteed
     * to be >= 0, but it may be over 0xffff.
     *
     * This rules out clamping using the UQADD16 instruction,
     * since the addition itself needs to allow overflow. Instead,
     * we clamp using a separate USAT instruction.
     */

    int r8 = __USAT(iR + 0x80, 16) >> 8;
    int g8 = __USAT(iG + 0x80, 16) >> 8;
    int b8 = __USAT(iB + 0x80, 16) >> 8;

    /*
     * Compute the error, after expanding the 8-bit value back to 16-bit.
     * Clamping (e.g. via __SSAT) is not necessary, since the error will not
     * be greater than +/- 127.
     */

    pResidual[0] = iR - (r8 * 257);
    pResidual[1] = iG - (g8 * 257);
    pResidual[2] = iB - (b8 * 257);

    // Pack the result, in GRB order.
    return (g8 << 16) | (r8 << 8) | b8;
}

static void updateDrawBuffer(unsigned interpCoefficient)
{
    /*
     * Update the LED draw buffer. In one step, we do the interpolation,
     * gamma correction, dithering, and we convert packed-pixel data to the
     * planar format used for OctoWS2811 DMAs.
     *
     * "interpCoefficient" indicates how far between fbPrev and fbNext
     * we are. It is a fixed point value in the range [0x0000, 0x10000],
     * corresponding to 100% fbPrev and 100% fbNext, respectively.
     */

    // For each pixel, this is a 24-byte stream of bits (6 words)
    uint32_t *out = (uint32_t*) leds.getDrawBuffer();

    // Interpolation coefficients, including a multiply by 257 to convert 8-bit color to 16-bit color.
    uint32_t icPrev = 257 * (0x10000 - interpCoefficient);
    uint32_t icNext = 257 * interpCoefficient;

    for (int i = 0; i < LEDS_PER_STRIP; ++i) {

        // Eight bit planes
        union {
            uint32_t word;
            struct {
                uint32_t x0:1, x1:1, x2:1, x3:1, x4:1, x5:1, x6:1, x7:1,
                         y0:1, y1:1, y2:1, y3:1, y4:1, y5:1, y6:1, y7:1,
                         z0:1, z1:1, z2:1, z3:1, z4:1, z5:1, z6:1, z7:1,
                         spare:8;
            };
        } p0, p1, p2, p3, p4, p5, p6, p7;

        // Six output words
        union {
            uint32_t word;
            struct {
                uint32_t p0a:1, p1a:1, p2a:1, p3a:1, p4a:1, p5a:1, p6a:1, p7a:1,
                         p0b:1, p1b:1, p2b:1, p3b:1, p4b:1, p5b:1, p6b:1, p7b:1,
                         p0c:1, p1c:1, p2c:1, p3c:1, p4c:1, p5c:1, p6c:1, p7c:1,
                         p0d:1, p1d:1, p2d:1, p3d:1, p4d:1, p5d:1, p6d:1, p7d:1;
            };
        } o0, o1, o2, o3, o4, o5;

        /*
         * Remap bits.
         * This generates fairly efficient code using the UBFX and BFI instructions.
         */

        p0.word = updatePixel(icPrev, icNext, i + LEDS_PER_STRIP * 0);

        o5.p0d = p0.x0;
        o5.p0c = p0.x1;
        o5.p0b = p0.x2;
        o5.p0a = p0.x3;
        o4.p0d = p0.x4;
        o4.p0c = p0.x5;
        o4.p0b = p0.x6;
        o4.p0a = p0.x7;
        o3.p0d = p0.y0;
        o3.p0c = p0.y1;
        o3.p0b = p0.y2;
        o3.p0a = p0.y3;
        o2.p0d = p0.y4;
        o2.p0c = p0.y5;
        o2.p0b = p0.y6;
        o2.p0a = p0.y7;
        o1.p0d = p0.z0;
        o1.p0c = p0.z1;
        o1.p0b = p0.z2;
        o1.p0a = p0.z3;
        o0.p0d = p0.z4;
        o0.p0c = p0.z5;
        o0.p0b = p0.z6;
        o0.p0a = p0.z7;

        p1.word = updatePixel(icPrev, icNext, i + LEDS_PER_STRIP * 1);

        o5.p1d = p1.x0;
        o5.p1c = p1.x1;
        o5.p1b = p1.x2;
        o5.p1a = p1.x3;
        o4.p1d = p1.x4;
        o4.p1c = p1.x5;
        o4.p1b = p1.x6;
        o4.p1a = p1.x7;
        o3.p1d = p1.y0;
        o3.p1c = p1.y1;
        o3.p1b = p1.y2;
        o3.p1a = p1.y3;
        o2.p1d = p1.y4;
        o2.p1c = p1.y5;
        o2.p1b = p1.y6;
        o2.p1a = p1.y7;
        o1.p1d = p1.z0;
        o1.p1c = p1.z1;
        o1.p1b = p1.z2;
        o1.p1a = p1.z3;
        o0.p1d = p1.z4;
        o0.p1c = p1.z5;
        o0.p1b = p1.z6;
        o0.p1a = p1.z7;

        p2.word = updatePixel(icPrev, icNext, i + LEDS_PER_STRIP * 2);

        o5.p2d = p2.x0;
        o5.p2c = p2.x1;
        o5.p2b = p2.x2;
        o5.p2a = p2.x3;
        o4.p2d = p2.x4;
        o4.p2c = p2.x5;
        o4.p2b = p2.x6;
        o4.p2a = p2.x7;
        o3.p2d = p2.y0;
        o3.p2c = p2.y1;
        o3.p2b = p2.y2;
        o3.p2a = p2.y3;
        o2.p2d = p2.y4;
        o2.p2c = p2.y5;
        o2.p2b = p2.y6;
        o2.p2a = p2.y7;
        o1.p2d = p2.z0;
        o1.p2c = p2.z1;
        o1.p2b = p2.z2;
        o1.p2a = p2.z3;
        o0.p2d = p2.z4;
        o0.p2c = p2.z5;
        o0.p2b = p2.z6;
        o0.p2a = p2.z7;

        p3.word = updatePixel(icPrev, icNext, i + LEDS_PER_STRIP * 3);

        o5.p3d = p3.x0;
        o5.p3c = p3.x1;
        o5.p3b = p3.x2;
        o5.p3a = p3.x3;
        o4.p3d = p3.x4;
        o4.p3c = p3.x5;
        o4.p3b = p3.x6;
        o4.p3a = p3.x7;
        o3.p3d = p3.y0;
        o3.p3c = p3.y1;
        o3.p3b = p3.y2;
        o3.p3a = p3.y3;
        o2.p3d = p3.y4;
        o2.p3c = p3.y5;
        o2.p3b = p3.y6;
        o2.p3a = p3.y7;
        o1.p3d = p3.z0;
        o1.p3c = p3.z1;
        o1.p3b = p3.z2;
        o1.p3a = p3.z3;
        o0.p3d = p3.z4;
        o0.p3c = p3.z5;
        o0.p3b = p3.z6;
        o0.p3a = p3.z7;

        p4.word = updatePixel(icPrev, icNext, i + LEDS_PER_STRIP * 4);

        o5.p4d = p4.x0;
        o5.p4c = p4.x1;
        o5.p4b = p4.x2;
        o5.p4a = p4.x3;
        o4.p4d = p4.x4;
        o4.p4c = p4.x5;
        o4.p4b = p4.x6;
        o4.p4a = p4.x7;
        o3.p4d = p4.y0;
        o3.p4c = p4.y1;
        o3.p4b = p4.y2;
        o3.p4a = p4.y3;
        o2.p4d = p4.y4;
        o2.p4c = p4.y5;
        o2.p4b = p4.y6;
        o2.p4a = p4.y7;
        o1.p4d = p4.z0;
        o1.p4c = p4.z1;
        o1.p4b = p4.z2;
        o1.p4a = p4.z3;
        o0.p4d = p4.z4;
        o0.p4c = p4.z5;
        o0.p4b = p4.z6;
        o0.p4a = p4.z7;

        p5.word = updatePixel(icPrev, icNext, i + LEDS_PER_STRIP * 5);

        o5.p5d = p5.x0;
        o5.p5c = p5.x1;
        o5.p5b = p5.x2;
        o5.p5a = p5.x3;
        o4.p5d = p5.x4;
        o4.p5c = p5.x5;
        o4.p5b = p5.x6;
        o4.p5a = p5.x7;
        o3.p5d = p5.y0;
        o3.p5c = p5.y1;
        o3.p5b = p5.y2;
        o3.p5a = p5.y3;
        o2.p5d = p5.y4;
        o2.p5c = p5.y5;
        o2.p5b = p5.y6;
        o2.p5a = p5.y7;
        o1.p5d = p5.z0;
        o1.p5c = p5.z1;
        o1.p5b = p5.z2;
        o1.p5a = p5.z3;
        o0.p5d = p5.z4;
        o0.p5c = p5.z5;
        o0.p5b = p5.z6;
        o0.p5a = p5.z7;

        p6.word = updatePixel(icPrev, icNext, i + LEDS_PER_STRIP * 6);

        o5.p6d = p6.x0;
        o5.p6c = p6.x1;
        o5.p6b = p6.x2;
        o5.p6a = p6.x3;
        o4.p6d = p6.x4;
        o4.p6c = p6.x5;
        o4.p6b = p6.x6;
        o4.p6a = p6.x7;
        o3.p6d = p6.y0;
        o3.p6c = p6.y1;
        o3.p6b = p6.y2;
        o3.p6a = p6.y3;
        o2.p6d = p6.y4;
        o2.p6c = p6.y5;
        o2.p6b = p6.y6;
        o2.p6a = p6.y7;
        o1.p6d = p6.z0;
        o1.p6c = p6.z1;
        o1.p6b = p6.z2;
        o1.p6a = p6.z3;
        o0.p6d = p6.z4;
        o0.p6c = p6.z5;
        o0.p6b = p6.z6;
        o0.p6a = p6.z7;

        p7.word = updatePixel(icPrev, icNext, i + LEDS_PER_STRIP * 7);

        o5.p7d = p7.x0;
        o5.p7c = p7.x1;
        o5.p7b = p7.x2;
        o5.p7a = p7.x3;
        o4.p7d = p7.x4;
        o4.p7c = p7.x5;
        o4.p7b = p7.x6;
        o4.p7a = p7.x7;
        o3.p7d = p7.y0;
        o3.p7c = p7.y1;
        o3.p7b = p7.y2;
        o3.p7a = p7.y3;
        o2.p7d = p7.y4;
        o2.p7c = p7.y5;
        o2.p7b = p7.y6;
        o2.p7a = p7.y7;
        o1.p7d = p7.z0;
        o1.p7c = p7.z1;
        o1.p7b = p7.z2;
        o1.p7a = p7.z3;
        o0.p7d = p7.z4;
        o0.p7c = p7.z5;
        o0.p7b = p7.z6;
        o0.p7a = p7.z7;

        *(out++) = o0.word;
        *(out++) = o1.word;
        *(out++) = o2.word;
        *(out++) = o3.word;
        *(out++) = o4.word;
        *(out++) = o5.word;
    }
}

extern "C" int main()
{
    leds.begin();

    while (1) {
        buffers.handleUSB();

        updateDrawBuffer((millis() << 2) & 0xFFFF);
        leds.show();
    }
}
