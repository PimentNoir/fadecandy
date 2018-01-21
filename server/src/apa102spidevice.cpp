/*
 * Fadecandy driver for the APA102/APA102C/SK9822 via SPI.
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

#include "apa102spidevice.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "opc.h"
#include <sstream>
#include <iostream>

const char* APA102SPIDevice::DEVICE_TYPE = "apa102spi";

APA102SPIDevice::APA102SPIDevice(uint32_t numLights, bool verbose)
    : SPIDevice(DEVICE_TYPE, verbose),
      mConfigMap(0),
	  mNumLights(numLights)
{
	uint32_t bufferSize = sizeof(PixelFrame) * (numLights + 2); // Number of lights plus start and end frames
	mFrameBuffer = (PixelFrame*)malloc(bufferSize);

	uint32_t flushCount = (numLights / 2) + (numLights % 2);
	mFlushBuffer = (PixelFrame*)malloc(flushCount);

	// Initialize all buffers to zero
	memset(mFlushBuffer, 0, flushCount);
    memset(mFrameBuffer, 0, bufferSize);

	// Initialize start and end frames
	mFrameBuffer[0].value = START_FRAME;
	mFrameBuffer[numLights + 1].value = END_FRAME;
}

APA102SPIDevice::~APA102SPIDevice()
{
	free(mFrameBuffer);
	free(mFlushBuffer);

	flush();
}

void APA102SPIDevice::loadConfiguration(const Value &config)
{
    mConfigMap = findConfigMap(config);
}

std::string APA102SPIDevice::getName()
{
    std::ostringstream s;
    s << "APA102/APA102C/SK9822 via SPI Port " << mPort;
    return s.str();
}

void APA102SPIDevice::flush()
{
	/*
	* Flush the buffer by writing zeros through every LED
	* 
	* This is nessecary in the event that we are not following up a writeBuffer()
	* with another writeBuffer immediately.
	*
	*/
	uint32_t flushCount = (mNumLights / 2) + (mNumLights % 2);
	SPIDevice::write(mFlushBuffer, flushCount);
}

void APA102SPIDevice::writeBuffer()
{
	SPIDevice::write(mFrameBuffer, sizeof(PixelFrame) * (mNumLights + 2));
}

void APA102SPIDevice::writeMessage(Document &msg)
{
	/*
	* Dispatch a device-specific JSON command.
	*
	* This can be used to send frames or settings directly to one device,
	* bypassing the mapping we use for Open Pixel Control clients. This isn't
	* intended to be the fast path for regular applications, but it can be used
	* by configuration tools that need to operate regardless of the mapping setup.
	*/

	const char *type = msg["type"].GetString();

	if (!strcmp(type, "device_pixels")) {
		// Write raw pixels, without any mapping
		writeDevicePixels(msg);
		flush();
		return;
	}

	// Chain to default handler
	SPIDevice::writeMessage(msg);
}

void APA102SPIDevice::writeDevicePixels(Document &msg)
{
	/*
	* Write pixels without mapping, from a JSON integer
	* array in msg["pixels"]. The pixel array is removed from
	* the reply to save network bandwidth.
	*
	* Pixel values are clamped to [0, 255], for convenience.
	*/

	const Value &pixels = msg["pixels"];
	if (!pixels.IsArray()) {
		msg.AddMember("error", "Pixel array is missing", msg.GetAllocator());
	}
	else {

		// Truncate to the framebuffer size, and only deal in whole pixels.
		uint32_t numPixels = pixels.Size() / 3;
		if (numPixels > mNumLights)
			numPixels = mNumLights;

		for (uint32_t i = 0; i < numPixels; i++) {
			PixelFrame *out = fbPixel(i);

			const Value &r = pixels[i * 3 + 0];
			const Value &g = pixels[i * 3 + 1];
			const Value &b = pixels[i * 3 + 2];

			out->r = std::max(0, std::min(255, r.IsInt() ? r.GetInt() : 0));
			out->g = std::max(0, std::min(255, g.IsInt() ? g.GetInt() : 0));
			out->b = std::max(0, std::min(255, b.IsInt() ? b.GetInt() : 0));
			out->l = 0xEF; // todo: fix so we actually pass brightness
		}

		writeBuffer();
	}
}

void APA102SPIDevice::writeMessage(const OPC::Message &msg)
{
    /*
     * Dispatch an incoming OPC command
     */

    switch (msg.command) {

        case OPC::SetPixelColors:
            opcSetPixelColors(msg);
			writeBuffer();
            return;

        case OPC::SystemExclusive:
            // No relevant SysEx for this device
            return;
    }

    if (mVerbose) {
        std::clog << "Unsupported OPC command: " << unsigned(msg.command) << "\n";
    }
}

void APA102SPIDevice::opcSetPixelColors(const OPC::Message &msg)
{
    /*
     * Parse through our device's mapping, and store any relevant portions of 'msg'
     * in the framebuffer.
     */

    if (!mConfigMap) {
        // No mapping defined yet. This device is inactive.
        return;
    }

    const Value &map = *mConfigMap;
    for (unsigned i = 0, e = map.Size(); i != e; i++) {
        opcMapPixelColors(msg, map[i]);
    }
}

void APA102SPIDevice::opcMapPixelColors(const OPC::Message &msg, const Value &inst)
{
	/*
	* Parse one JSON mapping instruction, and copy any relevant parts of 'msg'
	* into our framebuffer. This looks for any mapping instructions that we
	* recognize:
	*
	*   [ OPC Channel, First OPC Pixel, First output pixel, Pixel count ]
	*/

	unsigned msgPixelCount = msg.length() / 3;

	if (inst.IsArray() && inst.Size() == 4) {
		// Map a range from an OPC channel to our framebuffer

		const Value &vChannel = inst[0u];
		const Value &vFirstOPC = inst[1];
		const Value &vFirstOut = inst[2];
		const Value &vCount = inst[3];

		if (vChannel.IsUint() && vFirstOPC.IsUint() && vFirstOut.IsUint() && vCount.IsInt()) {
			unsigned channel = vChannel.GetUint();
			unsigned firstOPC = vFirstOPC.GetUint();
			unsigned firstOut = vFirstOut.GetUint();
			unsigned count;
			int direction;
			if (vCount.GetInt() >= 0) {
				count = vCount.GetInt();
				direction = 1;
			}
			else {
				count = -vCount.GetInt();
				direction = -1;
			}

			if (channel != msg.channel) {
				return;
			}

			// Clamping, overflow-safe
			firstOPC = std::min<unsigned>(firstOPC, msgPixelCount);
			firstOut = std::min<unsigned>(firstOut, mNumLights);
			count = std::min<unsigned>(count, msgPixelCount - firstOPC);
			count = std::min<unsigned>(count,
				direction > 0 ? mNumLights - firstOut : firstOut + 1);

			// Copy pixels
			const uint8_t *inPtr = msg.data + (firstOPC * 3);
			unsigned outIndex = firstOut;
			while (count--) {
				PixelFrame *outPtr = fbPixel(outIndex);
				outIndex += direction;
				outPtr->r = inPtr[0];
				outPtr->g = inPtr[1];
				outPtr->b = inPtr[2];
				outPtr->l = 0xEF; // todo: fix so we actually pass brightness
				inPtr += 3;
			}

			return;
		}
	}

    // Still haven't found a match?
    if (mVerbose) {
        rapidjson::GenericStringBuffer<rapidjson::UTF8<> > buffer;
        rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF8<> > > writer(buffer);
        inst.Accept(writer);
        std::clog << "Unsupported JSON mapping instruction: " << buffer.GetString() << "\n";
    }
}

void APA102SPIDevice::describe(rapidjson::Value &object, Allocator &alloc)
{
	SPIDevice::describe(object, alloc);
	object.AddMember("numLights", mNumLights, alloc);
}
