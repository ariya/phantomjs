/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 *           (C) 2004 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
 *
 */

#ifndef PaintInfo_h
#define PaintInfo_h

#if ENABLE(SVG)
#include "AffineTransform.h"
#endif

#include "GraphicsContext.h"
#include "IntRect.h"
#include "LayoutRect.h"
#include "PaintPhase.h"
#include <limits>
#include <wtf/HashMap.h>
#include <wtf/ListHashSet.h>

namespace WebCore {

class OverlapTestRequestClient;
class RenderInline;
class RenderLayerModelObject;
class RenderObject;
class RenderRegion;

typedef HashMap<OverlapTestRequestClient*, IntRect> OverlapTestRequestMap;

/*
 * Paint the object and its children, clipped by (x|y|w|h).
 * (tx|ty) is the calculated position of the parent
 */
struct PaintInfo {
    PaintInfo(GraphicsContext* newContext, const IntRect& newRect, PaintPhase newPhase, PaintBehavior newPaintBehavior,
        RenderObject* newSubtreePaintRoot = 0, RenderRegion* region = 0, ListHashSet<RenderInline*>* newOutlineObjects = 0,
        OverlapTestRequestMap* overlapTestRequests = 0, const RenderLayerModelObject* newPaintContainer = 0)
        : context(newContext)
        , rect(newRect)
        , phase(newPhase)
        , paintBehavior(newPaintBehavior)
        , subtreePaintRoot(newSubtreePaintRoot)
        , renderRegion(region)
        , outlineObjects(newOutlineObjects)
        , overlapTestRequests(overlapTestRequests)
        , paintContainer(newPaintContainer)
    {
    }

    void updateSubtreePaintRootForChildren(const RenderObject* renderer)
    {
        if (!subtreePaintRoot)
            return;

        // If we're the painting root, kids draw normally, and see root of 0.
        if (subtreePaintRoot == renderer) {
            subtreePaintRoot = 0; 
            return;
        }
    }

    bool shouldPaintWithinRoot(const RenderObject* renderer) const
    {
        return !subtreePaintRoot || subtreePaintRoot == renderer;
    }

    bool forceBlackText() const { return paintBehavior & PaintBehaviorForceBlackText; }

    bool skipRootBackground() const { return paintBehavior & PaintBehaviorSkipRootBackground; }
    bool paintRootBackgroundOnly() const { return paintBehavior & PaintBehaviorRootBackgroundOnly; }

#if ENABLE(SVG)
    void applyTransform(const AffineTransform& localToAncestorTransform)
    {
        if (localToAncestorTransform.isIdentity())
            return;

        context->concatCTM(localToAncestorTransform);

        if (rect == infiniteRect())
            return;

        rect = localToAncestorTransform.inverse().mapRect(rect);
    }
#endif

    static IntRect infiniteRect() { return IntRect(LayoutRect::infiniteRect()); }

    // FIXME: Introduce setters/getters at some point. Requires a lot of changes throughout rendering/.
    GraphicsContext* context;
    IntRect rect;
    PaintPhase phase;
    PaintBehavior paintBehavior;
    RenderObject* subtreePaintRoot; // used to draw just one element and its visual children
    RenderRegion* renderRegion;
    ListHashSet<RenderInline*>* outlineObjects; // used to list outlines that should be painted by a block with inline children
    OverlapTestRequestMap* overlapTestRequests;
    const RenderLayerModelObject* paintContainer; // the layer object that originates the current painting
};

} // namespace WebCore

#endif // PaintInfo_h
