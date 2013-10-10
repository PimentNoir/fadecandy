/*
 * Simple control-only USB driver for DFU bootloader mode.
 * Originally based on:
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
#include "usb_desc.h"
#include "dfu.h"

// buffer descriptor table
typedef struct {
    uint32_t desc;
    void * addr;
} bdt_t;

__attribute__ ((section(".usbdescriptortable"), used))
static bdt_t table[64];  // BDT page (512 bytes)

#define BDT_OWN     0x80
#define BDT_DATA1   0x40
#define BDT_DATA0   0x00
#define BDT_DTS     0x08
#define BDT_STALL   0x04
#define BDT_PID(n)  (((n) >> 2) & 15)

#define BDT_DESC(count, data)   (BDT_OWN | BDT_DTS \
                | ((data) ? BDT_DATA1 : BDT_DATA0) \
                | ((count) << 16))

// No DTS, we want to accept either DATA0 or DATA1 for RX.
// This makes buffer management way less awful for EP0 OUT with
// multiple packets per transaction.
#define BDT_DESC_RX(count)      (BDT_OWN | ((count) << 16))

#define TX   1
#define RX   0
#define ODD  1
#define EVEN 0
#define DATA0 0
#define DATA1 1
#define index(endpoint, tx, odd) (((endpoint) << 2) | ((tx) << 1) | (odd))
#define stat2bufferdescriptor(stat) (table + ((stat) >> 2))


static union {
    struct {
        union {
            struct {
                uint8_t bmRequestType;
                uint8_t bRequest;
            };
            uint16_t wRequestAndType;
        };
        uint16_t wValue;
        uint16_t wIndex;
        uint16_t wLength;
    };
    struct {
        uint32_t word1;
        uint32_t word2;
    };
} setup;


#define GET_STATUS          0
#define CLEAR_FEATURE       1
#define SET_FEATURE         3
#define SET_ADDRESS         5
#define GET_DESCRIPTOR      6
#define SET_DESCRIPTOR      7
#define GET_CONFIGURATION   8
#define SET_CONFIGURATION   9
#define GET_INTERFACE       10
#define SET_INTERFACE       11
#define SYNCH_FRAME         12

// SETUP always uses a DATA0 PID for the data field of the SETUP transaction.
// transactions in the data phase start with DATA1 and toggle (figure 8-12, USB1.1)
// Status stage uses a DATA1 PID.

static uint8_t ep0_rx0_buf[EP0_SIZE] __attribute__ ((aligned (4)));
static uint8_t ep0_rx1_buf[EP0_SIZE] __attribute__ ((aligned (4)));
static const uint8_t *ep0_tx_ptr = NULL;
static uint16_t ep0_tx_len;
static uint16_t ep0_rx_offset;
static uint8_t ep0_tx_bdt_bank = 0;
static uint8_t ep0_tx_data_toggle = 0;

static uint8_t reply_buffer[8];

volatile uint8_t usb_configuration = 0;


static void endpoint0_stall(void)
{
    USB0_ENDPT0 = USB_ENDPT_EPSTALL | USB_ENDPT_EPRXEN | USB_ENDPT_EPTXEN | USB_ENDPT_EPHSHK;
}

static void endpoint0_transmit(const void *data, uint32_t len)
{
    table[index(0, TX, ep0_tx_bdt_bank)].addr = (void *)data;
    table[index(0, TX, ep0_tx_bdt_bank)].desc = BDT_DESC(len, ep0_tx_data_toggle);
    ep0_tx_data_toggle ^= 1;
    ep0_tx_bdt_bank ^= 1;
}


static void usb_setup(void)
{
    const uint8_t *data = NULL;
    uint32_t datalen = 0;
    const usb_descriptor_list_t *list;
    uint32_t size;
    int i;

    switch (setup.wRequestAndType) {
      case 0x0500: // SET_ADDRESS
        break;
      case 0x0900: // SET_CONFIGURATION
        usb_configuration = setup.wValue;
        break;
      case 0x0880: // GET_CONFIGURATION
        reply_buffer[0] = usb_configuration;
        datalen = 1;
        data = reply_buffer;
        break;
      case 0x0080: // GET_STATUS (device)
        reply_buffer[0] = 0;
        reply_buffer[1] = 0;
        datalen = 2;
        data = reply_buffer;
        break;
      case 0x0082: // GET_STATUS (endpoint)
        if (setup.wIndex > 0) {
            endpoint0_stall();
            return;
        }
        reply_buffer[0] = 0;
        reply_buffer[1] = 0;
        if (*(uint8_t *)(&USB0_ENDPT0 + setup.wIndex * 4) & 0x02) reply_buffer[0] = 1;
        data = reply_buffer;
        datalen = 2;
        break;
      case 0x0102: // CLEAR_FEATURE (endpoint)
        i = setup.wIndex & 0x7F;
        if (i > 0 || setup.wValue != 0) {
            // TODO: do we need to handle IN vs OUT here?
            endpoint0_stall();
            return;
        }
        (*(uint8_t *)(&USB0_ENDPT0 + setup.wIndex * 4)) &= ~0x02;
        // TODO: do we need to clear the data toggle here?
        break;
      case 0x0302: // SET_FEATURE (endpoint)
        i = setup.wIndex & 0x7F;
        if (i > 0 || setup.wValue != 0) {
            // TODO: do we need to handle IN vs OUT here?
            endpoint0_stall();
            return;
        }
        (*(uint8_t *)(&USB0_ENDPT0 + setup.wIndex * 4)) |= 0x02;
        // TODO: do we need to clear the data toggle here?
        break;
      case 0x0680: // GET_DESCRIPTOR
      case 0x0681:
        for (list = usb_descriptor_list; 1; list++) {
            if (list->addr == NULL) break;
            if (setup.wValue == list->wValue) {
                data = list->addr;
                if ((setup.wValue >> 8) == 3) {
                    // for string descriptors, use the descriptor's
                    // length field, allowing runtime configured
                    // length.
                    datalen = *(list->addr);
                } else {
                    datalen = list->length;
                }
                goto send;
            }
        }
        endpoint0_stall();
        return;

      case (MSFT_VENDOR_CODE << 8) | 0xC0:      // Get Microsoft descriptor
      case (MSFT_VENDOR_CODE << 8) | 0xC1:
        if (setup.wIndex == 0x0004) {
            // Return WCID descriptor
            data = usb_microsoft_wcid;
            datalen = MSFT_WCID_LEN;
            break;
        }
        endpoint0_stall();
        return;

      case 0x0121: // DFU_DNLOAD
        if (setup.wIndex > 0) {
            endpoint0_stall();
            return;
        }
        // Data comes in the OUT phase. But if it's a zero-length request, handle it now.
        if (setup.wLength == 0) {
            if (!dfu_download(setup.wValue, 0, 0, 0, NULL)) {
                endpoint0_stall();
            }
        }
        break;

      case 0x03a1: // DFU_GETSTATUS
        if (setup.wIndex > 0) {
            endpoint0_stall();
            return;
        }
        if (dfu_getstatus(reply_buffer)) {
            data = reply_buffer;
            datalen = 6;
            break;
        } else {
            endpoint0_stall();
            return;
        }

      case 0x0421: // DFU_CLRSTATUS
        if (setup.wIndex > 0) {
            endpoint0_stall();
            return;
        }
        if (dfu_clrstatus()) {
            break;
        } else {
            endpoint0_stall();
            return;
        }

      case 0x05a1: // DFU_GETSTATE
        if (setup.wIndex > 0) {
            endpoint0_stall();
            return;
        }
        reply_buffer[0] = dfu_getstate();
        data = reply_buffer;
        datalen = 1;
        break;

      case 0x0621: // DFU_ABORT
        if (setup.wIndex > 0) {
            endpoint0_stall();
            return;
        }
        if (dfu_abort()) {
            break;
        } else {
            endpoint0_stall();
            return;
        }

      default:
        endpoint0_stall();
        return;
    }
 send:

    if (datalen > setup.wLength) datalen = setup.wLength;
    size = datalen;
    if (size > EP0_SIZE) size = EP0_SIZE;
    endpoint0_transmit(data, size);
    data += size;
    datalen -= size;
    if (datalen == 0 && size < EP0_SIZE) return;

    size = datalen;
    if (size > EP0_SIZE) size = EP0_SIZE;
    endpoint0_transmit(data, size);
    data += size;
    datalen -= size;
    if (datalen == 0 && size < EP0_SIZE) return;

    ep0_tx_ptr = data;
    ep0_tx_len = datalen;
}


static void usb_control(uint32_t stat)
{
    bdt_t *b;
    uint32_t pid, size;
    uint8_t *buf;
    const uint8_t *data;

    b = stat2bufferdescriptor(stat);
    pid = BDT_PID(b->desc);
    buf = b->addr;

    switch (pid) {
    case 0x0D: // Setup received from host
        // grab the 8 byte setup info
        setup.word1 = *(uint32_t *)(buf);
        setup.word2 = *(uint32_t *)(buf + 4);

        // Give the buffer back
        b->desc = BDT_DESC_RX(EP0_SIZE);

        // clear any leftover pending IN transactions
        ep0_tx_ptr = NULL;

        // first IN or OUT after Setup is always DATA1
        ep0_tx_data_toggle = 1;

        // If we're receiving, start at the beginning
        ep0_rx_offset = 0;

        // actually "do" the setup request
        usb_setup();
        break;

    case 0x01:  // OUT transaction received from host

        // The only control OUT request we have now, DFU_DNLOAD
        if (setup.wRequestAndType == 0x0121) {

            if (setup.wIndex != 0 && ep0_rx_offset > setup.wLength) {
                endpoint0_stall();
            } else {
                size = setup.wLength - ep0_rx_offset;
                if (size > EP0_SIZE) size = EP0_SIZE;

                if (dfu_download(setup.wValue,   // blockNum
                                 setup.wLength,  // blockLength
                                 ep0_rx_offset,  // packetOffset
                                 size,           // packetLength
                                 buf)) {         // data

                    ep0_rx_offset += size;
                    if (ep0_rx_offset >= setup.wLength) {
                        // End of transaction, acknowledge with a zero-length IN                
                       endpoint0_transmit(reply_buffer, 0);
                    }
                } else {
                    endpoint0_stall();
                }
            }
        }

        // Give the buffer back
        b->desc = BDT_DESC_RX(EP0_SIZE);
        break;

    case 0x09: // IN transaction completed to host
        // send remaining data, if any...
        data = ep0_tx_ptr;
        if (data) {
            size = ep0_tx_len;
            if (size > EP0_SIZE) size = EP0_SIZE;
            endpoint0_transmit(data, size);
            data += size;
            ep0_tx_len -= size;
            ep0_tx_ptr = (ep0_tx_len > 0 || size == EP0_SIZE) ? data : NULL;
        }

        if (setup.bRequest == 5 && setup.bmRequestType == 0) {
            setup.bRequest = 0;
            USB0_ADDR = setup.wValue;
        }
        break;
    }
    USB0_CTL = USB_CTL_USBENSOFEN; // clear TXSUSPENDTOKENBUSY bit
}


void usb_isr(void)
{
    uint8_t status, stat;

restart:
    status = USB0_ISTAT;

    if ((status & USB_INTEN_SOFTOKEN /* 04 */ )) {
        // Clear SOF interrupt
        USB0_ISTAT = USB_INTEN_SOFTOKEN;
    }

    if ((status & USB_ISTAT_TOKDNE /* 08 */ )) {
        uint8_t endpoint;
        stat = USB0_STAT;
        endpoint = stat >> 4;
        if (endpoint == 0) {
            usb_control(stat);
        }
        USB0_ISTAT = USB_ISTAT_TOKDNE;
        goto restart;
    }

    if (status & USB_ISTAT_USBRST /* 01 */ ) {

        // initialize BDT toggle bits
        USB0_CTL = USB_CTL_ODDRST;
        ep0_tx_bdt_bank = 0;

        // set up buffers to receive Setup and OUT packets
        table[index(0, RX, EVEN)].desc = BDT_DESC_RX(EP0_SIZE);
        table[index(0, RX, EVEN)].addr = ep0_rx0_buf;
        table[index(0, RX, ODD)].desc = BDT_DESC_RX(EP0_SIZE);
        table[index(0, RX, ODD)].addr = ep0_rx1_buf;
        table[index(0, TX, EVEN)].desc = 0;
        table[index(0, TX, ODD)].desc = 0;
        
        // activate endpoint 0
        USB0_ENDPT0 = USB_ENDPT_EPRXEN | USB_ENDPT_EPTXEN | USB_ENDPT_EPHSHK;

        // clear all ending interrupts
        USB0_ERRSTAT = 0xFF;
        USB0_ISTAT = 0xFF;

        // set the address to zero during enumeration
        USB0_ADDR = 0;

        // enable other interrupts
        USB0_ERREN = 0xFF;
        USB0_INTEN = USB_INTEN_TOKDNEEN |
            USB_INTEN_SOFTOKEN |
            USB_INTEN_STALLEN |
            USB_INTEN_ERROREN |
            USB_INTEN_USBRSTEN |
            USB_INTEN_SLEEPEN;

        // is this necessary?
        USB0_CTL = USB_CTL_USBENSOFEN;
        return;
    }

    if ((status & USB_ISTAT_STALL /* 80 */ )) {
        USB0_ENDPT0 = USB_ENDPT_EPRXEN | USB_ENDPT_EPTXEN | USB_ENDPT_EPHSHK;
        USB0_ISTAT = USB_ISTAT_STALL;
    }

    if ((status & USB_ISTAT_ERROR /* 02 */ )) {
        uint8_t err = USB0_ERRSTAT;
        USB0_ERRSTAT = err;
        USB0_ISTAT = USB_ISTAT_ERROR;
    }

    if ((status & USB_ISTAT_SLEEP /* 10 */ )) {
        USB0_ISTAT = USB_ISTAT_SLEEP;
    }
}


void usb_init(void)
{
    // this basically follows the flowchart in the Kinetis
    // Quick Reference User Guide, Rev. 1, 03/2012, page 141

    // assume 48 MHz clock already running
    // SIM - enable clock
    SIM_SCGC4 |= SIM_SCGC4_USBOTG;

    // reset USB module
    USB0_USBTRC0 = USB_USBTRC_USBRESET;
    while ((USB0_USBTRC0 & USB_USBTRC_USBRESET) != 0) ; // wait for reset to end

    // set desc table base addr
    USB0_BDTPAGE1 = ((uint32_t)table) >> 8;
    USB0_BDTPAGE2 = ((uint32_t)table) >> 16;
    USB0_BDTPAGE3 = ((uint32_t)table) >> 24;

    // clear all ISR flags
    USB0_ISTAT = 0xFF;
    USB0_ERRSTAT = 0xFF;
    USB0_OTGISTAT = 0xFF;

    USB0_USBTRC0 |= 0x40; // undocumented bit

    // enable USB
    USB0_CTL = USB_CTL_USBENSOFEN;
    USB0_USBCTRL = 0;

    // enable reset interrupt
    USB0_INTEN = USB_INTEN_USBRSTEN;

    // enable interrupt in NVIC...
    NVIC_ENABLE_IRQ(IRQ_USBOTG);

    // enable d+ pullup
    USB0_CONTROL = USB_CONTROL_DPPULLUPNONOTG;
}



