#include "lib/effect_runner.h"
#include "dot.h"

int main(int argc, char **argv)
{
    EffectRunner r;

    DotEffect e("data/dot.png");
    r.addEffect(&e);

    // Defaults, overridable with command line options
    r.setLayout("../layouts/grid32x16z.json");

    return r.main(argc, argv);
}
