/*
 * Three-dimensional pattern in C++ based on the "Rings" Processing example.
 *
 * Uses noise functions modulated by sinusoidal rings, which themselves
 * wander and shift according to some noise functions.
 *
 * 2014 Micah Elizabeth Scott
 */

#include <math.h>
#include "color.h"
#include "effect.h"
#include "noise.h"

class Rings : public Effect
{
public:
    Rings()
        : d(0, 0, 0, 0) {}

    static const float xyzSpeed = 0.6;
    static const float xyzScale = 0.08;
    static const float wSpeed = 0.2;
    static const float wRate = 0.0001;
    static const float ringScale = 1.5;
    static const float ringScaleRate = 0.01;
    static const float ringDepth = 0.2;
    static const float wanderSpeed = 0.04;
    static const float wanderSize = 1.8;
    static const float hueScale = 5.0;
    static const float hueRate = 0.001;

    // State variables
    Vec4 d;

    // Calculated once per frame
    float hue, saturation;
    float spacing;
    Vec3 center;

    virtual void beginFrame(const FrameInfo &f)
    {
        hue = noise2(f.time * hueRate, 20.5) * hueScale;

        saturation = sq(std::min(std::max(0.7f * (0.5f + noise2(f.time * 0.01, 0.5)), 0.0f), 1.0f));
        spacing = sq(0.5 + noise2(f.time * ringScaleRate, 1.5)) * ringScale;

        // Rotate movement in the XZ plane
        float angle = noise2(f.time * 0.01, 30.5) * 10.0;
        float speed = pow(fabsf(noise2(f.time * 0.01, 40.5)), 2.5) * xyzSpeed;
        d[0] += cosf(angle) * speed * f.timeDelta;
        d[2] += sinf(angle) * speed * f.timeDelta;

        // Random wander along the W axis
        d[3] += noise2(f.time * wRate, 3.5) * wSpeed * f.timeDelta;

        // Update center position
        center = Vec3(noise2(f.time * wanderSpeed, 50.9),
                      noise2(f.time * wanderSpeed, 51.4),
                      noise2(f.time * wanderSpeed, 51.7)) * wanderSize;
    }

    virtual void calculatePixel(Vec3& rgb, const PixelInfo &p)
    {
        float dist = len(p.point - center);
        Vec4 pulse = Vec4(sinf(d[2] + dist * spacing) * ringDepth, 0, 0, 0);
        Vec4 s = Vec4(p.point * xyzScale, 0) + d;
        Vec4 chromaOffset = Vec4(0, 0, 0, 10);

        float n = fbm_noise4(s + pulse, 4) * 2.5f + 0.1f;
        float m = fbm_noise4(s + chromaOffset, 4);

        hsv2rgb(rgb,
            hue + 0.5 * m,
            saturation,
            std::min(0.9f, sq(std::max(0.0f, n)))
        );
    }
};

int main(int argc, char **argv)
{
    EffectRunner r;
    Rings e;
    r.setEffect(&e);
    r.setLayout("../layouts/grid32x16z.json");
    return r.main(argc, argv);
}

