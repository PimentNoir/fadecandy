/*
 * Three-dimensional pattern in C++ based on the "Rings" Processing example.
 *
 * This version samples colors from an image, rather than using the HSV colorspace.
 *
 * Uses noise functions modulated by sinusoidal rings, which themselves
 * wander and shift according to some noise functions.
 *
 * 2014 Micah Elizabeth Scott
 */

#include <math.h>
#include <time.h>
#include <stdlib.h>
#include "lib/color.h"
#include "lib/effect.h"
#include "lib/noise.h"
#include "lib/texture.h"

class Rings : public Effect
{
public:
    Rings(const char *palette)
        : palette(palette)
    {
        reseed();
    }

    static const float xyzSpeed = 0.6;
    static const float xyzScale = 0.08;
    static const float wSpeed = 0.2;
    static const float wRate = 0.015;
    static const float ringScale = 1.5;
    static const float ringScaleRate = 0.01;
    static const float ringDepth = 0.2;
    static const float wanderSpeed = 0.04;
    static const float wanderSize = 1.2;
    static const float brightnessContrast = 8.0;
    static const float colorContrast = 4.0;
    static const float targetBrightness = 0.1;
    static const float thresholdGain = 0.1;
    static const float thresholdStepLimit = 0.02;

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
    float pixelTotal;
    unsigned pixelCount;
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
        pixelTotal = 0;
        pixelCount = 0;
    }

    virtual void calculatePixel(Vec3& rgb, const PixelInfo &p)
    {
        float dist = len(p.point - center);
        Vec4 pulse = Vec4(sinf(d[2] + dist * spacing) * ringDepth, 0, 0, 0);
        Vec4 s = Vec4(p.point * xyzScale, seed) + d;

        float n = (fbm_noise4(s + pulse, 4) + threshold) * brightnessContrast;
        if (n > 0.0f) {
            // Positive brightness is possible

            Vec4 chromaOffset = Vec4(0, 0, 0, 10);
            float m = fbm_noise4(s + chromaOffset, 2) * colorContrast;

            rgb = color(colorParam + m, sq(n));

            // Keep a rough approximate brightness total, for closed-loop feedback
            for (unsigned i = 0; i < 3; i++) {
                pixelTotal += sq(std::min(1.0f, std::max(0.0f, rgb[i])));
            }
        }

        pixelCount += 3;
    }

    virtual void endFrame(const FrameInfo &f)
    {
        // Per-frame brightness calculations.
        // Adjust threshold in brightness-determining noise function, in order
        // to try and keep the average pixel brightness at a particular level.

        float target = targetBrightness;
        float current = pixelCount ? pixelTotal / pixelCount : 0.0f;

        if (wantToReseed()) {
            // Fade to black
            target = 0;

            if (current <= 0) {
                // At black level. Reseed invisibly!
                reseed();
            }
        }

        // Rate limited servo loop

        float step = (target - current) * thresholdGain;
        if (step > thresholdStepLimit) step = thresholdStepLimit;
        if (step < -thresholdStepLimit) step = -thresholdStepLimit;
        threshold += step;
    }

    virtual void debug(const DebugInfo &di)
    {
        fprintf(stderr, "\t[rings] seed = %f%s\n", seed, wantToReseed() ? " [reseed pending]" : "");
        fprintf(stderr, "\t[rings] timer = %f\n", timer);
        fprintf(stderr, "\t[rings] center = %f, %f, %f\n", center[0], center[1], center[2]);
        fprintf(stderr, "\t[rings] d = %f, %f, %f, %f\n", d[0], d[1], d[2], d[3]);
        fprintf(stderr, "\t[rings] threshold = %f\n", threshold);
    }

    virtual Vec3 color(float parameter, float brightness)
    {
        // Sample a color from our palette, using a lissajous curve within an image texture
        return palette.sample( sinf(parameter) * 0.5f + 0.5f,
                               sinf(parameter * 0.86f) * 0.5f + 0.5f) * brightness;
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
        threshold = -1.0f;
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
};

int main(int argc, char **argv)
{
    Rings e("data/glass.png");

    EffectRunner r;
    r.setEffect(&e);

    r.setLayout("../layouts/grid32x16z.json");
    return r.main(argc, argv);
}

