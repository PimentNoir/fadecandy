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

#include "fc_usb.h"
#include <algorithm>

// USB protocol definitions

#define TYPE_BITS           0xC0
#define FINAL_BIT           0x20
#define INDEX_BITS          0x1F

#define TYPE_FRAMEBUFFER    0x00
#define TYPE_LUT            0x40



void fcBuffers::handleUSB()
{
    /*
     * Look for incoming USB packets, and file them appropriately.
     * Our framebuffer and LUT buffers are arrays of references to USB packets
     * which we hold until a new packet arrives to replace them.
     */

    while (1) {
        usb_packet_t *packet = usb_rx(FC_OUT_ENDPOINT);
        if (!packet) {
            return;
        }

        unsigned control = packet->buf[0];
        unsigned type = control & TYPE_BITS;
        unsigned final = control & FINAL_BIT;
        unsigned index = control & INDEX_BITS;

        switch (type) {

            case TYPE_FRAMEBUFFER:
                fbNew->store(index, packet);
                if (final) {
                    finalizeFramebuffer();
                }
                break;

            case TYPE_LUT:
                lutNew->store(index, packet);
                if (final) {
                    finalizeLUT();
                }
                break;

            default:
                usb_free(packet);
                break;
        }
    }
}

void fcBuffers::finalizeFramebuffer()
{
    fcFramebuffer *recycle = fbPrev;
    fbPrev = fbNext;
    fbNext = fbNew;
    fbNew = recycle;
}

void fcBuffers::finalizeLUT()
{
    std::swap(lutCurrent, lutNew);
}
