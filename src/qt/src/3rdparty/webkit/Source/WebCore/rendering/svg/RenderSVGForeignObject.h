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

#if ENABLE(SVG) && ENABLE(SVG_FOREIGN_OBJECT)
#include "AffineTransform.h"
#include "FloatPoint.h"
#include "RenderSVGBlock.h"

namespace WebCore {

class SVGForeignObjectElement;

class RenderSVGForeignObject : public RenderSVGBlock {
public:
    explicit RenderSVGForeignObject(SVGForeignObjectElement*);
    virtual ~RenderSVGForeignObject();

    virtual const char* renderName() const { return "RenderSVGForeignObject"; }

    virtual void paint(PaintInfo&, int parentX, int parentY);

    virtual IntRect clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer);
    virtual void computeRectForRepaint(RenderBoxModelObject* repaintContainer, IntRect&, bool fixed = false);

    virtual bool requiresLayer() const { return false; }
    virtual void layout();

    virtual FloatRect objectBoundingBox() const { return m_viewport; }
    virtual FloatRect strokeBoundingBox() const { return m_viewport; }
    virtual FloatRect repaintRectInLocalCoordinates() const { return m_viewport; }

    virtual bool nodeAtFloatPoint(const HitTestRequest&, HitTestResult&, const FloatPoint& pointInParent, HitTestAction);
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, HitTestAction);
    virtual bool isSVGForeignObject() const { return true; }

    virtual void mapLocalToContainer(RenderBoxModelObject* repaintContainer, bool fixed , bool useTransforms, TransformState&) const;
    virtual void setNeedsTransformUpdate() { m_needsTransformUpdate = true; }

private:
    virtual void computeLogicalWidth();
    virtual void computeLogicalHeight();

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
