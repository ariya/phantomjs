/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2009 Google, Inc.  All rights reserved.
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef RenderSVGRoot_h
#define RenderSVGRoot_h

#if ENABLE(SVG)
#include "FloatRect.h"
#include "RenderBox.h"

#include "SVGRenderSupport.h"

namespace WebCore {

class SVGStyledElement;
class AffineTransform;

class RenderSVGRoot : public RenderBox {
public:
    explicit RenderSVGRoot(SVGStyledElement*);
    virtual ~RenderSVGRoot();

    const RenderObjectChildList* children() const { return &m_children; }
    RenderObjectChildList* children() { return &m_children; }

    bool isLayoutSizeChanged() const { return m_isLayoutSizeChanged; }
    virtual void setNeedsBoundariesUpdate() { m_needsBoundariesOrTransformUpdate = true; }
    virtual void setNeedsTransformUpdate() { m_needsBoundariesOrTransformUpdate = true; }

private:
    virtual RenderObjectChildList* virtualChildren() { return children(); }
    virtual const RenderObjectChildList* virtualChildren() const { return children(); }

    virtual bool isSVGRoot() const { return true; }
    virtual const char* renderName() const { return "RenderSVGRoot"; }

    virtual void computePreferredLogicalWidths();
    virtual int computeReplacedLogicalWidth(bool includeMaxWidth = true) const;
    virtual int computeReplacedLogicalHeight() const;
    virtual void layout();
    virtual void paint(PaintInfo&, int parentX, int parentY);

    virtual void destroy();
    virtual void styleWillChange(StyleDifference, const RenderStyle* newStyle);
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);
    virtual void updateFromElement();

    virtual const AffineTransform& localToParentTransform() const;

    bool fillContains(const FloatPoint&) const;
    bool strokeContains(const FloatPoint&) const;

    virtual FloatRect objectBoundingBox() const { return m_objectBoundingBox; }
    virtual FloatRect strokeBoundingBox() const { return m_strokeBoundingBox; }
    virtual FloatRect repaintRectInLocalCoordinates() const { return m_repaintBoundingBox; }

    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, HitTestAction);

    virtual IntRect clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer);
    virtual void computeRectForRepaint(RenderBoxModelObject* repaintContainer, IntRect& repaintRect, bool fixed);

    virtual void mapLocalToContainer(RenderBoxModelObject* repaintContainer, bool useTransforms, bool fixed, TransformState&) const;

    void calcViewport();

    bool selfWillPaint();
    void updateCachedBoundaries();

    IntSize parentOriginToBorderBox() const;
    IntSize borderOriginToContentBox() const;
    AffineTransform localToRepaintContainerTransform(const IntPoint& parentOriginInContainer) const;
    AffineTransform localToBorderBoxTransform() const;

    RenderObjectChildList m_children;
    FloatSize m_viewportSize;
    FloatRect m_objectBoundingBox;
    FloatRect m_strokeBoundingBox;
    FloatRect m_repaintBoundingBox;
    mutable AffineTransform m_localToParentTransform;
    bool m_isLayoutSizeChanged : 1;
    bool m_needsBoundariesOrTransformUpdate : 1;
};

inline RenderSVGRoot* toRenderSVGRoot(RenderObject* object)
{ 
    ASSERT(!object || object->isSVGRoot());
    return static_cast<RenderSVGRoot*>(object);
}

inline const RenderSVGRoot* toRenderSVGRoot(const RenderObject* object)
{ 
    ASSERT(!object || object->isSVGRoot());
    return static_cast<const RenderSVGRoot*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderSVGRoot(const RenderSVGRoot*);

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // RenderSVGRoot_h
