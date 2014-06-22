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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
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

#include "config.h"
#include "ShapeInfo.h"

#if ENABLE(CSS_SHAPES)

#include "RenderBlock.h"
#include "RenderBox.h"
#include "RenderRegion.h"
#include "RenderStyle.h"
#include "Shape.h"

namespace WebCore {
template<class RenderType, ShapeValue* (RenderStyle::*shapeGetter)() const, void (Shape::*intervalGetter)(LayoutUnit, LayoutUnit, SegmentList&) const>
const Shape* ShapeInfo<RenderType, shapeGetter, intervalGetter>::computedShape() const
{
    if (Shape* shape = m_shape.get())
        return shape;

    ShapeValue* shapeValue = (m_renderer->style()->*shapeGetter)();
    BasicShape* shape = (shapeValue && shapeValue->type() == ShapeValue::Shape) ? shapeValue->shape() : 0;

    ASSERT(shape);

    m_shape = Shape::createShape(shape, LayoutSize(m_shapeLogicalWidth, m_shapeLogicalHeight), m_renderer->style()->writingMode(), m_renderer->style()->shapeMargin(), m_renderer->style()->shapePadding());
    ASSERT(m_shape);
    return m_shape.get();
}

template<class RenderType, ShapeValue* (RenderStyle::*shapeGetter)() const, void (Shape::*intervalGetter)(LayoutUnit, LayoutUnit, SegmentList&) const>
bool ShapeInfo<RenderType, shapeGetter, intervalGetter>::computeSegmentsForLine(LayoutUnit lineTop, LayoutUnit lineHeight)
{
    ASSERT(lineHeight >= 0);
    m_shapeLineTop = lineTop - logicalTopOffset();
    m_lineHeight = lineHeight;
    m_segments.clear();

    if (lineOverlapsShapeBounds())
        (computedShape()->*intervalGetter)(m_shapeLineTop, std::min(m_lineHeight, shapeLogicalBottom() - lineTop), m_segments);

    LayoutUnit logicalLeftOffset = this->logicalLeftOffset();
    for (size_t i = 0; i < m_segments.size(); i++) {
        m_segments[i].logicalLeft += logicalLeftOffset;
        m_segments[i].logicalRight += logicalLeftOffset;
    }

    return m_segments.size();
}

template class ShapeInfo<RenderBlock, &RenderStyle::resolvedShapeInside, &Shape::getIncludedIntervals>;
template class ShapeInfo<RenderBox, &RenderStyle::shapeOutside, &Shape::getExcludedIntervals>;
}
#endif
