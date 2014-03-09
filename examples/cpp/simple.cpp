// Simple example effect:
// Draws a noise pattern modulated by an expanding sine wave.

#include <math.h>
#include "color.h"
#include "effect.h"
#include "noise.h"

class MyEffect : public Effect
{
public:
    MyEffect()
        : angle (0) {}

    float angle;

    virtual void nextFrame(float timeDelta)
    {
        const float speed = 10.0;
        angle = fmodf(angle + timeDelta * speed, 2 * M_PI);
    }

    virtual void calculatePixel(float rgb[3], const PixelInfo &p)
    {
        float distance = sqrtf(sq(p.x) + sq(p.y) + sq(p.z));
        float wave = sinf(3.0 * distance - angle) + noise3(p.x, p.y, p.z);
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