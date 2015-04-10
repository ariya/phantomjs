/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "GestureTapHighlighter.h"

#include "Element.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "GraphicsTypes.h"
#include "Node.h"
#include "Page.h"
#include "RenderBoxModelObject.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderObject.h"

namespace WebCore {

namespace {

inline LayoutPoint ownerFrameToMainFrameOffset(const RenderObject* o)
{
    ASSERT(o->node());
    Frame* containingFrame = o->frame();
    if (!containingFrame)
        return LayoutPoint();

    Frame* mainFrame = containingFrame->page()->mainFrame();

    LayoutPoint mainFramePoint = mainFrame->view()->windowToContents(containingFrame->view()->contentsToWindow(IntPoint()));
    return mainFramePoint;
}

AffineTransform localToAbsoluteTransform(const RenderObject* o)
{
    AffineTransform transform;
    LayoutPoint referencePoint;

    while (o) {
        RenderObject* nextContainer = o->container();
        if (!nextContainer)
            break;

        LayoutSize containerOffset = o->offsetFromContainer(nextContainer, referencePoint);
        TransformationMatrix t;
        o->getTransformFromContainer(nextContainer, containerOffset, t);

        transform = t.toAffineTransform() * transform;
        referencePoint.move(containerOffset);
        o = nextContainer;
    }

    return transform;
}

inline bool contains(const LayoutRect& rect, int x)
{
    return !rect.isEmpty() && x >= rect.x() && x <= rect.maxX();
}

inline bool strikes(const LayoutRect& a, const LayoutRect& b)
{
    return !a.isEmpty() && !b.isEmpty()
        && a.x() <= b.maxX() && b.x() <= a.maxX()
        && a.y() <= b.maxY() && b.y() <= a.maxY();
}

inline void shiftXEdgesToContainIfStrikes(LayoutRect& rect, LayoutRect& other, bool isFirst)
{
    if (rect.isEmpty())
        return;

    if (other.isEmpty() || !strikes(rect, other))
        return;

    LayoutUnit leftSide = std::min(rect.x(), other.x());
    LayoutUnit rightSide = std::max(rect.maxX(), other.maxX());

    rect.shiftXEdgeTo(leftSide);
    rect.shiftMaxXEdgeTo(rightSide);

    if (isFirst)
        other.shiftMaxXEdgeTo(rightSide);
    else
        other.shiftXEdgeTo(leftSide);
}

inline void addHighlightRect(Path& path, const LayoutRect& rect, const LayoutRect& prev, const LayoutRect& next)
{
    // The rounding check depends on the rects not intersecting eachother,
    // or being contained for that matter.
    ASSERT(!rect.intersects(prev));
    ASSERT(!rect.intersects(next));

    if (rect.isEmpty())
        return;

    const int rounding = 4;

    FloatRect copy(rect);
    copy.inflateX(rounding);
    copy.inflateY(rounding / 2);

    FloatSize rounded(rounding * 1.8, rounding * 1.8);
    FloatSize squared(0, 0);

    path.addBeziersForRoundedRect(copy,
            contains(prev, rect.x()) ? squared : rounded,
            contains(prev, rect.maxX()) ? squared : rounded,
            contains(next, rect.x()) ? squared : rounded,
            contains(next, rect.maxX()) ? squared : rounded);
}

Path absolutePathForRenderer(RenderObject* const o)
{
    ASSERT(o);

    Vector<IntRect> rects;
    LayoutPoint frameOffset = ownerFrameToMainFrameOffset(o);
    o->addFocusRingRects(rects, frameOffset);

    if (rects.isEmpty())
        return Path();

    // The basic idea is to allow up to three different boxes in order to highlight
    // text with line breaks more nicer than using a bounding box.

    // Merge all center boxes (all but the first and the last).
    LayoutRect mid;

    // Set the end value to integer. It ensures that no unsigned int overflow occurs
    // in the test expression, in case of empty rects vector.
    int end = rects.size() - 1;
    for (int i = 1; i < end; ++i)
        mid.uniteIfNonZero(rects.at(i));

    LayoutRect first;
    LayoutRect last;

    // Add the first box, but merge it with the center boxes if it intersects or if the center box is empty.
    if (rects.size() && !rects.first().isEmpty()) {
        // If the mid box is empty at this point, unite it with the first box. This allows the first box to be
        // united with the last box if they intersect in the following check for last. Not uniting them would
        // trigger in assert in addHighlighRect due to the first and the last box intersecting, but being passed
        // as two separate boxes.
        if (mid.isEmpty() || mid.intersects(rects.first()))
            mid.unite(rects.first());
        else {
            first = rects.first();
            shiftXEdgesToContainIfStrikes(mid, first, /* isFirst */ true);
        }
    }

    // Add the last box, but merge it with the center boxes if it intersects.
    if (rects.size() > 1 && !rects.last().isEmpty()) {
        // Adjust center boxes to boundary of last
        if (mid.intersects(rects.last()))
            mid.unite(rects.last());
        else {
            last = rects.last();
            shiftXEdgesToContainIfStrikes(mid, last, /* isFirst */ false);
        }
    }

    Vector<LayoutRect> drawableRects;
    if (!first.isEmpty())
        drawableRects.append(first);
    if (!mid.isEmpty())
        drawableRects.append(mid);
    if (!last.isEmpty())
        drawableRects.append(last);

    // Clip the overflow rects if needed, before the ring path is formed to
    // ensure rounded highlight rects.
    for (int i = drawableRects.size() - 1; i >= 0; --i) {
        LayoutRect& ringRect = drawableRects.at(i);
        LayoutPoint ringRectLocation = ringRect.location();

        ringRect.moveBy(-frameOffset);

        RenderLayer* layer = o->enclosingLayer();
        RenderObject* currentRenderer = o;

        // Check ancestor layers for overflow clip and intersect them.
        for (; layer; layer = layer->parent()) {
            RenderLayerModelObject* layerRenderer = layer->renderer();

            if (layerRenderer->hasOverflowClip() && layerRenderer != currentRenderer) {
                bool containerSkipped = false;
                // Skip ancestor layers that are not containers for the current renderer.
                currentRenderer->container(layerRenderer, &containerSkipped);
                if (containerSkipped)
                    continue;
                FloatQuad ringQuad = currentRenderer->localToContainerQuad(FloatQuad(ringRect), layerRenderer);
                // Ignore quads that are not rectangular, since we can not currently highlight them nicely.
                if (ringQuad.isRectilinear())
                    ringRect = ringQuad.enclosingBoundingBox();
                else
                    ringRect = LayoutRect();
                currentRenderer = layerRenderer;

                ASSERT(layerRenderer->isBox());
                ringRect.intersect(toRenderBox(layerRenderer)->borderBoxRect());

                if (ringRect.isEmpty())
                    break;
            }
        }

        if (ringRect.isEmpty()) {
            drawableRects.remove(i);
            continue;
        }
        // After clipping, reset the original position so that parents' transforms apply correctly.
        ringRect.setLocation(ringRectLocation);
    }

    Path path;
    for (size_t i = 0; i < drawableRects.size(); ++i) {
        LayoutRect prev = i ? drawableRects.at(i - 1) : LayoutRect();
        LayoutRect next = i < (drawableRects.size() - 1) ? drawableRects.at(i + 1) : LayoutRect();
        addHighlightRect(path, drawableRects.at(i), prev, next);
    }

    path.transform(localToAbsoluteTransform(o));
    return path;
}

} // anonymous namespace

namespace GestureTapHighlighter {

Path pathForNodeHighlight(const Node* node)
{
    RenderObject* renderer = node->renderer();

    if (!renderer || (!renderer->isBox() && !renderer->isRenderInline()))
        return Path();

    return absolutePathForRenderer(renderer);
}

} // namespace GestureTapHighlighter

} // namespace WebCore
