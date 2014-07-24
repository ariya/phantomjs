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

#ifndef SVGResources_h
#define SVGResources_h

#if ENABLE(SVG)
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class Document;
class RenderObject;
class RenderSVGResourceClipper;
class RenderSVGResourceContainer;
class RenderSVGResourceFilter;
class RenderSVGResourceMarker;
class RenderSVGResourceMasker;
class SVGRenderStyle;

// Holds a set of resources associated with a RenderObject
class SVGResources {
    WTF_MAKE_NONCOPYABLE(SVGResources); WTF_MAKE_FAST_ALLOCATED;
public:
    SVGResources();

    bool buildCachedResources(const RenderObject*, const SVGRenderStyle*);

    // Ordinary resources
    RenderSVGResourceClipper* clipper() const { return m_clipperFilterMaskerData ? m_clipperFilterMaskerData->clipper : 0; }
    RenderSVGResourceMarker* markerStart() const { return m_markerData ? m_markerData->markerStart : 0; }
    RenderSVGResourceMarker* markerMid() const { return m_markerData ? m_markerData->markerMid : 0; }
    RenderSVGResourceMarker* markerEnd() const { return m_markerData ? m_markerData->markerEnd : 0; }
    RenderSVGResourceMasker* masker() const { return m_clipperFilterMaskerData ? m_clipperFilterMaskerData->masker : 0; }

    RenderSVGResourceFilter* filter() const
    {
#if ENABLE(FILTERS)
        if (m_clipperFilterMaskerData)
            return m_clipperFilterMaskerData->filter;
#endif
        return 0;
    }

    // Paint servers
    RenderSVGResourceContainer* fill() const { return m_fillStrokeData ? m_fillStrokeData->fill : 0; }
    RenderSVGResourceContainer* stroke() const { return m_fillStrokeData ? m_fillStrokeData->stroke : 0; }

    // Chainable resources - linked through xlink:href
    RenderSVGResourceContainer* linkedResource() const { return m_linkedResource; }

    void buildSetOfResources(HashSet<RenderSVGResourceContainer*>&);

    // Methods operating on all cached resources
    void removeClientFromCache(RenderObject*, bool markForInvalidation = true) const;
    void resourceDestroyed(RenderSVGResourceContainer*);

#ifndef NDEBUG
    void dump(const RenderObject*);
#endif

private:
    friend class SVGResourcesCycleSolver;

    // Only used by SVGResourcesCache cycle detection logic
    void resetClipper();
#if ENABLE(FILTERS)
    void resetFilter();
#endif
    void resetMarkerStart();
    void resetMarkerMid();
    void resetMarkerEnd();
    void resetMasker();
    void resetFill();
    void resetStroke();
    void resetLinkedResource();

private:
    bool setClipper(RenderSVGResourceClipper*);
#if ENABLE(FILTERS)
    bool setFilter(RenderSVGResourceFilter*);
#endif
    bool setMarkerStart(RenderSVGResourceMarker*);
    bool setMarkerMid(RenderSVGResourceMarker*);
    bool setMarkerEnd(RenderSVGResourceMarker*);
    bool setMasker(RenderSVGResourceMasker*);
    bool setFill(RenderSVGResourceContainer*);
    bool setStroke(RenderSVGResourceContainer*);
    bool setLinkedResource(RenderSVGResourceContainer*);

    // From SVG 1.1 2nd Edition
    // clipper: 'container elements' and 'graphics elements'
    // filter:  'container elements' and 'graphics elements'
    // masker:  'container elements' and 'graphics elements'
    // -> a, circle, defs, ellipse, glyph, g, image, line, marker, mask, missing-glyph, path, pattern, polygon, polyline, rect, svg, switch, symbol, text, use
    struct ClipperFilterMaskerData {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        ClipperFilterMaskerData()
            : clipper(0)
#if ENABLE(FILTERS)
            , filter(0)
#endif
            , masker(0)
        {
        }

        static PassOwnPtr<ClipperFilterMaskerData> create()
        {
            return adoptPtr(new ClipperFilterMaskerData);
        }

        RenderSVGResourceClipper* clipper;
#if ENABLE(FILTERS)
        RenderSVGResourceFilter* filter;
#endif
        RenderSVGResourceMasker* masker;
    };

    // From SVG 1.1 2nd Edition
    // marker: line, path, polygon, polyline
    struct MarkerData {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        MarkerData()
            : markerStart(0)
            , markerMid(0)
            , markerEnd(0)
        {
        }

        static PassOwnPtr<MarkerData> create()
        {
            return adoptPtr(new MarkerData);
        }

        RenderSVGResourceMarker* markerStart;
        RenderSVGResourceMarker* markerMid;
        RenderSVGResourceMarker* markerEnd;
    };

    // From SVG 1.1 2nd Edition
    // fill:       'shapes' and 'text content elements'
    // stroke:     'shapes' and 'text content elements'
    // -> altGlyph, circle, ellipse, line, path, polygon, polyline, rect, text, textPath, tref, tspan
    struct FillStrokeData {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        FillStrokeData()
            : fill(0)
            , stroke(0)
        {
        }

        static PassOwnPtr<FillStrokeData> create()
        {
            return adoptPtr(new FillStrokeData);
        }

        RenderSVGResourceContainer* fill;
        RenderSVGResourceContainer* stroke;
    };

    OwnPtr<ClipperFilterMaskerData> m_clipperFilterMaskerData;
    OwnPtr<MarkerData> m_markerData;
    OwnPtr<FillStrokeData> m_fillStrokeData;
    RenderSVGResourceContainer* m_linkedResource;
};

}

#endif
#endif
