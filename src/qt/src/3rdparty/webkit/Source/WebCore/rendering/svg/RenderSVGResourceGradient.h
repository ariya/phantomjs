/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2008 Eric Seidel <eric@webkit.org>
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

#ifndef RenderSVGResourceGradient_h
#define RenderSVGResourceGradient_h

#if ENABLE(SVG)
#include "AffineTransform.h"
#include "FloatRect.h"
#include "Gradient.h"
#include "ImageBuffer.h"
#include "RenderSVGResourceContainer.h"
#include "SVGGradientElement.h"

#include <wtf/HashMap.h>

namespace WebCore {

struct GradientData {
    RefPtr<Gradient> gradient;
    AffineTransform userspaceTransform;
};

class GraphicsContext;

class RenderSVGResourceGradient : public RenderSVGResourceContainer {
public:
    RenderSVGResourceGradient(SVGGradientElement*);
    virtual ~RenderSVGResourceGradient();

    virtual void removeAllClientsFromCache(bool markForInvalidation = true);
    virtual void removeClientFromCache(RenderObject*, bool markForInvalidation = true);

    virtual bool applyResource(RenderObject*, RenderStyle*, GraphicsContext*&, unsigned short resourceMode);
    virtual void postApplyResource(RenderObject*, GraphicsContext*&, unsigned short resourceMode, const Path*);
    virtual FloatRect resourceBoundingBox(RenderObject*) { return FloatRect(); }

protected:
    void addStops(GradientData*, const Vector<Gradient::ColorStop>&) const;

    virtual bool boundingBoxMode() const = 0;
    virtual void calculateGradientTransform(AffineTransform&) = 0;
    virtual void collectGradientAttributes(SVGGradientElement*) = 0;
    virtual void buildGradient(GradientData*, SVGGradientElement*) const = 0;

private:
    bool m_shouldCollectGradientAttributes : 1;
    HashMap<RenderObject*, GradientData*> m_gradient;

#if USE(CG)
    GraphicsContext* m_savedContext;
    OwnPtr<ImageBuffer> m_imageBuffer;
#endif
};

}

#endif
#endif
