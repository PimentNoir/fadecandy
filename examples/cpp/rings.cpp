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
        : dx(0), dz(0), dw(0), now(0) {}

    static const float xyzSpeed = 0.006;
    static const float wSpeed = 0.002;
    static const float wRate = 0.0001;
    static const float xyzScale = 0.08;
    static const float ringScale = 1.5;
    static const float ringScaleRate = 0.01;
    static const float ringDepth = 0.2;
    static const float wanderSpeed = 0.04;
    static const float wanderSize = 1.8;
    static const float hueScale = 5.0;
    static const float hueRate = 0.001;

    // State variables
    float dx, dz, dw;
    double now;

    // Calculated once per frame
    float hue, saturation;
    float spacing;
    float centerx, centery, centerz;

    virtual void nextFrame(float timeDelta)
    {
        now += timeDelta;

        hue = noise2(now * hueRate, 20.5) * hueScale;

        saturation = sq(std::min(std::max(0.7f * (0.5f + noise2(now * 0.01, 0.5)), 0.0f), 1.0f));
        spacing = sq(0.5 + noise2(now * ringScaleRate, 1.5)) * ringScale;

        // Rotate movement in the XZ plane
        float angle = noise2(now * 0.01, 30.5) * 10.0;
        float speed = pow(fabsf(noise2(now * 0.01, 40.5)), 2.5) * xyzSpeed;
        dx += cosf(angle) * speed;
        dz += sinf(angle) * speed;

        // Random wander along the W axis
        dw += noise2(now * wRate, 3.5) * wSpeed;

        // Update center position
        centerx = noise2(now * wanderSpeed, 50.9) * wanderSize;
        centery = noise2(now * wanderSpeed, 51.4) * wanderSize;
        centerz = noise2(now * wanderSpeed, 51.7) * wanderSize;
    }

    virtual void calculatePixel(float rgb[3], const PixelInfo &p)
    {
        float distx = p.x - centerx;
        float disty = p.y - centery;
        float distz = p.z - centerz;

        float dist = sqrtf(sq(distx) + sq(disty) + sq(distz));
        float pulse = sinf(dz + dist * spacing) * ringDepth;
      
        float n = fbm_noise4(
            p.x * xyzScale + dx + pulse,
            p.y * xyzScale,
            p.z * xyzScale + dz,
            dw,
            4, 0.5, 2
        ) * 2.5f + 0.1f;

        float m = fbm_noise4(
            p.x * xyzScale + dx,
            p.y * xyzScale,
            p.z * xyzScale + dz,
            dw  + 10.0,
            4, 0.5, 2
        );

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
    r.setMaxFrameRate(60);
    r.setLayout("../layouts/grid32x16z.json");
    return r.main(argc, argv);
}

