// Simple particle system example for C++. Ported from the "particle_trail" Node.js example.

#include <math.h>
#include "lib/color.h"
#include "lib/effect.h"
#include "lib/particle.h"

class ParticleTrail : public ParticleEffect
{
public:
    virtual void beginFrame(const FrameInfo &f)
    {
        float t = 9.0f * f.time;
        const int numParticles = 200;

        displayList.resize(numParticles);
        for (unsigned i = 0; i < numParticles; i++) {
            float s = float(i) / numParticles;

            float radius = 0.2 + 1.5 * s;
            float theta = t + 0.04 * i;
            float x = radius * cos(theta);
            float y = radius * sin(theta + 10.0 * sin(theta * 0.15));
            float hue = t * 0.01 + s * 0.2;

            ParticleAppearance& p = displayList[i];
            p.point = Vec3(x, 0, y);
            p.intensity = 0.2f * s;
            p.falloff = 60.0f;
            hsv2rgb(p.color, hue, 0.5, 0.8);
        }
    }
};

int main(int argc, char **argv)
{
    EffectRunner r;
    ParticleTrail e;
    r.setEffect(&e);
    r.setLayout("../layouts/grid32x16z.json");
    return r.main(argc, argv);
}
