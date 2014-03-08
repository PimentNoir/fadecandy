/*
 * Model creation script for a 32x16 grid made of zig-zag 8x8 grids.
 *
 * 2014 Micah Elizabeth Scott
 * This file is released into the public domain.
 */

var model = []
var scale = -1 / 8.0;
var centerX = 31 / 2.0;
var centerY = 15 / 2.0;

function grid8x8(index, x, y) {
    // Instance of a zig-zag 8x8 grid with upper-left corner at (x, y)
    for (var v = 0; v < 8; v++) {
        for (var u = 0; u < 8; u++) {
            var px = (v & 1) ? (x+7-u) : (x+u);
            var py = y + v;
            model[index++] = {
                point: [  (px - centerX) * scale, 0, (py - centerY) * scale ]
            };
        }
    }
}

// Eight zig-zag grids
var index = 0;
for (var v = 0; v < 2; v++) {
    for (var u = 0; u < 4; u++) {
        grid8x8(index, u*8, v*8);
        index += 64;
    }
}

console.log(JSON.stringify(model));
