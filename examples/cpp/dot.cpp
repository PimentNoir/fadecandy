// Texture example: "dot" image, wandering around the frame.

#include "lib/color.h"
#include "lib/effect.h"
#include "lib/texture.h"

class DotEffect : public Effect
{
public:
    DotEffect()
        : dot ("data/dot.png"),
          angle (0)
    {}

    Texture dot;
    float angle;

    virtual void beginFrame(const FrameInfo &f)
    {
        const float speed = 6.0;
        angle += f.timeDelta * speed;
    }

    virtual void calculatePixel(Vec3& rgb, const PixelInfo &p)
    {
        // Project onto the XZ plane
        Vec2 plane = Vec2(p.point[0], p.point[2]);

        // Moving dot
        Vec2 position = 0.8 * Vec2( sinf(angle), cosf(angle + sin(angle * 0.2f)) );

        // Texture geometry
        Vec2 center(0.5, 0.5);
        float size = 1.5 + 0.8 * sinf(angle * 0.7f);

        rgb = dot.sample( (plane - position) / size + center );
    }
};

int main(int argc, char **argv)
{
    EffectRunner r;

    DotEffect e;
    r.setEffect(&e);

    // Defaults, overridable with command line options
    r.setLayout("../layouts/grid32x16z.json");

    return r.main(argc, argv);
}