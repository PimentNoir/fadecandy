/*
 * Effect that controls the total brightness of another effect
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


class Brightness : public Effect {
public:
    Brightness(Effect &next);

    void set(float averageBrightness);
    void set(float lowerLimit, float upperLimit);

    // Set the gamma value we assume when performing total brightness calculations.
    // Doesn't affect the actual output gamma! We need to sum the brightness in a
    // physically linear space, but we perform the scaling back in our perceptually
    // linear-ish space.
    void setAssumedGamma(float gamma);

    virtual void beginFrame(const FrameInfo& f);
    virtual void endFrame(const FrameInfo& f);
    virtual void calculatePixel(Vec3& rgb, const PixelInfo& p);

private:
    Effect &next;
    float lowerLimit, upperLimit;
    std::vector<Vec3> colors;
    float currentScale;
    float gamma;
};


inline Brightness::Brightness(Effect &next)
    : next(next),
      lowerLimit(0), upperLimit(1),
      currentScale(0)
{
    // Fadecandy default
    setAssumedGamma(2.5);
}

inline void Brightness::set(float averageBrightness)
{
    lowerLimit = upperLimit = averageBrightness;
}

inline void Brightness::set(float lowerLimit, float upperLimit)
{
    this->lowerLimit = lowerLimit;
    this->upperLimit = upperLimit;
}

inline void Brightness::setAssumedGamma(float gamma)
{
    this->gamma = gamma;
}

inline void Brightness::beginFrame(const FrameInfo& f)
{
    next.beginFrame(f);
    colors.resize(f.pixels.size());

    PixelInfoIter pi = f.pixels.begin();
    PixelInfoIter pe = f.pixels.end();
    std::vector<Vec3>::iterator ci = colors.begin();

    float avg = 0;
    unsigned count = 0;

    for (;pi != pe; ++pi, ++ci) {
        Vec3 &rgb = *ci;

        if (pi->isMapped()) {
            next.calculatePixel(rgb, *pi);

            avg += powf(rgb[0], gamma) + powf(rgb[1], gamma) + powf(rgb[2], gamma);
            count++;
        }
    }

    if (count && avg > 0) {
        avg /= count;

        if (avg < lowerLimit) {
            currentScale = powf(lowerLimit / avg, 1.0f / gamma);
        } else if (avg > upperLimit) {
            currentScale = powf(upperLimit / avg, 1.0f / gamma);
        } else {
            currentScale = 1.0f;
        }
    }
}

inline void Brightness::endFrame(const FrameInfo& f)
{
    next.endFrame(f);
}

inline void Brightness::calculatePixel(Vec3& rgb, const PixelInfo& p)
{
    rgb = colors[p.index] * currentScale;
}
