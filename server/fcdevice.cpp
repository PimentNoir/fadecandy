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

#include "fcdevice.h"


FCDevice::FCDevice(libusb_device *device, bool verbose)
	: mVerbose(verbose),
	  mDevice(libusb_ref_device(device)),
	  mHandle(0),
	  mConfig(0)
{
	mSerial[0] = '\0';
}

FCDevice::~FCDevice()
{
	if (mHandle) {
		libusb_close(mHandle);
	}
	if (mDevice) {
		libusb_unref_device(mDevice);
	}
}

bool FCDevice::isFadecandy()
{
	libusb_device_descriptor dd;

	if (libusb_get_device_descriptor(mDevice, &dd) < 0) {
		// Can't access descriptor?
		return false;
	}

	return dd.idVendor == 0x1d50 && dd.idProduct == 0x607a;
}

int FCDevice::open()
{
	int r = libusb_open(mDevice, &mHandle);
	if (r < 0) {
		return r;
	}

	libusb_device_descriptor dd;
	r = libusb_get_device_descriptor(mDevice, &dd);
	if (r < 0) {
		return r;
	}

	return libusb_get_string_descriptor_ascii(mHandle, dd.iSerialNumber, (uint8_t*)mSerial, sizeof mSerial);
}

void FCDevice::setConfiguration(const Value *config)
{
	mConfig = config;
}

void FCDevice::writeColorCorrection(const Value &color)
{
}

void FCDevice::writeMessage(const OPCSink::Message &msg)
{
}
