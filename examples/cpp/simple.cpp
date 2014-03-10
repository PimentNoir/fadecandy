// Simple example effect:
// Draws a noise pattern modulated by an expanding sine wave.

#include <math.h>
#include "lib/color.h"
#include "lib/effect.h"
#include "lib/noise.h"

class MyEffect : public Effect
{
public:
    MyEffect()
        : angle (0) {}

    float angle;

    virtual void beginFrame(const FrameInfo &f)
    {
        const float speed = 10.0;
        angle = fmodf(angle + f.timeDelta * speed, 2 * M_PI);
    }

    virtual void calculatePixel(Vec3& rgb, const PixelInfo &p)
    {
        float distance = len(p.point);
        float wave = sinf(3.0 * distance - angle) + noise3(p.point);
        hsv2rgb(rgb, 0.2, 0.3, wave);
    }
};

int main(int argc, char **argv)
{
    EffectRunner r;

    MyEffect e;
    r.setEffect(&e);

    // Defaults, overridable with command line options
    r.setMaxFrameRate(100);
    r.setLayout("../layouts/grid32x16z.json");

    return r.main(argc, argv);
}