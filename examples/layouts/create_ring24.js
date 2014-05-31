/*
 * Model creation script for a 24-pixel ring
 *
 * 2014 Micah Elizabeth Scott
 * This file is released into the public domain.
 */

var model = []
var count = 24;
var radius = 0.2;
var index = 0;

for (var i = 0; i < count; i++) {
    var angle = i * 2 * Math.PI / count;
    model[index++] = {
        point: [ -radius * Math.sin(angle), 0, -radius * Math.cos(angle) ]
    };
}

console.log(JSON.stringify(model));
