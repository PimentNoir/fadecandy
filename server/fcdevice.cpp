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
#include <iostream>


FCDevice::Transfer::Transfer(FCDevice *device, void *buffer, int length)
	: transfer(libusb_alloc_transfer(0)),
	  device(device)
{
	libusb_fill_bulk_transfer(transfer, device->mHandle,
		OUT_ENDPOINT, (uint8_t*) buffer, length, FCDevice::completeTransfer, this, 2000);
}

FCDevice::Transfer::~Transfer()
{
	libusb_free_transfer(transfer);
}

FCDevice::FCDevice(libusb_device *device, bool verbose)
	: mVerbose(verbose),
	  mDevice(libusb_ref_device(device)),
	  mHandle(0),
	  mConfig(0)
{
	mSerial[0] = '\0';
	memset(mFramebuffer, 0, sizeof mFramebuffer);

	// Framebuffer headers
	for (unsigned i = 0; i < FRAMEBUFFER_PACKETS; ++i) {
		mFramebuffer[i].control = TYPE_FRAMEBUFFER | i;
	}
	mFramebuffer[FRAMEBUFFER_PACKETS - 1].control |= FINAL;
}

FCDevice::~FCDevice()
{
	/*
	 * If we have pending transfers, cancel them and jettison them
	 * from the FCDevice. The Transfer objects themselves will be freed
	 * once libusb completes them.
	 */

	for (std::set<Transfer*>::iterator i = mPending.begin(), e = mPending.end(); i != e; ++i) {
		Transfer *fct = *i;
		libusb_cancel_transfer(fct->transfer);
		fct->device = 0;
	}

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
	libusb_device_descriptor dd;
	int r = libusb_get_device_descriptor(mDevice, &dd);
	if (r < 0) {
		return r;
	}

	r = libusb_open(mDevice, &mHandle);
	if (r < 0) {
		return r;
	}

	r = libusb_claim_interface(mHandle, 0);
	if (r < 0) {
		return r;
	}

	return libusb_get_string_descriptor_ascii(mHandle, dd.iSerialNumber, (uint8_t*)mSerial, sizeof mSerial);
}

void FCDevice::setConfiguration(const Value *config)
{
	mConfig = config;
}

void FCDevice::submitTransfer(Transfer *fct)
{
	/*
	 * Submit a new USB transfer. The Transfer object is guaranteed to be freed eventually.
	 * On error, it's freed right away.
	 */

	int r = libusb_submit_transfer(fct->transfer);

	if (r < 0) {
		if (mVerbose) {
			std::clog << "Error submitting USB transfer: " << libusb_strerror(libusb_error(r)) << "\n";
		}
		delete fct;
	} else {
		mPending.insert(fct);
	}
}

void FCDevice::completeTransfer(struct libusb_transfer *transfer)
{
	/*
	 * Transfer complete. The FCDevice may or may not still exist; if the device was unplugged,
	 * fct->device will be set to 0 by ~FCDevice().
	 */

	FCDevice::Transfer *fct = static_cast<FCDevice::Transfer*>(transfer->user_data);
	FCDevice *self = fct->device;

	if (self) {
		self->mPending.erase(fct);
	}

	delete fct;
}

void FCDevice::writeColorCorrection(const Value &color)
{
}

void FCDevice::writeFramebuffer()
{
	/*
	 * Asynchronously write the current framebuffer.
	 * Note that the OS will copy our framebuffer at submit-time.
	 */

	submitTransfer(new Transfer(this, &mFramebuffer, sizeof mFramebuffer));
}

void FCDevice::writeMessage(const OPCSink::Message &msg)
{
	if (mVerbose) {
		std::clog << "msg " << msg.length() << "\n";
	}

	writeFramebuffer();
}
