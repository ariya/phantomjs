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

#ifndef ShapeInfo_h
#define ShapeInfo_h

#if ENABLE(CSS_SHAPES)

#include "FloatRect.h"
#include "LayoutUnit.h"
#include "RenderStyle.h"
#include "Shape.h"
#include "ShapeValue.h"
#include <wtf/OwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

template<class KeyType, class InfoType>
class MappedInfo {
public:
    static InfoType* ensureInfo(const KeyType* key)
    {
        InfoMap& infoMap = MappedInfo<KeyType, InfoType>::infoMap();
        if (InfoType* info = infoMap.get(key))
            return info;
        typename InfoMap::AddResult result = infoMap.add(key, InfoType::createInfo(key));
        return result.iterator->value.get();
    }
    static void removeInfo(const KeyType* key) { infoMap().remove(key); }
    static InfoType* info(const KeyType* key) { return infoMap().get(key); }
private:
    typedef HashMap<const KeyType*, OwnPtr<InfoType> > InfoMap;
    static InfoMap& infoMap()
    {
        DEFINE_STATIC_LOCAL(InfoMap, staticInfoMap, ());
        return staticInfoMap;
    }
};

template<class RenderType, ShapeValue* (RenderStyle::*shapeGetter)() const, void (Shape::*intervalGetter)(LayoutUnit, LayoutUnit, SegmentList&) const>
class ShapeInfo {
    WTF_MAKE_FAST_ALLOCATED;
public:
    virtual ~ShapeInfo() { }

    void setShapeSize(LayoutUnit logicalWidth, LayoutUnit logicalHeight)
    {
        if (m_renderer->style()->boxSizing() == CONTENT_BOX) {
            logicalWidth -= m_renderer->borderAndPaddingLogicalWidth();
            logicalHeight -= m_renderer->borderAndPaddingLogicalHeight();
        }

        if (m_shapeLogicalWidth == logicalWidth && m_shapeLogicalHeight == logicalHeight)
            return;
        dirtyShapeSize();
        m_shapeLogicalWidth = logicalWidth;
        m_shapeLogicalHeight = logicalHeight;
    }

    virtual bool computeSegmentsForLine(LayoutUnit lineTop, LayoutUnit lineHeight);
    void clearSegments() { m_segments.clear(); }

    LayoutUnit shapeLogicalTop() const { return computedShapeLogicalBoundingBox().y() + logicalTopOffset(); }
    LayoutUnit shapeLogicalBottom() const { return computedShapeLogicalBoundingBox().maxY() + logicalTopOffset(); }
    LayoutUnit shapeLogicalLeft() const { return computedShapeLogicalBoundingBox().x() + logicalLeftOffset(); }
    LayoutUnit shapeLogicalRight() const { return computedShapeLogicalBoundingBox().maxX() + logicalLeftOffset(); }
    LayoutUnit shapeLogicalWidth() const { return computedShapeLogicalBoundingBox().width(); }
    LayoutUnit shapeLogicalHeight() const { return computedShapeLogicalBoundingBox().height(); }

    LayoutUnit logicalLineTop() const { return m_shapeLineTop + logicalTopOffset(); }
    LayoutUnit logicalLineBottom() const { return m_shapeLineTop + m_lineHeight + logicalTopOffset(); }

    LayoutUnit shapeContainingBlockHeight() const { return (m_renderer->style()->boxSizing() == CONTENT_BOX) ? (m_shapeLogicalHeight + m_renderer->borderAndPaddingLogicalHeight()) : m_shapeLogicalHeight; }

    bool lineOverlapsShapeBounds() const { return logicalLineTop() < shapeLogicalBottom() && shapeLogicalTop() <= logicalLineBottom(); }

    void dirtyShapeSize() { m_shape.clear(); }
    bool shapeSizeDirty() { return !m_shape.get(); }
    const RenderType* owner() const { return m_renderer; }

protected:
    ShapeInfo(const RenderType* renderer): m_renderer(renderer) { }

    const Shape* computedShape() const;
    virtual LayoutRect computedShapeLogicalBoundingBox() const = 0;

    LayoutUnit logicalTopOffset() const { return m_renderer->style()->boxSizing() == CONTENT_BOX ? m_renderer->borderAndPaddingBefore() : LayoutUnit(); };
    LayoutUnit logicalLeftOffset() const { return (m_renderer->style()->boxSizing() == CONTENT_BOX && !m_renderer->isRenderRegion()) ? m_renderer->borderAndPaddingStart() : LayoutUnit(); }

    LayoutUnit m_shapeLineTop;
    LayoutUnit m_lineHeight;
    SegmentList m_segments;

    const RenderType* m_renderer;

private:
    mutable OwnPtr<Shape> m_shape;

    LayoutUnit m_shapeLogicalWidth;
    LayoutUnit m_shapeLogicalHeight;
};
}
#endif
#endif
