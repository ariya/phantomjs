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

#ifndef RenderSVGContainer_h
#define RenderSVGContainer_h

#if ENABLE(SVG)

#include "RenderSVGModelObject.h"

namespace WebCore {

class SVGElement;

class RenderSVGContainer : public RenderSVGModelObject {
public:
    explicit RenderSVGContainer(SVGStyledElement*);
    virtual ~RenderSVGContainer();

    const RenderObjectChildList* children() const { return &m_children; }
    RenderObjectChildList* children() { return &m_children; }

    virtual void paint(PaintInfo&, int parentX, int parentY);
    virtual void setNeedsBoundariesUpdate() { m_needsBoundariesUpdate = true; }

protected:
    virtual RenderObjectChildList* virtualChildren() { return children(); }
    virtual const RenderObjectChildList* virtualChildren() const { return children(); }

    virtual bool isSVGContainer() const { return true; }
    virtual const char* renderName() const { return "RenderSVGContainer"; }

    virtual void layout();

    virtual void addFocusRingRects(Vector<IntRect>&, int tx, int ty);

    virtual FloatRect objectBoundingBox() const { return m_objectBoundingBox; }
    virtual FloatRect strokeBoundingBox() const { return m_strokeBoundingBox; }
    virtual FloatRect repaintRectInLocalCoordinates() const { return m_repaintBoundingBox; }

    virtual bool nodeAtFloatPoint(const HitTestRequest&, HitTestResult&, const FloatPoint& pointInParent, HitTestAction);

    // Allow RenderSVGTransformableContainer to hook in at the right time in layout()
    virtual bool calculateLocalTransform() { return false; }

    // Allow RenderSVGViewportContainer to hook in at the right times in layout(), paint() and nodeAtFloatPoint()
    virtual void calcViewport() { }
    virtual void applyViewportClip(PaintInfo&) { }
    virtual bool pointIsInsideViewportClip(const FloatPoint& /*pointInParent*/) { return true; }

    bool selfWillPaint();
    void updateCachedBoundaries();

private:
    RenderObjectChildList m_children;
    FloatRect m_objectBoundingBox;
    FloatRect m_strokeBoundingBox;
    FloatRect m_repaintBoundingBox;
    bool m_needsBoundariesUpdate : 1;
};
  
inline RenderSVGContainer* toRenderSVGContainer(RenderObject* object)
{
    // Note: isSVGContainer is also true for RenderSVGViewportContainer, which is not derived from this.
    ASSERT(!object || (object->isSVGContainer() && strcmp(object->renderName(), "RenderSVGViewportContainer")));
    return static_cast<RenderSVGContainer*>(object);
}

inline const RenderSVGContainer* toRenderSVGContainer(const RenderObject* object)
{
    // Note: isSVGContainer is also true for RenderSVGViewportContainer, which is not derived from this.
    ASSERT(!object || (object->isSVGContainer() && strcmp(object->renderName(), "RenderSVGViewportContainer")));
    return static_cast<const RenderSVGContainer*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderSVGContainer(const RenderSVGContainer*);

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // RenderSVGContainer_h
