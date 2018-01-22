/*
 * Radial rotating pattern with noise functions.
 *
 * 2014 Micah Elizabeth Scott
 */

#pragma once

#include <cmath>
#include "lib/color.h"
#include "lib/effect.h"
#include "lib/noise.h"
#include "lib/brightness.h"

class SpokesEffect : public Effect
{
public:
    SpokesEffect()
        : spin(0) {}

    static constexpr float cycleRate = 0.001;
    static constexpr float wanderSpeed = 40.0;
    static constexpr float wanderSize = 1.8;
    static constexpr float hueScale = 5.0;
    static constexpr float hueRate = 1.0;
    static constexpr float satRate = 10.0;
    static constexpr float hueShift = 0.02;
    static constexpr float spinSpeed = 3.0;
    static constexpr float spinRate = 100.0;
    static constexpr float noiseDepth = 3.0;
    static constexpr float noiseScale = 0.2;
    static constexpr float noiseSpeed = 1000.0;

    // State variables
    float spin;
    float cycle; 

    // Calculated once per frame
    float hue, saturation;
    Vec3 center, noiseOffset;

    virtual void beginFrame(const FrameInfo &f)
    {
        // Slow cyclic change. Values stay bounded over time.
        cycle = fmodf(cycle + f.timeDelta * cycleRate, 2 * M_PI);
        float cyclePos = sinf(cycle);

        noiseOffset[2] = cyclePos * noiseSpeed;

        spin = fmodf(spin + f.timeDelta * noise2(cyclePos * spinRate, 5.8f) * spinSpeed, 2 * M_PI);

        hue = noise2(cyclePos * hueRate, 20.5) * hueScale;
        saturation = sq(std::min(std::max(0.7f * (0.5f + noise2(cyclePos * satRate, 0.5)), 0.0f), 1.0f));

        // Update center position
        center = Vec3(noise2(cyclePos * wanderSpeed, 50.9), 0,
                      noise2(cyclePos * wanderSpeed, 51.7)) * wanderSize;
    }

    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        // Vector to center
        Vec3 s = p.point - center;

        // Distort with noise function
        s += Vec3(fbm_noise3(p.point * noiseScale + noiseOffset, 4) * noiseDepth, 0, 0);

        float angle = atan2(s[2], s[0]) + spin;

        hsv2rgb(rgb,
            hue + angle * hueShift,
            saturation,
            sq(std::max(0.0f, sinf(angle * 5.0f))) * std::min(0.8f, sqrlen(s))
        );
    }
};
