/*
 * Fadecandy device interface
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
#include "rapidjson/document.h"
#include "opcsink.h"
#include <libusb.h>
#include <set>


class FCDevice
{
public:
	typedef rapidjson::Value Value;
	static const unsigned NUM_PIXELS = 512;

    FCDevice(libusb_device *device, bool verbose = false);
    ~FCDevice();

    libusb_device *getDevice() { return mDevice; };
    bool isFadecandy();
    int open();

    // Valid after open():

    const char *getSerial() { return mSerial; }
    void setConfiguration(const Value *config);

 	// High level OPC message entry point
    void writeMessage(const OPCSink::Message &msg);

    // Write color LUT from parsed JSON
	void writeColorCorrection(const Value &color);

	// Send current buffer contents
	void writeFramebuffer();

	// Framebuffer accessor
	uint8_t *fbPixel(unsigned num) {
		return &mFramebuffer[num / PIXELS_PER_PACKET].data[3 * (num % PIXELS_PER_PACKET)];
	}

 
private:
	static const unsigned PIXELS_PER_PACKET = 21;
	static const unsigned LUT_ENTRIES_PER_PACKET = 31;
	static const unsigned FRAMEBUFFER_PACKETS = 25;
	static const unsigned LUT_PACKETS = 25;
	static const unsigned LUT_ENTRIES = 257;
	static const unsigned OUT_ENDPOINT = 1;

	static const uint8_t TYPE_FRAMEBUFFER = 0x00;
	static const uint8_t TYPE_LUT = 0x40;
	static const uint8_t TYPE_CONFIG = 0x80;
	static const uint8_t FINAL = 0x20;

	struct Packet {
		uint8_t control;
		uint8_t data[63];
	};

	struct Transfer {
		Transfer(FCDevice *device, void *buffer, int length);
		~Transfer();
		libusb_transfer *transfer;
		FCDevice *device;
	};

	bool mVerbose;
    libusb_device *mDevice;
    libusb_device_handle *mHandle;
    const Value *mConfig;
    std::set<Transfer*> mPending;

    char mSerial[256];
    Packet mFramebuffer[FRAMEBUFFER_PACKETS];
    Packet mColorLUT[LUT_PACKETS];

    void submitTransfer(Transfer *fct);
    static void completeTransfer(struct libusb_transfer *transfer);
};
