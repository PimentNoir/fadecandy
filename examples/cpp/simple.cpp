// Simple example effect:
// Uses the grid32x16z layout, connects to a local OPC server.
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

    virtual void calculatePixel(rapidjson::Value &layout, float rgb[3])
    {
        float x = layout["point"][0u].GetDouble();
        float y = layout["point"][1u].GetDouble();
        float z = layout["point"][2u].GetDouble();

        float distance = sqrtf(x*x + y*y + z*z);
        float wave = sinf(3.0 * distance - angle) + noise3(x, y, z);

        hsv2rgb(rgb, 0.2, 0.3, wave);
    }
};

int main()
{
    EffectRunner r;
    MyEffect e;
    r.setEffect(&e);
    r.setMaxFrameRate(100);
    r.setServer("localhost");
    r.setLayout("../layouts/grid32x16z.json");
    r.run();
    return 0;
}