/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2006 Apple Computer, Inc
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

#ifndef RenderSVGPath_h
#define RenderSVGPath_h

#if ENABLE(SVG)
#include "AffineTransform.h"
#include "FloatRect.h"
#include "RenderSVGModelObject.h"
#include "SVGMarkerLayoutInfo.h"

namespace WebCore {

class FloatPoint;
class RenderSVGContainer;
class SVGStyledTransformableElement;

class RenderSVGPath : public RenderSVGModelObject {
public:
    explicit RenderSVGPath(SVGStyledTransformableElement*);
    virtual ~RenderSVGPath();

    const Path& path() const { return m_path; }
    void setNeedsPathUpdate() { m_needsPathUpdate = true; }
    virtual void setNeedsBoundariesUpdate() { m_needsBoundariesUpdate = true; }
    virtual void setNeedsTransformUpdate() { m_needsTransformUpdate = true; }

private:
    // Hit-detection seperated for the fill and the stroke
    bool fillContains(const FloatPoint&, bool requiresFill = true, WindRule fillRule = RULE_NONZERO);
    bool strokeContains(const FloatPoint&, bool requiresStroke = true);

    virtual FloatRect objectBoundingBox() const { return m_fillBoundingBox; }
    virtual FloatRect strokeBoundingBox() const { return m_strokeAndMarkerBoundingBox; }
    virtual FloatRect repaintRectInLocalCoordinates() const { return m_repaintBoundingBox; }
    virtual const AffineTransform& localToParentTransform() const { return m_localTransform; }

    virtual bool isSVGPath() const { return true; }
    virtual const char* renderName() const { return "RenderSVGPath"; }

    virtual void layout();
    virtual void paint(PaintInfo&, int parentX, int parentY);
    virtual void addFocusRingRects(Vector<IntRect>&, int tx, int ty);

    virtual bool nodeAtFloatPoint(const HitTestRequest&, HitTestResult&, const FloatPoint& pointInParent, HitTestAction);

    FloatRect calculateMarkerBoundsIfNeeded();
    void updateCachedBoundaries();

private:
    virtual AffineTransform localTransform() const { return m_localTransform; }
    void fillAndStrokePath(GraphicsContext*);

    bool m_needsBoundariesUpdate : 1;
    bool m_needsPathUpdate : 1;
    bool m_needsTransformUpdate : 1;

    mutable Path m_path;
    FloatRect m_fillBoundingBox;
    FloatRect m_strokeAndMarkerBoundingBox;
    FloatRect m_repaintBoundingBox;
    SVGMarkerLayoutInfo m_markerLayoutInfo;
    AffineTransform m_localTransform;
};

inline RenderSVGPath* toRenderSVGPath(RenderObject* object)
{
    ASSERT(!object || object->isSVGPath());
    return static_cast<RenderSVGPath*>(object);
}

inline const RenderSVGPath* toRenderSVGPath(const RenderObject* object)
{
    ASSERT(!object || object->isSVGPath());
    return static_cast<const RenderSVGPath*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderSVGPath(const RenderSVGPath*);

}

#endif // ENABLE(SVG)
#endif
