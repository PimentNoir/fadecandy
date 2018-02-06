#include "lib/effect_runner.h"
#include "spokes.h"

int main(int argc, char **argv)
{
    SpokesEffect e;

    // Global brightness control
    Brightness br(e);
    br.set(0.2);

    EffectRunner r;
    r.addEffect(&br);
    r.setLayout("../layouts/grid32x16z.json");
    return r.main(argc, argv);
}

