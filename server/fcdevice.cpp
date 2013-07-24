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
#include <math.h>
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

	// Framebuffer headers
	memset(mFramebuffer, 0, sizeof mFramebuffer);
	for (unsigned i = 0; i < FRAMEBUFFER_PACKETS; ++i) {
		mFramebuffer[i].control = TYPE_FRAMEBUFFER | i;
	}
	mFramebuffer[FRAMEBUFFER_PACKETS - 1].control |= FINAL;

	// Color LUT headers
	memset(mColorLUT, 0, sizeof mColorLUT);
	for (unsigned i = 0; i < LUT_PACKETS; ++i) {
		mColorLUT[i].control = TYPE_LUT | i;
	}
	mColorLUT[LUT_PACKETS - 1].control |= FINAL;
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
	/*
	 * Populate the color correction table based on a JSON configuration object,
	 * and send the new color LUT out over USB.
	 *
	 * 'color' may be 'null' to load an identity-mapped LUT, or it may be
	 * a dictionary of options including 'gamma' and 'whitepoint'.
	 */

	// Default color LUT parameters
	double gamma = 1.0;
	double whitepoint[3] = {1.0, 1.0, 1.0};

	/*
	 * Parse the JSON object
	 */

	if (color.IsObject()) {
		const Value &vGamma = color["gamma"];
		const Value &vWhitepoint = color["whitepoint"];

		if (vGamma.IsNumber()) {
			gamma = vGamma.GetDouble();
		} else if (!vGamma.IsNull() && mVerbose) {
			std::clog << "Gamma value must be a number.\n";
		}

		if (vWhitepoint.IsArray() &&
			vWhitepoint.Size() == 3 &&
			vWhitepoint[0u].IsNumber() &&
			vWhitepoint[1].IsNumber() &&
			vWhitepoint[2].IsNumber()) {
			whitepoint[0] = vWhitepoint[0u].GetDouble();
			whitepoint[1] = vWhitepoint[1].GetDouble();
			whitepoint[2] = vWhitepoint[2].GetDouble();
		} else if (!vWhitepoint.IsNull() && mVerbose) {
			std::clog << "Whitepoint value must be a list of 3 numbers.\n";
		}

	} else if (!color.IsNull() && mVerbose) {
		std::clog << "Color correction value must be a JSON dictionary object.\n";
	}

	/*
	 * Calculate the color LUT, stowing the result in an array of USB packets.
	 */

	Packet *packet = mColorLUT;
	const unsigned firstByteOffset = 1;	 // Skip padding byte
	unsigned byteOffset = firstByteOffset;

	for (unsigned channel = 0; channel < 3; channel++) {
		for (unsigned entry = 0; entry < LUT_ENTRIES; entry++) {

			/*
			 * Normalized input value corresponding to this LUT entry.
			 * Ranges from 0 to slightly higher than 1. (The last LUT entry
			 * can't quite be reached.)
			 */
			double input = (entry << 8) / 65535.0;

			// Color conversion
			double output = pow(input * whitepoint[channel], gamma);

			// Round to the nearest integer, and clamp. Overflow-safe.
			int64_t longValue = (output * 0xFFFF) + 0.5;
			int intValue = std::max<int64_t>(0, std::min<int64_t>(0xFFFF, longValue));

			// Store LUT entry, little-endian order.
			packet->data[byteOffset++] = uint8_t(intValue);
			packet->data[byteOffset++] = uint8_t(intValue >> 8);
			if (byteOffset >= sizeof packet->data) {
				byteOffset = firstByteOffset;
				packet++;
			}
		}
	}

	// Start asynchronously sending the LUT.
	submitTransfer(new Transfer(this, &mColorLUT, sizeof mColorLUT));
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
	/*
	 * Dispatch an incoming OPC command
	 */

	switch (msg.command) {

		case OPCSink::SetPixelColors:
			opcSetPixelColors(msg);
			writeFramebuffer();
			return;

		case OPCSink::SetGlobalColorCorrection:
			opcSetGlobalColorCorrection(msg);
			return;
	}

	if (mVerbose) {
		std::clog << "Unsupported OPC command: " << msg.command << "\n";
	}
}

void FCDevice::opcSetPixelColors(const OPCSink::Message &msg)
{
	/*
	 * Parse through our device's mapping, and store any relevant portions of 'msg'
	 * in the framebuffer.
	 */
}

void FCDevice::opcSetGlobalColorCorrection(const OPCSink::Message &msg)
{
	/*
	 * Parse the message as JSON text, and if successful, write new
	 * color correction data to the device.
	 */

	// Mutable NUL-terminated copy of the message string
	std::string text((char*)msg.data, msg.length());

	// Parse it in-place
	rapidjson::Document doc;
	doc.ParseInsitu<0>(&text[0]);

	if (doc.HasParseError()) {
		if (mVerbose) {
			std::clog << "Parse error in color correction JSON at character "
				<< doc.GetErrorOffset() << ": " << doc.GetParseError() << "\n";
		}
		return;
    }

    /*
     * Successfully parsed the JSON. From here, it's handled identically to
     * objects that come through the config file.
     */
    writeColorCorrection(doc);
}

