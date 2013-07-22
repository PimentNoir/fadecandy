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
#include "fc_usb.h"
#include "fc_defs.h"

// USB data buffers
static fcBuffers buffers;

// Double-buffered DMA memory for raw bit planes of output
static DMAMEM int ledBuffer[LEDS_PER_STRIP * 12];
static OctoWS2811z leds(LEDS_PER_STRIP, ledBuffer, WS2811_800kHz);

// Residuals for temporal dithering
static int8_t residual[CHANNELS_TOTAL];


static uint32_t lutInterpolate(unsigned channel, uint32_t arg)
{
    /*
     * Using our color LUT for the indicated channel, convert the
     * 16-bit intensity "arg" in our input colorspace to a corresponding
     * 16-bit intensity in the device colorspace.
     */

    unsigned index = arg >> 8;
    unsigned alpha = arg & 0xFF;
    unsigned invAlpha = 0x100 - alpha;

    unsigned v1 = buffers.lutCurrent->entry(channel, index);
    unsigned v2 = buffers.lutCurrent->entry(channel, index + 1);

    return (v1 * invAlpha + v2 * alpha) >> 8;
}

static uint32_t updatePixel(uint32_t icPrev, uint32_t icNext, unsigned n)
{
    /*
     * Update pipeline for one pixel:
     *
     *    1. Interpolate framebuffer
     *    2. Interpolate LUT
     *    3. Dithering
     */

    digitalWrite(DEBUG_PIN, HIGH);

    const uint8_t *pixelPrev = buffers.fbPrev->pixel(n);
    const uint8_t *pixelNext = buffers.fbNext->pixel(n);

    // Per-channel linear interpolation and conversion to 16-bit color.
    uint32_t iR = (pixelPrev[0] * icPrev + pixelNext[0] * icNext) >> 16;
    uint32_t iG = (pixelPrev[1] * icPrev + pixelNext[1] * icNext) >> 16;
    uint32_t iB = (pixelPrev[2] * icPrev + pixelNext[2] * icNext) >> 16;

    // Pass through our color LUT
    iR = lutInterpolate(0, iR);
    iG = lutInterpolate(1, iG);
    iB = lutInterpolate(2, iB);

    // Pointer to the residual buffer for this pixel
    int8_t *pResidual = &residual[n * 3];

    // Incorporate the residual from last frame
    iR += pResidual[0];
    iG += pResidual[1];
    iB += pResidual[2];

    // Round to the nearest 8-bit value
    int r8 = std::min<int>(0xff, std::max<int>(0, (iR + 0x80) >> 8));
    int g8 = std::min<int>(0xff, std::max<int>(0, (iG + 0x80) >> 8));
    int b8 = std::min<int>(0xff, std::max<int>(0, (iB + 0x80) >> 8));

    // Compute the error, after expanding the 8-bit value back to 16-bit.
    pResidual[0] = iR - (r8 * 257);
    pResidual[1] = iG - (g8 * 257);
    pResidual[2] = iB - (b8 * 257);

    digitalWrite(DEBUG_PIN, LOW);

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

        uint32_t plane0 = updatePixel(icPrev, icNext, i + LEDS_PER_STRIP * 0);
        uint32_t plane1 = updatePixel(icPrev, icNext, i + LEDS_PER_STRIP * 1);
        uint32_t plane2 = updatePixel(icPrev, icNext, i + LEDS_PER_STRIP * 2);
        uint32_t plane3 = updatePixel(icPrev, icNext, i + LEDS_PER_STRIP * 3);
        uint32_t plane4 = updatePixel(icPrev, icNext, i + LEDS_PER_STRIP * 4);
        uint32_t plane5 = updatePixel(icPrev, icNext, i + LEDS_PER_STRIP * 5);
        uint32_t plane6 = updatePixel(icPrev, icNext, i + LEDS_PER_STRIP * 6);
        uint32_t plane7 = updatePixel(icPrev, icNext, i + LEDS_PER_STRIP * 7);

        *(out++) = ( (plane0 >> 23) & (1 << 0 ) ) |     // Bit 23
                   ( (plane1 >> 22) & (1 << 1 ) ) |
                   ( (plane2 >> 21) & (1 << 2 ) ) |
                   ( (plane3 >> 20) & (1 << 3 ) ) |
                   ( (plane4 >> 19) & (1 << 4 ) ) |
                   ( (plane5 >> 18) & (1 << 5 ) ) |
                   ( (plane6 >> 17) & (1 << 6 ) ) |
                   ( (plane7 >> 16) & (1 << 7 ) ) |

                   ( (plane0 >> 14) & (1 << 8 ) ) |     // Bit 22
                   ( (plane1 >> 13) & (1 << 9 ) ) |
                   ( (plane2 >> 12) & (1 << 10) ) |
                   ( (plane3 >> 11) & (1 << 11) ) |
                   ( (plane4 >> 10) & (1 << 12) ) |
                   ( (plane5 >> 9 ) & (1 << 13) ) |
                   ( (plane6 >> 8 ) & (1 << 14) ) |
                   ( (plane7 >> 7 ) & (1 << 15) ) |

                   ( (plane0 >> 5 ) & (1 << 16) ) |     // Bit 21
                   ( (plane1 >> 4 ) & (1 << 17) ) |
                   ( (plane2 >> 3 ) & (1 << 18) ) |
                   ( (plane3 >> 2 ) & (1 << 19) ) |
                   ( (plane4 >> 1 ) & (1 << 20) ) |
                   ( (plane5      ) & (1 << 21) ) |
                   ( (plane6 << 1 ) & (1 << 22) ) |
                   ( (plane7 << 2 ) & (1 << 23) ) |

                   ( (plane0 << 4 ) & (1 << 24) ) |     // Bit 20
                   ( (plane1 << 5 ) & (1 << 25) ) |
                   ( (plane2 << 6 ) & (1 << 26) ) |
                   ( (plane3 << 7 ) & (1 << 27) ) |
                   ( (plane4 << 8 ) & (1 << 28) ) |
                   ( (plane5 << 9 ) & (1 << 29) ) |
                   ( (plane6 << 10) & (1 << 30) ) |
                   ( (plane7 << 11) & (1 << 31) ) ;

        *(out++) = ( (plane0 >> 19) & (1 << 0 ) ) |     // Bit 19
                   ( (plane1 >> 18) & (1 << 1 ) ) |
                   ( (plane2 >> 17) & (1 << 2 ) ) |
                   ( (plane3 >> 16) & (1 << 3 ) ) |
                   ( (plane4 >> 15) & (1 << 4 ) ) |
                   ( (plane5 >> 14) & (1 << 5 ) ) |
                   ( (plane6 >> 13) & (1 << 6 ) ) |
                   ( (plane7 >> 12) & (1 << 7 ) ) |

                   ( (plane0 >> 10) & (1 << 8 ) ) |     // Bit 18
                   ( (plane1 >> 9 ) & (1 << 9 ) ) |
                   ( (plane2 >> 8 ) & (1 << 10) ) |
                   ( (plane3 >> 7 ) & (1 << 11) ) |
                   ( (plane4 >> 6 ) & (1 << 12) ) |
                   ( (plane5 >> 5 ) & (1 << 13) ) |
                   ( (plane6 >> 4 ) & (1 << 14) ) |
                   ( (plane7 >> 3 ) & (1 << 15) ) |

                   ( (plane0 >> 1 ) & (1 << 16) ) |     // Bit 17
                   ( (plane1      ) & (1 << 17) ) |
                   ( (plane2 << 1 ) & (1 << 18) ) |
                   ( (plane3 << 2 ) & (1 << 19) ) |
                   ( (plane4 << 3 ) & (1 << 20) ) |
                   ( (plane5 << 4 ) & (1 << 21) ) |
                   ( (plane6 << 5 ) & (1 << 22) ) |
                   ( (plane7 << 6 ) & (1 << 23) ) |

                   ( (plane0 << 8 ) & (1 << 24) ) |     // Bit 16
                   ( (plane1 << 9 ) & (1 << 25) ) |
                   ( (plane2 << 10) & (1 << 26) ) |
                   ( (plane3 << 11) & (1 << 27) ) |
                   ( (plane4 << 12) & (1 << 28) ) |
                   ( (plane5 << 13) & (1 << 29) ) |
                   ( (plane6 << 14) & (1 << 30) ) |
                   ( (plane7 << 15) & (1 << 31) ) ;

        *(out++) = ( (plane0 >> 15) & (1 << 0 ) ) |     // Bit 15
                   ( (plane1 >> 14) & (1 << 1 ) ) |
                   ( (plane2 >> 13) & (1 << 2 ) ) |
                   ( (plane3 >> 12) & (1 << 3 ) ) |
                   ( (plane4 >> 11) & (1 << 4 ) ) |
                   ( (plane5 >> 10) & (1 << 5 ) ) |
                   ( (plane6 >> 9 ) & (1 << 6 ) ) |
                   ( (plane7 >> 8 ) & (1 << 7 ) ) |

                   ( (plane0 >> 6 ) & (1 << 8 ) ) |     // Bit 14
                   ( (plane1 >> 5 ) & (1 << 9 ) ) |
                   ( (plane2 >> 4 ) & (1 << 10) ) |
                   ( (plane3 >> 3 ) & (1 << 11) ) |
                   ( (plane4 >> 2 ) & (1 << 12) ) |
                   ( (plane5 >> 1 ) & (1 << 13) ) |
                   ( (plane6      ) & (1 << 14) ) |
                   ( (plane7 << 1 ) & (1 << 15) ) |

                   ( (plane0 << 3 ) & (1 << 16) ) |     // Bit 13
                   ( (plane1 << 4 ) & (1 << 17) ) |
                   ( (plane2 << 5 ) & (1 << 18) ) |
                   ( (plane3 << 6 ) & (1 << 19) ) |
                   ( (plane4 << 7 ) & (1 << 20) ) |
                   ( (plane5 << 8 ) & (1 << 21) ) |
                   ( (plane6 << 9 ) & (1 << 22) ) |
                   ( (plane7 << 10) & (1 << 23) ) |

                   ( (plane0 << 12) & (1 << 24) ) |     // Bit 12
                   ( (plane1 << 13) & (1 << 25) ) |
                   ( (plane2 << 14) & (1 << 26) ) |
                   ( (plane3 << 15) & (1 << 27) ) |
                   ( (plane4 << 16) & (1 << 28) ) |
                   ( (plane5 << 17) & (1 << 29) ) |
                   ( (plane6 << 18) & (1 << 30) ) |
                   ( (plane7 << 19) & (1 << 31) ) ;

        *(out++) = ( (plane0 >> 11) & (1 << 0 ) ) |     // Bit 11
                   ( (plane1 >> 10) & (1 << 1 ) ) |
                   ( (plane2 >> 9 ) & (1 << 2 ) ) |
                   ( (plane3 >> 8 ) & (1 << 3 ) ) |
                   ( (plane4 >> 7 ) & (1 << 4 ) ) |
                   ( (plane5 >> 6 ) & (1 << 5 ) ) |
                   ( (plane6 >> 5 ) & (1 << 6 ) ) |
                   ( (plane7 >> 4 ) & (1 << 7 ) ) |

                   ( (plane0 >> 2 ) & (1 << 8 ) ) |     // Bit 10
                   ( (plane1 >> 1 ) & (1 << 9 ) ) |
                   ( (plane2      ) & (1 << 10) ) |
                   ( (plane3 << 1 ) & (1 << 11) ) |
                   ( (plane4 << 2 ) & (1 << 12) ) |
                   ( (plane5 << 3 ) & (1 << 13) ) |
                   ( (plane6 << 4 ) & (1 << 14) ) |
                   ( (plane7 << 5 ) & (1 << 15) ) |

                   ( (plane0 << 7 ) & (1 << 16) ) |     // Bit 9
                   ( (plane1 << 8 ) & (1 << 17) ) |
                   ( (plane2 << 9 ) & (1 << 18) ) |
                   ( (plane3 << 10) & (1 << 19) ) |
                   ( (plane4 << 11) & (1 << 20) ) |
                   ( (plane5 << 12) & (1 << 21) ) |
                   ( (plane6 << 13) & (1 << 22) ) |
                   ( (plane7 << 14) & (1 << 23) ) |

                   ( (plane0 << 16) & (1 << 24) ) |     // Bit 8
                   ( (plane1 << 17) & (1 << 25) ) |
                   ( (plane2 << 18) & (1 << 26) ) |
                   ( (plane3 << 19) & (1 << 27) ) |
                   ( (plane4 << 20) & (1 << 28) ) |
                   ( (plane5 << 21) & (1 << 29) ) |
                   ( (plane6 << 22) & (1 << 30) ) |
                   ( (plane7 << 23) & (1 << 31) ) ;

        *(out++) = ( (plane0 >> 7 ) & (1 << 0 ) ) |     // Bit 7
                   ( (plane1 >> 6 ) & (1 << 1 ) ) |
                   ( (plane2 >> 5 ) & (1 << 2 ) ) |
                   ( (plane3 >> 4 ) & (1 << 3 ) ) |
                   ( (plane4 >> 3 ) & (1 << 4 ) ) |
                   ( (plane5 >> 2 ) & (1 << 5 ) ) |
                   ( (plane6 >> 1 ) & (1 << 6 ) ) |
                   ( (plane7      ) & (1 << 7 ) ) |

                   ( (plane0 << 2 ) & (1 << 8 ) ) |     // Bit 6
                   ( (plane1 << 3 ) & (1 << 9 ) ) |
                   ( (plane2 << 4 ) & (1 << 10) ) |
                   ( (plane3 << 5 ) & (1 << 11) ) |
                   ( (plane4 << 6 ) & (1 << 12) ) |
                   ( (plane5 << 7 ) & (1 << 13) ) |
                   ( (plane6 << 8 ) & (1 << 14) ) |
                   ( (plane7 << 9 ) & (1 << 15) ) |

                   ( (plane0 << 11) & (1 << 16) ) |     // Bit 5
                   ( (plane1 << 12) & (1 << 17) ) |
                   ( (plane2 << 13) & (1 << 18) ) |
                   ( (plane3 << 14) & (1 << 19) ) |
                   ( (plane4 << 15) & (1 << 20) ) |
                   ( (plane5 << 16) & (1 << 21) ) |
                   ( (plane6 << 17) & (1 << 22) ) |
                   ( (plane7 << 18) & (1 << 23) ) |

                   ( (plane0 << 20) & (1 << 24) ) |     // Bit 4
                   ( (plane1 << 21) & (1 << 25) ) |
                   ( (plane2 << 22) & (1 << 26) ) |
                   ( (plane3 << 23) & (1 << 27) ) |
                   ( (plane4 << 24) & (1 << 28) ) |
                   ( (plane5 << 25) & (1 << 29) ) |
                   ( (plane6 << 26) & (1 << 30) ) |
                   ( (plane7 << 27) & (1 << 31) ) ;

        *(out++) = ( (plane0 >> 3 ) & (1 << 0 ) ) |     // Bit 3
                   ( (plane1 >> 2 ) & (1 << 1 ) ) |
                   ( (plane2 >> 1 ) & (1 << 2 ) ) |
                   ( (plane3      ) & (1 << 3 ) ) |
                   ( (plane4 << 1 ) & (1 << 4 ) ) |
                   ( (plane5 << 2 ) & (1 << 5 ) ) |
                   ( (plane6 << 3 ) & (1 << 6 ) ) |
                   ( (plane7 << 4 ) & (1 << 7 ) ) |

                   ( (plane0 << 6 ) & (1 << 8 ) ) |     // Bit 2
                   ( (plane1 << 7 ) & (1 << 9 ) ) |
                   ( (plane2 << 8 ) & (1 << 10) ) |
                   ( (plane3 << 9 ) & (1 << 11) ) |
                   ( (plane4 << 10) & (1 << 12) ) |
                   ( (plane5 << 11) & (1 << 13) ) |
                   ( (plane6 << 12) & (1 << 14) ) |
                   ( (plane7 << 13) & (1 << 15) ) |

                   ( (plane0 << 15) & (1 << 16) ) |     // Bit 1
                   ( (plane1 << 16) & (1 << 17) ) |
                   ( (plane2 << 17) & (1 << 18) ) |
                   ( (plane3 << 18) & (1 << 19) ) |
                   ( (plane4 << 19) & (1 << 20) ) |
                   ( (plane5 << 20) & (1 << 21) ) |
                   ( (plane6 << 21) & (1 << 22) ) |
                   ( (plane7 << 22) & (1 << 23) ) |

                   ( (plane0 << 24) & (1 << 24) ) |     // Bit 0
                   ( (plane1 << 25) & (1 << 25) ) |
                   ( (plane2 << 26) & (1 << 26) ) |
                   ( (plane3 << 27) & (1 << 27) ) |
                   ( (plane4 << 28) & (1 << 28) ) |
                   ( (plane5 << 29) & (1 << 29) ) |
                   ( (plane6 << 30) & (1 << 30) ) |
                   ( (plane7 << 31) & (1 << 31) ) ;
    }
}

extern "C" int main()
{
    leds.begin();
    pinMode(DEBUG_PIN, OUTPUT);

    while (1) {
        buffers.handleUSB();

        updateDrawBuffer((millis() << 6) & 0xFFFF);
        leds.show();
    }
}
