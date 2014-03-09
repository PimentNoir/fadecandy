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

#include <math.h>
#include <unistd.h>
#include <algorithm>
#include "effect.h"
#include "rapidjson/filestream.h"


void Effect::nextFrame(float timeDelta)
{
    // Default implementation; do nothing.
}

EffectRunner::EffectRunner()
    : minTimeDelta(0), effect(0)
{
    lastTime.tv_sec = 0;
    lastTime.tv_usec = 0;
}

void EffectRunner::setMaxFrameRate(float fps)
{
    minTimeDelta = 1.0 / fps;
}

bool EffectRunner::setServer(const char *hostport)
{
    return opc.resolve(hostport);
}

bool EffectRunner::setLayout(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
        return false;
    }

    rapidjson::FileStream istr(f);
    layout.ParseStream<0>(istr);
    fclose(f);

    if (layout.HasParseError()) {
        return false;
    }
    if (!layout.IsArray()) {
        return false;
    }

    // Set up an empty framebuffer, with OPC packet header
    int frameBytes = layout.Size() * 3;
    frameBuffer.resize(sizeof(OPCClient::Header) + frameBytes);
    OPCClient::Header::view(frameBuffer).init(0, opc.SET_PIXEL_COLORS, frameBytes);

    return true;
}

const rapidjson::Document& EffectRunner::getLayout()
{
    return layout;
}

void EffectRunner::setEffect(Effect *effect)
{
    this->effect = effect;
}

Effect* EffectRunner::getEffect()
{
    return effect;
}

void EffectRunner::run()
{
    while (true) {
        doFrame();
    }
}

void EffectRunner::doFrame()
{
    struct timeval now;

    gettimeofday(&now, 0);
    float delta = (now.tv_sec - lastTime.tv_sec)
        + 1e-6 * (now.tv_usec - lastTime.tv_usec);
    lastTime = now;

    // Max timestep; jump ahead if we get too far behind.
    const float maxStep = 0.1;
    if (delta > maxStep) {
        delta = maxStep;
    }

    doFrame(delta);
}

void EffectRunner::doFrame(float timeDelta)
{
    if (!effect || !layout.IsArray()) {
        return;
    }

    effect->nextFrame(timeDelta);

    uint8_t *dest = OPCClient::Header::view(frameBuffer).data();

    for (unsigned i = 0; i < layout.Size(); i++) {
        float rgb[3] = { 0, 0, 0 };

        rapidjson::Value &pixelLayout = layout[i];

        if (pixelLayout.IsObject()) {
            effect->calculatePixel(pixelLayout, rgb);
        }

        for (unsigned i = 0; i < 3; i++) {
            *(dest++) = std::min<int>(255, std::max<int>(0, rgb[i] * 255 + 0.5));
        }
    }

    opc.write(frameBuffer);

    // Extra delay, to adjust frame rate
    if (timeDelta < minTimeDelta) {
        usleep((minTimeDelta - timeDelta) * 1e6);
    }
}

OPCClient& EffectRunner::getClient()
{
    return opc;
}
