/*
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2009 Google, Inc.
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

#ifndef RenderSVGForeignObject_h
#define RenderSVGForeignObject_h

#if ENABLE(SVG)
#include "AffineTransform.h"
#include "FloatPoint.h"
#include "FloatRect.h"
#include "RenderSVGBlock.h"

namespace WebCore {

class SVGForeignObjectElement;

class RenderSVGForeignObject : public RenderSVGBlock {
public:
    explicit RenderSVGForeignObject(SVGForeignObjectElement*);
    virtual ~RenderSVGForeignObject();

    virtual const char* renderName() const { return "RenderSVGForeignObject"; }

    virtual void paint(PaintInfo&, const LayoutPoint&);

    virtual LayoutRect clippedOverflowRectForRepaint(const RenderLayerModelObject* repaintContainer) const OVERRIDE;
    virtual void computeFloatRectForRepaint(const RenderLayerModelObject* repaintContainer, FloatRect&, bool fixed = false) const OVERRIDE;

    virtual bool requiresLayer() const { return false; }
    virtual void layout();

    virtual FloatRect objectBoundingBox() const { return FloatRect(FloatPoint(), m_viewport.size()); }
    virtual FloatRect strokeBoundingBox() const { return FloatRect(FloatPoint(), m_viewport.size()); }
    virtual FloatRect repaintRectInLocalCoordinates() const { return FloatRect(FloatPoint(), m_viewport.size()); }

    virtual bool nodeAtFloatPoint(const HitTestRequest&, HitTestResult&, const FloatPoint& pointInParent, HitTestAction);
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction) OVERRIDE;
    virtual bool isSVGForeignObject() const { return true; }

    virtual void mapLocalToContainer(const RenderLayerModelObject* repaintContainer, TransformState&, MapCoordinatesFlags = ApplyContainerFlip, bool* wasFixed = 0) const OVERRIDE;
    virtual const RenderObject* pushMappingToContainer(const RenderLayerModelObject* ancestorToStopAt, RenderGeometryMap&) const OVERRIDE;
    virtual void setNeedsTransformUpdate() { m_needsTransformUpdate = true; }

private:
    virtual void updateLogicalWidth() OVERRIDE;
    virtual void computeLogicalHeight(LayoutUnit logicalHeight, LayoutUnit logicalTop, LogicalExtentComputedValues&) const OVERRIDE;

    virtual const AffineTransform& localToParentTransform() const;
    virtual AffineTransform localTransform() const { return m_localTransform; }

    bool m_needsTransformUpdate : 1;
    FloatRect m_viewport;
    AffineTransform m_localTransform;
    mutable AffineTransform m_localToParentTransform;
};

}

#endif
#endif
