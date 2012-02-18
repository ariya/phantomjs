/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
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

#ifndef RenderSVGResourceFilter_h
#define RenderSVGResourceFilter_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FloatRect.h"
#include "ImageBuffer.h"
#include "RenderSVGResourceContainer.h"
#include "SVGFilter.h"
#include "SVGFilterBuilder.h"
#include "SVGFilterElement.h"
#include "SVGUnitTypes.h"

#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

struct FilterData {
    FilterData()
        : savedContext(0)
        , builded(false)
        , markedForRemoval(false)
    {
    }

    RefPtr<SVGFilter> filter;
    RefPtr<SVGFilterBuilder> builder;
    OwnPtr<ImageBuffer> sourceGraphicBuffer;
    GraphicsContext* savedContext;
    AffineTransform shearFreeAbsoluteTransform;
    FloatRect boundaries;
    FloatSize scale;
    bool builded : 1;
    bool markedForRemoval : 1;
};

class GraphicsContext;

class RenderSVGResourceFilter : public RenderSVGResourceContainer {
public:
    RenderSVGResourceFilter(SVGFilterElement*);
    virtual ~RenderSVGResourceFilter();

    virtual const char* renderName() const { return "RenderSVGResourceFilter"; }
    virtual bool isSVGResourceFilter() const { return true; }

    virtual void removeAllClientsFromCache(bool markForInvalidation = true);
    virtual void removeClientFromCache(RenderObject*, bool markForInvalidation = true);

    virtual bool applyResource(RenderObject*, RenderStyle*, GraphicsContext*&, unsigned short resourceMode);
    virtual void postApplyResource(RenderObject*, GraphicsContext*&, unsigned short resourceMode, const Path*);

    virtual FloatRect resourceBoundingBox(RenderObject*);

    PassRefPtr<SVGFilterBuilder> buildPrimitives(Filter*);

    SVGUnitTypes::SVGUnitType filterUnits() const { return toUnitType(static_cast<SVGFilterElement*>(node())->filterUnits()); }
    SVGUnitTypes::SVGUnitType primitiveUnits() const { return toUnitType(static_cast<SVGFilterElement*>(node())->primitiveUnits()); }

    void primitiveAttributeChanged(RenderObject*, const QualifiedName&);

    virtual RenderSVGResourceType resourceType() const { return s_resourceType; }
    static RenderSVGResourceType s_resourceType;

private:
    bool fitsInMaximumImageSize(const FloatSize&, FloatSize&);

    HashMap<RenderObject*, FilterData*> m_filter;
};

}

#endif
#endif
