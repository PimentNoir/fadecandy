/*
 * Model creation script for more complex layout, a window display
 * made from a 6x12 grid of architectural glass blocks.
 *
 * Each block is backed by a square ring of 40 LEDs, organized
 * clockwise from the top-left. This string is connected to a
 * dedicated Fadecandy output channel.
 *
 * These blocks are organized into panels of 2x4, each controlled
 * by a separate Fadecandy board. These boards are then arranged in
 * a 3x3 grid, left-to-right top-to-bottom.
 *
 * The JSON for this layout includes multiple kinds of data about
 * each LED. Each LED has the following fields:
 *
 *        point: A 3D vector, arbitrary coordinates. Scaled to look good
 *               in the Open Pixel Control "gl_server" pre-visualizer
 *
 *       gridXY: Integer xy location of the block in the overall window grid
 *      blockXY: Location within the block, in XY coordinates within [-1, 1]
 *   blockAngle: Angle within the block, in radians. Zero is +Y.
 *
 * 2014 Micah Elizabeth Scott
 * This file is released into the public domain.
 */

var model = []
var blockSize = 0.3;
var centerX = blockSize * 6/2;
var centerY = blockSize * 12/2;

function blockEdge(index, gridXY, angle)
{
    // Lay out one LED strip corresponding to a block edge

    var count = 10;        // How many LEDs?
    var y = 0.75;          // Distance from center, in model

    var spacing = 2 * y / (count + 1);
    var s = Math.sin(angle);
    var c = Math.cos(angle);

    for (var i = 0; i < count; i++) {
        // Distance from vertical Y axis
        var x = (i - (count-1)/2.0) * spacing;

        // Rotated XY
        var rx = x * c - y * s;
        var ry = x * s + y * c;

        model[index++] = {
            point: [
                blockSize * -(gridXY[0] + rx * 0.5 + 0.5) + centerX,
                0,
                blockSize * -(gridXY[1] - ry * 0.5 + 0.5) + centerY
            ],
            gridXY: gridXY,
            blockXY: [rx, ry],
            blockAngle: Math.atan2(rx, ry)
        }
    }
}

function block(index, gridXY)
{
    for (var i = 0; i < 4; i++) {
        blockEdge(index + i * 10, gridXY, i * -Math.PI / 2);
    }
}

function blockModule(index, gridXY, width, height)
{
    for (var y = 0; y < height; y++) {
        for (var x = 0; x < width; x++) {
            block(index, [gridXY[0] + x, gridXY[1] + y]);
            index += 64;
        }
    }
}

function moduleGrid(index, width, height, modWidth, modHeight)
{
    for (var y = 0; y < height; y++) {
        for (var x = 0; x < width; x++) {
            blockModule(index, [x * modWidth, y * modHeight], modWidth, modHeight);
            index += 512;
        }
    }
}

moduleGrid(0, 3, 3, 2, 4);

console.log(JSON.stringify(model));
