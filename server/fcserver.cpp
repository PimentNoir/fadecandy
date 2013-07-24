/*
 * Open Pixel Control server for Fadecandy
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

#include "util.h"
#include "fcserver.h"
#include "fcdevice.h"
#include <netdb.h>
#include <ctype.h>
#include <iostream>


FCServer::FCServer(rapidjson::Document &config)
	: mListen(config["listen"]),
	  mColor(config["color"]),
	  mDevices(config["devices"]),
	  mVerbose(config["verbose"].IsTrue()),
	  mListenAddr(0),
	  mOPCSink(cbMessage, this, mVerbose),
	  mUSB(0)
{
	/*
	 * Parse the listen [host, port] list.
	 */

	if (mListen.IsArray() && mListen.Size() == 2) {
		const Value &host = mListen[0u];
		const Value &port = mListen[1];
		const char *hostStr = 0;
		std::ostringstream portStr;

		if (host.IsString()) {
			hostStr = host.GetString();
		} else if (!host.IsNull()) {
			mError << "Hostname in 'listen' must be null (any) or a hostname string.\n";
		}

		if (port.IsUint()) {
			portStr << port.GetUint();
		} else {
			mError << "The 'listen' port must be an integer.\n";
		}

		struct addrinfo hints;
		memset(&hints, 0, sizeof hints);
		hints.ai_family = PF_UNSPEC;
		hints.ai_flags = AI_PASSIVE;

		if (getaddrinfo(hostStr, portStr.str().c_str(), &hints, &mListenAddr) || !mListenAddr) {
			mError << "Failed to resolve hostname '" << host.GetString() << "'\n";
		}
	} else {
		mError << "The required 'listen' configuration key must be a [host, port] list.\n";
	}

	/*
	 * Minimal validation on 'devices'
	 */

	if (!mDevices.IsArray()) {
		mError << "The required 'devices' configuration key must be an array.\n";
	}
}

FCServer::~FCServer()
{
	if (mListenAddr) {
		freeaddrinfo(mListenAddr);
	}
}

void FCServer::start(struct ev_loop *loop)
{
	mOPCSink.start(loop, mListenAddr);
	startUSB(loop);
}

void FCServer::startUSB(struct ev_loop *loop)
{	
	if (libusb_init(&mUSB)) {
		std::clog << "Error initializing USB library!\n";
		return;
	}

	// Attach to our libev event loop
	mUSBEvent.init(mUSB, loop);

	// Enumerate all attached devices, and get notified of hotplug events
	libusb_hotplug_register_callback(mUSB,
		libusb_hotplug_event(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
				             LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
		LIBUSB_HOTPLUG_ENUMERATE,
		LIBUSB_HOTPLUG_MATCH_ANY,
		LIBUSB_HOTPLUG_MATCH_ANY,
		LIBUSB_HOTPLUG_MATCH_ANY,
		cbHotplug, this, 0);
}

void FCServer::cbMessage(OPCSink::Message &msg, void *context)
{
	/*
	 * Broadcast the OPC message to all configured devices.
	 */

	FCServer *self = static_cast<FCServer*>(context);

	for (std::vector<FCDevice*>::iterator i = self->mFCDevices.begin(), e = self->mFCDevices.end(); i != e; ++i) {
		FCDevice *fcd = *i;
		fcd->writeMessage(msg);
	}
}

int FCServer::cbHotplug(libusb_context *ctx, libusb_device *device, libusb_hotplug_event event, void *user_data)
{
	FCServer *self = static_cast<FCServer*>(user_data);

	if (event & LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
		self->usbDeviceArrived(device);
	}
	if (event & LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
		self->usbDeviceLeft(device);
	}

	return false;
}

void FCServer::usbDeviceArrived(libusb_device *device)
{
	/*
	 * New USB device. Is this a device we recognize?
     *
	 * Right now we're only looking for FCDevices, but in the future
	 * we can look for other types of USB devices here too.
	 */

	FCDevice *fcd = new FCDevice(device, mVerbose);
	if (!fcd->isFadecandy()) {
		// Not a recognized device.
		delete fcd;
		return;
	}

	int r = fcd->open();
	if (r < 0) {
		if (mVerbose) {
			std::clog << "Error opening Fadecandy USB device: " << libusb_strerror(libusb_error(r)) << "\n";
		}
		delete fcd;
		return;
	}

	// Look for a matching device in the JSON
	const Value *fcjson = matchFCDevice(fcd->getSerial());

	if (mVerbose) {
		std::clog << "USB Fadecandy attached, serial: \"" << fcd->getSerial() << "\"";
		if (fcjson) {
			std::clog << " (configuration found)\n";
		} else {
			std::clog << " (not matched in config file)\n";
		}
	}

	if (fcjson) {
		// Store the configuration, use it for future messages
		fcd->setConfiguration(*fcjson);
	} else {
		delete fcd;
		return;
	}

	// Send the default color lookup table
	fcd->writeColorCorrection(mColor);

	// Remember this device for later. It's now active, and we should broadcast messages to it.
	mFCDevices.push_back(fcd);
}

void FCServer::usbDeviceLeft(libusb_device *device)
{
	/*
	 * Is this a device we recognize? If so, delete it.
	 */

	for (std::vector<FCDevice*>::iterator i = mFCDevices.begin(), e = mFCDevices.end(); i != e; ++i) {
		FCDevice *fcd = *i;
		if (fcd->getDevice() == device) {
			if (mVerbose) {
				std::clog << "USB Fadecandy removed, serial: \"" << fcd->getSerial() << "\"\n";
			}
			mFCDevices.erase(i);
			delete fcd;
			break;
		}
	}
}

const FCServer::Value *FCServer::matchFCDevice(const char *serial)
{
	/*
	 * Look for a record in mDevices that matches a Fadecandy board with the given serial number.
	 * Returns 0 if nothing matches.
	 */

	for (unsigned i = 0; i < mDevices.Size(); ++i) {
		const Value &v = mDevices[i];
		const Value &vtype = v["type"];
		const Value &vserial = v["serial"];

		if (!vtype.IsString() || strcmp(vtype.GetString(), "fadecandy")) {
			// Wrong type
			continue;
		}

		if (!vserial.IsNull()) {
			// Not a wildcard serial number?
			// If a serial was not specified, it matches any device.

			if (!vserial.IsString()) {
				// Non-string serial number. Bad form.
				continue;
			}

			if (strcmp(vserial.GetString(), serial)) {
				// Not a match
				continue;
			}
		}

		// Match
		return &v;
	}

	return 0;
}
