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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef ShapeOutsideInfo_h
#define ShapeOutsideInfo_h

#if ENABLE(CSS_SHAPES)

#include "LayoutSize.h"
#include "ShapeInfo.h"

namespace WebCore {

class RenderBox;

class ShapeOutsideInfo : public ShapeInfo<RenderBox, &RenderStyle::shapeOutside, &Shape::getExcludedIntervals>, public MappedInfo<RenderBox, ShapeOutsideInfo> {
public:
    LayoutUnit leftSegmentMarginBoxDelta() const { return m_leftSegmentMarginBoxDelta; }
    LayoutUnit rightSegmentMarginBoxDelta() const { return m_rightSegmentMarginBoxDelta; }

    bool computeSegmentsForContainingBlockLine(LayoutUnit lineTop, LayoutUnit floatTop, LayoutUnit lineHeight);
    virtual bool computeSegmentsForLine(LayoutUnit lineTop, LayoutUnit lineHeight) OVERRIDE;

    static PassOwnPtr<ShapeOutsideInfo> createInfo(const RenderBox* renderer) { return adoptPtr(new ShapeOutsideInfo(renderer)); }
    static bool isEnabledFor(const RenderBox*);

protected:
    virtual LayoutRect computedShapeLogicalBoundingBox() const OVERRIDE { return computedShape()->shapeMarginLogicalBoundingBox(); }

private:
    ShapeOutsideInfo(const RenderBox* renderer) : ShapeInfo<RenderBox, &RenderStyle::shapeOutside, &Shape::getExcludedIntervals>(renderer) { }

    LayoutUnit m_leftSegmentMarginBoxDelta;
    LayoutUnit m_rightSegmentMarginBoxDelta;
    LayoutUnit m_lineTop;
};

}
#endif
#endif
