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

#include "config.h"
#include "SVGResourcesCache.h"

#if ENABLE(SVG)
#include "HTMLNames.h"
#include "RenderSVGResourceContainer.h"
#include "SVGDocumentExtensions.h"
#include "SVGResources.h"
#include "SVGResourcesCycleSolver.h"
#include "SVGStyledElement.h"

namespace WebCore {

SVGResourcesCache::SVGResourcesCache()
{
}

SVGResourcesCache::~SVGResourcesCache()
{
}

void SVGResourcesCache::addResourcesFromRenderObject(RenderObject* object, const RenderStyle* style)
{
    ASSERT(object);
    ASSERT(style);
    ASSERT(!m_cache.contains(object));

    const SVGRenderStyle* svgStyle = style->svgStyle();
    ASSERT(svgStyle);

    // Build a list of all resources associated with the passed RenderObject
    OwnPtr<SVGResources> newResources = adoptPtr(new SVGResources);
    if (!newResources->buildCachedResources(object, svgStyle))
        return;

    // Put object in cache.
    SVGResources* resources = m_cache.set(object, newResources.release()).iterator->value.get();

    // Run cycle-detection _afterwards_, so self-references can be caught as well.
    SVGResourcesCycleSolver solver(object, resources);
    solver.resolveCycles();

    // Walk resources and register the render object at each resources.
    HashSet<RenderSVGResourceContainer*> resourceSet;
    resources->buildSetOfResources(resourceSet);

    HashSet<RenderSVGResourceContainer*>::iterator end = resourceSet.end();
    for (HashSet<RenderSVGResourceContainer*>::iterator it = resourceSet.begin(); it != end; ++it)
        (*it)->addClient(object);
}

void SVGResourcesCache::removeResourcesFromRenderObject(RenderObject* object)
{
    if (!m_cache.contains(object))
        return;

    OwnPtr<SVGResources> resources = m_cache.take(object);

    // Walk resources and register the render object at each resources.
    HashSet<RenderSVGResourceContainer*> resourceSet;
    resources->buildSetOfResources(resourceSet);

    HashSet<RenderSVGResourceContainer*>::iterator end = resourceSet.end();
    for (HashSet<RenderSVGResourceContainer*>::iterator it = resourceSet.begin(); it != end; ++it)
        (*it)->removeClient(object);
}

static inline SVGResourcesCache* resourcesCacheFromRenderObject(const RenderObject* renderer)
{
    Document* document = renderer->document();
    ASSERT(document);

    SVGDocumentExtensions* extensions = document->accessSVGExtensions();
    ASSERT(extensions);

    SVGResourcesCache* cache = extensions->resourcesCache();
    ASSERT(cache);

    return cache;
}

SVGResources* SVGResourcesCache::cachedResourcesForRenderObject(const RenderObject* renderer)
{
    ASSERT(renderer);
    return resourcesCacheFromRenderObject(renderer)->m_cache.get(renderer);
}

void SVGResourcesCache::clientLayoutChanged(RenderObject* object)
{
    SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(object);
    if (!resources)
        return;

    // Invalidate the resources if either the RenderObject itself changed,
    // or we have filter resources, which could depend on the layout of children.
    if (object->selfNeedsLayout())
        resources->removeClientFromCache(object);
}

static inline bool rendererCanHaveResources(RenderObject* renderer)
{
    ASSERT(renderer);
    return renderer->node() && renderer->node()->isSVGElement() && !renderer->isSVGInlineText();
}

void SVGResourcesCache::clientStyleChanged(RenderObject* renderer, StyleDifference diff, const RenderStyle* newStyle)
{
    ASSERT(renderer);
    if (diff == StyleDifferenceEqual || !renderer->parent())
        return;

    // In this case the proper SVGFE*Element will decide whether the modified CSS properties require a relayout or repaint.
    if (renderer->isSVGResourceFilterPrimitive() && (diff == StyleDifferenceRepaint || diff == StyleDifferenceRepaintIfText))
        return;

    // Dynamic changes of CSS properties like 'clip-path' may require us to recompute the associated resources for a renderer.
    // FIXME: Avoid passing in a useless StyleDifference, but instead compare oldStyle/newStyle to see which resources changed
    // to be able to selectively rebuild individual resources, instead of all of them.
    if (rendererCanHaveResources(renderer)) {
        SVGResourcesCache* cache = resourcesCacheFromRenderObject(renderer);
        cache->removeResourcesFromRenderObject(renderer);
        cache->addResourcesFromRenderObject(renderer, newStyle);
    }

    RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer, false);

    if (renderer->node() && !renderer->node()->isSVGElement())
        renderer->node()->setNeedsStyleRecalc(SyntheticStyleChange);
}

void SVGResourcesCache::clientWasAddedToTree(RenderObject* renderer, const RenderStyle* newStyle)
{
    if (!renderer->node())
        return;
    RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer, false);

    if (!rendererCanHaveResources(renderer))
        return;
    SVGResourcesCache* cache = resourcesCacheFromRenderObject(renderer);
    cache->addResourcesFromRenderObject(renderer, newStyle);
}

void SVGResourcesCache::clientWillBeRemovedFromTree(RenderObject* renderer)
{
    if (!renderer->node())
        return;
    RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer, false);

    if (!rendererCanHaveResources(renderer))
        return;
    SVGResourcesCache* cache = resourcesCacheFromRenderObject(renderer);
    cache->removeResourcesFromRenderObject(renderer);
}

void SVGResourcesCache::clientDestroyed(RenderObject* renderer)
{
    ASSERT(renderer);

    SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(renderer);
    if (resources)
        resources->removeClientFromCache(renderer);

    SVGResourcesCache* cache = resourcesCacheFromRenderObject(renderer);
    cache->removeResourcesFromRenderObject(renderer);
}

void SVGResourcesCache::resourceDestroyed(RenderSVGResourceContainer* resource)
{
    ASSERT(resource);
    SVGResourcesCache* cache = resourcesCacheFromRenderObject(resource);

    // The resource itself may have clients, that need to be notified.
    cache->removeResourcesFromRenderObject(resource);

    CacheMap::iterator end = cache->m_cache.end();
    for (CacheMap::iterator it = cache->m_cache.begin(); it != end; ++it) {
        it->value->resourceDestroyed(resource);

        // Mark users of destroyed resources as pending resolution based on the id of the old resource.
        Element* resourceElement = toElement(resource->node());
        Element* clientElement = toElement(it->key->node());
        SVGDocumentExtensions* extensions = clientElement->document()->accessSVGExtensions();

        extensions->addPendingResource(resourceElement->fastGetAttribute(HTMLNames::idAttr), clientElement);
    }
}

}

#endif
