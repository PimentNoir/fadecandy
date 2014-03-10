/*
 * Simple 2D texture sampler.
 *
 * Textures are loaded from PNG files on disk,
 * and sampled with bilinear interpolation.
 *
 * Copyright (c) 2014 Micah Elizabeth Scott <micah@scanlime.org>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <math.h>
#include <stdio.h>
#include <vector>
#include <algorithm>

#include "picopng.h"
#include "svl/SVL.h"


class Texture {
public:
    Texture();
    Texture(const char *filename);

    bool load(const char *filename);
    bool load(std::vector<unsigned char> png);
    bool isLoaded();

    // Interpolated sampling. Texture coordinates in the range [0, 1]
    Vec3 sample(Vec2 texcoord);

    // Raw sampling, integer pixel coordinates.
    uint32_t sampleRGBA32(int x, int y);
    Vec3 sample(int x, int y);

private:
    unsigned long width, height;
    std::vector<unsigned char> pixels;
    void init();
};


/*****************************************************************************************
 *                                   Implementation
 *****************************************************************************************/


inline Texture::Texture()
{
    init();
}

inline Texture::Texture(const char *filename)
{
    load(filename);
}

inline void Texture::init()
{
    width = 0;
    height = 0;
}

inline bool Texture::load(const char *filename)
{
    init();

    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Can't open texture %s\n", filename);
        return false;
    }

    std::vector<unsigned char> buffer;
    fseek(f, 0L, SEEK_END);
    long size = ftell(f);
    fseek(f, 0L, SEEK_SET);

    if (size > 0) {
        buffer.resize((size_t)size);
        if (fread(&buffer[0], size, 1, f) == 1) {
            if (load(buffer)) {
                // Success
                fclose(f);
                return true;
            }
        }
    }

    fprintf(stderr, "Error loading texture PNG from %s\n", filename);
    fclose(f);
    return false;
}

inline bool Texture::load(std::vector<unsigned char> png)
{
    if (decodePNG(pixels, width, height, &png[0], png.size()) != 0) {
        init();
        return false;
    }

    return true;
}

inline bool Texture::isLoaded()
{
    return width && height;
}

inline uint32_t Texture::sampleRGBA32(int x, int y)
{
    if (!isLoaded()) {
        return 0;
    }

    x = std::max<int>(0, std::min<int>(width - 1, x));
    y = std::max<int>(0, std::min<int>(height - 1, y));
    return ((uint32_t*)(&pixels[0]))[ x + y * width ];
}

inline Vec3 Texture::sample(int x, int y)
{
    uint32_t rgba = sampleRGBA32(x, y);
    return Vec3(
        ((rgba      ) & 0xFF) / 255.0f,
        ((rgba >> 8 ) & 0xFF) / 255.0f,
        ((rgba >> 16) & 0xFF) / 255.0f );
}

inline Vec3 Texture::sample(Vec2 texcoord)
{
    float fx = texcoord[0] * width;
    float fy = texcoord[1] * height;

    int ix = fx;
    int iy = fy;

    // Sample four points
    Vec3 aa = sample(ix,     iy);
    Vec3 ba = sample(ix + 1, iy);
    Vec3 ab = sample(ix,     iy + 1);
    Vec3 bb = sample(ix + 1, iy + 1);

    // X interpolation
    Vec3 ca = aa + (ba - aa) * (fx - ix);
    Vec3 cb = ab + (bb - ab) * (fx - ix);

    // Y interpolation
    return ca + (cb - ca) * (fy - iy);
}
