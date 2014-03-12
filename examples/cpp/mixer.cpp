// Simple example that mixes multiple effects.

#include "lib/effect.h"
#include "rings.h"
#include "dot.h"
#include "spokes.h"

int main(int argc, char **argv)
{
    RingsEffect rings("data/glass.png");
    DotEffect dot("data/dot.png");
    SpokesEffect spokes;

    EffectMixer mix;
    mix.effects.push_back(&rings);
    mix.effects.push_back(&dot);
    mix.effects.push_back(&spokes);
    mix.faders.resize(mix.effects.size());

    EffectRunner r;
    r.setEffect(&mix);
    r.setLayout("../layouts/grid32x16z.json");
    if (!r.parseArguments(argc, argv)) {
        return 1;
    }

    float state = 0;

    while (true) {
        float timeDelta = r.doFrame();
        const float speed = 0.1f;

        // Animate the mixer's fader controls
        state = fmod(state + timeDelta * speed, 2 * M_PI);
        for (int i = 0; i < mix.faders.size(); i++) {
            float theta = state + i * (2 * M_PI) / mix.faders.size();
            mix.faders[i] = std::max(0.0f, sinf(theta));
        }
    }
}