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

static usb_packet_t *rx_first[NUM_ENDPOINTS];
static usb_packet_t *rx_last[NUM_ENDPOINTS];
static usb_packet_t *tx_first[NUM_ENDPOINTS];
static usb_packet_t *tx_last[NUM_ENDPOINTS];
uint16_t usb_rx_byte_count_data[NUM_ENDPOINTS];

static uint8_t reply_buffer[8];

// Performance counters
volatile uint32_t perf_frameCounter;

static uint8_t tx_state[NUM_ENDPOINTS];
#define TX_STATE_BOTH_FREE_EVEN_FIRST   0
#define TX_STATE_BOTH_FREE_ODD_FIRST    1
#define TX_STATE_EVEN_FREE      2
#define TX_STATE_ODD_FREE       3
#define TX_STATE_NONE_FREE_EVEN_FIRST   4
#define TX_STATE_NONE_FREE_ODD_FIRST    5

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
uint8_t usb_rx_memory_needed = 0;

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
        // free all queued packets
        for (i=0; i < NUM_ENDPOINTS; i++) {
            usb_packet_t *p, *n;
            p = rx_first[i];
            while (p) {
                n = p->next;
                usb_free(p);
                p = n;
            }
            rx_first[i] = NULL;
            rx_last[i] = NULL;
            p = tx_first[i];
            while (p) {
                n = p->next;
                usb_free(p);
                p = n;
            }
            tx_first[i] = NULL;
            tx_last[i] = NULL;
            usb_rx_byte_count_data[i] = 0;
            switch (tx_state[i]) {
              case TX_STATE_EVEN_FREE:
              case TX_STATE_NONE_FREE_EVEN_FIRST:
                tx_state[i] = TX_STATE_BOTH_FREE_EVEN_FIRST;
                break;
              case TX_STATE_ODD_FREE:
              case TX_STATE_NONE_FREE_ODD_FIRST:
                tx_state[i] = TX_STATE_BOTH_FREE_ODD_FIRST;
                break;
              default:
                break;
            }
        }
        usb_rx_memory_needed = 0;
        for (i=1; i <= NUM_ENDPOINTS; i++) {
            epconf = *cfg++;
            *reg = epconf;
            reg += 4;
            if (epconf & USB_ENDPT_EPRXEN) {
                usb_packet_t *p;
                p = usb_malloc();
                if (p) {
                    table[index(i, RX, EVEN)].addr = p->buf;
                    table[index(i, RX, EVEN)].desc = BDT_DESC(64, 0);
                } else {
                    table[index(i, RX, EVEN)].desc = 0;
                    usb_rx_memory_needed++;
                }
                p = usb_malloc();
                if (p) {
                    table[index(i, RX, ODD)].addr = p->buf;
                    table[index(i, RX, ODD)].desc = BDT_DESC(64, 1);
                } else {
                    table[index(i, RX, ODD)].desc = 0;
                    usb_rx_memory_needed++;
                }
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

      case 0x01C0:      // Read frame counter
      case 0x01C1:
        data = (uint8_t*) &perf_frameCounter;
        datalen = sizeof perf_frameCounter;
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



// #define stat2bufferdescriptor(stat) (table + ((stat) >> 2))

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


usb_packet_t *usb_rx(uint32_t endpoint)
{
    usb_packet_t *ret;
    endpoint--;
    if (endpoint >= NUM_ENDPOINTS) return NULL;
    __disable_irq();
    ret = rx_first[endpoint];
    if (ret) rx_first[endpoint] = ret->next;
    usb_rx_byte_count_data[endpoint] -= ret->len;
    __enable_irq();
    return ret;
}

static uint32_t usb_queue_byte_count(const usb_packet_t *p)
{
    uint32_t count=0;

    __disable_irq();
    for ( ; p; p = p->next) {
        count += p->len;
    }
    __enable_irq();
    return count;
}

// TODO: make this an inline function...
/*
uint32_t usb_rx_byte_count(uint32_t endpoint)
{
    endpoint--;
    if (endpoint >= NUM_ENDPOINTS) return 0;
    return usb_rx_byte_count_data[endpoint];
    //return usb_queue_byte_count(rx_first[endpoint]);
}
*/

uint32_t usb_tx_byte_count(uint32_t endpoint)
{
    endpoint--;
    if (endpoint >= NUM_ENDPOINTS) return 0;
    return usb_queue_byte_count(tx_first[endpoint]);
}

uint32_t usb_tx_packet_count(uint32_t endpoint)
{
    const usb_packet_t *p;
    uint32_t count=0;

    endpoint--;
    if (endpoint >= NUM_ENDPOINTS) return 0;
    p = tx_first[endpoint];
    __disable_irq();
    for ( ; p; p = p->next) count++;
    __enable_irq();
    return count;
}


// Called from usb_free, but only when usb_rx_memory_needed > 0, indicating
// receive endpoints are starving for memory.  The intention is to give
// endpoints needing receive memory priority over the user's code, which is
// likely calling usb_malloc to obtain memory for transmitting.  When the
// user is creating data very quickly, their consumption could starve reception
// without this prioritization.  The packet buffer (input) is assigned to the
// first endpoint needing memory.
//
void usb_rx_memory(usb_packet_t *packet)
{
    unsigned int i;
    const uint8_t *cfg;

    cfg = usb_endpoint_config_table;
    __disable_irq();
    for (i=1; i <= NUM_ENDPOINTS; i++) {
        if (*cfg++ & USB_ENDPT_EPRXEN) {
            if (table[index(i, RX, EVEN)].desc == 0) {
                table[index(i, RX, EVEN)].addr = packet->buf;
                table[index(i, RX, EVEN)].desc = BDT_DESC(64, 0);
                usb_rx_memory_needed--;
                __enable_irq();
                return;
            }
            if (table[index(i, RX, ODD)].desc == 0) {
                table[index(i, RX, ODD)].addr = packet->buf;
                table[index(i, RX, ODD)].desc = BDT_DESC(64, 1);
                usb_rx_memory_needed--;
                __enable_irq();
                return;
            }
        }
    }
    __enable_irq();

    // we should never reach this point.  If we get here, it means
    // usb_rx_memory_needed was set greater than zero, but no memory
    // was actually needed.  
    usb_rx_memory_needed = 0;
    usb_free(packet);
    return;
}

//#define index(endpoint, tx, odd) (((endpoint) << 2) | ((tx) << 1) | (odd))
//#define stat2bufferdescriptor(stat) (table + ((stat) >> 2))

void usb_tx(uint32_t endpoint, usb_packet_t *packet)
{
    bdt_t *b = &table[index(endpoint, TX, EVEN)];
    uint8_t next;

    endpoint--;
    if (endpoint >= NUM_ENDPOINTS) return;
    __disable_irq();

    switch (tx_state[endpoint]) {
      case TX_STATE_BOTH_FREE_EVEN_FIRST:
        next = TX_STATE_ODD_FREE;
        break;
      case TX_STATE_BOTH_FREE_ODD_FIRST:
        b++;
        next = TX_STATE_EVEN_FREE;
        break;
      case TX_STATE_EVEN_FREE:
        next = TX_STATE_NONE_FREE_ODD_FIRST;
        break;
      case TX_STATE_ODD_FREE:
        b++;
        next = TX_STATE_NONE_FREE_EVEN_FIRST;
        break;
      default:
        if (tx_first[endpoint] == NULL) {
            tx_first[endpoint] = packet;
        } else {
            tx_last[endpoint]->next = packet;
        }
        tx_last[endpoint] = packet;
        __enable_irq();
        return;
    }
    tx_state[endpoint] = next;
    b->addr = packet->buf;
    b->desc = BDT_DESC(packet->len, ((uint32_t)b & 8) ? DATA1 : DATA0);
    __enable_irq();
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
        } else {
            bdt_t *b = stat2bufferdescriptor(stat);
            usb_packet_t *packet = (usb_packet_t *)((uint8_t *)(b->addr) - 8);
            endpoint--; // endpoint is index to zero-based arrays

            if (stat & 0x08) { // transmit
                usb_free(packet);
                packet = tx_first[endpoint];
                if (packet) {
                    tx_first[endpoint] = packet->next;
                    b->addr = packet->buf;
                    switch (tx_state[endpoint]) {
                      case TX_STATE_BOTH_FREE_EVEN_FIRST:
                        tx_state[endpoint] = TX_STATE_ODD_FREE;
                        break;
                      case TX_STATE_BOTH_FREE_ODD_FIRST:
                        tx_state[endpoint] = TX_STATE_EVEN_FREE;
                        break;
                      case TX_STATE_EVEN_FREE:
                        tx_state[endpoint] = TX_STATE_NONE_FREE_ODD_FIRST;
                        break;
                      case TX_STATE_ODD_FREE:
                        tx_state[endpoint] = TX_STATE_NONE_FREE_EVEN_FIRST;
                        break;
                      default:
                        break;
                    }
                    b->desc = BDT_DESC(packet->len, ((uint32_t)b & 8) ? DATA1 : DATA0);
                } else {
                    switch (tx_state[endpoint]) {
                      case TX_STATE_BOTH_FREE_EVEN_FIRST:
                      case TX_STATE_BOTH_FREE_ODD_FIRST:
                        break;
                      case TX_STATE_EVEN_FREE:
                        tx_state[endpoint] = TX_STATE_BOTH_FREE_EVEN_FIRST;
                        break;
                      case TX_STATE_ODD_FREE:
                        tx_state[endpoint] = TX_STATE_BOTH_FREE_ODD_FIRST;
                        break;
                      default:
                        tx_state[endpoint] = ((uint32_t)b & 8) ?
                          TX_STATE_ODD_FREE : TX_STATE_EVEN_FREE;
                        break;
                    }
                }
            } else { // receive
                packet->len = b->desc >> 16;
                if (packet->len > 0) {
                    packet->next = NULL;
                    if (rx_first[endpoint] == NULL) {
                        rx_first[endpoint] = packet;
                    } else {
                        rx_last[endpoint]->next = packet;
                    }
                    rx_last[endpoint] = packet;
                    usb_rx_byte_count_data[endpoint] += packet->len;
                    // TODO: implement a per-endpoint maximum # of allocated packets
                    // so a flood of incoming data on 1 endpoint doesn't starve
                    // the others if the user isn't reading it regularly
                    packet = usb_malloc();
                    if (packet) {
                        b->addr = packet->buf;
                        b->desc = BDT_DESC(64, ((uint32_t)b & 8) ? DATA1 : DATA0);
                    } else {
                        b->desc = 0;
                        usb_rx_memory_needed++;
                    }
                } else {
                    b->desc = BDT_DESC(64, ((uint32_t)b & 8) ? DATA1 : DATA0);
                }
            }




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



