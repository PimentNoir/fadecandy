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
#include "HardwareSerial.h"

// USB data buffers
static fcBuffers buffers;
fcLinearLUT fcBuffers::lutCurrent;

// Double-buffered DMA memory for raw bit planes of output
static DMAMEM int ledBuffer[LEDS_PER_STRIP * 12];
static OctoWS2811z leds(LEDS_PER_STRIP, ledBuffer, WS2811_800kHz);

/*
 * Residuals for temporal dithering. Usually 8 bits is enough, but
 * there are edge cases when it isn't, and we don't have the spare CPU cycles
 * to saturate values before storing. So, 16-bit it is.
 */
typedef int16_t residual_t;
static residual_t residual[CHANNELS_TOTAL];

// Reserved RAM area for signalling entry to bootloader
extern uint32_t boot_token;

/*
 * Low-level drawing code, which we want to compile in the same unit as the main loop.
 * We compile this multiple times, with different config flags.
 */

#define FCP_INTERPOLATION   0
#define FCP_DITHERING       0
#define FCP_FN(name)        name##_I0_D0
#include "fc_pixel_lut.cpp"
#include "fc_pixel.cpp"
#include "fc_draw.cpp"
#undef FCP_INTERPOLATION
#undef FCP_DITHERING
#undef FCP_FN

#define FCP_INTERPOLATION   1
#define FCP_DITHERING       0
#define FCP_FN(name)        name##_I1_D0
#include "fc_pixel_lut.cpp"
#include "fc_pixel.cpp"
#include "fc_draw.cpp"
#undef FCP_INTERPOLATION
#undef FCP_DITHERING
#undef FCP_FN

#define FCP_INTERPOLATION   0
#define FCP_DITHERING       1
#define FCP_FN(name)        name##_I0_D1
#include "fc_pixel_lut.cpp"
#include "fc_pixel.cpp"
#include "fc_draw.cpp"
#undef FCP_INTERPOLATION
#undef FCP_DITHERING
#undef FCP_FN

#define FCP_INTERPOLATION   1
#define FCP_DITHERING       1
#define FCP_FN(name)        name##_I1_D1
#include "fc_pixel_lut.cpp"
#include "fc_pixel.cpp"
#include "fc_draw.cpp"
#undef FCP_INTERPOLATION
#undef FCP_DITHERING
#undef FCP_FN


static inline uint32_t calculateInterpCoefficient()
{
    /*
     * Calculate our interpolation coefficient. This is a value between
     * 0x0000 and 0x10000, representing some point in between fbPrev and fbNext.
     *
     * We timestamp each frame at the moment its final packet has been received.
     * In other words, fbNew has no valid timestamp yet, and fbPrev/fbNext both
     * have timestamps in the recent past.
     *
     * fbNext's timestamp indicates when both fbPrev and fbNext entered their current
     * position in the keyframe queue. The difference between fbPrev and fbNext indicate
     * how long the interpolation between those keyframes should take.
     */

    uint32_t now = millis();
    uint32_t tsPrev = buffers.fbPrev->timestamp;
    uint32_t tsNext = buffers.fbNext->timestamp;
    uint32_t tsDiff = tsNext - tsPrev;
    uint32_t tsElapsed = now - tsNext;

    // Careful to avoid overflows if the frames stop coming...
    return (std::min<uint32_t>(tsElapsed, tsDiff) << 16) / tsDiff;
}

static void dfu_reboot()
{
    // Reboot to the Fadecandy Bootloader
    boot_token = 0x74624346;

    // Short delay to allow the host to receive the response to DFU_DETACH.
    uint32_t deadline = millis() + 10;
    while (millis() < deadline) {
        watchdog_refresh();
    }

    // Detach from USB, and use the watchdog to time out a 10ms USB disconnect.
    __disable_irq();
    USB0_CONTROL = 0;
    while (1);
}

extern "C" int usb_rx_handler(usb_packet_t *packet)
{
    // USB packet interrupt handler. Invoked by the ISR dispatch code in usb_dev.c
    return buffers.handleUSB(packet);
}

extern "C" int main()
{
    pinMode(LED_BUILTIN, OUTPUT);
    leds.begin();

    // Announce firmware version
    serial_begin(BAUD2DIV(115200));
    serial_print("Fadecandy v" DEVICE_VER_STRING "\r\n");

    // Application main loop
    while (usb_dfu_state == DFU_appIDLE) {
        watchdog_refresh();

        // Select a different drawing loop based on our firmware config flags
        switch (buffers.flags & (CFLAG_NO_INTERPOLATION | CFLAG_NO_DITHERING)) {
            case 0:
            default:
                updateDrawBuffer_I1_D1(calculateInterpCoefficient());
                break;
            case CFLAG_NO_INTERPOLATION:
                updateDrawBuffer_I0_D1(0x10000);
                break;
            case CFLAG_NO_DITHERING:
                updateDrawBuffer_I1_D0(calculateInterpCoefficient());
                break;
            case CFLAG_NO_INTERPOLATION | CFLAG_NO_DITHERING:
                updateDrawBuffer_I0_D0(0x10000);
                break;
        }

        // Start sending the next frame over DMA
        leds.show();

        // We can switch to the next frame's buffer now.
        buffers.finalizeFrame();

        // Performance counter, for monitoring frame rate externally
        perf_frameCounter++;
    }

    // Reboot into DFU bootloader
    dfu_reboot();
}
