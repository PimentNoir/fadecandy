#!/usr/bin/env node

// Simple particle system example with Node and opc.js
// Specify a layout file on the command line, or use the default (grid32x16z)

var OPC = new require('./opc');
var model = OPC.loadModel(process.argv[2] || '../layouts/grid32x16z.json');
var client = new OPC('localhost', 7890);

function draw() {

    var t = 0.009 * new Date().getTime();
    var numParticles = 200;
    var particles = [];

    for (var i = 0; i < numParticles; i++) {

        var r = 0.2 + 1.5 * i / numParticles;
        var x = r * Math.cos(t);
        var y = r * Math.sin(t + 10.0 * Math.sin(t * 0.15));

        particles[i] = {
            point: [0, x, y],
            intensity: 40 * i / numParticles,
            falloff: 60,
            color: [0.9, 0.2, i * 0.01]
        };
        t += 0.04;
    }

    client.mapParticles(particles, model);
}

setInterval(draw, 10);
