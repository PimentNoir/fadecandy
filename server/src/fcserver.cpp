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

#include "fcserver.h"
#include "usbdevice.h"
#include "fcdevice.h"
#include "enttecdmxdevice.h"
#include <libusbi.h>
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

        if (host.IsString()) {
            hostStr = host.GetString();
        } else if (!host.IsNull()) {
            mError << "Hostname in 'listen' must be null (any) or a hostname string.\n";
        }

        if (port.IsUint()) {
            mListenAddr = OPCSink::newAddr(hostStr, port.GetUint());
            if (!mListenAddr) {
                mError << "Failed to resolve hostname '" << hostStr << "'\n";
            }
        } else {
            mError << "The 'listen' port must be an integer.\n";
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
        OPCSink::freeAddr(mListenAddr);
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

    for (std::vector<USBDevice*>::iterator i = self->mUSBDevices.begin(), e = self->mUSBDevices.end(); i != e; ++i) {
        USBDevice *dev = *i;
        dev->writeMessage(msg);
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
     */

    USBDevice *dev;

    if (FCDevice::probe(device)) {
        dev = new FCDevice(device, mVerbose);

    } else if (EnttecDMXDevice::probe(device)) {
        dev = new EnttecDMXDevice(device, mVerbose);

    } else {
        return;
    }

    int r = dev->open();
    if (r < 0) {
        if (mVerbose) {
            std::clog << "Error opening " << dev->getName() << ": " << libusb_strerror(libusb_error(r)) << "\n";
        }
        delete dev;
        return;
    }

    if (!dev->probeAfterOpening()) {
        // We were mistaken, this device isn't actually one we want.
        delete dev;
        return;
    }

    for (unsigned i = 0; i < mDevices.Size(); ++i) {
        if (dev->matchConfiguration(mDevices[i])) {
            // Found a matching configuration for this device. We're keeping it!

            dev->writeColorCorrection(mColor);
            mUSBDevices.push_back(dev);

            if (mVerbose) {
                std::clog << "USB device " << dev->getName() << " attached.\n";
            }
            return;
        }
    }

    if (mVerbose) {
        std::clog << "USB device " << dev->getName() << " has no matching configuration. Not using it.\n";
    }
    delete dev;
}

void FCServer::usbDeviceLeft(libusb_device *device)
{
    /*
     * Is this a device we recognize? If so, delete it.
     */

    for (std::vector<USBDevice*>::iterator i = mUSBDevices.begin(), e = mUSBDevices.end(); i != e; ++i) {
        USBDevice *dev = *i;
        if (dev->getDevice() == device) {
            if (mVerbose) {
                std::clog << "USB device " << dev->getName() << " removed.\n";
            }
            mUSBDevices.erase(i);
            delete dev;
            break;
        }
    }
}
