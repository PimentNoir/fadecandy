/*
 * Effect base class for particle systems
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


class ParticleEffect : public Effect {
public:
    /*
     * Information for drawing particles. If your effect needs to keep additional
     * data about particles, use a parallel array or other separate data structure.
     */
    struct ParticleAppearance {
        Vec3 point;
        Vec3 color;
        float radius;
        float intensity;
    };

    virtual void calculatePixel(Vec3& rgb, const PixelInfo& p);

protected:
    /*
     * List of appearances for particles we're drawing. Calculate this in beginFrame(),
     * or keep it persistent across frames and update the parts you're changing.
     */
    std::vector<ParticleAppearance> displayList;

    /*
     * Kernel function; determines particle shape
     * Poly6 kernel, Müller, Charypar, & Gross (2003)
     * q normalized in range [0, 1].
     * Has compact support; kernel forced to zero outside this range.
     */
    float kernel(float q);

    // Variant of kernel function called with q^2
    float kernel2(float q2);

    // First derivative of kernel()
    float kernelDerivative(float q);
};


/*****************************************************************************************
 *                                   Implementation
 *****************************************************************************************/


inline float ParticleEffect::kernel(float q)
{
    float a = 1 - q * q;
    return a * a * a;
}

inline float ParticleEffect::kernel2(float q2)
{
    float a = 1 - q2;
    return a * a * a;
}

inline float ParticleEffect::kernelDerivative(float q)
{
    float a = 1 - q * q;
    return -6.0f * q * a * a;
}


inline void ParticleEffect::calculatePixel(Vec3& rgb, const PixelInfo& p)
{
    Vec3 accumulator(0, 0, 0);
    Vec3 point = p.point;
    std::vector<ParticleAppearance>::iterator i = displayList.begin();
    std::vector<ParticleAppearance>::iterator e = displayList.end();

    for (; i != e; ++i) {
        ParticleAppearance &particle = *i;
        float dist2 = sqrlen(particle.point - point);

        // Normalized distance
        float q2 = dist2 / sq(particle.radius);
        if (q2 < 1.0f) {
            accumulator += particle.color * (particle.intensity * kernel2(q2));
        }
    }

    rgb = accumulator;
}
