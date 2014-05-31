#!/usr/bin/env node

/*
 * Three dimensional Node.js pattern based on the "Rings" Processing example.
 *
 * Uses noise functions modulated by sinusoidal rings, which themselves
 * wander and shift according to some noise functions.
 *
 * 2014 Micah Elizabeth Scott
 */

var SimplexNoise = require('simplex-noise');
var simplex = new SimplexNoise(Math.random);

var OPC = new require('./opc');
var model = OPC.loadModel(process.argv[2] || '../layouts/grid32x16z.json');
var client = new OPC('localhost', 7890);

var noiseScale = 0.02;
var speed = 0.002;
var wspeed = 0.01;
var scale = 0.1;
var ringScale = 3.0;
var wanderSpeed = 0.00005;
var dx = 0, dz = 0, dw = 0;

var min = Math.min;
var max = Math.max;
var sin = Math.sin;
var cos = Math.cos;
var pow = Math.pow;
var sqrt = Math.sqrt;

function fractalNoise(x, y, z, w)
{
    // 4D fractal noise (fractional brownian motion)

    var r = 0;
    var amp = 0.5;
    for (var octave = 0; octave < 4; octave++) {
        r += (simplex.noise4D(x, y, z, w) + 1) * amp;
        amp /= 2;
        x *= 2;
        y *= 2;
        z *= 2;
        w *= 2;
    }
    return r;
}

function noise(x, spin)
{
    // 1-dimensional noise. Cut a zig-zag path through
    // the simplex 2D noise space, so we repeat much less often.
    spin = spin || 0.01;
    return simplex.noise2D(x, x * spin) * 0.5 + 0.5;
}

function draw()
{
    var now = new Date().getTime();

    var angle = sin(now * 0.001);
    var hue = now * 1.0;

    var saturation = min(max(pow(1.15 * noise(now * 0.000122), 2.5), 0), 1);
    var spacing = noise(now * 0.000124) * ringScale;

    // Rotate movement in the XZ plane
    dx += cos(angle) * speed;
    dz += sin(angle) * speed;

    // Random wander along the W axis
    dw += (noise(now * 0.00002) - 0.5) * wspeed;

    var centerx = (noise(now * wanderSpeed, 0.9) - 0.5) * 1.25;
    var centery = (noise(now * wanderSpeed, 1.4) - 0.5) * 1.25;
    var centerz = (noise(now * wanderSpeed, 1.7) - 0.5) * 1.25;

    function shader(p)
    {
        var x = p.point[0];
        var y = p.point[1];
        var z = p.point[2];

        var distx = x - centerx;
        var disty = y - centery;
        var distz = z - centerz;

        var dist = sqrt(distx*distx + disty*disty + distz*distz);
        var pulse = (sin(dz + dist * spacing) - 0.3) * 0.3;
      
        var n = fractalNoise(
            x * scale + dx + pulse,
            y * scale,
            z * scale + dz,
            dw
        ) - 0.95;

        var m = fractalNoise(
            x * scale + dx,
            y * scale,
            z * scale + dz,
            dw  + 10.0
        ) - 0.75;

        return OPC.hsv(
            hue + 0.2 * m,
            saturation,
            min(max(pow(3.0 * n, 1.5), 0), 0.9)
        );
    }

    client.mapPixels(shader, model);
}

setInterval(draw, 10);
