/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ShapeInterval.h"

#include <wtf/MathExtras.h>

namespace WebCore {

struct IntervalX1Comparator {
    bool operator() (const ShapeInterval& i1, const ShapeInterval& i2) const
    {
        return i1.x1 < i2.x1;
    }
};

bool ShapeInterval::intersect(const ShapeInterval& i, ShapeInterval& rv) const
{
    if (x2 < i.x1 || x1 > i.x2)
        return false;
    rv.x1 = std::max(x1, i.x1);
    rv.x2 = std::min(x2, i.x2);
    return true;
}

void sortShapeIntervals(Vector<ShapeInterval>& v)
{
    std::sort(v.begin(), v.end(), IntervalX1Comparator());
}

void mergeShapeIntervals(const Vector<ShapeInterval>& v1, const Vector<ShapeInterval>& v2, Vector<ShapeInterval>& rv)
{
    if (!v1.size())
        rv.appendRange(v2.begin(), v2.end());
    else if (!v2.size())
        rv.appendRange(v1.begin(), v1.end());
    else {
        Vector<ShapeInterval> v(v1.size() + v2.size());
        ShapeInterval* interval = 0;

        std::merge(v1.begin(), v1.end(), v2.begin(), v2.end(), v.begin(), IntervalX1Comparator());

        for (size_t i = 0; i < v.size(); i++) {
            if (!interval)
                interval = &v[i];
            else if (v[i].x1 >= interval->x1 && v[i].x1 <= interval->x2) // FIXME: 1st <= test not needed?
                interval->x2 = std::max(interval->x2, v[i].x2);
            else {
                rv.append(*interval);
                interval = &v[i];
            }
        }

        if (interval)
            rv.append(*interval);
    }
}

void intersectShapeIntervals(const Vector<ShapeInterval>& v1, const Vector<ShapeInterval>& v2, Vector<ShapeInterval>& rv)
{
    size_t v1Size = v1.size();
    size_t v2Size = v2.size();

    if (!v1Size || !v2Size)
        return;

    ShapeInterval interval;
    bool overlap = false;
    size_t i1 = 0;
    size_t i2 = 0;

    while (i1 < v1Size && i2 < v2Size) {
        ShapeInterval v12;
        if (v1[i1].intersect(v2[i2], v12)) {
            if (!overlap || !v12.intersect(interval, interval)) {
                if (overlap)
                    rv.append(interval);
                interval = v12;
                overlap = true;
            }
            if (v1[i1].x2 < v2[i2].x2)
                i1++;
            else
                i2++;
        } else {
            if (overlap)
                rv.append(interval);
            overlap = false;
            if (v1[i1].x1 < v2[i2].x1)
                i1++;
            else
                i2++;
        }
    }

    if (overlap)
        rv.append(interval);
}

void subtractShapeIntervals(const Vector<ShapeInterval>& v1, const Vector<ShapeInterval>& v2, Vector<ShapeInterval>& rv)
{
    size_t v1Size = v1.size();
    size_t v2Size = v2.size();

    if (!v1Size)
        return;

    if (!v2Size)
        rv.appendRange(v1.begin(), v1.end());
    else {
        size_t i1 = 0, i2 = 0;
        rv.appendRange(v1.begin(), v1.end());

        while (i1 < rv.size() && i2 < v2Size) {
            ShapeInterval& interval1 = rv[i1];
            const ShapeInterval& interval2 = v2[i2];

            if (interval2.x1 <= interval1.x1 && interval2.x2 >= interval1.x2)
                rv.remove(i1);
            else if (interval2.x2 < interval1.x1)
                i2 += 1;
            else if (interval2.x1 > interval1.x2)
                i1 += 1;
            else if (interval2.x1 > interval1.x1 && interval2.x2 < interval1.x2) {
                rv.insert(i1, ShapeInterval(interval1.x1, interval2.x1));
                interval1.x1 = interval2.x2;
                i2 += 1;
            } else if (interval2.x1 <= interval1.x1) {
                interval1.x1 = interval2.x2;
                i2 += 1;
            } else  { // (interval2.x2 >= interval1.x2)
                interval1.x2 = interval2.x1;
                i1 += 1;
            }
        }
    }
}

} // namespace WebCore
