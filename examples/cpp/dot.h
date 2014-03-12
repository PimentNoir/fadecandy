// Texture example: "dot" image, wandering around the frame.

#pragma once

#include "lib/color.h"
#include "lib/effect.h"
#include "lib/texture.h"

class DotEffect : public Effect
{
public:
    DotEffect(const char *filename)
        : dot(filename),
          angle1(0), angle2(0), angle3(0)
    {}

    Texture dot;

    float angle1;
    float angle2;
    float angle3;

    virtual void beginFrame(const FrameInfo &f)
    {
        const float speed = 6.0;

        // Several bounded state variables, rotating at different rates
        angle1 = fmodf(angle1 + f.timeDelta * speed, 2 * M_PI);
        angle2 = fmodf(angle2 + f.timeDelta * speed * 0.2f, 2 * M_PI);
        angle3 = fmodf(angle3 + f.timeDelta * speed * 0.7f, 2 * M_PI);
    }

    virtual void shader(Vec3& rgb, const PixelInfo &p) const
    {
        // Project onto the XZ plane
        Vec2 plane = Vec2(p.point[0], p.point[2]);

        // Moving dot
        Vec2 position = 0.8 * Vec2( sinf(angle1), cosf(angle1 + sin(angle2)) );

        // Texture geometry
        Vec2 center(0.5, 0.5);
        float size = 1.5 + 0.8 * sinf(angle3);

        rgb = dot.sample( (plane - position) / size + center );
    }
};
