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


class FCDevice {
public:
	typedef rapidjson::Value Value;

    FCDevice(libusb_device *device, bool verbose = false);
    ~FCDevice();

    libusb_device *getDevice() { return mDevice; };
    bool isFadecandy();
    int open();

    // Valid after open():

    const char *getSerial() { return mSerial; }
    void setConfiguration(const Value *config);
    void writeColorCorrection(const Value &color);
    void writeMessage(const OPCSink::Message &msg);

private:
	bool mVerbose;
    libusb_device *mDevice;
    libusb_device_handle *mHandle;
    const Value *mConfig;
    char mSerial[256];
};
