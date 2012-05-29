/*
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

#ifndef RenderSVGResourceSolidColor_h
#define RenderSVGResourceSolidColor_h

#if ENABLE(SVG)
#include "Color.h"
#include "FloatRect.h"
#include "RenderSVGResource.h"

namespace WebCore {

class RenderSVGResourceSolidColor : public RenderSVGResource {
public:
    RenderSVGResourceSolidColor();
    virtual ~RenderSVGResourceSolidColor();

    virtual void removeAllClientsFromCache(bool = true) { }
    virtual void removeClientFromCache(RenderObject*, bool = true) { }

    virtual bool applyResource(RenderObject*, RenderStyle*, GraphicsContext*&, unsigned short resourceMode);
    virtual void postApplyResource(RenderObject*, GraphicsContext*&, unsigned short resourceMode, const Path*);
    virtual FloatRect resourceBoundingBox(RenderObject*) { return FloatRect(); }

    virtual RenderSVGResourceType resourceType() const { return s_resourceType; }
    static RenderSVGResourceType s_resourceType;

    const Color& color() const { return m_color; }
    void setColor(const Color& color) { m_color = color; }

private:
    Color m_color;
};

}

#endif
#endif
