/*
 * LED Effect Framework
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
#include <unistd.h>
#include <vector>
#include <string.h>
#include <stdlib.h>

#include "nanoflann.h"  // Tiny KD-tree library
#include "svl/SVL.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

class EffectRunner;


// Abstract base class for one LED effect
class Effect {
public:
    class PixelInfo;
    class FrameInfo;
    class DebugInfo;

    /*
     * Calculate a pixel value, using floating point RGB in the nominal range [0, 1].
     *
     * The 'rgb' vector is initialized to (0, 0, 0). Caller is responsible for
     * clamping the results if necessary. This supports effects that layer with
     * other effects using full floating point precision and dynamic range.
     *
     * This function may be run in parallel, not run at all, or run multiple times
     * per pixel. It therefore can't have side-effects other than producing an RGB
     * value.
     */
    virtual void shader(Vec3& rgb, const PixelInfo& p) const = 0;

    /*
     * Serialized post-processing on one pixel. This runs after shader(), once
     * per mapped pixel, with the ability to modify Effect data. This shoudln't
     * be used for anything CPU-intensive, but some effects require closed-loop
     * feedback based on the calculated color.
     */
    virtual void postProcess(const Vec3& rgb, const PixelInfo& p);

    // Optional begin/end frame callbacks
    virtual void beginFrame(const FrameInfo& f);
    virtual void endFrame(const FrameInfo& f);

    // Optional callback, invoked once per second when verbose mode is enabled.
    // This can print parameters out to the console.
    virtual void debug(const DebugInfo& d);


    // Information about one LED pixel
    class PixelInfo {
    public:
        PixelInfo(unsigned index, const rapidjson::Value* layout);

        // Point coordinates
        Vec3 point;

        // Index in the framebuffer
        unsigned index;

        // Parsed JSON for this pixel's layout
        const rapidjson::Value* layout;

        // Is this pixel being used, or is it a placeholder?
        bool isMapped() const;

        // Look up data from the JSON layout
        const rapidjson::Value& get(const char *attribute) const;
        double getNumber(const char *attribute) const;
        double getArrayNumber(const char *attribute, int index) const;
        Vec2 getVec2(const char *attribute) const;
        Vec3 getVec3(const char *attribute) const;
    };

    typedef std::vector<PixelInfo> PixelInfoVec;
    typedef std::vector<PixelInfo>::const_iterator PixelInfoIter;

    // Information about one Effect frame
    class FrameInfo {
    public:
        FrameInfo();
        void init(const rapidjson::Value &layout);

        // Seconds passed since the last frame
        float timeDelta;

        // Info for every pixel
        PixelInfoVec pixels;

        // Model axis-aligned bounding box
        Vec3 modelMin, modelMax;

        // Radius measured from center
        Real modelRadius;

        // Calculated model info
        Vec3 modelCenter() const;
        Vec3 modelSize() const;
        Real distanceOutsideBoundingBox(Vec3 p) const;

        // K-D Tree, for fast spatial lookups

        typedef nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor< Real, FrameInfo >,
            FrameInfo, 3> IndexTree;

        typedef std::vector<std::pair<size_t, Real> > ResultSet_t;

        void radiusSearch(ResultSet_t& hits, Vec3 point, float radius) const;

        IndexTree tree;

        // Adapter functions for the K-D tree implementation

        inline size_t kdtree_get_point_count() const {
            return pixels.size();
        }

        inline Real kdtree_distance(const Real *p1, const size_t idx_p2, size_t size) const {
            Real d0 = p1[0] - pixels[idx_p2].point[0];
            Real d1 = p1[1] - pixels[idx_p2].point[1];
            Real d2 = p1[2] - pixels[idx_p2].point[2];
            return d0*d0 + d1*d1 + d2*d2;
        }

        Real kdtree_get_pt(const size_t idx, int dim) const {
            return pixels[idx].point[dim];
        }

        template <class BBOX> bool kdtree_get_bbox(BBOX &bb) const {
            bb[0].low  = modelMin[0];
            bb[1].low  = modelMin[1];
            bb[2].low  = modelMin[2];
            bb[0].high = modelMax[0];
            bb[1].high = modelMax[1];
            bb[2].high = modelMax[2];
            return true;
        }
    };

    // Information passed to debug() callbacks
    class DebugInfo {
    public:
        DebugInfo(EffectRunner &runner);

        EffectRunner &runner;
    };
};


/*****************************************************************************************
 *                                   Implementation
 *****************************************************************************************/


inline Effect::PixelInfo::PixelInfo(unsigned index, const rapidjson::Value* layout)
    : index(index), layout(layout)
{
    point = isMapped() ? getVec3("point") : Vec3(0, 0, 0);
}

inline bool Effect::PixelInfo::isMapped() const
{
    return layout && layout->IsObject();
}

inline const rapidjson::Value& Effect::PixelInfo::get(const char *attribute) const
{
    return (*layout)[attribute];
}

inline double Effect::PixelInfo::getNumber(const char *attribute) const
{
    const rapidjson::Value& n = get(attribute);
    return n.IsNumber() ? n.GetDouble() : 0.0;
}

inline double Effect::PixelInfo::getArrayNumber(const char *attribute, int index) const
{
    const rapidjson::Value& a = get(attribute);
    if (a.IsArray()) {
        const rapidjson::Value& b = a[index];
        if (b.IsNumber()) {
            return b.GetDouble();
        }
    }
    return 0.0;
}

inline Vec2 Effect::PixelInfo::getVec2(const char *attribute) const
{
    return Vec2( getArrayNumber(attribute, 0),
                 getArrayNumber(attribute, 1) );
}

inline Vec3 Effect::PixelInfo::getVec3(const char *attribute) const
{
    return Vec3( getArrayNumber(attribute, 0),
                 getArrayNumber(attribute, 1),
                 getArrayNumber(attribute, 2) );
}

inline Effect::FrameInfo::FrameInfo()
    : timeDelta(0), tree(3, *this)
{}

inline void Effect::FrameInfo::init(const rapidjson::Value &layout)
{
    timeDelta = 0;
    pixels.clear();

    // Create PixelInfo instances

    for (unsigned i = 0; i < layout.Size(); i++) {
        PixelInfo p(i, &layout[i]);
        pixels.push_back(p);
    }

    // Calculate min/max

    modelMin = modelMax = pixels[0].point;
    for (unsigned i = 1; i < pixels.size(); i++) {
        for (unsigned j = 0; j < 3; j++) {
            modelMin[j] = std::min(modelMin[j], pixels[i].point[j]);
            modelMax[j] = std::max(modelMax[j], pixels[i].point[j]);
        }
    }

    // Calculate radius

    modelRadius = 0;
    for (unsigned i = 0; i < pixels.size(); i++) {
        modelRadius = std::max(modelRadius, len(pixels[i].point - modelCenter()));
    }

    // Build K-D Tree index, for fast spatial lookups later

    tree.buildIndex();
}

inline Vec3 Effect::FrameInfo::modelCenter() const
{
    return (modelMin + modelMax) * 0.5;
}

inline Vec3 Effect::FrameInfo::modelSize() const
{
    return modelMax - modelMin;
}

inline Real Effect::FrameInfo::distanceOutsideBoundingBox(Vec3 p) const
{
    Real d = 0;

    for (unsigned j = 0; j < 3; j++) {
        d = std::max(d, modelMin[j] - p[j]);
        d = std::max(d, p[j] - modelMax[j]);
    }

    return d;
}

inline void Effect::FrameInfo::radiusSearch(ResultSet_t& hits, Vec3 point, float radius) const
{
    nanoflann::SearchParams params;
    params.sorted = false;
    tree.radiusSearch(&point[0], radius * radius, hits, params);
}

inline Effect::DebugInfo::DebugInfo(EffectRunner &runner)
    : runner(runner) {}


inline void Effect::beginFrame(const FrameInfo &f) {}
inline void Effect::endFrame(const FrameInfo &f) {}
inline void Effect::debug(const DebugInfo &f) {}
inline void Effect::postProcess(const Vec3& rgb, const PixelInfo& p) {}


static inline float sq(float a)
{
    // Fast square
    return a * a;
}

static inline Vec3 XZ(Vec2 v)
{
    // Put a 2D vector on the XZ plane
    return Vec3(v[0], 0, v[1]);
}
