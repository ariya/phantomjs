/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2009 Google, Inc.
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

#ifndef RenderSVGViewportContainer_h
#define RenderSVGViewportContainer_h

#if ENABLE(SVG)
#include "RenderSVGContainer.h"

namespace WebCore {

// This is used for non-root <svg> elements and <marker> elements, neither of which are SVGTransformable
// thus we inherit from RenderSVGContainer instead of RenderSVGTransformableContainer
class RenderSVGViewportContainer : public RenderSVGContainer {
public:
    explicit RenderSVGViewportContainer(SVGStyledElement*);
    FloatRect viewport() const { return m_viewport; }

    bool isLayoutSizeChanged() const { return m_isLayoutSizeChanged; }
    virtual bool didTransformToRootUpdate() { return m_didTransformToRootUpdate; }

    virtual void determineIfLayoutSizeChanged();
    virtual void setNeedsTransformUpdate() { m_needsTransformUpdate = true; }

    virtual void paint(PaintInfo&, const LayoutPoint&) OVERRIDE;

private:
    virtual bool isSVGViewportContainer() const { return true; }
    virtual const char* renderName() const { return "RenderSVGViewportContainer"; }

    AffineTransform viewportTransform() const;
    virtual const AffineTransform& localToParentTransform() const { return m_localToParentTransform; }

    virtual void calcViewport();
    virtual bool calculateLocalTransform();

    virtual void applyViewportClip(PaintInfo&);
    virtual bool pointIsInsideViewportClip(const FloatPoint& pointInParent);

    FloatRect m_viewport;
    mutable AffineTransform m_localToParentTransform;
    bool m_didTransformToRootUpdate : 1;
    bool m_isLayoutSizeChanged : 1;
    bool m_needsTransformUpdate : 1;
};
  
inline RenderSVGViewportContainer* toRenderSVGViewportContainer(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isSVGViewportContainer());
    return static_cast<RenderSVGViewportContainer*>(object);
}

inline const RenderSVGViewportContainer* toRenderSVGViewportContainer(const RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isSVGViewportContainer());
    return static_cast<const RenderSVGViewportContainer*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderSVGViewportContainer(const RenderSVGViewportContainer*);

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // RenderSVGViewportContainer_h
