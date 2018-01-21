/*
* Abstract base class for SPI-attached devices.
*
* Copyright (c) 2013 Micah Elizabeth Scott
* Copyright (c) 2017 Lance Gilbert <lance@lancegilbert.us>
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

#include "spidevice.h"
#include <iostream>

#ifdef FCSERVER_HAS_WIRINGPI
#include <wiringPi.h>
#include <wiringPiSPI.h>
#endif

#ifndef SPI_FREQUENCY_MHZ
#define SPI_FREQUENCY_MHZ 20
#endif

#define SPI_FREQUENCY (SPI_FREQUENCY_MHZ*1000000)

SPIDevice::SPIDevice(const char *type, bool verbose)
	: mTypeString(type),
	  mVerbose(verbose),
	  mPort(0)
{
	gettimeofday(&mTimestamp, NULL);
}

SPIDevice::~SPIDevice()
{

}

int SPIDevice::open(uint32_t port)
{
	mPort = port;

#ifdef FCSERVER_HAS_WIRINGPI
	return wiringPiSPISetup(mPort, SPI_FREQUENCY);
#else
	return -1;
#endif
}

void SPIDevice::write(void* buffer, int length)
{
#ifdef FCSERVER_HAS_WIRINGPI
	wiringPiSPIDataRW(mPort, (unsigned char*)buffer, length);
#endif
}

void SPIDevice::writeColorCorrection(const Value &color)
{
	// Optional. By default, ignore color correction messages.
}

bool SPIDevice::matchConfiguration(const Value &config)
{
	if (!config.IsObject()) {
		return false;
	}

	const Value &vtype = config["type"];
	const Value &vport = config["port"];

	if (!vtype.IsNull() && (!vtype.IsString() || strcmp(vtype.GetString(), mTypeString))) {
		return false;
	}

	if (!vport.IsNull() && (!vport.IsUint() || vport.GetUint() != mPort)) {
		return false;
	}

	return true;
}

const SPIDevice::Value *SPIDevice::findConfigMap(const Value &config)
{
	const Value &vmap = config["map"];

	if (vmap.IsArray()) {
		// The map is optional, but if it exists it needs to be an array.
		return &vmap;
	}

	if (!vmap.IsNull() && mVerbose) {
		std::clog << "Device configuration 'map' must be an array.\n";
	}

	return 0;
}

void SPIDevice::writeMessage(Document &msg)
{
	const char *type = msg["type"].GetString();

	if (!strcmp(type, "device_color_correction")) {
		// Single-device color correction
		writeColorCorrection(msg["color"]);
		return;
	}

	msg.AddMember("error", "Unknown device-specific message type", msg.GetAllocator());
}

void SPIDevice::describe(rapidjson::Value &object, Allocator &alloc)
{
	object.AddMember("type", mTypeString, alloc);

	object.AddMember("port", mPort, alloc);

	/*
	* The connection timestamp lets a particular connection instance be identified
	* reliably, even if the same device connects and disconnects.
	*
	* We encode the timestamp as 64-bit millisecond count, so we don't have to worry about
	* the portability of string/float conversions. This also matches a common JS format.
	*/

	uint64_t timestamp = (uint64_t)mTimestamp.tv_sec * 1000 + mTimestamp.tv_usec / 1000;
	object.AddMember("timestamp", timestamp, alloc);
}
