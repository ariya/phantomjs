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

#ifndef RenderSVGResourceContainer_h
#define RenderSVGResourceContainer_h

#if ENABLE(SVG)
#include "RenderSVGHiddenContainer.h"
#include "RenderSVGResource.h"

namespace WebCore {

class RenderSVGResourceContainer : public RenderSVGHiddenContainer,
                                   public RenderSVGResource {
public:
    RenderSVGResourceContainer(SVGStyledElement*);
    virtual ~RenderSVGResourceContainer();

    virtual void layout();
    virtual void destroy();
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

    virtual bool isSVGResourceContainer() const { return true; }
    virtual RenderSVGResourceContainer* toRenderSVGResourceContainer() { return this; }

    static AffineTransform transformOnNonScalingStroke(RenderObject*, const AffineTransform& resourceTransform);

    void idChanged();

protected:
    enum InvalidationMode {
        LayoutAndBoundariesInvalidation,
        BoundariesInvalidation,
        RepaintInvalidation,
        ParentOnlyInvalidation
    };

    // Used from the invalidateClient/invalidateClients methods from classes, inheriting from us.
    void markAllClientsForInvalidation(InvalidationMode);
    void markClientForInvalidation(RenderObject*, InvalidationMode);

private:
    friend class SVGResourcesCache;
    void addClient(RenderObject*);
    void removeClient(RenderObject*);

private:
    void registerResource();

    AtomicString m_id;
    bool m_registered;
    HashSet<RenderObject*> m_clients;
};

inline RenderSVGResourceContainer* getRenderSVGResourceContainerById(Document* document, const AtomicString& id)
{
    if (id.isEmpty())
        return 0;

    if (RenderSVGResourceContainer* renderResource = document->accessSVGExtensions()->resourceById(id))
        return renderResource;

    return 0;
}

template<typename Renderer>
Renderer* getRenderSVGResourceById(Document* document, const AtomicString& id)
{
    if (RenderSVGResourceContainer* container = getRenderSVGResourceContainerById(document, id))
        return container->cast<Renderer>();

    return 0;
}

}

#endif
#endif
