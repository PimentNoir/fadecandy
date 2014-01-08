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

#include "mk20dx128.h"
#include "usb_dev.h"
#include "usb_mem.h"

__attribute__ ((section(".usbbuffers"), used))
unsigned char usb_buffer_memory[NUM_USB_BUFFERS * sizeof(usb_packet_t)];

static uint32_t usb_buffer_available[4];

void usb_init_mem()
{
    usb_buffer_available[0] = -1;
    usb_buffer_available[1] = -1;
    usb_buffer_available[2] = -1;
    usb_buffer_available[3] = -1;
}

// use bitmask and CLZ instruction to implement fast free list
// http://www.archivum.info/gnu.gcc.help/2006-08/00148/Re-GCC-Inline-Assembly.html
// http://gcc.gnu.org/ml/gcc/2012-06/msg00015.html
// __builtin_clz()

usb_packet_t * usb_malloc(void)
{
    unsigned int n, avail, idx = 0;
    uint8_t *p;

    __disable_irq();

    do {
        avail = usb_buffer_available[idx];
        if (avail) {
            break;
        }
        idx++;
    } while (idx < sizeof(usb_buffer_available) / sizeof(usb_buffer_available[0]));

    n = __builtin_clz(avail) + (idx << 5);
    if (n >= NUM_USB_BUFFERS) {
        // Oops, no more memory. This is a fatal error- our firmware should be designed
        // to never allocate more buffers than we have. Wedge ourselves in an infinite
        // loop and the watchdog will reset.
        while (1);
    }

    usb_buffer_available[idx] = avail & ~(0x80000000 >> (n & 31));
    __enable_irq();

    p = usb_buffer_memory + (n * sizeof(usb_packet_t));

    return (usb_packet_t *)p;
}

void usb_free(usb_packet_t *p)
{
    unsigned int n, mask, idx;

    n = ((uint8_t *)p - usb_buffer_memory) / sizeof(usb_packet_t);
    if (n >= NUM_USB_BUFFERS) return;

    idx = n >> 5;
    mask = 0x80000000 >> (n & 31);
    __disable_irq();
    usb_buffer_available[idx] |= mask;
    __enable_irq();
}

