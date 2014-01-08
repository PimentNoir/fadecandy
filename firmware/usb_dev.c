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
#include "usb_desc.h"

// buffer descriptor table

typedef struct {
    uint32_t desc;
    void * addr;
} bdt_t;

__attribute__ ((section(".usbdescriptortable"), used))
static bdt_t table[(NUM_ENDPOINTS+1)*4];

// Which bdt_t entries have been deferred for retry later
static uint8_t bdt_deferred_map;

static uint8_t reply_buffer[8];

// Performance counters
volatile uint32_t perf_frameCounter;
volatile uint32_t perf_receivedKeyframeCounter;

#define BDT_OWN     0x80
#define BDT_DATA1   0x40
#define BDT_DATA0   0x00
#define BDT_DTS     0x08
#define BDT_STALL   0x04
#define BDT_PID(n)  (((n) >> 2) & 15)

#define BDT_DESC(count, data)   (BDT_OWN | BDT_DTS \
                | ((data) ? BDT_DATA1 : BDT_DATA0) \
                | ((count) << 16))

#define TX   1
#define RX   0
#define ODD  1
#define EVEN 0
#define DATA0 0
#define DATA1 1
#define index(endpoint, tx, odd) (((endpoint) << 2) | ((tx) << 1) | (odd))
#define stat2tableindex(stat)       ((stat) >> 2)


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


#define GET_STATUS      0
#define CLEAR_FEATURE       1
#define SET_FEATURE     3
#define SET_ADDRESS     5
#define GET_DESCRIPTOR      6
#define SET_DESCRIPTOR      7
#define GET_CONFIGURATION   8
#define SET_CONFIGURATION   9
#define GET_INTERFACE       10
#define SET_INTERFACE       11
#define SYNCH_FRAME     12

// SETUP always uses a DATA0 PID for the data field of the SETUP transaction.
// transactions in the data phase start with DATA1 and toggle (figure 8-12, USB1.1)
// Status stage uses a DATA1 PID.

static uint8_t ep0_rx0_buf[EP0_SIZE] __attribute__ ((aligned (4)));
static uint8_t ep0_rx1_buf[EP0_SIZE] __attribute__ ((aligned (4)));
static uint8_t ep0_tx0_buf[EP0_SIZE] __attribute__ ((aligned (4)));
static uint8_t ep0_tx1_buf[EP0_SIZE] __attribute__ ((aligned (4)));
static const uint8_t *ep0_tx_ptr = NULL;
static uint16_t ep0_tx_len;
static uint8_t ep0_tx_bdt_bank = 0;
static uint8_t ep0_tx_data_toggle = 0;

volatile uint8_t usb_configuration = 0;
volatile uint8_t usb_dfu_state = DFU_appIDLE;

static void endpoint0_stall(void)
{
    USB0_ENDPT0 = USB_ENDPT_EPSTALL | USB_ENDPT_EPRXEN | USB_ENDPT_EPTXEN | USB_ENDPT_EPHSHK;
}


static void endpoint0_transmit(const void *data, uint32_t len)
{
    // Use a local transmit buffer pair in RAM, and copy in the source data (usually decriptors).
    // We can't reliably serve USB data from flash apparently, and this is a little more RAM
    // efficient than keeping all descriptors in RAM.

    uint8_t *buffer = ep0_tx_bdt_bank ? ep0_tx1_buf : ep0_tx0_buf;
    uint32_t count = len;
    while (count--) {
        *buffer = *(const uint8_t*)data;
        data++;
        buffer++;
    }

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
    volatile uint8_t *reg;
    uint8_t epconf;
    const uint8_t *cfg;
    int i;

    switch (setup.wRequestAndType) {
      case 0x0500: // SET_ADDRESS
        break;
      case 0x0900: // SET_CONFIGURATION
        usb_configuration = setup.wValue;
        reg = &USB0_ENDPT1;
        cfg = usb_endpoint_config_table;
        // clear all BDT entries, free any allocated memory...
        for (i=4; i <= NUM_ENDPOINTS*4; i++) {
            if (table[i].desc & BDT_OWN) {
                usb_free((usb_packet_t *)((uint8_t *)(table[i].addr) - 8));
            }
        }

        for (i=1; i <= NUM_ENDPOINTS; i++) {
            epconf = *cfg++;
            *reg = epconf;
            reg += 4;
            if (epconf & USB_ENDPT_EPRXEN) {
                usb_packet_t *p;

                p = usb_malloc();
                table[index(i, RX, EVEN)].addr = p->buf;
                table[index(i, RX, EVEN)].desc = BDT_DESC(64, 0);

                p = usb_malloc();
                table[index(i, RX, ODD)].addr = p->buf;
                table[index(i, RX, ODD)].desc = BDT_DESC(64, 1);
            }
            table[index(i, TX, EVEN)].desc = 0;
            table[index(i, TX, ODD)].desc = 0;
        }
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
        if (setup.wIndex > NUM_ENDPOINTS) {
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
        if (i > NUM_ENDPOINTS || setup.wValue != 0) {
            endpoint0_stall();
            return;
        }
        (*(uint8_t *)(&USB0_ENDPT0 + setup.wIndex * 4)) &= ~0x02;
        break;
      case 0x0302: // SET_FEATURE (endpoint)
        i = setup.wIndex & 0x7F;
        if (i > NUM_ENDPOINTS || setup.wValue != 0) {
            endpoint0_stall();
            return;
        }
        (*(uint8_t *)(&USB0_ENDPT0 + setup.wIndex * 4)) |= 0x02;
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

      case 0x01C0:      // Read performance counter
      case 0x01C1:
        datalen = 4;
        switch (setup.wIndex) {
            case 0:
                data = (uint8_t*) &perf_frameCounter;
                break;
            case 1:
                data = (uint8_t*) &perf_receivedKeyframeCounter;
                break;
            default:
                endpoint0_stall();
                return;
        }
        break;

      case (MSFT_VENDOR_CODE << 8) | 0xC0:      // Get Microsoft descriptor
      case (MSFT_VENDOR_CODE << 8) | 0xC1:
        if (setup.wIndex == 0x0004) {
            // Return WCID descriptor
            data = usb_microsoft_wcid;
            datalen = usb_microsoft_wcid[0];
            break;
        } else if (setup.wIndex == 0x0005) {
            // Return Extended Properties descriptor
            data = usb_microsoft_extprop;
            datalen = usb_microsoft_extprop[0];
            break;
        } 
        endpoint0_stall();
        return;

      case 0x03a1: // DFU_GETSTATUS
        if (setup.wIndex != DFU_INTERFACE) {
            endpoint0_stall();
            return;
        }
        reply_buffer[0] = 0;    // bStatus = OK
        reply_buffer[1] = 1;    // bwPollTimeout LSB = 1
        reply_buffer[2] = 0;    // bwPollTimeout
        reply_buffer[3] = 0;    // bwPollTimeout
        reply_buffer[4] = usb_dfu_state;
        reply_buffer[5] = 0;    // iString = 0
        data = reply_buffer;
        datalen = 6;
        break;

      case 0x05a1: // DFU_GETSTATE
        if (setup.wIndex != DFU_INTERFACE) {
            endpoint0_stall();
            return;
        }
        reply_buffer[0] = usb_dfu_state;
        data = reply_buffer;
        datalen = 1;
        break;

      case 0x0021: // DFU_DETACH
        if (setup.wIndex != DFU_INTERFACE) {
            endpoint0_stall();
            return;
        }
        usb_dfu_state = DFU_appDETACH;
        break;

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



//A bulk endpoint's toggle sequence is initialized to DATA0 when the endpoint
//experiences any configuration event (configuration events are explained in
//Sections 9.1.1.5 and 9.4.5).

//Configuring a device or changing an alternate setting causes all of the status
//and configuration values associated with endpoints in the affected interfaces
//to be set to their default values. This includes setting the data toggle of
//any endpoint using data toggles to the value DATA0.

//For endpoints using data toggle, regardless of whether an endpoint has the
//Halt feature set, a ClearFeature(ENDPOINT_HALT) request always results in the
//data toggle being reinitialized to DATA0.


static void usb_control(uint32_t stat)
{
    bdt_t *b;
    uint32_t pid, size;
    uint8_t *buf;
    const uint8_t *data;

    b = &table[stat2tableindex(stat)];
    pid = BDT_PID(b->desc);
    buf = b->addr;

    switch (pid) {
    case 0x0D: // Setup received from host
        // grab the 8 byte setup info
        setup.word1 = *(uint32_t *)(buf);
        setup.word2 = *(uint32_t *)(buf + 4);

        // give the buffer back
        b->desc = BDT_DESC(EP0_SIZE, DATA1);

        // clear any leftover pending IN transactions
        ep0_tx_ptr = NULL;

        // first IN after Setup is always DATA1
        ep0_tx_data_toggle = 1;

        // actually "do" the setup request
        usb_setup();
        // unfreeze the USB, now that we're ready
        USB0_CTL = USB_CTL_USBENSOFEN; // clear TXSUSPENDTOKENBUSY bit
        break;

    case 0x01:  // OUT transaction received from host
        // give the buffer back
        b->desc = BDT_DESC(EP0_SIZE, DATA1);
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


static void usb_try_rx(unsigned index)
{
    // We have a packet waiting in a receive buffer. Try to deliver it
    // with usb_rx_handler. If it can't take the packet yet, defer processing
    // and leave it in the serial engine's buffer. (This will stall, providing
    // back-pressure to the host controller.)

    bdt_t *b = &table[index];
    usb_packet_t *packet = (usb_packet_t *)((uint8_t *)(b->addr) - 4);
    unsigned len = b->desc >> 16;

    if (len > 0) {

        packet->len = len;
        if (!usb_rx_handler(packet)) {
            // Deferred! We'll try again in usb_rx_resume()

            __disable_irq();
            bdt_deferred_map |= 1 << index;
            __enable_irq();

            return;
        }

        // Allocate a new packet
        packet = usb_malloc();
        b->addr = packet->buf;
    }

    // Unblock the serial engine to receive another packet into this buffer
    b->desc = BDT_DESC(64, ((uint32_t)b & 8) ? DATA1 : DATA0);
}


void usb_rx_resume()
{
    // If we have any deferred packets, try them again.

    int idx = 0;

    __disable_irq();
    unsigned deferred = bdt_deferred_map;
    bdt_deferred_map = 0;
    __enable_irq();

    while (deferred) {

        if (deferred & 1) {
            usb_try_rx(idx);
        }

        idx++;
        deferred >>= 1;
    }
}


void usb_isr(void)
{
    uint8_t status, stat;

restart:
    status = USB0_ISTAT;

    if ((status & USB_INTEN_SOFTOKEN /* 04 */ )) {
        USB0_ISTAT = USB_INTEN_SOFTOKEN;
    }

    if ((status & USB_ISTAT_TOKDNE /* 08 */ )) {
        uint8_t endpoint;
        stat = USB0_STAT;
        endpoint = stat >> 4;

        if (endpoint == 0) {
            usb_control(stat);
        } else if (stat & 0x08) {
            // We have no transmit endpoints; stub.
        } else {
            usb_try_rx(stat2tableindex(stat));
        }

        USB0_ISTAT = USB_ISTAT_TOKDNE;
        goto restart;
    }

    if (status & USB_ISTAT_USBRST /* 01 */ ) {

        // initialize BDT toggle bits
        USB0_CTL = USB_CTL_ODDRST;
        ep0_tx_bdt_bank = 0;

        // set up buffers to receive Setup and OUT packets
        table[index(0, RX, EVEN)].desc = BDT_DESC(EP0_SIZE, 0);
        table[index(0, RX, EVEN)].addr = ep0_rx0_buf;
        table[index(0, RX, ODD)].desc = BDT_DESC(EP0_SIZE, 0);
        table[index(0, RX, ODD)].addr = ep0_rx1_buf;
        table[index(0, TX, EVEN)].desc = 0;
        table[index(0, TX, EVEN)].addr = ep0_tx0_buf;
        table[index(0, TX, ODD)].desc = 0;
        table[index(0, TX, ODD)].addr = ep0_tx1_buf;

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
    int i;

    usb_init_mem();
    usb_init_serialnumber();

    for (i=0; i <= NUM_ENDPOINTS*4; i++) {
        table[i].desc = 0;
        table[i].addr = 0;
    }

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



