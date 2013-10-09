/* Teensyduino Core Library
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

#include "usb_desc.h"
#include "usb_names.h"
#include "mk20dx128.h"

// USB Descriptors are binary data which the USB host reads to
// automatically detect a USB device's capabilities.  The format
// and meaning of every field is documented in numerous USB
// standards.  When working with USB descriptors, despite the
// complexity of the standards and poor writing quality in many
// of those documents, remember descriptors are nothing more
// than constant binary data that tells the USB host what the
// device can do.  Computers will load drivers based on this data.
// Those drivers then communicate on the endpoints specified by
// the descriptors.

// To configure a new combination of interfaces or make minor
// changes to existing configuration (eg, change the name or ID
// numbers), usually you would edit "usb_desc.h".  This file
// is meant to be configured by the header, so generally it is
// only edited to add completely new USB interfaces or features.



// **************************************************************
//   USB Device
// **************************************************************

#define LSB(n) ((n) & 255)
#define MSB(n) (((n) >> 8) & 255)

// USB Device Descriptor.  The USB host reads this first, to learn
// what type of device is connected.
static uint8_t device_descriptor[] = {
        18,                                     // bLength
        1,                                      // bDescriptorType
        0x00, 0x02,                             // bcdUSB
#ifdef DEVICE_CLASS
        DEVICE_CLASS,                           // bDeviceClass
#else
        0,
#endif
#ifdef DEVICE_SUBCLASS
        DEVICE_SUBCLASS,                        // bDeviceSubClass
#else
        0,
#endif
#ifdef DEVICE_PROTOCOL
        DEVICE_PROTOCOL,                        // bDeviceProtocol
#else
        0,
#endif
        EP0_SIZE,                               // bMaxPacketSize0
        LSB(VENDOR_ID), MSB(VENDOR_ID),         // idVendor
        LSB(PRODUCT_ID), MSB(PRODUCT_ID),       // idProduct
        LSB(DEVICE_VER), MSB(DEVICE_VER),       // bcdDevice
        1,                                      // iManufacturer
        2,                                      // iProduct
        3,                                      // iSerialNumber
        1                                       // bNumConfigurations
};

// These descriptors must NOT be "const", because the USB DMA
// has trouble accessing flash memory with enough bandwidth
// while the processor is executing from flash.


// **************************************************************
//   USB Configuration
// **************************************************************

// USB Configuration Descriptor.  This huge descriptor tells all
// of the devices capbilities.
static uint8_t config_descriptor[CONFIG_DESC_SIZE] = {
        // configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
        9,                                      // bLength;
        2,                                      // bDescriptorType;
        LSB(CONFIG_DESC_SIZE),                 // wTotalLength
        MSB(CONFIG_DESC_SIZE),
        NUM_INTERFACE,                          // bNumInterfaces
        1,                                      // bConfigurationValue
        0,                                      // iConfiguration
        0x80,                                   // bmAttributes
        50,                                     // bMaxPower

#ifdef FC_INTERFACE
        // interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
        9,                                      // bLength
        4,                                      // bDescriptorType
        FC_INTERFACE,                           // bInterfaceNumber
        0,                                      // bAlternateSetting
        1,                                      // bNumEndpoints
        0xff,                                   // bInterfaceClass (Vendor specific)
        0x00,                                   // bInterfaceSubClass
        0x00,                                   // bInterfaceProtocol
        0,                                      // iInterface
        // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
        7,                                      // bLength
        5,                                      // bDescriptorType
        FC_OUT_ENDPOINT,                        // bEndpointAddress
        0x02,                                   // bmAttributes (0x02=bulk)
        FC_OUT_SIZE, 0,                         // wMaxPacketSize
        0,                                      // bInterval
#endif // FC_INTERFACE

#ifdef DFU_INTERFACE
        // interface descriptor, DFU Mode (DFU spec Table 4.4)
        9,                                      // bLength
        4,                                      // bDescriptorType
        DFU_INTERFACE,                          // bInterfaceNumber
        0,                                      // bAlternateSetting
        0,                                      // bNumEndpoints
        0xFE,                                   // bInterfaceClass
        0x01,                                   // bInterfaceSubClass
        0x01,                                   // bInterfaceProtocol (Runtime)
        4,                                      // iInterface
        // DFU Functional Descriptor (DFU spec TAble 4.2)
        9,                                      // bLength
        0x21,                                   // bDescriptorType
        0x0D,                                   // bmAttributes
        LSB(DFU_DETACH_TIMEOUT),                // wDetachTimeOut
        MSB(DFU_DETACH_TIMEOUT),
        LSB(DFU_TRANSFER_SIZE),                 // wTransferSize
        MSB(DFU_TRANSFER_SIZE),
        0x01,0x01,                              // bcdDFUVersion
#endif // DFU_INTERFACE
};


// **************************************************************
//   String Descriptors
// **************************************************************

// The descriptors above can provide human readable strings,
// referenced by index numbers.  These descriptors are the
// actual string data

/* defined in usb_names.h
struct usb_string_descriptor_struct {
        uint8_t bLength;
        uint8_t bDescriptorType;
        uint16_t wString[];
};
*/

extern struct usb_string_descriptor_struct usb_string_manufacturer_name
        __attribute__ ((weak, alias("usb_string_manufacturer_name_default")));
extern struct usb_string_descriptor_struct usb_string_product_name
        __attribute__ ((weak, alias("usb_string_product_name_default")));
extern struct usb_string_descriptor_struct usb_string_serial_number
        __attribute__ ((weak, alias("usb_string_serial_number_default")));

struct usb_string_descriptor_struct string0 = {
    4,
    3,
    {0x0409}
};

struct usb_string_descriptor_struct usb_string_manufacturer_name_default = {
    2 + MANUFACTURER_NAME_LEN * 2,
    3,
    MANUFACTURER_NAME
};
struct usb_string_descriptor_struct usb_string_product_name_default = {
    2 + PRODUCT_NAME_LEN * 2,
    3,
    PRODUCT_NAME
};
struct usb_string_descriptor_struct usb_string_dfu_name = {
    2 + DFU_NAME_LEN * 2,
    3,
    DFU_NAME
};

// Microsoft OS String Descriptor. See: https://github.com/pbatard/libwdi/wiki/WCID-Devices
struct usb_string_descriptor_struct usb_string_microsoft = {
    18, 3,
    {'M','S','F','T','1','0','0', MSFT_VENDOR_CODE}
};

// Microsoft WCID
const uint8_t usb_microsoft_wcid[MSFT_WCID_LEN] = {
    MSFT_WCID_LEN, 0, 0, 0,         // Length
    0x00, 0x01,                     // Version
    0x04, 0x00,                     // Compatibility ID descriptor index
    0x01,                           // Number of sections
    0, 0, 0, 0, 0, 0, 0,            // Reserved (7 bytes)

    DFU_INTERFACE,                  // Interface number
    0x01,                           // Reserved
    'W','I','N','U','S','B',0,0,    // Compatible ID
    0,0,0,0,0,0,0,0,                // Sub-compatible ID (unused)
    0,0,0,0,0,0,                    // Reserved
};

// 32-digit hex string, corresponding to the MK20DX128's built-in unique 128-bit ID.
struct usb_string_descriptor_struct usb_string_serial_number_default = {
    2 + 16 * 2,
    3,
    {0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0}
};

void usb_init_serialnumber(void)
{
    /*
     * The CPU has a 128-bit unique serial number. But it seems to encode
     * some manufacturing information in a way that makes it hard to visually
     * parse. Distinct devices may have very similar-looking serial numbers.
     *
     * To make this more user-friendly, we mix up the serial number using
     * an FNV hash and alphabetically encode it.
     */

    union {
        uint8_t bytes[16];
        struct {
            uint32_t a, b, c, d;
        };
    } serial;

    unsigned i;
    uint32_t hash = 0x811c9dc5;

    serial.d = SIM_UIDH;
    serial.c = SIM_UIDMH;
    serial.b = SIM_UIDML;
    serial.a = SIM_UIDL;

    // Initial hash
    for (i = 0; i < 16; ++i) {
        hash = (hash ^ serial.bytes[i]) * 0x1000193;
    }

    // Output letters
    for (i = 0; i < 16; ++i) {
        hash = (hash ^ serial.bytes[i]) * 0x1000193;
        usb_string_serial_number_default.wString[i] = 'A' + (hash % 26);
    }
}


// **************************************************************
//   Descriptors List
// **************************************************************

// This table provides access to all the descriptor data above.

const usb_descriptor_list_t usb_descriptor_list[] = {
    {0x0100, device_descriptor, sizeof(device_descriptor)},
    {0x0200, config_descriptor, sizeof(config_descriptor)},
    {0x0300, (const uint8_t *)&string0, 0},
    {0x0301, (const uint8_t *)&usb_string_manufacturer_name, 0},
    {0x0302, (const uint8_t *)&usb_string_product_name, 0},
    {0x0303, (const uint8_t *)&usb_string_serial_number, 0},
    {0x0304, (const uint8_t *)&usb_string_dfu_name, 0},
    {0x03EE, (const uint8_t *)&usb_string_microsoft, 0},
    {0, NULL, 0}
};


// **************************************************************
//   Endpoint Configuration
// **************************************************************

const uint8_t usb_endpoint_config_table[NUM_ENDPOINTS] = 
{
#if (defined(ENDPOINT1_CONFIG) && NUM_ENDPOINTS >= 1)
        ENDPOINT1_CONFIG,
#elif (NUM_ENDPOINTS >= 1)
        ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT2_CONFIG) && NUM_ENDPOINTS >= 2)
        ENDPOINT2_CONFIG,
#elif (NUM_ENDPOINTS >= 2)
        ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT3_CONFIG) && NUM_ENDPOINTS >= 3)
        ENDPOINT3_CONFIG,
#elif (NUM_ENDPOINTS >= 3)
        ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT4_CONFIG) && NUM_ENDPOINTS >= 4)
        ENDPOINT4_CONFIG,
#elif (NUM_ENDPOINTS >= 4)
        ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT5_CONFIG) && NUM_ENDPOINTS >= 5)
        ENDPOINT5_CONFIG,
#elif (NUM_ENDPOINTS >= 5)
        ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT6_CONFIG) && NUM_ENDPOINTS >= 6)
        ENDPOINT6_CONFIG,
#elif (NUM_ENDPOINTS >= 6)
        ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT7_CONFIG) && NUM_ENDPOINTS >= 7)
        ENDPOINT7_CONFIG,
#elif (NUM_ENDPOINTS >= 7)
        ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT8_CONFIG) && NUM_ENDPOINTS >= 8)
        ENDPOINT8_CONFIG,
#elif (NUM_ENDPOINTS >= 8)
        ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT9_CONFIG) && NUM_ENDPOINTS >= 9)
        ENDPOINT9_CONFIG,
#elif (NUM_ENDPOINTS >= 9)
        ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT10_CONFIG) && NUM_ENDPOINTS >= 10)
        ENDPOINT10_CONFIG,
#elif (NUM_ENDPOINTS >= 10)
        ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT11_CONFIG) && NUM_ENDPOINTS >= 11)
        ENDPOINT11_CONFIG,
#elif (NUM_ENDPOINTS >= 11)
        ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT12_CONFIG) && NUM_ENDPOINTS >= 12)
        ENDPOINT12_CONFIG,
#elif (NUM_ENDPOINTS >= 12)
        ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT13_CONFIG) && NUM_ENDPOINTS >= 13)
        ENDPOINT13_CONFIG,
#elif (NUM_ENDPOINTS >= 13)
        ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT14_CONFIG) && NUM_ENDPOINTS >= 14)
        ENDPOINT14_CONFIG,
#elif (NUM_ENDPOINTS >= 14)
        ENDPOINT_UNUSED,
#endif
#if (defined(ENDPOINT15_CONFIG) && NUM_ENDPOINTS >= 15)
        ENDPOINT15_CONFIG,
#elif (NUM_ENDPOINTS >= 15)
        ENDPOINT_UNUSED,
#endif
};




