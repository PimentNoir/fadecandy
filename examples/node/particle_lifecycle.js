#!/usr/bin/env node

// More involved particle system. Particles are persistent, we have a background
// color, and the particles have a life cycle. Particles come and go slowly, going
// through a period of brightness and a period of darkness.

// Specify a layout file on the command line, or use the default (grid32x16z)

var OPC = new require('./opc');
var model = OPC.loadModel(process.argv[2] || '../layouts/grid32x16z.json');
var client = new OPC('localhost', 7890);

var totalLife = 300;
var numParticles = 100;
var zoom = 0.998;
var intensity = -1.2;

var backgroundHue = Math.random();

function newBloomTimer() {
    return Math.random() * 5000;
}

function randomPoint() {
    // A random point somewhere relevant on our model
    return [
        4 * (Math.random() - 0.5),
        1 * (Math.random() - 0.5),
        4 * (Math.random() - 0.5)
    ];
}

// Start out with just a background
var particles = [
    {
        point: [],          // Arbitrary location
        intensity: 1,       // Unit intensity
        falloff: 0,         // No falloff, particle has infinite size
        // Color is filled in later
    }
];

// Create other nascent particles that 'bloom' once a timer runs out
for (var i = 0; i < numParticles; i++) {
    particles.push({
        point: [0, 0, 0],
        intensity: 0,
        falloff: 30,
        color: OPC.hsv(Math.random(), 0.6, 0.8),
        bloomTimer: 20 + newBloomTimer()
    })
}

function draw()
{
    // Update background color
    backgroundHue = (backgroundHue + 0.0002) % 1;
    particles[0].color = OPC.hsv(backgroundHue, 0.8, 0.2);

    // Update particle state
    for (var i = 0; i < particles.length; i++) {
        var p = particles[i];

        if (p.bloomTimer) {
            // Particle is blooming
            p.bloomTimer -= 1;
            if (p.bloomTimer <= 0) {
                // Done blooming. Start life cycle
                p.lifeTimer = totalLife;
                p.point = randomPoint();
                p.bloomTimer = null;
            }
        }

        if (p.lifeTimer) {
            // Particle is alive. Update intensity.
            // Particles go through a life cycle with positive and negative intensity
            p.intensity = intensity * Math.sin(p.lifeTimer * 2 * Math.PI / totalLife);

            // Collapse into the center
            p.point[0] *= zoom;
            p.point[1] *= zoom;
            p.point[2] *= zoom;

            p.lifeTimer -= 1;
            if (p.lifeTimer <= 0) {
                // Particle is dead. Go back to blooming.
                p.bloomTimer = newBloomTimer();
                p.lifeTimer = null;
            }
        }
    }

    client.mapParticles(particles, model);
}

setInterval(draw, 10);
