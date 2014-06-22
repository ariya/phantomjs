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
    explicit RenderInline(Element*);

    static RenderInline* createAnonymous(Document*);

    RenderObject* firstChild() const { ASSERT(children() == virtualChildren()); return children()->firstChild(); }
    RenderObject* lastChild() const { ASSERT(children() == virtualChildren()); return children()->lastChild(); }

    virtual void addChild(RenderObject* newChild, RenderObject* beforeChild = 0);

    Element* node() const { return toElement(RenderBoxModelObject::node()); }

    virtual LayoutUnit marginLeft() const;
    virtual LayoutUnit marginRight() const;
    virtual LayoutUnit marginTop() const;
    virtual LayoutUnit marginBottom() const;
    virtual LayoutUnit marginBefore(const RenderStyle* otherStyle = 0) const;
    virtual LayoutUnit marginAfter(const RenderStyle* otherStyle = 0) const;
    virtual LayoutUnit marginStart(const RenderStyle* otherStyle = 0) const;
    virtual LayoutUnit marginEnd(const RenderStyle* otherStyle = 0) const;

    virtual void absoluteRects(Vector<IntRect>&, const LayoutPoint& accumulatedOffset) const;
    virtual void absoluteQuads(Vector<FloatQuad>&, bool* wasFixed) const;

    virtual LayoutSize offsetFromContainer(RenderObject*, const LayoutPoint&, bool* offsetDependsOnPoint = 0) const;

    IntRect linesBoundingBox() const;
    LayoutRect linesVisualOverflowBoundingBox() const;

    InlineFlowBox* createAndAppendInlineFlowBox();

    void dirtyLineBoxes(bool fullLayout);
    void deleteLineBoxTree();

    RenderLineBoxList* lineBoxes() { return &m_lineBoxes; }
    const RenderLineBoxList* lineBoxes() const { return &m_lineBoxes; }

    InlineFlowBox* firstLineBox() const { return m_lineBoxes.firstLineBox(); }
    InlineFlowBox* lastLineBox() const { return m_lineBoxes.lastLineBox(); }
    InlineBox* firstLineBoxIncludingCulling() const { return alwaysCreateLineBoxes() ? firstLineBox() : culledInlineFirstLineBox(); }
    InlineBox* lastLineBoxIncludingCulling() const { return alwaysCreateLineBoxes() ? lastLineBox() : culledInlineLastLineBox(); }

    virtual RenderBoxModelObject* virtualContinuation() const { return continuation(); }
    RenderInline* inlineElementContinuation() const;

    virtual void updateDragState(bool dragOn);
    
    LayoutSize offsetForInFlowPositionedInline(const RenderBox* child) const;

    virtual void addFocusRingRects(Vector<IntRect>&, const LayoutPoint& additionalOffset, const RenderLayerModelObject* paintContainer = 0) OVERRIDE;
    void paintOutline(PaintInfo&, const LayoutPoint&);

    using RenderBoxModelObject::continuation;
    using RenderBoxModelObject::setContinuation;

    bool alwaysCreateLineBoxes() const { return m_alwaysCreateLineBoxes; }
    void setAlwaysCreateLineBoxes() { m_alwaysCreateLineBoxes = true; }
    void updateAlwaysCreateLineBoxes(bool fullLayout);

    virtual LayoutRect localCaretRect(InlineBox*, int, LayoutUnit* extraWidthToEndOfLine) OVERRIDE;

    bool hitTestCulledInline(const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset);

protected:
    virtual void willBeDestroyed();

    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle) OVERRIDE;

private:
    virtual RenderObjectChildList* virtualChildren() { return children(); }
    virtual const RenderObjectChildList* virtualChildren() const { return children(); }
    const RenderObjectChildList* children() const { return &m_children; }
    RenderObjectChildList* children() { return &m_children; }

    virtual const char* renderName() const;

    virtual bool isRenderInline() const { return true; }

    LayoutRect culledInlineVisualOverflowBoundingBox() const;
    InlineBox* culledInlineFirstLineBox() const;
    InlineBox* culledInlineLastLineBox() const;

    template<typename GeneratorContext>
    void generateLineBoxRects(GeneratorContext& yield) const;
    template<typename GeneratorContext>
    void generateCulledLineBoxRects(GeneratorContext& yield, const RenderInline* container) const;

    void addChildToContinuation(RenderObject* newChild, RenderObject* beforeChild);
    virtual void addChildIgnoringContinuation(RenderObject* newChild, RenderObject* beforeChild = 0);

    void splitInlines(RenderBlock* fromBlock, RenderBlock* toBlock, RenderBlock* middleBlock,
                      RenderObject* beforeChild, RenderBoxModelObject* oldCont);
    void splitFlow(RenderObject* beforeChild, RenderBlock* newBlockBox,
                   RenderObject* newChild, RenderBoxModelObject* oldCont);

    virtual void layout() { ASSERT_NOT_REACHED(); } // Do nothing for layout()

    virtual void paint(PaintInfo&, const LayoutPoint&);

    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction) OVERRIDE;

    virtual bool requiresLayer() const { return isInFlowPositioned() || createsGroup() || hasClipPath(); }

    virtual LayoutUnit offsetLeft() const;
    virtual LayoutUnit offsetTop() const;
    virtual LayoutUnit offsetWidth() const { return linesBoundingBox().width(); }
    virtual LayoutUnit offsetHeight() const { return linesBoundingBox().height(); }

    virtual LayoutRect clippedOverflowRectForRepaint(const RenderLayerModelObject* repaintContainer) const OVERRIDE;
    virtual LayoutRect rectWithOutlineForRepaint(const RenderLayerModelObject* repaintContainer, LayoutUnit outlineWidth) const OVERRIDE;
    virtual void computeRectForRepaint(const RenderLayerModelObject* repaintContainer, LayoutRect&, bool fixed) const OVERRIDE;

    virtual void mapLocalToContainer(const RenderLayerModelObject* repaintContainer, TransformState&, MapCoordinatesFlags = ApplyContainerFlip, bool* wasFixed = 0) const OVERRIDE;
    virtual const RenderObject* pushMappingToContainer(const RenderLayerModelObject* ancestorToStopAt, RenderGeometryMap&) const OVERRIDE;

    virtual VisiblePosition positionForPoint(const LayoutPoint&);

    virtual LayoutRect frameRectForStickyPositioning() const OVERRIDE { return linesBoundingBox(); }

    virtual IntRect borderBoundingBox() const
    {
        IntRect boundingBox = linesBoundingBox();
        return IntRect(0, 0, boundingBox.width(), boundingBox.height());
    }

    virtual InlineFlowBox* createInlineFlowBox(); // Subclassed by SVG and Ruby

    virtual void dirtyLinesFromChangedChild(RenderObject* child) { m_lineBoxes.dirtyLinesFromChangedChild(this, child); }

    virtual LayoutUnit lineHeight(bool firstLine, LineDirectionMode, LinePositionMode = PositionOnContainingLine) const;
    virtual int baselinePosition(FontBaseline, bool firstLine, LineDirectionMode, LinePositionMode = PositionOnContainingLine) const;
    
    virtual void childBecameNonInline(RenderObject* child);

    virtual void updateHitTestResult(HitTestResult&, const LayoutPoint&);

    virtual void imageChanged(WrappedImagePtr, const IntRect* = 0);

#if ENABLE(DASHBOARD_SUPPORT) || ENABLE(DRAGGABLE_REGION)
    virtual void addAnnotatedRegions(Vector<AnnotatedRegionValue>&);
#endif
    
    virtual void updateFromStyle() OVERRIDE;
    
    RenderInline* clone() const;

    void paintOutlineForLine(GraphicsContext*, const LayoutPoint&, const LayoutRect& prevLine, const LayoutRect& thisLine,
                             const LayoutRect& nextLine, const Color);
    RenderBoxModelObject* continuationBefore(RenderObject* beforeChild);

    RenderObjectChildList m_children;
    RenderLineBoxList m_lineBoxes;   // All of the line boxes created for this inline flow.  For example, <i>Hello<br>world.</i> will have two <i> line boxes.

    bool m_alwaysCreateLineBoxes : 1;
};

inline RenderInline* toRenderInline(RenderObject* object)
{ 
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isRenderInline());
    return static_cast<RenderInline*>(object);
}

inline const RenderInline* toRenderInline(const RenderObject* object)
{ 
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isRenderInline());
    return static_cast<const RenderInline*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderInline(const RenderInline*);

} // namespace WebCore

#endif // RenderInline_h
