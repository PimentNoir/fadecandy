// Simple example effect:
// Draws a noise pattern modulated by an expanding sine wave.

#include <math.h>
#include "lib/color.h"
#include "lib/effect.h"
#include "lib/effect_runner.h"
#include "lib/noise.h"

class MyEffect : public Effect
{
public:
    MyEffect()
        : cycle (0) {}

    float cycle;

    virtual void beginFrame(const FrameInfo &f)
    {
        const float speed = 10.0;
        cycle = fmodf(cycle + f.timeDelta * speed, 2 * M_PI);
    }

    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        float distance = len(p.point);
        float wave = sinf(3.0 * distance - cycle) + noise3(p.point);
        hsv2rgb(rgb, 0.2, 0.3, wave);
    }
};

int main(int argc, char **argv)
{
    EffectRunner r;

    MyEffect e;
    r.addEffect(&e);

    // Defaults, overridable with command line options
    r.setMaxFrameRate(100);
    r.setLayout("../layouts/grid32x16z.json");

    return r.main(argc, argv);
}
