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

#pragma once
#include "spidevice.h"
#include "opc.h"
#include <set>


class APA102SPIDevice : public SPIDevice
{
public:
    APA102SPIDevice(uint32_t numLights, bool verbose);
    virtual ~APA102SPIDevice();

    virtual void loadConfiguration(const Value &config);
    virtual void writeMessage(const OPC::Message &msg);
	virtual void writeMessage(Document &msg);
    virtual std::string getName();
    virtual void flush();

	static const char* DEVICE_TYPE;

	virtual void describe(rapidjson::Value &object, Allocator &alloc);

private:
    static const uint32_t START_FRAME = 0x00000000;
	static const uint32_t END_FRAME = 0xFFFFFFFF;
    static const uint32_t BRIGHTNESS_MASK = 0xE0;

	union PixelFrame
	{
		struct
		{
			uint8_t l;
			uint8_t b;
			uint8_t g;
			uint8_t r;
		};

		uint32_t value;
	};

    const Value *mConfigMap;
	PixelFrame* mFrameBuffer;
	PixelFrame* mFlushBuffer;
	uint32_t mNumLights;

	// buffer accessor
	PixelFrame *fbPixel(unsigned num) {
		return &mFrameBuffer[num + 1];
	}

	void writeBuffer();
	void writeDevicePixels(Document &msg);

    void opcSetPixelColors(const OPC::Message &msg);
    void opcMapPixelColors(const OPC::Message &msg, const Value &inst);	
};
