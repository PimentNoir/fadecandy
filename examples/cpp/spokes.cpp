/*
 * Radial rotating pattern with noise functions.
 *
 * 2014 Micah Elizabeth Scott
 */

#include <math.h>
#include "lib/color.h"
#include "lib/effect.h"
#include "lib/noise.h"
#include "lib/brightness.h"

class Spokes : public Effect
{
public:
    Spokes()
        : spin(0) {}

    static const float wanderSpeed = 0.04;
    static const float wanderSize = 1.8;
    static const float hueScale = 5.0;
    static const float hueRate = 0.001;
    static const float hueShift = 0.02;
    static const float spinSpeed = 3.0;
    static const float spinRate = 0.1;
    static const float noiseDepth = 3.0;
    static const float noiseScale = 0.2;
    static const float noiseSpeed = 1.0;

    // State variables
    double spin;

    // Calculated once per frame
    float hue, saturation;
    Vec3 center, noiseOffset;

    virtual void beginFrame(const FrameInfo &f)
    {
        noiseOffset[2] += f.timeDelta * noiseSpeed;

        spin = fmodf(spin + f.timeDelta * noise2(f.time * spinRate, 5.8f) * spinSpeed, M_PI * 2.0f);

        hue = noise2(f.time * hueRate, 20.5) * hueScale;
        saturation = sq(std::min(std::max(0.7f * (0.5f + noise2(f.time * 0.01, 0.5)), 0.0f), 1.0f));

        // Update center position
        center = Vec3(noise2(f.time * wanderSpeed, 50.9), 0,
                      noise2(f.time * wanderSpeed, 51.7)) * wanderSize;
    }

    virtual void calculatePixel(Vec3& rgb, const PixelInfo &p)
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

int main(int argc, char **argv)
{
    Spokes e;

    // Global brightness control
    Brightness br(e);
    br.set(0.2);

    EffectRunner r;
    r.setEffect(&br);
    r.setLayout("../layouts/grid32x16z.json");
    return r.main(argc, argv);
}

