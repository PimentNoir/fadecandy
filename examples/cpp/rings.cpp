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
    Rings(const char *palette, float seed)
        : palette(palette),
          seed(seed),
          d(0, 0, 0, 0),
          threshold(-1.0)
    {}

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

    // Sample colors along a curved path through a texture
    Texture palette;

    // Pseudorandom number seed
    float seed;

    // State variables
    Vec4 d;
    float threshold;

    // Calculated once per frame
    float spacing;
    float colorParam;
    float pixelTotal;
    unsigned pixelCount;
    Vec3 center;

    virtual void beginFrame(const FrameInfo &f)
    {
        spacing = sq(0.5 + noise2(f.time * ringScaleRate, 1.5)) * ringScale;

        // Rotate movement in the XZ plane
        float angle = noise2(f.time * 0.01, seed + 30.5) * 10.0;
        float speed = pow(fabsf(noise2(f.time * 0.01, seed + 40.5)), 2.5) * xyzSpeed;
        d[0] += cosf(angle) * speed * f.timeDelta;
        d[2] += sinf(angle) * speed * f.timeDelta;

        // Random wander along the W axis
        d[3] += noise2(f.time * wRate, seed + 3.5) * wSpeed * f.timeDelta;

        // Update center position
        center = Vec3(noise2(f.time * wanderSpeed, seed + 50.9),
                      noise2(f.time * wanderSpeed, seed + 51.4),
                      noise2(f.time * wanderSpeed, seed + 51.7)) * wanderSize;

        // Wander around the color palette
        colorParam = seed + f.time * 0.05f;

        // Reset pixel total accumulators, used for the brightness calc in endFrame
        pixelTotal = 0;
        pixelCount = 0;
    }

    virtual void calculatePixel(Vec3& rgb, const PixelInfo &p)
    {
        float dist = len(p.point - center);
        Vec4 pulse = Vec4(sinf(d[2] + dist * spacing) * ringDepth, 0, 0, 0);
        Vec4 s = Vec4(p.point * xyzScale, seed) + d;
        Vec4 chromaOffset = Vec4(0, 0, 0, 10);

        float n = (fbm_noise4(s + pulse, 4) + threshold) * brightnessContrast;
        float m = fbm_noise4(s + chromaOffset, 2) * colorContrast;

        rgb = color(colorParam + m, sq(std::max(0.0f, n)));

        // Keep a rough approximate brightness total, for closed-loop feedback
        for (unsigned i = 0; i < 3; i++) {
            pixelTotal += sq(std::min(1.0f, std::max(0.0f, rgb[i])));
            pixelCount++;
        }
    }

    virtual void endFrame(const FrameInfo &f)
    {
        // Adjust threshold in brightness-determining noise function, in order
        // to try and keep the average pixel brightness at a particular level.

        if (pixelCount > 0) {
            threshold += (targetBrightness - pixelTotal / pixelCount) * thresholdGain;
        }
    }

    virtual void debug(const DebugInfo &di)
    {
        fprintf(stderr, "\t[rings] seed = %f\n", seed);
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
};

int main(int argc, char **argv)
{
    srand(time(0));
    float seed = fmod(rand() / 1e3, 1e3);

    Rings e("data/glass.png", seed);

    EffectRunner r;
    r.setEffect(&e);

    r.setLayout("../layouts/grid32x16z.json");
    return r.main(argc, argv);
}

