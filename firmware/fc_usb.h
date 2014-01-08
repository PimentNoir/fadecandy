/*
 * Fadecandy Firmware - USB Support
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
#include "WProgram.h"
#include "usb_dev.h"
#include "fc_defs.h"


/*
 * A buffer of references to USB packets.
 */

template <unsigned tSize>
struct fcPacketBuffer
{
    usb_packet_t *packets[tSize];
    uint32_t timestamp;

    fcPacketBuffer()
    {
        // Allocate packets. They'll have zero'ed contents initially.
        for (unsigned i = 0; i < tSize; ++i) {
            usb_packet_t *p = usb_malloc();
            for (unsigned j = 0; j < sizeof p->buf; j++)
                p->buf[j] = 0;
            packets[i] = p;
        }
    }

    void store(unsigned index, usb_packet_t *packet)
    {
        if (index < tSize) {
            // Store a packet, holding a reference to it.
            usb_packet_t *prev = packets[index];
            packets[index] = packet;
            usb_free(prev);
        } else {
            // Error; ignore this packet.
            usb_free(packet);
        }
    }
};


/*
 * Framebuffer
 */

struct fcFramebuffer : public fcPacketBuffer<PACKETS_PER_FRAME>
{
    ALWAYS_INLINE const uint8_t* pixel(unsigned index)
    {
        return &packets[index / PIXELS_PER_PACKET]->buf[1 + (index % PIXELS_PER_PACKET) * 3];
    }
};


/*
 * Color Lookup table
 */

struct fcColorLUT : public fcPacketBuffer<PACKETS_PER_LUT>
{
    ALWAYS_INLINE const unsigned entry(unsigned index)
    {
        const uint8_t *p = &packets[index / LUTENTRIES_PER_PACKET]->buf[2 + (index % LUTENTRIES_PER_PACKET) * 2];
        return *(uint16_t*)p;
    }
};


/*
 * Configuration flag values
 */

#define CFLAG_NO_DITHERING      (1 << 0)
#define CFLAG_NO_INTERPOLATION  (1 << 1)
#define CFLAG_NO_ACTIVITY_LED   (1 << 2)
#define CFLAG_LED_CONTROL       (1 << 3)

/*
 * Data type for current color LUT
 */

union fcLinearLUT
{
    uint16_t entries[LUT_TOTAL_SIZE];
    struct {
        uint16_t r[LUT_CH_SIZE];
        uint16_t g[LUT_CH_SIZE];
        uint16_t b[LUT_CH_SIZE];
    };
};

/*
 * All USB-writable buffers
 */

struct fcBuffers
{
    fcFramebuffer *fbPrev;      // Frame we're interpolating from
    fcFramebuffer *fbNext;      // Frame we're interpolating to
    fcFramebuffer *fbNew;       // Partial frame, getting ready to become fbNext

    fcFramebuffer fb[3];        // Triple-buffered video frames

    fcColorLUT lutNew;                // Partial LUT, not yet finalized
    static fcLinearLUT lutCurrent;    // Active LUT, linearized for efficiency

    uint8_t flags;              // Configuration flags

    fcBuffers()
    {
        fbPrev = &fb[0];
        fbNext = &fb[1];
        fbNew = &fb[2];
    }

    // Interrupt context
    bool handleUSB(usb_packet_t *packet);

    // Main loop context
    void finalizeFrame();

private:
    void finalizeFramebuffer();
    void finalizeLUT();

    // Status communicated between handleUSB() and finalizeFrame()
    bool handledAnyPacketsThisFrame;
    bool pendingFinalizeFrame;
    bool pendingFinalizeLUT;
};
