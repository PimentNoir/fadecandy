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

#include <math.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "opcclient.h"
#include "svl/SVL.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/filestream.h"
#include "rapidjson/document.h"

class EffectRunner;


// Abstract base class for one LED effect
class Effect {
public:
    class PixelInfo;
    class FrameInfo;
    class DebugInfo;

    /*
     * Calculate a pixel value, using floating point RGB in the nominal range [0, 1].
     *
     * The 'rgb' vector is initialized to (0, 0, 0). Caller is responsible for
     * clamping the results if necessary. This supports effects that layer with
     * other effects using full floating point precision and dynamic range.
     *
     * This function may be run in parallel, not run at all, or run multiple times
     * per pixel. It therefore can't have side-effects other than producing an RGB
     * value.
     */
    virtual void shader(Vec3& rgb, const PixelInfo& p) const = 0;

    /*
     * Serialized post-processing on one pixel. This runs after shader(), once
     * per mapped pixel, with the ability to modify Effect data. This shoudln't
     * be used for anything CPU-intensive, but some effects require closed-loop
     * feedback based on the calculated color.
     */
    virtual void postProcess(const Vec3& rgb, const PixelInfo& p);

    // Optional begin/end frame callbacks
    virtual void beginFrame(const FrameInfo& f);
    virtual void endFrame(const FrameInfo& f);

    // Optional callback, invoked once per second when verbose mode is enabled.
    // This can print parameters out to the console.
    virtual void debug(const DebugInfo& d);


    // Information about one LED pixel
    class PixelInfo {
    public:
        PixelInfo(unsigned index, const rapidjson::Value* layout);

        // Point coordinates
        Vec3 point;

        // Index in the framebuffer
        unsigned index;

        // Parsed JSON for this pixel's layout
        const rapidjson::Value* layout;

        // Is this pixel being used, or is it a placeholder?
        bool isMapped() const;

	// Look up data from the JSON layout
	const rapidjson::Value& get(const char *attribute);
	double getNumber(const char *attribute);
	double getArrayNumber(const char *attribute, int index);
    };

    typedef std::vector<PixelInfo> PixelInfoVec;
    typedef std::vector<PixelInfo>::const_iterator PixelInfoIter;

    // Information about one Effect frame
    class FrameInfo {
    public:
        FrameInfo();
        void init(const rapidjson::Value &layout);

        // Seconds passed since the last frame
        float timeDelta;

        // Info for every pixel
        PixelInfoVec pixels;
    };

    // Information passed to debug() callbacks
    class DebugInfo {
    public:
        DebugInfo(EffectRunner &runner);

        EffectRunner &runner;
    };
};


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


class EffectMixer : public Effect {
public:
    std::vector<Effect*> effects;
    std::vector<float> faders;

    virtual void shader(Vec3& rgb, const PixelInfo& p) const;
    virtual void beginFrame(const FrameInfo& f);
    virtual void endFrame(const FrameInfo& f);
    virtual void debug(const DebugInfo& d);
};


/*****************************************************************************************
 *                                   Implementation
 *****************************************************************************************/


inline Effect::PixelInfo::PixelInfo(unsigned index, const rapidjson::Value* layout)
    : index(index), layout(layout)
{
    if (isMapped()) {
        for (unsigned i = 0; i < 3; i++) {
            point[i] = getArrayNumber("point", i);
        }
    }
}

inline bool Effect::PixelInfo::isMapped() const
{
    return layout && layout->IsObject();
}

inline const rapidjson::Value& Effect::PixelInfo::get(const char *attribute)
{
    return (*layout)[attribute];
}

inline double Effect::PixelInfo::getNumber(const char *attribute)
{
    const rapidjson::Value& n = get(attribute);
    return n.IsNumber() ? n.GetDouble() : 0.0;
}

inline double Effect::PixelInfo::getArrayNumber(const char *attribute, int index)
{
    const rapidjson::Value& a = get(attribute);
    if (a.IsArray()) {
        const rapidjson::Value& b = a[index];
        if (b.IsNumber()) {
            return b.GetDouble();
        }
    }
    return 0.0;
}

inline Effect::FrameInfo::FrameInfo()
    : timeDelta(0) {}

inline void Effect::FrameInfo::init(const rapidjson::Value &layout)
{
    timeDelta = 0;
    pixels.clear();

    for (unsigned i = 0; i < layout.Size(); i++) {
        PixelInfo p(i, &layout[i]);
        pixels.push_back(p);
    }
}


inline Effect::DebugInfo::DebugInfo(EffectRunner &runner)
    : runner(runner) {}


inline void Effect::beginFrame(const FrameInfo &f) {}
inline void Effect::endFrame(const FrameInfo &f) {}
inline void Effect::debug(const DebugInfo &f) {}
inline void Effect::postProcess(const Vec3& rgb, const PixelInfo& p) {}


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
    const float debugInterval = 1.0f;
    if ((debugTimer += timeDelta) > debugInterval) {
        debugTimer = fmodf(debugTimer, debugInterval);
        debug();
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

inline void EffectMixer::shader(Vec3& rgb, const PixelInfo& p) const
{
    unsigned count = std::min<unsigned>(effects.size(), faders.size());
    Vec3 total(0,0,0);

    for (unsigned i = 0; i < count; i++) {
        float f = faders[i];
        if (f != 0) {
            Vec3 sub(0,0,0);
            effects[i]->shader(sub, p);
            effects[i]->postProcess(sub, p);
            total += f * sub;
        }
    }

    rgb = total;
}

inline void EffectMixer::beginFrame(const FrameInfo& f)
{
    for (unsigned i = 0; i < effects.size(); ++i) {
        effects[i]->beginFrame(f);
    }
}

inline void EffectMixer::endFrame(const FrameInfo& f)
{
    for (unsigned i = 0; i < effects.size(); ++i) {
        effects[i]->endFrame(f);
    }
}

inline void EffectMixer::debug(const DebugInfo& d)
{
    for (unsigned i = 0; i < effects.size(); ++i) {
        effects[i]->debug(d);
    }
}


static inline float sq(float a)
{
    // Fast square
    return a * a;
}
