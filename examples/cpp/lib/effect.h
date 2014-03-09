/*
 * LED Effect framework
 *
 * Copyright (c) 2014 Micah Elizabeth Scott <micah@scanlime.org>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once
#include <vector>
#include <sys/time.h>
#include "rapidjson/document.h"
#include "opcclient.h"


class Effect {
public:
    virtual void nextFrame(float timeDelta);

    // Calculate a pixel value, using floating point RGB in the range [0, 1].
    // Caller is responsible for clamping if necessary. This supports effects
    // that layer with other effects using greater than 8-bit precision.
    virtual void calculatePixel(rapidjson::Value &layout, float rgb[3]) = 0;
};


class EffectRunner {
public:
    EffectRunner();

    bool setServer(const char *hostport);
    bool setLayout(const char *filename);
    void setEffect(Effect* effect);
    void setMaxFrameRate(float fps);

    const rapidjson::Document& getLayout();
    Effect* getEffect();
    OPCClient& getClient();

    void doFrame();
    void doFrame(float timeDelta);

    void run();

private:
    float minTimeDelta;
    rapidjson::Document layout;
    OPCClient opc;
    Effect *effect;
    struct timeval lastTime;
    std::vector<uint8_t> frameBuffer;
};
