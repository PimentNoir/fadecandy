/*
 * Three-dimensional pattern in C++ based on the "Rings" Processing example.
 *
 * This version samples colors from an image, rather than using the HSV colorspace.
 *
 * Uses noise functions modulated by sinusoidal rings, which themselves
 * wander and shift according to some noise functions.
 *
 * (c) 2014 Micah Elizabeth Scott
 * http://creativecommons.org/licenses/by/3.0/
 */

#pragma once

#include <cmath>
#include <ctime>
#include <cstdlib>
#include "lib/color.h"
#include "lib/effect.h"
#include "lib/noise.h"
#include "lib/texture.h"

class RingsEffect : public Effect
{
public:
    RingsEffect(const char *palette)
        : palette(palette)
    {
        reseed();
    }

    static constexpr float xyzSpeed = 0.6;
    static constexpr float xyzScale = 0.08;
    static constexpr float wSpeed = 0.2;
    static constexpr float wRate = 0.015;
    static constexpr float ringScale = 1.5;
    static constexpr float ringScaleRate = 0.01;
    static constexpr float ringDepth = 0.2;
    static constexpr float wanderSpeed = 0.04;
    static constexpr float wanderSize = 1.2;
    static constexpr float brightnessContrast = 8.0;
    static constexpr float colorContrast = 4.0;
    static constexpr float targetBrightness = 0.1;
    static constexpr float thresholdGain = 0.1;
    static constexpr float thresholdStepLimit = 0.02;
    static constexpr float initialThreshold = -1.0f;
    static constexpr unsigned brightnessOctaves = 4;
    static constexpr unsigned colorOctaves = 2;

    // Sample colors along a curved path through a texture
    Texture palette;

    // State variables
    Vec4 d;
    float timer;
    float seed;
    float threshold;

    // Calculated once per frame
    float spacing;
    float colorParam;
    float pixelTotalNumerator;
    unsigned pixelTotalDenominator;
    bool is3D;
    Vec3 center;

    virtual void beginFrame(const FrameInfo &f)
    {
        timer += f.timeDelta;

        spacing = sq(0.5 + noise2(timer * ringScaleRate, 1.5)) * ringScale;

        // Rotate movement in the XZ plane
        float angle = noise2(timer * 0.01, seed + 30.5) * 10.0;
        float speed = pow(fabsf(noise2(timer * 0.01, seed + 40.5)), 2.5) * xyzSpeed;
        d[0] += cosf(angle) * speed * f.timeDelta;
        d[2] += sinf(angle) * speed * f.timeDelta;

        // Random wander along the W axis
        d[3] += noise2(timer * wRate, seed + 3.5) * wSpeed * f.timeDelta;

        // Update center position
        center = Vec3(noise2(timer * wanderSpeed, seed + 50.9),
                      noise2(timer * wanderSpeed, seed + 51.4),
                      noise2(timer * wanderSpeed, seed + 51.7)) * wanderSize;

        // Wander around the color palette
        colorParam = seed + timer * 0.05f;

        // Reset pixel total accumulators, used for the brightness calc in endFrame
        pixelTotalNumerator = 0;
        pixelTotalDenominator = 0;

        // Is this 2D or 3D?
        is3D = false;
        for (Effect::PixelInfoIter i = f.pixels.begin(), e = f.pixels.end(); i != e; ++i) {
            const Effect::PixelInfo &p = *i;
            if (p.point[1] != 0.0f) {
                is3D = true;
            }
        }
    }

    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        // Noise sampling location
        Vec4 s = Vec4(p.point * xyzScale, seed) + d;

        // Ring function, displaces the noise sampling coordinate
        float dist = len(p.point - center);
        Vec4 pulse = Vec4(sinf(d[2] + dist * spacing) * ringDepth, 0, 0, 0);

        /*
         * Brightness is calculated by:
         *
         *    n = (fbm_noise4(s + pulse, octaves) + threshold) * brightnessContrast;
         *
         * But if we can determine that n <= 0, we can exit early. Check this after
         * each fbm octave, to see if we can save another costly noise calculation.
         * Also, use 3D noise instead of 4D if the Y axis is unused.
         */

        float n = threshold * brightnessContrast;
        float amplitude = brightnessContrast;
        Vec4 arg = s + pulse;
        unsigned i = brightnessOctaves;

        while (true) {
            n += amplitude * dNoise(arg);
            --i;
            if (!(n > -amplitude * fbmTotal(i))) {
                // Too low for further octaves to bring back above 0.
                // On the last octave, note fbmTotal(0) == 0
                // Should also exit in case of NaN.
                return;
            }
            if (!i) {
                break;
            }

            amplitude *= 0.5f;
            arg *= 2.0f;
        }
        n /= fbmTotal(brightnessOctaves);

        /*
         * Another hybrid 2D/3D fbm for chroma. Use half the octaves.
         */

        float m = 0;
        amplitude = colorContrast;
        arg = s + Vec4(0, 0, 0, 10);
        i = colorOctaves;

        while (true) {
            m += amplitude * dNoise(arg);
            if (--i == 0) {
                break;
            }

            amplitude *= 0.5f;
            arg *= 2.0f;
        }
        m /= fbmTotal(colorOctaves);

        // Assemble color using a lookup through our palette
        rgb = color(colorParam + m, sq(n));
    }

    inline void postProcess(const Vec3& rgb, const PixelInfo& p)
    {
        // Keep a rough approximate brightness total, for closed-loop feedback
        for (unsigned i = 0; i < 3; i++) {
            pixelTotalNumerator += sq(std::min(1.0f, std::max(0.0f, rgb[i])));
            pixelTotalDenominator++;
        }
    }

    virtual bool endFrame(const FrameInfo &f)
    {
        // Per-frame brightness calculations.
        // Adjust threshold in brightness-determining noise function, in order
        // to try and keep the average pixel brightness at a particular level.

        float target = targetBrightness;
        float current = pixelTotalDenominator ? pixelTotalNumerator / pixelTotalDenominator : 0.0f;
        bool blackLevel = current <= 0.0f;

        if (wantToReseed()) {
            // Fade to black
            target = 0;

            if (blackLevel) {
                // At black level. Reseed invisibly!
                reseed();
            }
        }

        // Rate limited servo loop.
        // Disabled if we aren't calculating pixel values.

        if (pixelTotalDenominator) {
            float step = (target - current) * thresholdGain;
            if (step > thresholdStepLimit) step = thresholdStepLimit;
            if (step < -thresholdStepLimit) step = -thresholdStepLimit;
            threshold += step;
        }
        return Effect::endFrame(f);
    }

    virtual void debug(const DebugInfo &di)
    {
        fprintf(stderr, "\t[rings] %s model\n", is3D ? "3D" : "2D"); 
        fprintf(stderr, "\t[rings] seed = %f%s\n", seed, wantToReseed() ? " [reseed pending]" : "");
        fprintf(stderr, "\t[rings] timer = %f\n", timer);
        fprintf(stderr, "\t[rings] center = %f, %f, %f\n", center[0], center[1], center[2]);
        fprintf(stderr, "\t[rings] d = %f, %f, %f, %f\n", d[0], d[1], d[2], d[3]);
        fprintf(stderr, "\t[rings] threshold = %f\n", threshold);
    }

private:
    // Totally reinitialize our state variables. We do this periodically
    // during normal operation, during blank periods.

    void reseed()
    {
        // Get okay seed mixing even with depressing rand() implementations
        srand(time(0));
        for (int i = 0; i < 50; i++) {
            rand();
        }
        seed = rand() / double(RAND_MAX / 1024);

        // Starting point
        d = Vec4(0,0,0,0);
        timer = 0;

        // Initial threshold gives us time to fade in
        threshold = initialThreshold;
    }

    // Do our state variables need resetting? This is like a watchdog timer,
    // keeping an eye on the simulation parameters. If we need to start over,
    // we'll start fading out and reseed during the darkness. This will happen
    // periodically in order to keep our numbers within the useful resolution of
    // a 32-bit float.

    bool wantToReseed()
    {
        // Comparisons carefully written to NaN always causes reseed
        return !(timer < 9000.0f) |
               !(threshold <  10.0f) |
               !(threshold > -10.0f) |
               !(d[0] < 1000.0f) |
               !(d[1] < 1000.0f) |
               !(d[2] < 1000.0f) |
               !(d[3] < 1000.0f) |
               !(d[0] > -1000.0f) |
               !(d[1] > -1000.0f) |
               !(d[2] > -1000.0f) |
               !(d[3] > -1000.0f) ;
    }

    // Sample a color from our palette, using a lissajous curve within an image texture

    Vec3 color(float parameter, float brightness) const
    {
        return palette.sample( sinf(parameter) * 0.5f + 0.5f,
                               sinf(parameter * 0.86f) * 0.5f + 0.5f) * brightness;
    }

    // Sample 3 or 4 dimensional noise. If (!is3D), we use 3 dimensional noise, ignoring the Y axis.

    float dNoise(Vec4 v) const
    {
        return is3D ? noise4(v) : noise3(v[0], v[2], v[3]);
    }

    // Normalization factor for fractional brownian motion with N octaves

    static float fbmTotal(int i)
    {
        float n = 0;
        float amp = 1.0f;
        while (i > 0) {
            n += amp;
            amp *= 0.5;
            i--;
        }
        return n;
    }
};
