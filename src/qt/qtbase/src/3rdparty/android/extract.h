/*
 * Copyright (C) 2005 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EXTRACT_H
#define EXTRACT_H

#include <cstdint>
#include <cstring>
// shamelessly stolen from ResourceTypes.h Android's sources
/**
 * This chunk specifies how to split an image into segments for
 * scaling.
 *
 * There are J horizontal and K vertical segments.  These segments divide
 * the image into J*K regions as follows (where J=4 and K=3):
 *
 *      F0   S0    F1     S1
 *   +-----+----+------+-------+
 * S2|  0  |  1 |  2   |   3   |
 *   +-----+----+------+-------+
 *   |     |    |      |       |
 *   |     |    |      |       |
 * F2|  4  |  5 |  6   |   7   |
 *   |     |    |      |       |
 *   |     |    |      |       |
 *   +-----+----+------+-------+
 * S3|  8  |  9 |  10  |   11  |
 *   +-----+----+------+-------+
 *
 * Each horizontal and vertical segment is considered to by either
 * stretchable (marked by the Sx labels) or fixed (marked by the Fy
 * labels), in the horizontal or vertical axis, respectively. In the
 * above example, the first is horizontal segment (F0) is fixed, the
 * next is stretchable and then they continue to alternate. Note that
 * the segment list for each axis can begin or end with a stretchable
 * or fixed segment.
 *
 * The relative sizes of the stretchy segments indicates the relative
 * amount of stretchiness of the regions bordered by the segments.  For
 * example, regions 3, 7 and 11 above will take up more horizontal space
 * than regions 1, 5 and 9 since the horizontal segment associated with
 * the first set of regions is larger than the other set of regions.  The
 * ratios of the amount of horizontal (or vertical) space taken by any
 * two stretchable slices is exactly the ratio of their corresponding
 * segment lengths.
 *
 * xDivs and yDivs point to arrays of horizontal and vertical pixel
 * indices.  The first pair of Divs (in either array) indicate the
 * starting and ending points of the first stretchable segment in that
 * axis. The next pair specifies the next stretchable segment, etc. So
 * in the above example xDiv[0] and xDiv[1] specify the horizontal
 * coordinates for the regions labeled 1, 5 and 9.  xDiv[2] and
 * xDiv[3] specify the coordinates for regions 3, 7 and 11. Note that
 * the leftmost slices always start at x=0 and the rightmost slices
 * always end at the end of the image. So, for example, the regions 0,
 * 4 and 8 (which are fixed along the X axis) start at x value 0 and
 * go to xDiv[0] and slices 2, 6 and 10 start at xDiv[1] and end at
 * xDiv[2].
 *
 * The array pointed to by the colors field lists contains hints for
 * each of the regions.  They are ordered according left-to-right and
 * top-to-bottom as indicated above. For each segment that is a solid
 * color the array entry will contain that color value; otherwise it
 * will contain NO_COLOR.  Segments that are completely transparent
 * will always have the value TRANSPARENT_COLOR.
 *
 * The PNG chunk type is "npTc".
 */
struct Res_png_9patch
{
    Res_png_9patch() : wasDeserialized(false), xDivs(NULL),
                       yDivs(NULL), colors(NULL) { }

    int8_t wasDeserialized;
    int8_t numXDivs;
    int8_t numYDivs;
    int8_t numColors;

    // These tell where the next section of a patch starts.
    // For example, the first patch includes the pixels from
    // 0 to xDivs[0]-1 and the second patch includes the pixels
    // from xDivs[0] to xDivs[1]-1.
    // Note: allocation/free of these pointers is left to the caller.
    int32_t* xDivs;
    int32_t* yDivs;

    int32_t paddingLeft, paddingRight;
    int32_t paddingTop, paddingBottom;

    enum {
        // The 9 patch segment is not a solid color.
        NO_COLOR = 0x00000001,

        // The 9 patch segment is completely transparent.
        TRANSPARENT_COLOR = 0x00000000
    };
    // Note: allocation/free of this pointer is left to the caller.
    uint32_t* colors;

    // Deserialize/Unmarshall the patch data
    static Res_png_9patch* deserialize(const void* data);
};

struct Res_png_9patch20
{
    Res_png_9patch20() : wasDeserialized(false), numXDivs(0), numYDivs(0), numColors(0), xDivsOffset(0),
                       yDivsOffset(0),paddingLeft(0), paddingRight(0), paddingTop(0), paddingBottom(0),
                       colorsOffset(0) { }

    int8_t wasDeserialized;
    int8_t numXDivs;
    int8_t numYDivs;
    int8_t numColors;

    // The offset (from the start of this structure) to the xDivs & yDivs
    // array for this 9patch. To get a pointer to this array, call
    // getXDivs or getYDivs. Note that the serialized form for 9patches places
    // the xDivs, yDivs and colors arrays immediately after the location
    // of the Res_png_9patch struct.
    uint32_t xDivsOffset;
    uint32_t yDivsOffset;

    int32_t paddingLeft, paddingRight;
    int32_t paddingTop, paddingBottom;

    enum {
        // The 9 patch segment is not a solid color.
        NO_COLOR = 0x00000001,

        // The 9 patch segment is completely transparent.
        TRANSPARENT_COLOR = 0x00000000
    };

    // The offset (from the start of this structure) to the colors array
    // for this 9patch.
    uint32_t colorsOffset;

    // Deserialize/Unmarshall the patch data
    static Res_png_9patch20* deserialize(void* data);

    // These tell where the next section of a patch starts.
    // For example, the first patch includes the pixels from
    // 0 to xDivs[0]-1 and the second patch includes the pixels
    // from xDivs[0] to xDivs[1]-1.
    inline int32_t* getXDivs() const {
        return reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(this) + xDivsOffset);
    }
    inline int32_t* getYDivs() const {
        return reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(this) + yDivsOffset);
    }
    inline uint32_t* getColors() const {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(this) + colorsOffset);
    }

} __attribute__((packed));

#endif
