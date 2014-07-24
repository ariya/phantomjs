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

#ifndef RenderSVGPath_h
#define RenderSVGPath_h

#if ENABLE(SVG)
#include "RenderSVGShape.h"

namespace WebCore {

class RenderSVGPath : public RenderSVGShape {
public:
    explicit RenderSVGPath(SVGGraphicsElement*);
    virtual ~RenderSVGPath();

private:
    virtual bool isSVGPath() const OVERRIDE { return true; }
    virtual const char* renderName() const { return "RenderSVGPath"; }

    virtual void updateShapeFromElement() OVERRIDE;
    FloatRect calculateUpdatedStrokeBoundingBox() const;

    virtual void strokeShape(GraphicsContext*) const OVERRIDE;
    virtual bool shapeDependentStrokeContains(const FloatPoint&) OVERRIDE;

    bool shouldStrokeZeroLengthSubpath() const;
    Path* zeroLengthLinecapPath(const FloatPoint&) const;
    FloatRect zeroLengthSubpathRect(const FloatPoint&, float) const;
    void updateZeroLengthSubpaths();

    Vector<FloatPoint> m_zeroLengthLinecapLocations;
};

inline RenderSVGPath* toRenderSVGPath(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isSVGPath());
    return static_cast<RenderSVGPath*>(object);
}

}

#endif // ENABLE(SVG)
#endif
