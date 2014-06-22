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
#include "SVGResourcesCycleSolver.h"

// Set to a value > 0, to debug the resource cache.
#define DEBUG_CYCLE_DETECTION 0

#if ENABLE(SVG)
#include "RenderSVGResourceClipper.h"
#include "RenderSVGResourceFilter.h"
#include "RenderSVGResourceMarker.h"
#include "RenderSVGResourceMasker.h"
#include "SVGFilterElement.h"
#include "SVGGradientElement.h"
#include "SVGPatternElement.h"
#include "SVGResources.h"
#include "SVGResourcesCache.h"

namespace WebCore {

SVGResourcesCycleSolver::SVGResourcesCycleSolver(RenderObject* renderer, SVGResources* resources)
    : m_renderer(renderer)
    , m_resources(resources)
{
    ASSERT(m_renderer);
    ASSERT(m_resources);
}

SVGResourcesCycleSolver::~SVGResourcesCycleSolver()
{
}

bool SVGResourcesCycleSolver::resourceContainsCycles(RenderObject* renderer) const
{
    ASSERT(renderer);

    // First operate on the resources of the given renderer.
    // <marker id="a"> <path marker-start="url(#b)"/> ...
    // <marker id="b" marker-start="url(#a)"/>
    if (SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(renderer)) {
        HashSet<RenderSVGResourceContainer*> resourceSet;
        resources->buildSetOfResources(resourceSet);

        // Walk all resources and check wheter they reference any resource contained in the resources set.
        HashSet<RenderSVGResourceContainer*>::iterator end = resourceSet.end();
        for (HashSet<RenderSVGResourceContainer*>::iterator it = resourceSet.begin(); it != end; ++it) {
            if (m_allResources.contains(*it))
                return true;
        }
    }

    // Then operate on the child resources of the given renderer.
    // <marker id="a"> <path marker-start="url(#b)"/> ...
    // <marker id="b"> <path marker-start="url(#a)"/> ...
    for (RenderObject* child = renderer->firstChild(); child; child = child->nextSibling()) {
        SVGResources* childResources = SVGResourcesCache::cachedResourcesForRenderObject(child);
        if (!childResources)
            continue;
        
        // A child of the given 'resource' contains resources. 
        HashSet<RenderSVGResourceContainer*> childSet;
        childResources->buildSetOfResources(childSet);

        // Walk all child resources and check wheter they reference any resource contained in the resources set.
        HashSet<RenderSVGResourceContainer*>::iterator end = childSet.end();
        for (HashSet<RenderSVGResourceContainer*>::iterator it = childSet.begin(); it != end; ++it) {
            if (m_allResources.contains(*it))
                return true;
        }

        // Walk children recursively, stop immediately if we found a cycle
        if (resourceContainsCycles(child))
            return true;
    }

    return false;
}

void SVGResourcesCycleSolver::resolveCycles()
{
    ASSERT(m_allResources.isEmpty());

#if DEBUG_CYCLE_DETECTION > 0
    fprintf(stderr, "\nBefore cycle detection:\n");
    m_resources->dump(m_renderer);
#endif

    // Stash all resources into a HashSet for the ease of traversing.
    HashSet<RenderSVGResourceContainer*> localResources;
    m_resources->buildSetOfResources(localResources);
    ASSERT(!localResources.isEmpty());

    // Add all parent resource containers to the HashSet.
    HashSet<RenderSVGResourceContainer*> parentResources;
    RenderObject* parent = m_renderer->parent();
    while (parent) {
        if (parent->isSVGResourceContainer())
            parentResources.add(parent->toRenderSVGResourceContainer());
        parent = parent->parent();
    }

#if DEBUG_CYCLE_DETECTION > 0
    fprintf(stderr, "\nDetecting wheter any resources references any of following objects:\n");
    {
        fprintf(stderr, "Local resources:\n");
        HashSet<RenderSVGResourceContainer*>::iterator end = localResources.end();
        for (HashSet<RenderSVGResourceContainer*>::iterator it = localResources.begin(); it != end; ++it)
            fprintf(stderr, "|> %s: object=%p (node=%p)\n", (*it)->renderName(), *it, (*it)->node());

        fprintf(stderr, "Parent resources:\n");
        end = parentResources.end();
        for (HashSet<RenderSVGResourceContainer*>::iterator it = parentResources.begin(); it != end; ++it)
            fprintf(stderr, "|> %s: object=%p (node=%p)\n", (*it)->renderName(), *it, (*it)->node());
    }
#endif

    // Build combined set of local and parent resources.
    m_allResources = localResources;
    HashSet<RenderSVGResourceContainer*>::iterator end = parentResources.end();
    for (HashSet<RenderSVGResourceContainer*>::iterator it = parentResources.begin(); it != end; ++it)
        m_allResources.add(*it);

    // If we're a resource, add ourselves to the HashSet.
    if (m_renderer->isSVGResourceContainer())
        m_allResources.add(m_renderer->toRenderSVGResourceContainer());

    ASSERT(!m_allResources.isEmpty());

    // The job of this function is to determine wheter any of the 'resources' associated with the given 'renderer'
    // references us (or wheter any of its kids references us) -> that's a cycle, we need to find and break it.
    end = localResources.end();
    for (HashSet<RenderSVGResourceContainer*>::iterator it = localResources.begin(); it != end; ++it) {
        RenderSVGResourceContainer* resource = *it;
        if (parentResources.contains(resource) || resourceContainsCycles(resource))
            breakCycle(resource);
    }

#if DEBUG_CYCLE_DETECTION > 0
    fprintf(stderr, "\nAfter cycle detection:\n");
    m_resources->dump(m_renderer);
#endif

    m_allResources.clear();
}

void SVGResourcesCycleSolver::breakCycle(RenderSVGResourceContainer* resourceLeadingToCycle)
{
    ASSERT(resourceLeadingToCycle);
    if (resourceLeadingToCycle == m_resources->linkedResource()) {
        m_resources->resetLinkedResource();
        return;
    }

    switch (resourceLeadingToCycle->resourceType()) {
    case MaskerResourceType:
        ASSERT(resourceLeadingToCycle == m_resources->masker());
        m_resources->resetMasker();
        break;
    case MarkerResourceType:
        ASSERT(resourceLeadingToCycle == m_resources->markerStart() || resourceLeadingToCycle == m_resources->markerMid() || resourceLeadingToCycle == m_resources->markerEnd());
        if (m_resources->markerStart() == resourceLeadingToCycle)
            m_resources->resetMarkerStart();
        if (m_resources->markerMid() == resourceLeadingToCycle)
            m_resources->resetMarkerMid();
        if (m_resources->markerEnd() == resourceLeadingToCycle)
            m_resources->resetMarkerEnd();
        break;
    case PatternResourceType:
    case LinearGradientResourceType:
    case RadialGradientResourceType:
        ASSERT(resourceLeadingToCycle == m_resources->fill() || resourceLeadingToCycle == m_resources->stroke());
        if (m_resources->fill() == resourceLeadingToCycle)
            m_resources->resetFill();
        if (m_resources->stroke() == resourceLeadingToCycle)
            m_resources->resetStroke();
        break;
    case FilterResourceType:
#if ENABLE(FILTERS)
        ASSERT(resourceLeadingToCycle == m_resources->filter());
        m_resources->resetFilter();
#endif
        break;
    case ClipperResourceType:
        ASSERT(resourceLeadingToCycle == m_resources->clipper());
        m_resources->resetClipper();
        break;
    case SolidColorResourceType:
        ASSERT_NOT_REACHED();
        break;
    }
}

}

#endif
