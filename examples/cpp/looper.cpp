#include "lib/effect_runner.h"
#include "dot.h"
#include "particle_trail.h"
#include "rings.h"
#include "spokes.h"

int main(int argc, char **argv)
{
    EffectRunner r;

    int fps = 300;
    for( int i = 1; i < argc; ++i )
    {
        if (!strcmp(argv[i], "-fps") && (i+1 < argc)) {
            fps = atoi(argv[++i]);
            if (fps <= 0) {
                fprintf(stderr, "Invalid frame rate\n");
                return 1;
            }
        }
    }

    DotEffect dot("data/dot.png");
    ParticleTrailEffect trail;
    RingsEffect rings("data/glass.png");
    SpokesEffect spokes;
    dot.number_frames = fps * 3;
    trail.number_frames = fps * 3;
    rings.number_frames = fps * 3;
    spokes.number_frames = fps * 3;

    r.addEffect(&dot);
    r.addEffect(&trail);
    r.addEffect(&rings);
    r.addEffect(&spokes);

    // Defaults, overridable with command line options
    r.setLayout("../layouts/grid32x16z.json");

    return r.main(argc, argv);
}
