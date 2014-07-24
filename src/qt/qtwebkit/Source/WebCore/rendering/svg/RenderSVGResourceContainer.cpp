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

#if ENABLE(SVG)
#include "RenderSVGResourceContainer.h"

#include "RenderLayer.h"
#include "RenderSVGRoot.h"
#include "RenderView.h"
#include "SVGGraphicsElement.h"
#include "SVGRenderingContext.h"
#include "SVGResourcesCache.h"
#include <wtf/StackStats.h>

namespace WebCore {

static inline SVGDocumentExtensions* svgExtensionsFromNode(Node* node)
{
    ASSERT(node);
    ASSERT(node->document());
    return node->document()->accessSVGExtensions();
}

RenderSVGResourceContainer::RenderSVGResourceContainer(SVGStyledElement* node)
    : RenderSVGHiddenContainer(node)
    , m_id(node->getIdAttribute())
    , m_registered(false)
    , m_isInvalidating(false)
{
}

RenderSVGResourceContainer::~RenderSVGResourceContainer()
{
    if (m_registered)
        svgExtensionsFromNode(node())->removeResource(m_id);
}

void RenderSVGResourceContainer::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    // Invalidate all resources if our layout changed.
    if (everHadLayout() && selfNeedsLayout())
        RenderSVGRoot::addResourceForClientInvalidation(this);

    RenderSVGHiddenContainer::layout();
}

void RenderSVGResourceContainer::willBeDestroyed()
{
    SVGResourcesCache::resourceDestroyed(this);
    RenderSVGHiddenContainer::willBeDestroyed();
}

void RenderSVGResourceContainer::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderSVGHiddenContainer::styleDidChange(diff, oldStyle);

    if (!m_registered) {
        m_registered = true;
        registerResource();
    }
}

void RenderSVGResourceContainer::idChanged()
{
    // Invalidate all our current clients.
    removeAllClientsFromCache();

    // Remove old id, that is guaranteed to be present in cache.
    SVGDocumentExtensions* extensions = svgExtensionsFromNode(node());
    extensions->removeResource(m_id);
    m_id = toElement(node())->getIdAttribute();

    registerResource();
}

void RenderSVGResourceContainer::markAllClientsForInvalidation(InvalidationMode mode)
{
    if ((m_clients.isEmpty() && m_clientLayers.isEmpty()) || m_isInvalidating)
        return;

    m_isInvalidating = true;
    bool needsLayout = mode == LayoutAndBoundariesInvalidation;
    bool markForInvalidation = mode != ParentOnlyInvalidation;

    HashSet<RenderObject*>::iterator end = m_clients.end();
    for (HashSet<RenderObject*>::iterator it = m_clients.begin(); it != end; ++it) {
        RenderObject* client = *it;
        if (client->isSVGResourceContainer()) {
            client->toRenderSVGResourceContainer()->removeAllClientsFromCache(markForInvalidation);
            continue;
        }

        if (markForInvalidation)
            markClientForInvalidation(client, mode);

        RenderSVGResource::markForLayoutAndParentResourceInvalidation(client, needsLayout);
    }

    markAllClientLayersForInvalidation();

    m_isInvalidating = false;
}

void RenderSVGResourceContainer::markAllClientLayersForInvalidation()
{
#if ENABLE(CSS_FILTERS)
    HashSet<RenderLayer*>::iterator layerEnd = m_clientLayers.end();
    for (HashSet<RenderLayer*>::iterator it = m_clientLayers.begin(); it != layerEnd; ++it)
        (*it)->filterNeedsRepaint();
#endif
}

void RenderSVGResourceContainer::markClientForInvalidation(RenderObject* client, InvalidationMode mode)
{
    ASSERT(client);
    ASSERT(!m_clients.isEmpty());

    switch (mode) {
    case LayoutAndBoundariesInvalidation:
    case BoundariesInvalidation:
        client->setNeedsBoundariesUpdate();
        break;
    case RepaintInvalidation:
        if (client->view())
            client->repaint();
        break;
    case ParentOnlyInvalidation:
        break;
    }
}

void RenderSVGResourceContainer::addClient(RenderObject* client)
{
    ASSERT(client);
    m_clients.add(client);
}

void RenderSVGResourceContainer::removeClient(RenderObject* client)
{
    ASSERT(client);
    removeClientFromCache(client, false);
    m_clients.remove(client);
}

void RenderSVGResourceContainer::addClientRenderLayer(RenderLayer* client)
{
    ASSERT(client);
    m_clientLayers.add(client);
}

void RenderSVGResourceContainer::removeClientRenderLayer(RenderLayer* client)
{
    ASSERT(client);
    m_clientLayers.remove(client);
}

void RenderSVGResourceContainer::registerResource()
{
    SVGDocumentExtensions* extensions = svgExtensionsFromNode(node());
    if (!extensions->hasPendingResource(m_id)) {
        extensions->addResource(m_id, this);
        return;
    }

    OwnPtr<SVGDocumentExtensions::SVGPendingElements> clients(extensions->removePendingResource(m_id));

    // Cache us with the new id.
    extensions->addResource(m_id, this);

    // Update cached resources of pending clients.
    const SVGDocumentExtensions::SVGPendingElements::const_iterator end = clients->end();
    for (SVGDocumentExtensions::SVGPendingElements::const_iterator it = clients->begin(); it != end; ++it) {
        ASSERT((*it)->hasPendingResources());
        extensions->clearHasPendingResourcesIfPossible(*it);
        RenderObject* renderer = (*it)->renderer();
        if (!renderer)
            continue;
        SVGResourcesCache::clientStyleChanged(renderer, StyleDifferenceLayout, renderer->style());
        renderer->setNeedsLayout(true);
    }
}

bool RenderSVGResourceContainer::shouldTransformOnTextPainting(RenderObject* object, AffineTransform& resourceTransform)
{
    ASSERT_UNUSED(object, object);
#if USE(CG)
    UNUSED_PARAM(resourceTransform);
    return false;
#else
    // This method should only be called for RenderObjects that deal with text rendering. Cmp. RenderObject.h's is*() methods.
    ASSERT(object->isSVGText() || object->isSVGTextPath() || object->isSVGInline());

    // In text drawing, the scaling part of the graphics context CTM is removed, compare SVGInlineTextBox::paintTextWithShadows.
    // So, we use that scaling factor here, too, and then push it down to pattern or gradient space
    // in order to keep the pattern or gradient correctly scaled.
    float scalingFactor = SVGRenderingContext::calculateScreenFontSizeScalingFactor(object);
    if (scalingFactor == 1)
        return false;
    resourceTransform.scale(scalingFactor);
    return true;
#endif
}

// FIXME: This does not belong here.
AffineTransform RenderSVGResourceContainer::transformOnNonScalingStroke(RenderObject* object, const AffineTransform& resourceTransform)
{
    if (!object->isSVGShape())
        return resourceTransform;

    SVGGraphicsElement* element = toSVGGraphicsElement(object->node());
    AffineTransform transform = element->getScreenCTM(SVGLocatable::DisallowStyleUpdate);
    transform *= resourceTransform;
    return transform;
}

}

#endif
