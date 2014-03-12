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

#include "effect.h"


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
