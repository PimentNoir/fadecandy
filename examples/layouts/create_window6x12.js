/*
 * Model creation script for the full 6x12 window, with 72 glass blocks.
 *
 * Each glass block is backed by a folded diffuser containing a ring
 * of 36 LEDs, 9 per side in a square. The diffuser spreads light
 * inward, then the glass block re-texturizes it.
 *
 * The full installation is made from six modules, each with 6x2 blocks.
 * Each module uses one Fadecandy controller, with a max power consumption
 * of about 21 Amps.
 *
 * Each module is a 2x2 grid of repeating patterns, a 3-block chain which
 * is wired to use two Fadecandy channels with a minimum of extra wire. Each
 * channel drives six block edges.
 *
 * 3x1 Chain, first Fadecandy channel:
 *
 *         A           B           C
 *        +--------+  +--------+  +--------+      
 *        |        |--|        |--|        |      
 *        |       v|  |^      v|  |^       |      
 *        |       v|  |^      v|  |^       |      
 *        |        |  | <<<<<< |  | <<<<<< | <- Input 0
 *        +--------+  +--------+  +--------+
 *
 * Second Fadecandy channel:
 *
 *         A           B           C
 *        +--------+  +--------+  +--------+      
 *        | <<<<<< |--| <<<<<< |--| <<<<<< |      
 *        |v       |  |        |  |       ^|      
 *        |v       |  |        |  |       ^|      
 *        | >>>>>> |  |        |  |        | <- Input 1    
 *        +--------+  +--------+  +--------+
 *
 * 6x2 Module, stacking vertically, with chain of FC boards on the right edge:
 *
 *          .......      .......
 *        [ Chain 1 ]  [ Chain 0 ]  [FC]
 *        [ Chain 3 ]  [ Chain 2 ]  
 *          .......      .......
 *
 * Chains 2 and 3 are VERTICALLY MIRRORED, so that all inputs originate at the
 * right-middle of the module, where the FC board is. This means that the chains
 * 2 and 3 corresponding to FC channels 4-7 look like:
 *
 *         D           E           F
 *        +--------+  +--------+  +--------+      
 *        |        |  | <<<<<< |  | <<<<<< | <- Input 0   
 *        |       ^|  |v      ^|  |v       |      
 *        |       ^|  |v      ^|  |v       |      
 *        |        |--|        |--|        |      
 *        +--------+  +--------+  +--------+
 *
 * And:
 *
 *         D           E           F
 *        +--------+  +--------+  +--------+      
 *        | >>>>>> |  |        |  |        | <- Input 1    
 *        |^       |  |        |  |       v|      
 *        |^       |  |        |  |       v|      
 *        | <<<<<< |--| <<<<<< |--| <<<<<< |      
 *        +--------+  +--------+  +--------+
 *
 * There are 6 distinct wiring layouts for a single block, named A through F:
 *
 *     A: +--------+  B: +--------+  C: +--------+  
 *        |* <<<<  |     |  <<<<  |     |  <<<< *|   
 *        |v      v|     |^      v|     |^      ^|   
 *        |v      v|     |^      v|     |^      ^|   
 *        |* >>>>  |     |* <<<< *|     |* <<<<  |   
 *        +--------+     +--------+     +--------+  
 *
 *     D: +--------+  E: +--------+  F: +--------+  
 *        |* >>>>  |     |* <<<< *|     |* <<<<  |   
 *        |^      ^|     |v      ^|     |v      v|   
 *        |^      ^|     |v      ^|     |v      v|   
 *        |* <<<<  |     |  <<<<  |     |  <<<< *|   
 *        +--------+     +--------+     +--------+  
 *
 * These different blocks come together to make a 6x2 module:
 *
 *     [A  B  C] [A  B  C] [Fc]
 *     [D  E  F] [D  E  F]
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
 * http://creativecommons.org/licenses/by/3.0/
 */

var model = []
var blockSize = 0.3;
var centerX = blockSize * 6/2;
var centerY = blockSize * 12/2;

function blockEdge(index, gridXY, side, ccw)
{
    // Lay out one LED strip corresponding to a block edge.
    // side: 0=top, 1=left, 2=bottom, 3=right
    // ccw: 0=clockwise, 1=counterclockwise

    var count = 9;        // How many LEDs?
    var y = 0.75;         // Distance from center, in model

    var spacing = 2 * y / (count + 1);
    var angle = side * Math.PI / 2;
    var s = Math.sin(angle);
    var c = Math.cos(angle);

    for (var i = 0; i < count; i++) {
        // Distance from vertical Y axis
        var x = ((ccw ? (count - 1 - i) : i) - (count-1)/2.0) * spacing;

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

function chainABC(index, gridXY)
{
    blockEdge(index + 9 * 0 + 64 * 0, [ gridXY[0] + 2, gridXY[1] ], 2, 0);
    blockEdge(index + 9 * 1 + 64 * 0, [ gridXY[0] + 2, gridXY[1] ], 1, 0);
    blockEdge(index + 9 * 2 + 64 * 0, [ gridXY[0] + 1, gridXY[1] ], 3, 0);
    blockEdge(index + 9 * 3 + 64 * 0, [ gridXY[0] + 1, gridXY[1] ], 2, 0);
    blockEdge(index + 9 * 4 + 64 * 0, [ gridXY[0] + 1, gridXY[1] ], 1, 0);
    blockEdge(index + 9 * 5 + 64 * 0, [ gridXY[0] + 0, gridXY[1] ], 3, 0);

    blockEdge(index + 9 * 0 + 64 * 1, [ gridXY[0] + 2, gridXY[1] ], 3, 1);
    blockEdge(index + 9 * 1 + 64 * 1, [ gridXY[0] + 2, gridXY[1] ], 0, 1);
    blockEdge(index + 9 * 2 + 64 * 1, [ gridXY[0] + 1, gridXY[1] ], 0, 1);
    blockEdge(index + 9 * 3 + 64 * 1, [ gridXY[0] + 0, gridXY[1] ], 0, 1);
    blockEdge(index + 9 * 4 + 64 * 1, [ gridXY[0] + 0, gridXY[1] ], 1, 1);
    blockEdge(index + 9 * 5 + 64 * 1, [ gridXY[0] + 0, gridXY[1] ], 2, 1);
}

function chainDEF(index, gridXY)
{
    blockEdge(index + 9 * 0 + 64 * 0, [ gridXY[0] + 2, gridXY[1] ], 0, 1);
    blockEdge(index + 9 * 1 + 64 * 0, [ gridXY[0] + 2, gridXY[1] ], 1, 1);
    blockEdge(index + 9 * 2 + 64 * 0, [ gridXY[0] + 1, gridXY[1] ], 3, 1);
    blockEdge(index + 9 * 3 + 64 * 0, [ gridXY[0] + 1, gridXY[1] ], 0, 1);
    blockEdge(index + 9 * 4 + 64 * 0, [ gridXY[0] + 1, gridXY[1] ], 1, 1);
    blockEdge(index + 9 * 5 + 64 * 0, [ gridXY[0] + 0, gridXY[1] ], 3, 1);

    blockEdge(index + 9 * 0 + 64 * 1, [ gridXY[0] + 2, gridXY[1] ], 3, 0);
    blockEdge(index + 9 * 1 + 64 * 1, [ gridXY[0] + 2, gridXY[1] ], 2, 0);
    blockEdge(index + 9 * 2 + 64 * 1, [ gridXY[0] + 1, gridXY[1] ], 2, 0);
    blockEdge(index + 9 * 3 + 64 * 1, [ gridXY[0] + 0, gridXY[1] ], 2, 0);
    blockEdge(index + 9 * 4 + 64 * 1, [ gridXY[0] + 0, gridXY[1] ], 1, 0);
    blockEdge(index + 9 * 5 + 64 * 1, [ gridXY[0] + 0, gridXY[1] ], 0, 0);
}

function module(index, gridXY)
{
    chainABC(index + 128 * 0, [ gridXY[0] + 3, gridXY[1] + 0 ]);
    chainABC(index + 128 * 1, [ gridXY[0] + 0, gridXY[1] + 0 ]);
    chainDEF(index + 128 * 2, [ gridXY[0] + 3, gridXY[1] + 1 ]);
    chainDEF(index + 128 * 3, [ gridXY[0] + 0, gridXY[1] + 1 ]);
}

for (var i = 0; i < 6; i++) {
    module(512 * i, [ 0, 2 * i ]);
}

console.log(JSON.stringify(model));
