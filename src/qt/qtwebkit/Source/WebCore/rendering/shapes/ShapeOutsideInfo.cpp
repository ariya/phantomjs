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

#include "config.h"

#if ENABLE(CSS_SHAPES)

#include "ShapeOutsideInfo.h"

#include "RenderBox.h"

namespace WebCore {
bool ShapeOutsideInfo::isEnabledFor(const RenderBox* box)
{
    ShapeValue* value = box->style()->shapeOutside();
    return box->isFloatingWithShapeOutside() && value->type() == ShapeValue::Shape && value->shape();
}

bool ShapeOutsideInfo::computeSegmentsForContainingBlockLine(LayoutUnit lineTop, LayoutUnit floatTop, LayoutUnit lineHeight)
{
    LayoutUnit lineTopInShapeCoordinates = lineTop - floatTop + logicalTopOffset();
    return computeSegmentsForLine(lineTopInShapeCoordinates, lineHeight);
}

bool ShapeOutsideInfo::computeSegmentsForLine(LayoutUnit lineTop, LayoutUnit lineHeight)
{
    if (shapeSizeDirty() || m_lineTop != lineTop || m_lineHeight != lineHeight) {
        if (ShapeInfo<RenderBox, &RenderStyle::shapeOutside, &Shape::getExcludedIntervals>::computeSegmentsForLine(lineTop, lineHeight)) {
            m_leftSegmentMarginBoxDelta = m_segments[0].logicalLeft + m_renderer->marginStart();
            m_rightSegmentMarginBoxDelta = m_segments[m_segments.size()-1].logicalRight - m_renderer->logicalWidth() - m_renderer->marginEnd();
        } else {
            m_leftSegmentMarginBoxDelta = m_renderer->logicalWidth() + m_renderer->marginStart();
            m_rightSegmentMarginBoxDelta = -m_renderer->logicalWidth() - m_renderer->marginEnd();
        }
        m_lineTop = lineTop;
    }

    return m_segments.size();
}

}
#endif
