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
#include "SVGResources.h"

#if ENABLE(SVG)
#include "RenderSVGResourceClipper.h"
#include "RenderSVGResourceFilter.h"
#include "RenderSVGResourceMarker.h"
#include "RenderSVGResourceMasker.h"
#include "SVGFilterElement.h"
#include "SVGGradientElement.h"
#include "SVGNames.h"
#include "SVGPaint.h"
#include "SVGPatternElement.h"
#include "SVGRenderStyle.h"
#include "SVGURIReference.h"

namespace WebCore {

SVGResources::SVGResources()
    : m_linkedResource(0)
{
}

static HashSet<AtomicStringImpl*>& clipperFilterMaskerTags()
{
    DEFINE_STATIC_LOCAL(HashSet<AtomicStringImpl*>, s_tagList, ());
    if (s_tagList.isEmpty()) {
        // "container elements": http://www.w3.org/TR/SVG11/intro.html#TermContainerElement
        // "graphics elements" : http://www.w3.org/TR/SVG11/intro.html#TermGraphicsElement
        s_tagList.add(SVGNames::aTag.localName().impl());
        s_tagList.add(SVGNames::circleTag.localName().impl());
        s_tagList.add(SVGNames::ellipseTag.localName().impl());
        s_tagList.add(SVGNames::glyphTag.localName().impl());
        s_tagList.add(SVGNames::gTag.localName().impl());
        s_tagList.add(SVGNames::imageTag.localName().impl());
        s_tagList.add(SVGNames::lineTag.localName().impl());
        s_tagList.add(SVGNames::markerTag.localName().impl());
        s_tagList.add(SVGNames::maskTag.localName().impl());
        s_tagList.add(SVGNames::missing_glyphTag.localName().impl());
        s_tagList.add(SVGNames::pathTag.localName().impl());
        s_tagList.add(SVGNames::polygonTag.localName().impl());
        s_tagList.add(SVGNames::polylineTag.localName().impl());
        s_tagList.add(SVGNames::rectTag.localName().impl());
        s_tagList.add(SVGNames::svgTag.localName().impl());
        s_tagList.add(SVGNames::textTag.localName().impl());
        s_tagList.add(SVGNames::useTag.localName().impl());

        // Not listed in the definitions is the clipPath element, the SVG spec says though:
        // The "clipPath" element or any of its children can specify property "clip-path".
        // So we have to add clipPathTag here, otherwhise clip-path on clipPath will fail.
        // (Already mailed SVG WG, waiting for a solution)
        s_tagList.add(SVGNames::clipPathTag.localName().impl());

        // Not listed in the definitions are the text content elements, though filter/clipper/masker on tspan/text/.. is allowed.
        // (Already mailed SVG WG, waiting for a solution)
        s_tagList.add(SVGNames::altGlyphTag.localName().impl());
        s_tagList.add(SVGNames::textPathTag.localName().impl());
        s_tagList.add(SVGNames::trefTag.localName().impl());
        s_tagList.add(SVGNames::tspanTag.localName().impl());

        // Elements that we ignore, as it doesn't make any sense.
        // defs, pattern, switch (FIXME: Mail SVG WG about these)
        // symbol (is converted to a svg element, when referenced by use, we can safely ignore it.)
    }

    return s_tagList;
}

static HashSet<AtomicStringImpl*>& markerTags()
{
    DEFINE_STATIC_LOCAL(HashSet<AtomicStringImpl*>, s_tagList, ());
    if (s_tagList.isEmpty()) {
        s_tagList.add(SVGNames::lineTag.localName().impl());
        s_tagList.add(SVGNames::pathTag.localName().impl());
        s_tagList.add(SVGNames::polygonTag.localName().impl());
        s_tagList.add(SVGNames::polylineTag.localName().impl());
    }

    return s_tagList;
}

static HashSet<AtomicStringImpl*>& fillAndStrokeTags()
{
    DEFINE_STATIC_LOCAL(HashSet<AtomicStringImpl*>, s_tagList, ());
    if (s_tagList.isEmpty()) {
        s_tagList.add(SVGNames::altGlyphTag.localName().impl());
        s_tagList.add(SVGNames::circleTag.localName().impl());
        s_tagList.add(SVGNames::ellipseTag.localName().impl());
        s_tagList.add(SVGNames::lineTag.localName().impl());
        s_tagList.add(SVGNames::pathTag.localName().impl());
        s_tagList.add(SVGNames::polygonTag.localName().impl());
        s_tagList.add(SVGNames::polylineTag.localName().impl());
        s_tagList.add(SVGNames::rectTag.localName().impl());
        s_tagList.add(SVGNames::textTag.localName().impl());
        s_tagList.add(SVGNames::textPathTag.localName().impl());
        s_tagList.add(SVGNames::trefTag.localName().impl());
        s_tagList.add(SVGNames::tspanTag.localName().impl());
    }

    return s_tagList;
}

static HashSet<AtomicStringImpl*>& chainableResourceTags()
{
    DEFINE_STATIC_LOCAL(HashSet<AtomicStringImpl*>, s_tagList, ());
    if (s_tagList.isEmpty()) {
        s_tagList.add(SVGNames::linearGradientTag.localName().impl());
        s_tagList.add(SVGNames::filterTag.localName().impl());
        s_tagList.add(SVGNames::patternTag.localName().impl());
        s_tagList.add(SVGNames::radialGradientTag.localName().impl());
    }

    return s_tagList;
}

static inline String targetReferenceFromResource(SVGElement* element)
{
    String target;
    if (element->hasTagName(SVGNames::patternTag))
        target = static_cast<SVGPatternElement*>(element)->href();
    else if (element->hasTagName(SVGNames::linearGradientTag) || element->hasTagName(SVGNames::radialGradientTag))
        target = static_cast<SVGGradientElement*>(element)->href();
#if ENABLE(FILTERS)
    else if (element->hasTagName(SVGNames::filterTag))
        target = static_cast<SVGFilterElement*>(element)->href();
#endif
    else
        ASSERT_NOT_REACHED();

    return SVGURIReference::getTarget(target);
}

static inline RenderSVGResourceContainer* paintingResourceFromSVGPaint(Document* document, const SVGPaint::SVGPaintType& paintType, const String& paintUri, AtomicString& id, bool& hasPendingResource)
{
    if (paintType != SVGPaint::SVG_PAINTTYPE_URI && paintType != SVGPaint::SVG_PAINTTYPE_URI_RGBCOLOR)
        return 0;

    id = SVGURIReference::getTarget(paintUri);
    RenderSVGResourceContainer* container = getRenderSVGResourceContainerById(document, id);
    if (!container) {
        hasPendingResource = true;
        return 0;
    }

    RenderSVGResourceType resourceType = container->resourceType();
    if (resourceType != PatternResourceType && resourceType != LinearGradientResourceType && resourceType != RadialGradientResourceType)
        return 0;

    return container;
}

static inline void registerPendingResource(SVGDocumentExtensions* extensions, const AtomicString& id, SVGElement* element)
{
    ASSERT(element);
    ASSERT(element->isStyled());
    extensions->addPendingResource(id, static_cast<SVGStyledElement*>(element));
}

bool SVGResources::buildCachedResources(const RenderObject* object, const SVGRenderStyle* style)
{
    ASSERT(object);
    ASSERT(style);

    Node* node = object->node();
    ASSERT(node);
    ASSERT(node->isSVGElement());

    SVGElement* element = static_cast<SVGElement*>(node);
    if (!element)
        return false;

    Document* document = object->document();
    ASSERT(document);

    SVGDocumentExtensions* extensions = document->accessSVGExtensions();
    ASSERT(extensions);

    AtomicStringImpl* tagNameImpl = element->tagQName().localName().impl();
    if (!tagNameImpl)
        return false;

    bool foundResources = false;
    if (clipperFilterMaskerTags().contains(tagNameImpl)) {
        if (style->hasClipper()) {
            AtomicString id(style->clipperResource());
            if (setClipper(getRenderSVGResourceById<RenderSVGResourceClipper>(document, id)))
                foundResources = true;
            else
                registerPendingResource(extensions, id, element);
        }

#if ENABLE(FILTERS)
        if (style->hasFilter()) {
            AtomicString id(style->filterResource());
            if (setFilter(getRenderSVGResourceById<RenderSVGResourceFilter>(document, id)))
                foundResources = true;
            else
                registerPendingResource(extensions, id, element);
        }
#endif

        if (style->hasMasker()) {
            AtomicString id(style->maskerResource());
            if (setMasker(getRenderSVGResourceById<RenderSVGResourceMasker>(document, id)))
                foundResources = true;
            else
                registerPendingResource(extensions, id, element);
        }
    }

    if (markerTags().contains(tagNameImpl) && style->hasMarkers()) {
        AtomicString markerStartId(style->markerStartResource());
        if (setMarkerStart(getRenderSVGResourceById<RenderSVGResourceMarker>(document, markerStartId)))
            foundResources = true;
        else
            registerPendingResource(extensions, markerStartId, element);

        AtomicString markerMidId(style->markerMidResource());
        if (setMarkerMid(getRenderSVGResourceById<RenderSVGResourceMarker>(document, markerMidId)))
            foundResources = true;
        else
            registerPendingResource(extensions, markerMidId, element);

        AtomicString markerEndId(style->markerEndResource());
        if (setMarkerEnd(getRenderSVGResourceById<RenderSVGResourceMarker>(document, markerEndId)))
            foundResources = true;
        else
            registerPendingResource(extensions, markerEndId, element);
    }

    if (fillAndStrokeTags().contains(tagNameImpl)) {
        if (style->hasFill()) {
            bool hasPendingResource = false;
            AtomicString id;
            if (setFill(paintingResourceFromSVGPaint(document, style->fillPaintType(), style->fillPaintUri(), id, hasPendingResource)))
                foundResources = true;
            else if (hasPendingResource)
                registerPendingResource(extensions, id, element);
        }

        if (style->hasStroke()) {
            bool hasPendingResource = false;
            AtomicString id;
            if (setStroke(paintingResourceFromSVGPaint(document, style->strokePaintType(), style->strokePaintUri(), id, hasPendingResource)))
                foundResources = true;
            else if (hasPendingResource)
                registerPendingResource(extensions, id, element);
        }
    }

    if (chainableResourceTags().contains(tagNameImpl)) {
        AtomicString id(targetReferenceFromResource(element));
        if (setLinkedResource(getRenderSVGResourceContainerById(document, id)))
            foundResources = true;
        else
            registerPendingResource(extensions, id, element);
    }

    return foundResources;
}

void SVGResources::removeClientFromCache(RenderObject* object, bool markForInvalidation) const
{
    if (!m_clipperFilterMaskerData && !m_markerData && !m_fillStrokeData && !m_linkedResource)
        return;

    if (m_linkedResource) {
        ASSERT(!m_clipperFilterMaskerData);
        ASSERT(!m_markerData);
        ASSERT(!m_fillStrokeData);
        m_linkedResource->removeClientFromCache(object, markForInvalidation);
        return;
    }

    if (m_clipperFilterMaskerData) {
        if (m_clipperFilterMaskerData->clipper)
            m_clipperFilterMaskerData->clipper->removeClientFromCache(object, markForInvalidation);
#if ENABLE(FILTERS)
        if (m_clipperFilterMaskerData->filter)
            m_clipperFilterMaskerData->filter->removeClientFromCache(object, markForInvalidation);
#endif
        if (m_clipperFilterMaskerData->masker)
            m_clipperFilterMaskerData->masker->removeClientFromCache(object, markForInvalidation);
    }

    if (m_markerData) {
        if (m_markerData->markerStart)
            m_markerData->markerStart->removeClientFromCache(object, markForInvalidation);
        if (m_markerData->markerMid)
            m_markerData->markerMid->removeClientFromCache(object, markForInvalidation);
        if (m_markerData->markerEnd)
            m_markerData->markerEnd->removeClientFromCache(object, markForInvalidation);
    }

    if (m_fillStrokeData) {
        if (m_fillStrokeData->fill)
            m_fillStrokeData->fill->removeClientFromCache(object, markForInvalidation);
        if (m_fillStrokeData->stroke)
            m_fillStrokeData->stroke->removeClientFromCache(object, markForInvalidation);
    }
}

void SVGResources::resourceDestroyed(RenderSVGResourceContainer* resource)
{
    ASSERT(resource);
    if (!m_clipperFilterMaskerData && !m_markerData && !m_fillStrokeData && !m_linkedResource)
        return;

    if (m_linkedResource == resource) {
        ASSERT(!m_clipperFilterMaskerData);
        ASSERT(!m_markerData);
        ASSERT(!m_fillStrokeData);
        m_linkedResource->removeAllClientsFromCache();
        m_linkedResource = 0;
        return;
    }

    switch (resource->resourceType()) {
    case MaskerResourceType:
        if (!m_clipperFilterMaskerData)
            break;
        if (m_clipperFilterMaskerData->masker == resource) {
            m_clipperFilterMaskerData->masker->removeAllClientsFromCache();
            m_clipperFilterMaskerData->masker = 0;
        }
        break;
    case MarkerResourceType:
        if (!m_markerData)
            break;
        if (m_markerData->markerStart == resource) {
            m_markerData->markerStart->removeAllClientsFromCache();
            m_markerData->markerStart = 0;
        }
        if (m_markerData->markerMid == resource) {
            m_markerData->markerMid->removeAllClientsFromCache();
            m_markerData->markerMid = 0;
        }
        if (m_markerData->markerEnd == resource) {
            m_markerData->markerEnd->removeAllClientsFromCache();
            m_markerData->markerEnd = 0;
        }
        break;
    case PatternResourceType:
    case LinearGradientResourceType:
    case RadialGradientResourceType:
        if (!m_fillStrokeData)
            break;
        if (m_fillStrokeData->fill == resource) {
            m_fillStrokeData->fill->removeAllClientsFromCache();
            m_fillStrokeData->fill = 0;
        }
        if (m_fillStrokeData->stroke == resource) {
            m_fillStrokeData->stroke->removeAllClientsFromCache();
            m_fillStrokeData->stroke = 0;
        }
        break;
    case FilterResourceType:
#if ENABLE(FILTERS)
        if (!m_clipperFilterMaskerData)
            break;
        if (m_clipperFilterMaskerData->filter == resource) {
            m_clipperFilterMaskerData->filter->removeAllClientsFromCache();
            m_clipperFilterMaskerData->filter = 0;
        }
#else
        ASSERT_NOT_REACHED();
#endif
        break;
    case ClipperResourceType:
        if (!m_clipperFilterMaskerData)
            break; 
        if (m_clipperFilterMaskerData->clipper == resource) {
            m_clipperFilterMaskerData->clipper->removeAllClientsFromCache();
            m_clipperFilterMaskerData->clipper = 0;
        }
        break;
    case SolidColorResourceType:
        ASSERT_NOT_REACHED();
    }
}

void SVGResources::buildSetOfResources(HashSet<RenderSVGResourceContainer*>& set)
{
    if (!m_clipperFilterMaskerData && !m_markerData && !m_fillStrokeData && !m_linkedResource)
        return;

    if (m_linkedResource) {
        ASSERT(!m_clipperFilterMaskerData);
        ASSERT(!m_markerData);
        ASSERT(!m_fillStrokeData);
        set.add(m_linkedResource);
        return;
    }

    if (m_clipperFilterMaskerData) {
        if (m_clipperFilterMaskerData->clipper)
            set.add(m_clipperFilterMaskerData->clipper);
#if ENABLE(FILTERS)
        if (m_clipperFilterMaskerData->filter)
            set.add(m_clipperFilterMaskerData->filter);
#endif
        if (m_clipperFilterMaskerData->masker)
            set.add(m_clipperFilterMaskerData->masker);
    }

    if (m_markerData) {
        if (m_markerData->markerStart)
            set.add(m_markerData->markerStart);
        if (m_markerData->markerMid)
            set.add(m_markerData->markerMid);
        if (m_markerData->markerEnd)
            set.add(m_markerData->markerEnd);
    }

    if (m_fillStrokeData) {
        if (m_fillStrokeData->fill)
            set.add(m_fillStrokeData->fill);
        if (m_fillStrokeData->stroke)
            set.add(m_fillStrokeData->stroke);
    }
}

bool SVGResources::setClipper(RenderSVGResourceClipper* clipper)
{
    if (!clipper)
        return false;

    ASSERT(clipper->resourceType() == ClipperResourceType);

    if (!m_clipperFilterMaskerData)
        m_clipperFilterMaskerData = ClipperFilterMaskerData::create();

    m_clipperFilterMaskerData->clipper = clipper;
    return true;
}

void SVGResources::resetClipper()
{
    ASSERT(m_clipperFilterMaskerData);
    ASSERT(m_clipperFilterMaskerData->clipper);
    m_clipperFilterMaskerData->clipper = 0;
}

#if ENABLE(FILTERS)
bool SVGResources::setFilter(RenderSVGResourceFilter* filter)
{
    if (!filter)
        return false;

    ASSERT(filter->resourceType() == FilterResourceType);

    if (!m_clipperFilterMaskerData)
        m_clipperFilterMaskerData = ClipperFilterMaskerData::create();

    m_clipperFilterMaskerData->filter = filter;
    return true;
}

void SVGResources::resetFilter()
{
    ASSERT(m_clipperFilterMaskerData);
    ASSERT(m_clipperFilterMaskerData->filter);
    m_clipperFilterMaskerData->filter = 0;
}
#endif

bool SVGResources::setMarkerStart(RenderSVGResourceMarker* markerStart)
{
    if (!markerStart)
        return false;

    ASSERT(markerStart->resourceType() == MarkerResourceType);

    if (!m_markerData)
        m_markerData = MarkerData::create();

    m_markerData->markerStart = markerStart;
    return true;
}

void SVGResources::resetMarkerStart()
{
    ASSERT(m_markerData);
    ASSERT(m_markerData->markerStart);
    m_markerData->markerStart = 0;
}

bool SVGResources::setMarkerMid(RenderSVGResourceMarker* markerMid)
{
    if (!markerMid)
        return false;

    ASSERT(markerMid->resourceType() == MarkerResourceType);

    if (!m_markerData)
        m_markerData = MarkerData::create();

    m_markerData->markerMid = markerMid;
    return true;
}

void SVGResources::resetMarkerMid()
{
    ASSERT(m_markerData);
    ASSERT(m_markerData->markerMid);
    m_markerData->markerMid = 0;
}

bool SVGResources::setMarkerEnd(RenderSVGResourceMarker* markerEnd)
{
    if (!markerEnd)
        return false;

    ASSERT(markerEnd->resourceType() == MarkerResourceType);

    if (!m_markerData)
        m_markerData = MarkerData::create();

    m_markerData->markerEnd = markerEnd;
    return true;
}

void SVGResources::resetMarkerEnd()
{
    ASSERT(m_markerData);
    ASSERT(m_markerData->markerEnd);
    m_markerData->markerEnd = 0;
}

bool SVGResources::setMasker(RenderSVGResourceMasker* masker)
{
    if (!masker)
        return false;

    ASSERT(masker->resourceType() == MaskerResourceType);

    if (!m_clipperFilterMaskerData)
        m_clipperFilterMaskerData = ClipperFilterMaskerData::create();

    m_clipperFilterMaskerData->masker = masker;
    return true;
}

void SVGResources::resetMasker()
{
    ASSERT(m_clipperFilterMaskerData);
    ASSERT(m_clipperFilterMaskerData->masker);
    m_clipperFilterMaskerData->masker = 0;
}

bool SVGResources::setFill(RenderSVGResourceContainer* fill)
{
    if (!fill)
        return false;

    ASSERT(fill->resourceType() == PatternResourceType
           || fill->resourceType() == LinearGradientResourceType
           || fill->resourceType() == RadialGradientResourceType);

    if (!m_fillStrokeData)
        m_fillStrokeData = FillStrokeData::create();

    m_fillStrokeData->fill = fill;
    return true;
}

void SVGResources::resetFill()
{
    ASSERT(m_fillStrokeData);
    ASSERT(m_fillStrokeData->fill);
    m_fillStrokeData->fill = 0;
}

bool SVGResources::setStroke(RenderSVGResourceContainer* stroke)
{
    if (!stroke)
        return false;

    ASSERT(stroke->resourceType() == PatternResourceType
           || stroke->resourceType() == LinearGradientResourceType
           || stroke->resourceType() == RadialGradientResourceType);

    if (!m_fillStrokeData)
        m_fillStrokeData = FillStrokeData::create();

    m_fillStrokeData->stroke = stroke;
    return true;
}

void SVGResources::resetStroke()
{
    ASSERT(m_fillStrokeData);
    ASSERT(m_fillStrokeData->stroke);
    m_fillStrokeData->stroke = 0;
}

bool SVGResources::setLinkedResource(RenderSVGResourceContainer* linkedResource)
{
    if (!linkedResource)
        return false;

    m_linkedResource = linkedResource;
    return true;
}

void SVGResources::resetLinkedResource()
{
    ASSERT(m_linkedResource);
    m_linkedResource = 0;
}

#ifndef NDEBUG
void SVGResources::dump(const RenderObject* object)
{
    ASSERT(object);
    ASSERT(object->node());

    fprintf(stderr, "-> this=%p, SVGResources(renderer=%p, node=%p)\n", this, object, object->node());
    fprintf(stderr, " | DOM Tree:\n");
    object->node()->showTreeForThis();

    fprintf(stderr, "\n | List of resources:\n");
    if (m_clipperFilterMaskerData) {
        if (RenderSVGResourceClipper* clipper = m_clipperFilterMaskerData->clipper)
            fprintf(stderr, " |-> Clipper    : %p (node=%p)\n", clipper, clipper->node());
#if ENABLE(FILTERS)
        if (RenderSVGResourceFilter* filter = m_clipperFilterMaskerData->filter)
            fprintf(stderr, " |-> Filter     : %p (node=%p)\n", filter, filter->node());
#endif
        if (RenderSVGResourceMasker* masker = m_clipperFilterMaskerData->masker)
            fprintf(stderr, " |-> Masker     : %p (node=%p)\n", masker, masker->node());
    }

    if (m_markerData) {
        if (RenderSVGResourceMarker* markerStart = m_markerData->markerStart)
            fprintf(stderr, " |-> MarkerStart: %p (node=%p)\n", markerStart, markerStart->node());
        if (RenderSVGResourceMarker* markerMid = m_markerData->markerMid)
            fprintf(stderr, " |-> MarkerMid  : %p (node=%p)\n", markerMid, markerMid->node());
        if (RenderSVGResourceMarker* markerEnd = m_markerData->markerEnd)
            fprintf(stderr, " |-> MarkerEnd  : %p (node=%p)\n", markerEnd, markerEnd->node());
    }

    if (m_fillStrokeData) {
        if (RenderSVGResourceContainer* fill = m_fillStrokeData->fill)
            fprintf(stderr, " |-> Fill       : %p (node=%p)\n", fill, fill->node());
        if (RenderSVGResourceContainer* stroke = m_fillStrokeData->stroke)
            fprintf(stderr, " |-> Stroke     : %p (node=%p)\n", stroke, stroke->node());
    }

    if (m_linkedResource)
        fprintf(stderr, " |-> xlink:href : %p (node=%p)\n", m_linkedResource, m_linkedResource->node());
}
#endif

}

#endif
