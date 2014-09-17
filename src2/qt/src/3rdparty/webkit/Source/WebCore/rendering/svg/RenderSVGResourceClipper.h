/*
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
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

#ifndef RenderSVGResourceClipper_h
#define RenderSVGResourceClipper_h

#if ENABLE(SVG)
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "ImageBuffer.h"
#include "IntSize.h"
#include "Path.h"
#include "RenderSVGResourceContainer.h"
#include "SVGClipPathElement.h"
#include "SVGUnitTypes.h"

#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>

namespace WebCore {

struct ClipperData {
    WTF_MAKE_FAST_ALLOCATED;
public:
    OwnPtr<ImageBuffer> clipMaskImage;
};

class RenderSVGResourceClipper : public RenderSVGResourceContainer {
public:
    RenderSVGResourceClipper(SVGClipPathElement*);
    virtual ~RenderSVGResourceClipper();

    virtual const char* renderName() const { return "RenderSVGResourceClipper"; }

    virtual void removeAllClientsFromCache(bool markForInvalidation = true);
    virtual void removeClientFromCache(RenderObject*, bool markForInvalidation = true);

    virtual bool applyResource(RenderObject*, RenderStyle*, GraphicsContext*&, unsigned short resourceMode);
    virtual FloatRect resourceBoundingBox(RenderObject*);

    virtual RenderSVGResourceType resourceType() const { return ClipperResourceType; }
    
    bool hitTestClipContent(const FloatRect&, const FloatPoint&);

    SVGUnitTypes::SVGUnitType clipPathUnits() const { return toUnitType(static_cast<SVGClipPathElement*>(node())->clipPathUnits()); }

    static RenderSVGResourceType s_resourceType;
private:
    // clipPath can be clipped too, but don't have a boundingBox or repaintRect. So we can't call
    // applyResource directly and use the rects from the object, since they are empty for RenderSVGResources
    bool applyClippingToContext(RenderObject*, const FloatRect&, const FloatRect&, GraphicsContext*);
    bool pathOnlyClipping(GraphicsContext*, const FloatRect&);
    bool drawContentIntoMaskImage(ClipperData*, const FloatRect& objectBoundingBox);
    void calculateClipContentRepaintRect();

    bool m_invalidationBlocked;
    FloatRect m_clipBoundaries;
    HashMap<RenderObject*, ClipperData*> m_clipper;
};

}

#endif
#endif
