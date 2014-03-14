/*
 * Model creation script for a single 64-pixel strip
 *
 * 2014 Micah Elizabeth Scott
 * This file is released into the public domain.
 */

var model = []
var scale = -4 / 64.0;
var centerX = 63 / 2.0;
var index = 0;

for (var i = 0; i < 64; i++) {
    model[index++] = {
        point: [  (i - centerX) * scale, 0, 0 ]
    };
}

console.log(JSON.stringify(model));
