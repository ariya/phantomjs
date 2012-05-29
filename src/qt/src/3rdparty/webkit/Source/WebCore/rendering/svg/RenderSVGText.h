/*
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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
#include "SVGTextLayoutAttributes.h"

namespace WebCore {

class SVGTextElement;

class RenderSVGText : public RenderSVGBlock {
public:
    RenderSVGText(SVGTextElement*);

    virtual bool isChildAllowed(RenderObject*, RenderStyle*) const;

    void setNeedsPositioningValuesUpdate() { m_needsPositioningValuesUpdate = true; }
    virtual void setNeedsTransformUpdate() { m_needsTransformUpdate = true; }
    virtual FloatRect repaintRectInLocalCoordinates() const;

    static RenderSVGText* locateRenderSVGTextAncestor(RenderObject*);
    static const RenderSVGText* locateRenderSVGTextAncestor(const RenderObject*);

    Vector<SVGTextLayoutAttributes>& layoutAttributes() { return m_layoutAttributes; }
    bool needsReordering() const { return m_needsReordering; }

private:
    virtual const char* renderName() const { return "RenderSVGText"; }
    virtual bool isSVGText() const { return true; }

    virtual void paint(PaintInfo&, int tx, int ty);
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, HitTestAction);
    virtual bool nodeAtFloatPoint(const HitTestRequest&, HitTestResult&, const FloatPoint& pointInParent, HitTestAction);
    virtual VisiblePosition positionForPoint(const IntPoint&);

    virtual bool requiresLayer() const { return false; }
    virtual void layout();

    virtual void absoluteQuads(Vector<FloatQuad>&);

    virtual IntRect clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer);
    virtual void computeRectForRepaint(RenderBoxModelObject* repaintContainer, IntRect&, bool fixed = false);

    virtual void mapLocalToContainer(RenderBoxModelObject* repaintContainer, bool useTransforms, bool fixed, TransformState&) const;

    virtual FloatRect objectBoundingBox() const { return frameRect(); }
    virtual FloatRect strokeBoundingBox() const;

    virtual const AffineTransform& localToParentTransform() const { return m_localTransform; }
    virtual AffineTransform localTransform() const { return m_localTransform; }
    virtual RootInlineBox* createRootInlineBox();

    virtual RenderBlock* firstLineBlock() const;
    virtual void updateFirstLetter();

    bool m_needsReordering : 1;
    bool m_needsPositioningValuesUpdate : 1;
    bool m_needsTransformUpdate : 1;
    AffineTransform m_localTransform;
    Vector<SVGTextLayoutAttributes> m_layoutAttributes;
};

inline RenderSVGText* toRenderSVGText(RenderObject* object)
{
    ASSERT(!object || object->isSVGText());
    return static_cast<RenderSVGText*>(object);
}

inline const RenderSVGText* toRenderSVGText(const RenderObject* object)
{
    ASSERT(!object || object->isSVGText());
    return static_cast<const RenderSVGText*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderSVGText(const RenderSVGText*);

}

#endif // ENABLE(SVG)
#endif
