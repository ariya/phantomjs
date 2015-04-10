/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2006 Apple Computer, Inc
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) 2011 Renata Hodovan <reni@webkit.org>
 * Copyright (C) 2011 University of Szeged
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

#ifndef RenderSVGShape_h
#define RenderSVGShape_h

#if ENABLE(SVG)
#include "AffineTransform.h"
#include "FloatRect.h"
#include "RenderSVGModelObject.h"
#include "SVGMarkerData.h"
#include "StrokeStyleApplier.h"
#include <wtf/OwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class FloatPoint;
class GraphicsContextStateSaver;
class RenderSVGContainer;
class RenderSVGPath;
class RenderSVGResource;
class SVGGraphicsElement;

class BoundingRectStrokeStyleApplier : public StrokeStyleApplier {
public:
    BoundingRectStrokeStyleApplier(const RenderObject* object, RenderStyle* style)
        : m_object(object)
        , m_style(style)
    {
        ASSERT(style);
        ASSERT(object);
    }

    void strokeStyle(GraphicsContext* context)
    {
        SVGRenderSupport::applyStrokeStyleToContext(context, m_style, m_object);
    }

private:
    const RenderObject* m_object;
    RenderStyle* m_style;
};

class RenderSVGShape : public RenderSVGModelObject {
public:
    explicit RenderSVGShape(SVGGraphicsElement*);
    RenderSVGShape(SVGGraphicsElement*, Path*, bool);
    virtual ~RenderSVGShape();

    void setNeedsShapeUpdate() { m_needsShapeUpdate = true; }
    virtual void setNeedsBoundariesUpdate() { m_needsBoundariesUpdate = true; }
    virtual bool needsBoundariesUpdate() OVERRIDE { return m_needsBoundariesUpdate; }
    virtual void setNeedsTransformUpdate() { m_needsTransformUpdate = true; }
    virtual void fillShape(GraphicsContext*) const;
    virtual void strokeShape(GraphicsContext*) const;

    bool hasPath() const { return m_path.get(); }
    Path& path() const
    {
        ASSERT(m_path);
        return *m_path;
    }

protected:
    virtual void updateShapeFromElement();
    virtual bool isEmpty() const;
    virtual bool shapeDependentStrokeContains(const FloatPoint&);
    virtual bool shapeDependentFillContains(const FloatPoint&, const WindRule) const;
    float strokeWidth() const;
    bool hasSmoothStroke() const;

    bool hasNonScalingStroke() const { return style()->svgStyle()->vectorEffect() == VE_NON_SCALING_STROKE; }
    AffineTransform nonScalingStrokeTransform() const;
    Path* nonScalingStrokePath(const Path*, const AffineTransform&) const;

    FloatRect m_fillBoundingBox;
    FloatRect m_strokeBoundingBox;

private:
    // Hit-detection separated for the fill and the stroke
    bool fillContains(const FloatPoint&, bool requiresFill = true, const WindRule fillRule = RULE_NONZERO);
    bool strokeContains(const FloatPoint&, bool requiresStroke = true);

    virtual FloatRect repaintRectInLocalCoordinates() const { return m_repaintBoundingBox; }
    virtual FloatRect repaintRectInLocalCoordinatesExcludingSVGShadow() const OVERRIDE { return m_repaintBoundingBoxExcludingShadow; }
    virtual const AffineTransform& localToParentTransform() const { return m_localTransform; }
    virtual AffineTransform localTransform() const { return m_localTransform; }

    virtual bool isSVGShape() const { return true; }
    virtual const char* renderName() const { return "RenderSVGShape"; }

    virtual void layout();
    virtual void paint(PaintInfo&, const LayoutPoint&);
    virtual void addFocusRingRects(Vector<IntRect>&, const LayoutPoint& additionalOffset, const RenderLayerModelObject* paintContainer = 0) OVERRIDE;

    virtual bool nodeAtFloatPoint(const HitTestRequest&, HitTestResult&, const FloatPoint& pointInParent, HitTestAction);

    virtual FloatRect objectBoundingBox() const { return m_fillBoundingBox; }
    virtual FloatRect strokeBoundingBox() const { return m_strokeBoundingBox; }
    FloatRect calculateObjectBoundingBox() const;
    FloatRect calculateStrokeBoundingBox() const;
    void updateRepaintBoundingBox();

    bool setupNonScalingStrokeContext(AffineTransform&, GraphicsContextStateSaver&);

    bool shouldGenerateMarkerPositions() const;
    FloatRect markerRect(float strokeWidth) const;
    void processMarkerPositions();

    void fillShape(RenderStyle*, GraphicsContext*);
    void strokeShape(RenderStyle*, GraphicsContext*);
    void fillAndStrokeShape(GraphicsContext*);
    void drawMarkers(PaintInfo&);

private:
    FloatRect m_repaintBoundingBox;
    FloatRect m_repaintBoundingBoxExcludingShadow;
    AffineTransform m_localTransform;
    OwnPtr<Path> m_path;
    Vector<MarkerPosition> m_markerPositions;

    bool m_needsBoundariesUpdate : 1;
    bool m_needsShapeUpdate : 1;
    bool m_needsTransformUpdate : 1;
};

inline RenderSVGShape* toRenderSVGShape(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isSVGShape());
    return static_cast<RenderSVGShape*>(object);
}

inline const RenderSVGShape* toRenderSVGShape(const RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isSVGShape());
    return static_cast<const RenderSVGShape*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderSVGShape(const RenderSVGShape*);

}

#endif // ENABLE(SVG)
#endif
