/*
 * Copyright (C) 2006 Alexander Kellett <lypanov@kde.org>
 * Copyright (C) 2006, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
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

#ifndef RenderSVGImage_h
#define RenderSVGImage_h

#if ENABLE(SVG)
#include "AffineTransform.h"
#include "FloatRect.h"
#include "RenderSVGModelObject.h"
#include "SVGPreserveAspectRatio.h"
#include "SVGRenderSupport.h"

namespace WebCore {

class RenderImageResource;
class SVGImageElement;

class RenderSVGImage : public RenderSVGModelObject {
public:
    RenderSVGImage(SVGImageElement*);
    virtual ~RenderSVGImage();

    virtual void setNeedsTransformUpdate() { m_needsTransformUpdate = true; }
    virtual void updateFromElement();

    RenderImageResource* imageResource() { return m_imageResource.get(); }
    const RenderImageResource* imageResource() const { return m_imageResource.get(); }

private:
    virtual const char* renderName() const { return "RenderSVGImage"; }
    virtual bool isSVGImage() const { return true; }

    virtual const AffineTransform& localToParentTransform() const { return m_localTransform; }

    virtual FloatRect objectBoundingBox() const { return m_objectBoundingBox; }
    virtual FloatRect strokeBoundingBox() const { return m_objectBoundingBox; }
    virtual FloatRect repaintRectInLocalCoordinates() const { return m_repaintBoundingBox; }

    virtual void addFocusRingRects(Vector<IntRect>&, int tx, int ty);

    virtual void imageChanged(WrappedImagePtr, const IntRect* = 0);

    virtual void layout();
    virtual void paint(PaintInfo&, int parentX, int parentY);

    virtual bool requiresLayer() const { return false; }

    virtual bool nodeAtFloatPoint(const HitTestRequest&, HitTestResult&, const FloatPoint& pointInParent, HitTestAction);

    virtual AffineTransform localTransform() const { return m_localTransform; }

    bool m_updateCachedRepaintRect : 1;
    bool m_needsTransformUpdate : 1;
    AffineTransform m_localTransform;
    FloatRect m_objectBoundingBox;
    FloatRect m_repaintBoundingBox;
    OwnPtr<RenderImageResource> m_imageResource;
};

inline RenderSVGImage* toRenderSVGImage(RenderObject* object)
{
    ASSERT(!object || object->isSVGImage());
    return static_cast<RenderSVGImage*>(object);
}

inline const RenderSVGImage* toRenderSVGImage(const RenderObject* object)
{
    ASSERT(!object || object->isSVGImage());
    return static_cast<const RenderSVGImage*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderSVGImage(const RenderSVGImage*);

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // RenderSVGImage_h
