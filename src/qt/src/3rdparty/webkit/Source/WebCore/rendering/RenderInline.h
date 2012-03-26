/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef RenderInline_h
#define RenderInline_h

#include "InlineFlowBox.h"
#include "RenderBoxModelObject.h"
#include "RenderLineBoxList.h"

namespace WebCore {

class Position;

class RenderInline : public RenderBoxModelObject {
public:
    explicit RenderInline(Node*);

    virtual void destroy();

    virtual void addChild(RenderObject* newChild, RenderObject* beforeChild = 0);

    virtual int marginLeft() const;
    virtual int marginRight() const;
    virtual int marginTop() const;
    virtual int marginBottom() const;
    virtual int marginBefore() const;
    virtual int marginAfter() const;
    virtual int marginStart() const;
    virtual int marginEnd() const;

    virtual void absoluteRects(Vector<IntRect>&, int tx, int ty);
    virtual void absoluteQuads(Vector<FloatQuad>&);

    virtual IntSize offsetFromContainer(RenderObject*, const IntPoint&) const;

    IntRect linesBoundingBox() const;
    IntRect linesVisualOverflowBoundingBox() const;

    InlineFlowBox* createAndAppendInlineFlowBox();

    void dirtyLineBoxes(bool fullLayout);

    RenderLineBoxList* lineBoxes() { return &m_lineBoxes; }
    const RenderLineBoxList* lineBoxes() const { return &m_lineBoxes; }

    InlineFlowBox* firstLineBox() const { return m_lineBoxes.firstLineBox(); }
    InlineFlowBox* lastLineBox() const { return m_lineBoxes.lastLineBox(); }
    InlineBox* firstLineBoxIncludingCulling() const { return alwaysCreateLineBoxes() ? firstLineBox() : culledInlineFirstLineBox(); }
    InlineBox* lastLineBoxIncludingCulling() const { return alwaysCreateLineBoxes() ? lastLineBox() : culledInlineLastLineBox(); }

    virtual RenderBoxModelObject* virtualContinuation() const { return continuation(); }
    RenderInline* inlineElementContinuation() const;

    virtual void updateDragState(bool dragOn);
    
    IntSize relativePositionedInlineOffset(const RenderBox* child) const;

    virtual void addFocusRingRects(Vector<IntRect>&, int tx, int ty);
    void paintOutline(GraphicsContext*, int tx, int ty);

    using RenderBoxModelObject::continuation;
    using RenderBoxModelObject::setContinuation;

    bool alwaysCreateLineBoxes() const { return m_alwaysCreateLineBoxes; }
    void setAlwaysCreateLineBoxes() { m_alwaysCreateLineBoxes = true; }
    void updateAlwaysCreateLineBoxes();

protected:
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

private:
    virtual RenderObjectChildList* virtualChildren() { return children(); }
    virtual const RenderObjectChildList* virtualChildren() const { return children(); }
    const RenderObjectChildList* children() const { return &m_children; }
    RenderObjectChildList* children() { return &m_children; }

    virtual const char* renderName() const;

    virtual bool isRenderInline() const { return true; }

    FloatRect culledInlineBoundingBox(const RenderInline* container) const;
    IntRect culledInlineVisualOverflowBoundingBox() const;
    InlineBox* culledInlineFirstLineBox() const;
    InlineBox* culledInlineLastLineBox() const;
    void culledInlineAbsoluteRects(const RenderInline* container, Vector<IntRect>&, const IntSize&);
    void culledInlineAbsoluteQuads(const RenderInline* container, Vector<FloatQuad>&);

    void addChildToContinuation(RenderObject* newChild, RenderObject* beforeChild);
    virtual void addChildIgnoringContinuation(RenderObject* newChild, RenderObject* beforeChild = 0);

    void splitInlines(RenderBlock* fromBlock, RenderBlock* toBlock, RenderBlock* middleBlock,
                      RenderObject* beforeChild, RenderBoxModelObject* oldCont);
    void splitFlow(RenderObject* beforeChild, RenderBlock* newBlockBox,
                   RenderObject* newChild, RenderBoxModelObject* oldCont);

    virtual void layout() { ASSERT_NOT_REACHED(); } // Do nothing for layout()

    virtual void paint(PaintInfo&, int tx, int ty);

    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, HitTestAction);

    virtual bool requiresLayer() const { return isRelPositioned() || isTransparent() || hasMask(); }

    virtual int offsetLeft() const;
    virtual int offsetTop() const;
    virtual int offsetWidth() const { return linesBoundingBox().width(); }
    virtual int offsetHeight() const { return linesBoundingBox().height(); }

    virtual IntRect clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer);
    virtual IntRect rectWithOutlineForRepaint(RenderBoxModelObject* repaintContainer, int outlineWidth);
    virtual void computeRectForRepaint(RenderBoxModelObject* repaintContainer, IntRect& rect, bool fixed);

    virtual void mapLocalToContainer(RenderBoxModelObject* repaintContainer, bool fixed, bool useTransforms, TransformState&) const;
    virtual void mapAbsoluteToLocalPoint(bool fixed, bool useTransforms, TransformState&) const;

    virtual VisiblePosition positionForPoint(const IntPoint&);

    virtual IntRect borderBoundingBox() const
    {
        IntRect boundingBox = linesBoundingBox();
        return IntRect(0, 0, boundingBox.width(), boundingBox.height());
    }

    virtual InlineFlowBox* createInlineFlowBox(); // Subclassed by SVG and Ruby

    virtual void dirtyLinesFromChangedChild(RenderObject* child) { m_lineBoxes.dirtyLinesFromChangedChild(this, child); }

    virtual int lineHeight(bool firstLine, LineDirectionMode, LinePositionMode = PositionOnContainingLine) const;
    virtual int baselinePosition(FontBaseline, bool firstLine, LineDirectionMode, LinePositionMode = PositionOnContainingLine) const;
    
    virtual void childBecameNonInline(RenderObject* child);

    virtual void updateHitTestResult(HitTestResult&, const IntPoint&);

    virtual void imageChanged(WrappedImagePtr, const IntRect* = 0);

#if ENABLE(DASHBOARD_SUPPORT)
    virtual void addDashboardRegions(Vector<DashboardRegionValue>&);
#endif
    
    virtual void updateBoxModelInfoFromStyle();
    
    static RenderInline* cloneInline(RenderInline* src);

    void paintOutlineForLine(GraphicsContext*, int tx, int ty, const IntRect& prevLine, const IntRect& thisLine, const IntRect& nextLine);
    RenderBoxModelObject* continuationBefore(RenderObject* beforeChild);

    RenderObjectChildList m_children;
    RenderLineBoxList m_lineBoxes;   // All of the line boxes created for this inline flow.  For example, <i>Hello<br>world.</i> will have two <i> line boxes.

    mutable int m_lineHeight : 31;
    bool m_alwaysCreateLineBoxes : 1;
};

inline RenderInline* toRenderInline(RenderObject* object)
{ 
    ASSERT(!object || object->isRenderInline());
    return static_cast<RenderInline*>(object);
}

inline const RenderInline* toRenderInline(const RenderObject* object)
{ 
    ASSERT(!object || object->isRenderInline());
    return static_cast<const RenderInline*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderInline(const RenderInline*);

} // namespace WebCore

#endif // RenderInline_h
