/*
 * Model creation script for a single left-to-right 8x8 grid,
 * like the ones sold by AdaFruit.
 *
 * 2014 Micah Elizabeth Scott
 * This file is released into the public domain.
 */

var model = []
var scale = -1 / 4.0;
var centerX = 7 / 2.0;
var centerY = 7 / 2.0;

function grid8x8(index, x, y) {
    // Instance of a zig-zag 8x8 grid with upper-left corner at (x, y)
    for (var v = 0; v < 8; v++) {
        for (var u = 0; u < 8; u++) {
            var px = x + u;
            var py = y + v;
            model[index++] = {
                point: [  (px - centerX) * scale, 0, (py - centerY) * scale ]
            };
        }
    }
}

grid8x8(0, 0, 0);

console.log(JSON.stringify(model));
