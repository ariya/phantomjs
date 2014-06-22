/*
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) Research In Motion Limited 2010-2012. All rights reserved.
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
 */

#ifndef RenderSVGText_h
#define RenderSVGText_h

#if ENABLE(SVG)
#include "AffineTransform.h"
#include "RenderSVGBlock.h"
#include "SVGTextLayoutAttributesBuilder.h"

namespace WebCore {

class RenderSVGInlineText;
class SVGTextElement;
class RenderSVGInlineText;

class RenderSVGText : public RenderSVGBlock {
public:
    RenderSVGText(SVGTextElement*);
    virtual ~RenderSVGText();

    virtual bool isChildAllowed(RenderObject*, RenderStyle*) const;

    void setNeedsPositioningValuesUpdate() { m_needsPositioningValuesUpdate = true; }
    virtual void setNeedsTransformUpdate() { m_needsTransformUpdate = true; }
    void setNeedsTextMetricsUpdate() { m_needsTextMetricsUpdate = true; }
    virtual FloatRect repaintRectInLocalCoordinates() const;

    static RenderSVGText* locateRenderSVGTextAncestor(RenderObject*);
    static const RenderSVGText* locateRenderSVGTextAncestor(const RenderObject*);

    bool needsReordering() const { return m_needsReordering; }
    Vector<SVGTextLayoutAttributes*>& layoutAttributes() { return m_layoutAttributes; }

    void subtreeChildWasAdded(RenderObject*);
    void subtreeChildWillBeRemoved(RenderObject*, Vector<SVGTextLayoutAttributes*, 2>& affectedAttributes);
    void subtreeChildWasRemoved(const Vector<SVGTextLayoutAttributes*, 2>& affectedAttributes);
    void subtreeStyleDidChange(RenderSVGInlineText*);
    void subtreeTextDidChange(RenderSVGInlineText*);

private:
    virtual const char* renderName() const { return "RenderSVGText"; }
    virtual bool isSVGText() const { return true; }

    virtual void paint(PaintInfo&, const LayoutPoint&);
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction) OVERRIDE;
    virtual bool nodeAtFloatPoint(const HitTestRequest&, HitTestResult&, const FloatPoint& pointInParent, HitTestAction);
    virtual VisiblePosition positionForPoint(const LayoutPoint&);

    virtual bool requiresLayer() const { return false; }
    virtual void layout();

    virtual void absoluteQuads(Vector<FloatQuad>&, bool* wasFixed) const;

    virtual LayoutRect clippedOverflowRectForRepaint(const RenderLayerModelObject* repaintContainer) const OVERRIDE;
    virtual void computeRectForRepaint(const RenderLayerModelObject* repaintContainer, LayoutRect&, bool fixed = false) const OVERRIDE;
    virtual void computeFloatRectForRepaint(const RenderLayerModelObject* repaintContainer, FloatRect&, bool fixed = false) const OVERRIDE;

    virtual void mapLocalToContainer(const RenderLayerModelObject* repaintContainer, TransformState&, MapCoordinatesFlags = ApplyContainerFlip, bool* wasFixed = 0) const OVERRIDE;
    virtual const RenderObject* pushMappingToContainer(const RenderLayerModelObject* ancestorToStopAt, RenderGeometryMap&) const OVERRIDE;
    virtual void addChild(RenderObject* child, RenderObject* beforeChild = 0);
    virtual void removeChild(RenderObject*) OVERRIDE;
    virtual void willBeDestroyed() OVERRIDE;

    virtual FloatRect objectBoundingBox() const { return frameRect(); }
    virtual FloatRect strokeBoundingBox() const;

    virtual const AffineTransform& localToParentTransform() const { return m_localTransform; }
    virtual AffineTransform localTransform() const { return m_localTransform; }
    virtual RootInlineBox* createRootInlineBox();

    virtual RenderBlock* firstLineBlock() const;
    virtual void updateFirstLetter();

    bool shouldHandleSubtreeMutations() const;

    bool m_needsReordering : 1;
    bool m_needsPositioningValuesUpdate : 1;
    bool m_needsTransformUpdate : 1;
    bool m_needsTextMetricsUpdate : 1;
    AffineTransform m_localTransform;
    SVGTextLayoutAttributesBuilder m_layoutAttributesBuilder;
    Vector<SVGTextLayoutAttributes*> m_layoutAttributes;
};

inline RenderSVGText* toRenderSVGText(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isSVGText());
    return static_cast<RenderSVGText*>(object);
}

inline const RenderSVGText* toRenderSVGText(const RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isSVGText());
    return static_cast<const RenderSVGText*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderSVGText(const RenderSVGText*);

}

#endif // ENABLE(SVG)
#endif
