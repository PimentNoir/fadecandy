/*
 * LED Effect Framework
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

#include <math.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "effect.h"
#include "opc_client.h"
#include "svl/SVL.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/filestream.h"
#include "rapidjson/document.h"


class EffectRunner {
public:
    EffectRunner();

    bool setServer(const char *hostport);
    bool setLayout(const char *filename);
    void setEffect(Effect* effect);
    void setMaxFrameRate(float fps);
    void setVerbose(bool verbose = true);

    bool hasLayout();
    const rapidjson::Document& getLayout();
    Effect* getEffect();
    OPCClient& getClient();

    // Time stats
    float getFrameRate();
    float getTimePerFrame();
    float getBusyTimePerFrame();
    float getIdleTimePerFrame();
    float getPercentBusy();

    // Main loop body
    float doFrame();
    void doFrame(float timeDelta);

    // Minimal main loop
    void run();

    // Simple argument parsing and optional main loop
    bool parseArguments(int argc, char **argv);
    int main(int argc, char **argv);

protected:
    // Extensibility for argument parsing
    virtual bool parseArgument(int &i, int &argc, char **argv);
    virtual bool validateArguments();
    virtual void argumentUsage();

private:
    OPCClient opc;
    rapidjson::Document layout;
    Effect *effect;
    std::vector<uint8_t> frameBuffer;
    Effect::FrameInfo frameInfo;

    float minTimeDelta;
    float currentDelay;
    float filteredTimeDelta;
    float debugTimer;
    bool verbose;
    struct timeval lastTime;

    void usage(const char *name);
    void debug();
};


/*****************************************************************************************
 *                                   Implementation
 *****************************************************************************************/


inline EffectRunner::EffectRunner()
    : effect(0),
      minTimeDelta(0),
      currentDelay(0),
      filteredTimeDelta(0),
      debugTimer(0),
      verbose(false)
{
    lastTime.tv_sec = 0;
    lastTime.tv_usec = 0;

    // Defaults
    setMaxFrameRate(300);
    setServer("localhost");
}

inline void EffectRunner::setMaxFrameRate(float fps)
{
    minTimeDelta = 1.0 / fps;
}

inline void EffectRunner::setVerbose(bool verbose)
{
    this->verbose = verbose;
}

inline bool EffectRunner::setServer(const char *hostport)
{
    return opc.resolve(hostport);
}

inline bool EffectRunner::setLayout(const char *filename)
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

    // Init pixel info
    frameInfo.init(layout);

    return true;
}

inline const rapidjson::Document& EffectRunner::getLayout()
{
    return layout;
}

inline bool EffectRunner::hasLayout()
{
    return layout.IsArray();
}

inline void EffectRunner::setEffect(Effect *effect)
{
    this->effect = effect;
}

inline Effect* EffectRunner::getEffect()
{
    return effect;
}

inline float EffectRunner::getFrameRate()
{
    return filteredTimeDelta > 0.0f ? 1.0f / filteredTimeDelta : 0.0f;
}

inline float EffectRunner::getTimePerFrame()
{
    return filteredTimeDelta;
}

inline float EffectRunner::getBusyTimePerFrame()
{
    return getTimePerFrame() - getIdleTimePerFrame();
}

inline float EffectRunner::getIdleTimePerFrame()
{
    return std::max(0.0f, currentDelay);
}

inline float EffectRunner::getPercentBusy()
{
    return 100.0f * getBusyTimePerFrame() / getTimePerFrame();
}

inline void EffectRunner::run()
{
    while (true) {
        doFrame();
    }
}
   
inline float EffectRunner::doFrame()
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
    return delta;
}

inline void EffectRunner::doFrame(float timeDelta)
{
    frameInfo.timeDelta = timeDelta;

    if (getEffect() && hasLayout()) {
        effect->beginFrame(frameInfo);

        // Only calculate the effect if we have a connection
        if (opc.tryConnect()) {

            uint8_t *dest = OPCClient::Header::view(frameBuffer).data();

            for (Effect::PixelInfoIter i = frameInfo.pixels.begin(), e = frameInfo.pixels.end(); i != e; ++i) {
                Vec3 rgb(0, 0, 0);
                const Effect::PixelInfo &p = *i;

                if (p.isMapped()) {
                    effect->shader(rgb, p);
                    effect->postProcess(rgb, p);
                }

                for (unsigned i = 0; i < 3; i++) {
                    *(dest++) = std::min<int>(255, std::max<int>(0, rgb[i] * 255 + 0.5));
                }
            }

            opc.write(frameBuffer);
        }

        effect->endFrame(frameInfo);
    }

    // Low-pass filter for timeDelta, to estimate our frame rate
    const float filterGain = 0.05;
    filteredTimeDelta += (timeDelta - filteredTimeDelta) * filterGain;

    // Negative feedback loop to adjust the delay until we hit a target frame rate.
    // This lets us hit the target rate smoothly, without a lot of jitter between frames.
    // If we calculated a new delay value on each frame, we'd easily end up alternating
    // between too-long and too-short frame delays.
    currentDelay += (minTimeDelta - timeDelta) * filterGain;

    // Make sure filteredTimeDelta >= currentDelay. (The "busy time" estimate will be >= 0)
    filteredTimeDelta = std::max(filteredTimeDelta, currentDelay);

    // Periodically output debug info, if we're in verbose mode
    if (verbose) {
        const float debugInterval = 1.0f;
        if ((debugTimer += timeDelta) > debugInterval) {
            debugTimer = fmodf(debugTimer, debugInterval);
            debug();
        }
    }

    // Add the extra delay, if we have one. This is how we throttle down the frame rate.
    if (currentDelay > 0) {
        usleep(currentDelay * 1e6);
    }
}

inline OPCClient& EffectRunner::getClient()
{
    return opc;
}

inline bool EffectRunner::parseArguments(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (!parseArgument(i, argc, argv)) {
            usage(argv[0]);
            return false;
        }
    }

    if (!validateArguments()) {
        usage(argv[0]);
        return false;
    }

    return true;
}

inline int EffectRunner::main(int argc, char **argv)
{
    if (!parseArguments(argc, argv)) {
        return 1;
    }

    run();
    return 0;
}

inline void EffectRunner::usage(const char *name)
{
    fprintf(stderr, "usage: %s ", name);
    argumentUsage();
    fprintf(stderr, "\n");
}

inline void EffectRunner::debug()
{
    fprintf(stderr, " %7.2f FPS -- %6.2f%% CPU [%.2fms busy, %.2fms idle]\n",
        getFrameRate(),
        getPercentBusy(),
        1e3f * getBusyTimePerFrame(),
        1e3f * getIdleTimePerFrame());

    if (effect) {
        Effect::DebugInfo d(*this);
        effect->debug(d);
    }
}

inline bool EffectRunner::parseArgument(int &i, int &argc, char **argv)
{
    if (!strcmp(argv[i], "-v")) {
        setVerbose();
        return true;
    }

    if (!strcmp(argv[i], "-fps") && (i+1 < argc)) {
        float rate = atof(argv[++i]);
        if (rate <= 0) {
            fprintf(stderr, "Invalid frame rate\n");
            return false;
        }
        setMaxFrameRate(rate);
        return true;
    }

    if (!strcmp(argv[i], "-layout") && (i+1 < argc)) {
        if (!setLayout(argv[++i])) {
            fprintf(stderr, "Can't load layout from %s\n", argv[i]);
            return false;
        }
        return true;
    }

    if (!strcmp(argv[i], "-server") && (i+1 < argc)) {
        if (!setServer(argv[++i])) {
            fprintf(stderr, "Can't resolve server name %s\n", argv[i]);
            return false;
        }
        return true;
    }

    return false;
}

inline bool EffectRunner::validateArguments()
{
    if (!hasLayout()) {
        fprintf(stderr, "No layout specified\n");
        return false;
    }

    return true;
}

inline void EffectRunner::argumentUsage()
{
    fprintf(stderr, "[-v] [-fps LIMIT] [-layout FILE.json] [-server HOST[:port]]");
}
