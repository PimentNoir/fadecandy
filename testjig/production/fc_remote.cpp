/*
 * Remote control for the Fadecandy firmware, using the ARM debug interface.
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

#include <Arduino.h>
#include "fc_remote.h"
#include "testjig.h"
#include "firmware_data.h"


bool FcRemote::installFirmware()
{
    // Install firmware, blinking both target and local LEDs in unison.

    bool blink = false;
    ARMKinetisDebug::FlashProgrammer programmer(target, fw_data, fw_sectorCount);

    if (!programmer.begin())
        return false;

    while (!programmer.isComplete()) {
        blink = !blink;
        if (!programmer.next()) return false;
        if (!setLED(blink)) return false;
        digitalWrite(ledPin, blink);
    }

    return true;
}

bool FcRemote::boot()
{
    // Run the new firmware, and let it boot
    if (!target.reset())
        return false;
    delay(50);
    return true;
}

bool FcRemote::setLED(bool on)
{
    const unsigned pin = target.PTC5;
    return
        target.pinMode(pin, OUTPUT) &&
        target.digitalWrite(pin, on);
}

bool FcRemote::setFlags(uint8_t cflag)
{
    // Set control flags
    return target.memStoreByte(fw_pFlags, cflag);
}

bool FcRemote::initLUT()
{
    // Install a trivial identity-mapping LUT, writing directly to the firmware's LUT buffer.
    for (unsigned channel = 0; channel < 3; channel++) {
        for (unsigned index = 0; index < LUT_CH_SIZE; index++) {
            if (!setLUT(channel, index, index << 8))
                return false;
        }
    }
    return true;
}

bool FcRemote::setLUT(unsigned channel, unsigned index, int value)
{
    return target.memStoreHalf(fw_pLUT + 2*(index + channel*LUT_CH_SIZE), constrain(value, 0, 0xFFFF));
}

bool FcRemote::setPixel(unsigned index, int red, int green, int blue)
{
    // Write one pixel directly into the fbNext framebuffer on the target.

    uint32_t idPacket = index / PIXELS_PER_PACKET;
    uint32_t idOffset = fw_usbPacketBufOffset + 1 + (index % PIXELS_PER_PACKET) * 3;

    uint32_t fb;        // Address of the current fcFramebuffer bound to fbNext
    uint32_t packet;    // Pointer to usb_packet in question

    return
        target.memLoad(fw_pFbNext, fb) &&
        target.memLoad(fb + idPacket*4, packet) &&
        target.memStoreByte(packet + idOffset + 0, constrain(red, 0, 255)) &&
        target.memStoreByte(packet + idOffset + 1, constrain(green, 0, 255)) &&
        target.memStoreByte(packet + idOffset + 2, constrain(blue, 0, 255));
}

float FcRemote::measureFrameRate(float minDuration)
{
    // Use the end-to-end LED data signal to measure the overall system frame rate.
    // Gaps of >50us indicate frame boundaries

    pinMode(dataFeedbackPin, INPUT);

    uint32_t minMicros = minDuration * 1000000;
    uint32_t startTime = micros();
    uint32_t gapStart = 0;
    bool inGap = false;
    uint32_t frames = 0;
    uint32_t duration;
    bool anyData = false;

    while (1) {
        uint32_t now = micros();
        duration = now - startTime;
        if (duration >= minMicros)
            break;

        if (digitalRead(dataFeedbackPin)) {
            // Definitely not in a gap, found some data
            // set anyData true indicating we saw data at least once
            // anyData ensures we don't count frames if dataFeedbackPin
            // is stuck low
            inGap = false;
            gapStart = now;
            anyData = true;

        } else if (inGap) {
            // Already in a gap, wait for some data.

        } else if (anyData && (uint32_t(now - gapStart) >= 50)) {
            // We've seen data, and
            // We just found an inter-frame gap
            inGap = true;
            frames++;
        }
    }

    return frames / (duration * 1e-6);
}

bool FcRemote::testFrameRate()
{
    const float goalFPS = 375;
    const float maxFPS = 450;

    target.log(target.LOG_NORMAL, "FPS: Measuring frame rate...");
    float fps = measureFrameRate();
    target.log(target.LOG_NORMAL, "FPS: Measured %.2f frames/sec", fps);

    if (fps > maxFPS) {
        target.log(target.LOG_ERROR, "FPS: ERROR, frame rate of %.2f frames/sec is unrealistically high!", fps);
        return false;
    }

    if (fps < goalFPS) {
        target.log(target.LOG_ERROR, "FPS: ERROR, frame rate of %.2f frames/sec is below goal of %.2f!",
            fps, goalFPS);
        return false;
    }

    return true;
}
