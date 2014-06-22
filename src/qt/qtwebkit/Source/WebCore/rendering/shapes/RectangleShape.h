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

#ifndef RectangleShape_h
#define RectangleShape_h

#include "FloatPoint.h"
#include "FloatRect.h"
#include "FloatSize.h"
#include "Shape.h"
#include <wtf/Assertions.h>
#include <wtf/Vector.h>

namespace WebCore {

class FloatRoundedRect : public FloatRect {
public:
    FloatRoundedRect() { }
    FloatRoundedRect(const FloatRect& bounds, const FloatSize& radii)
        : FloatRect(bounds)
        , m_radii(radii)
    {
    }

    float rx() const { return m_radii.width(); }
    float ry() const { return m_radii.height(); }
    FloatRoundedRect marginBounds(float margin) const;
    FloatRoundedRect paddingBounds(float padding) const;
    FloatPoint cornerInterceptForWidth(float width) const;

private:
    FloatSize m_radii;
};

class RectangleShape : public Shape {
public:
    RectangleShape(const FloatRect& bounds, const FloatSize& radii)
        : Shape()
        , m_bounds(bounds, radii)
        , m_haveInitializedMarginBounds(false)
        , m_haveInitializedPaddingBounds(false)
    {
    }

    virtual LayoutRect shapeMarginLogicalBoundingBox() const OVERRIDE { return static_cast<LayoutRect>(shapeMarginBounds()); }
    virtual LayoutRect shapePaddingLogicalBoundingBox() const OVERRIDE { return static_cast<LayoutRect>(shapePaddingBounds()); }
    virtual bool isEmpty() const OVERRIDE { return m_bounds.isEmpty(); }
    virtual void getExcludedIntervals(LayoutUnit logicalTop, LayoutUnit logicalHeight, SegmentList&) const OVERRIDE;
    virtual void getIncludedIntervals(LayoutUnit logicalTop, LayoutUnit logicalHeight, SegmentList&) const OVERRIDE;
    virtual bool firstIncludedIntervalLogicalTop(LayoutUnit minLogicalIntervalTop, const LayoutSize& minLogicalIntervalSize, LayoutUnit&) const OVERRIDE;

private:
    FloatRoundedRect shapeMarginBounds() const;
    FloatRoundedRect shapePaddingBounds() const;

    FloatRoundedRect m_bounds;
    mutable FloatRoundedRect m_marginBounds;
    mutable FloatRoundedRect m_paddingBounds;
    mutable bool m_haveInitializedMarginBounds : 1;
    mutable bool m_haveInitializedPaddingBounds : 1;
};

} // namespace WebCore

#endif // RectangleShape_h
