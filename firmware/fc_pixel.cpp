/*
 * Fadecandy Firmware: Low-level pixel update code
 * (Included into fadecandy.cpp)
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


ALWAYS_INLINE static inline uint32_t lutInterpolate(const uint16_t *lut, uint32_t arg)
{
    /*
     * Using our color LUT for the indicated channel, convert the
     * 16-bit intensity "arg" in our input colorspace to a corresponding
     * 16-bit intensity in the device colorspace.
     *
     * Remember that our LUT is 257 entries long. The final entry corresponds to an
     * input of 0x10000, which can't quite be reached.
     *
     * 'arg' is in the range [0, 0xFFFF]
     *
     * This operation is equivalent to the following:
     *
     *      unsigned index = arg >> 8;          // Range [0, 0xFF]
     *      unsigned alpha = arg & 0xFF;        // Range [0, 0xFF]
     *      unsigned invAlpha = 0x100 - alpha;  // Range [1, 0x100]
     *
     *      // Result in range [0, 0xFFFF]
     *      return (lut[index] * invAlpha + lut[index + 1] * alpha) >> 8;
     *
     * This is easy to understand, but it turns out to be a serious bottleneck
     * in terms of speed and memory bandwidth, as well as register pressure that
     * affects the compilation of updatePixel().
     *
     * To speed this up, we try and do the lut[index] and lut[index+1] portions
     * in parallel using the SMUAD instruction. This is a pair of 16x16 multiplies,
     * and the results are added together. We can combine this with an unaligned load
     * to grab two adjacent entries from the LUT. The remaining complications are:
     *
     *   1. We wanted unsigned, not signed
     *   2. We still need to generate the input values efficiently.
     *
     * (1) is easy to solve if we're okay with 15-bit precision for the LUT instead
     * of 16-bit, which is fine. During LUT preparation, we right-shift each entry
     * by 1, keeping them within the positive range of a signed 16-bit int. 
     *
     * For (2), we need to quickly put 'alpha' in the high halfword and invAlpha in
     * the low halfword, or vice versa. One fast way to do this is (0x01000000 + x - (x << 16).
     */

    uint32_t index = arg >> 8;          // Range [0, 0xFF]

    // Load lut[index] into low halfword, lut[index+1] into high halfword.
    uint32_t pair = *(const uint32_t*)(lut + index);

    unsigned alpha = arg & 0xFF;        // Range [0, 0xFF]

    // Reversed halfword order
    uint32_t pairAlpha = (0x01000000 + alpha - (alpha << 16));

    return __SMUADX(pairAlpha, pair) >> 7;
}

static uint32_t updatePixel(uint32_t icPrev, uint32_t icNext,
    const uint8_t *pixelPrev, const uint8_t *pixelNext, residual_t *pResidual)
{
    /*
     * Update pipeline for one pixel:
     *
     *    1. Interpolate framebuffer
     *    2. Interpolate LUT
     *    3. Dithering
     *
     * icPrev in range [0, 0x1010000]
     * icNext in range [0, 0x1010000]
     * icPrev + icNext = 0x1010000
     */

    // Per-channel linear interpolation and conversion to 16-bit color.
    // Result range: [0, 0xFFFF] 
    int iR = (pixelPrev[0] * icPrev + pixelNext[0] * icNext) >> 16;
    int iG = (pixelPrev[1] * icPrev + pixelNext[1] * icNext) >> 16;
    int iB = (pixelPrev[2] * icPrev + pixelNext[2] * icNext) >> 16;

    // Pass through our color LUT
    // Result range: [0, 0xFFFF] 
    iR = lutInterpolate(buffers.lutCurrent.r, iR);
    iG = lutInterpolate(buffers.lutCurrent.g, iG);
    iB = lutInterpolate(buffers.lutCurrent.b, iB);

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

    // Compute the error, after expanding the 8-bit value back to 16-bit.
    pResidual[0] = iR - (r8 * 257);
    pResidual[1] = iG - (g8 * 257);
    pResidual[2] = iB - (b8 * 257);

    // Pack the result, in GRB order.
    return (g8 << 16) | (r8 << 8) | b8;
}
