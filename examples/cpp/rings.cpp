#include "lib/effect_runner.h"
#include "rings.h"

int main(int argc, char **argv)
{
    RingsEffect e("data/glass.png");

    EffectRunner r;
    r.setEffect(&e);

    r.setLayout("../layouts/grid32x16z.json");
    return r.main(argc, argv);
}
