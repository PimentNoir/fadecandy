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
      currentScale(1)
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

    unsigned count = 0;

    // Calculate the next effect's pixels, storing them all. Also count the total number
    // of mapped pixels, ignoring any unmapped ones.

    {
        PixelInfoIter pi = f.pixels.begin();
        PixelInfoIter pe = f.pixels.end();
        std::vector<Vec3>::iterator ci = colors.begin();

        for (;pi != pe; ++pi, ++ci) {
            if (pi->isMapped()) {
                next.calculatePixel(*ci, *pi);
                count++;
            }
        }
    }

    if (count == 0) {
        // No LEDs mapped
        return;
    }

    // Iterative algorithm to adjust brightness scaling. I'm not sure a closed-form
    // solution exists- this is complicated for multiple reasons. We want to scale the
    // entire image in a perceptually linear way, but the final brightness we're interested
    // in is related to the total linear intensity of all LEDs. Additionally, the brightness
    // is clamped at each LED, so we may need to increase the brightness of other LEDs to
    // compensate for individual LEDs that can't get any brighter. Usually this only takes
    // a few iterations to converge.

    const unsigned maxIters = 50;
    const float epsilon = 1e-3;

    for (unsigned iter = 0; iter < maxIters; iter++) {

        std::vector<Vec3>::iterator ci = colors.begin();
        std::vector<Vec3>::iterator ce = colors.end();
        float avg = 0;

        for (;ci != ce; ++ci) {
            Vec3& rgb = *ci;

            // Simulated linear brightness, using current scale
            for (unsigned i = 0; i < 3; i++) {
                avg += powf(std::max(0.0f, std::min(1.0f, rgb[i] * currentScale)), gamma);
            }
        }

        avg /= count;

        // Make the best estimate we can for this iteration
        float adjustment;
        if (avg < lowerLimit) {
            adjustment = powf(lowerLimit / avg, 1.0f / gamma);
        } else if (avg > upperLimit) {
            adjustment = powf(upperLimit / avg, 1.0f / gamma);
        } else {
            adjustment = 1.0f;
        }
        currentScale *= adjustment;

        // Was this adjustment negligible? We can quit early.
        if (fabsf(adjustment - 1.0f) < epsilon) {
            break;
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
