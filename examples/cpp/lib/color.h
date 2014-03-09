/*
 * Color Utilities
 */

#pragma once

#include "svl/SVL.h"

static inline void hsv2rgb(Vec3 &rgb, float h, float s, float v)
{
    /*
     * Converts an HSV color value to RGB.
     *
     * Normal hsv range is in [0, 1], RGB range is [0, 255].
     * Colors may extend outside these bounds. Hue values will wrap.
     *
     * Based on tinycolor:
     * https://github.com/bgrins/TinyColor/blob/master/tinycolor.js
     * 2013-08-10, Brian Grinstead, MIT License
     */

    h = fmodf(h, 1) * 6.0f;
    if (h < 0) h += 6.0f;

    int i = h;
    float f = h - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i) {
        case 0: rgb[0] = v; rgb[1] = t; rgb[2] = p; break;
        case 1: rgb[0] = q; rgb[1] = v; rgb[2] = p; break;
        case 2: rgb[0] = p; rgb[1] = v; rgb[2] = t; break;
        case 3: rgb[0] = p; rgb[1] = q; rgb[2] = v; break;
        case 4: rgb[0] = t; rgb[1] = p; rgb[2] = v; break;
        case 5: rgb[0] = v; rgb[1] = p; rgb[2] = q; break;
    }
}

static inline void hsv2rgb(Vec3 &rgb, Vec3 hsv)
{
    return hsv2rgb(rgb, hsv[0], hsv[1], hsv[2]);
}
