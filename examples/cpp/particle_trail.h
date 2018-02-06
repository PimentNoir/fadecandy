// Simple particle system example for C++.

#pragma once

#include <cmath>
#include "lib/color.h"
#include "lib/effect.h"
#include "lib/particle.h"

class ParticleTrailEffect : public ParticleEffect
{
public:
    ParticleTrailEffect()
        : angle1(0), angle2(0), baseHue(0)
    {}

    float angle1;
    float angle2;
    float baseHue;

    virtual void beginFrame(const FrameInfo &f)
    {
        const float tailLength = 8.0f;
        const float speed = 9.0f;
        const float lfoRatio = 0.15f;
        const float hueRate = 0.01f;
        const float brightness = 40.0f;
        const unsigned numParticles = 200;

        // Low frequency oscillators
        angle1 = fmodf(angle1 + f.timeDelta * speed, 2 * M_PI);
        angle2 = fmodf(angle2 + f.timeDelta * speed * lfoRatio, 2 * M_PI);
        baseHue = fmodf(baseHue + f.timeDelta * speed * hueRate, 1.0f);

        appearance.resize(numParticles);
        for (unsigned i = 0; i < numParticles; i++) {
            float s = float(i) / numParticles;
            float tail = s * tailLength;

            float radius = 0.2 + 1.5 * s;
            float x = radius * cos(angle1 + tail);
            float y = radius * sin(angle1 + tail + 10.0 * sin(angle2 + tail * lfoRatio));
            float hue = baseHue + s * 0.4;

            ParticleAppearance& p = appearance[i];
            p.point = Vec3(x, 0, y);
            p.intensity = (brightness / numParticles) * s;
            p.radius = 0.1 + 0.4f * s;
            hsv2rgb(p.color, hue, 0.5, 0.8);
        }

        ParticleEffect::beginFrame(f);
    }
};
