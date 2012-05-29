/*
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Antonio Gomes <tonikitoo@webkit.org>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SpatialNavigation.h"

#include "Frame.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "HTMLAreaElement.h"
#include "HTMLImageElement.h"
#include "HTMLMapElement.h"
#include "HTMLNames.h"
#include "IntRect.h"
#include "Node.h"
#include "Page.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "Settings.h"

namespace WebCore {

static RectsAlignment alignmentForRects(FocusDirection, const IntRect&, const IntRect&, const IntSize& viewSize);
static bool areRectsFullyAligned(FocusDirection, const IntRect&, const IntRect&);
static bool areRectsPartiallyAligned(FocusDirection, const IntRect&, const IntRect&);
static bool areRectsMoreThanFullScreenApart(FocusDirection direction, const IntRect& curRect, const IntRect& targetRect, const IntSize& viewSize);
static bool isRectInDirection(FocusDirection, const IntRect&, const IntRect&);
static void deflateIfOverlapped(IntRect&, IntRect&);
static IntRect rectToAbsoluteCoordinates(Frame* initialFrame, const IntRect&);
static void entryAndExitPointsForDirection(FocusDirection direction, const IntRect& startingRect, const IntRect& potentialRect, IntPoint& exitPoint, IntPoint& entryPoint);
static bool isScrollableNode(const Node*);

FocusCandidate::FocusCandidate(Node* node, FocusDirection direction)
    : visibleNode(0)
    , focusableNode(0)
    , enclosingScrollableBox(0)
    , distance(maxDistance())
    , parentDistance(maxDistance())
    , alignment(None)
    , parentAlignment(None)
    , isOffscreen(true)
    , isOffscreenAfterScrolling(true)
{
    ASSERT(node);
    ASSERT(node->isElementNode());

    if (node->hasTagName(HTMLNames::areaTag)) {
        HTMLAreaElement* area = static_cast<HTMLAreaElement*>(node);
        HTMLImageElement* image = area->imageElement();
        if (!image || !image->renderer())
            return;

        visibleNode = image;
        rect = virtualRectForAreaElementAndDirection(area, direction);
    } else {
        if (!node->renderer())
            return;

        visibleNode = node;
        rect = nodeRectInAbsoluteCoordinates(node, true /* ignore border */);
    }

    focusableNode = node;
    isOffscreen = hasOffscreenRect(visibleNode);
    isOffscreenAfterScrolling = hasOffscreenRect(visibleNode, direction);
}

bool isSpatialNavigationEnabled(const Frame* frame)
{
    return (frame && frame->settings() && frame->settings()->isSpatialNavigationEnabled());
}

static RectsAlignment alignmentForRects(FocusDirection direction, const IntRect& curRect, const IntRect& targetRect, const IntSize& viewSize)
{
    // If we found a node in full alignment, but it is too far away, ignore it.
    if (areRectsMoreThanFullScreenApart(direction, curRect, targetRect, viewSize))
        return None;

    if (areRectsFullyAligned(direction, curRect, targetRect))
        return Full;

    if (areRectsPartiallyAligned(direction, curRect, targetRect))
        return Partial;

    return None;
}

static inline bool isHorizontalMove(FocusDirection direction)
{
    return direction == FocusDirectionLeft || direction == FocusDirectionRight;
}

static inline int start(FocusDirection direction, const IntRect& rect)
{
    return isHorizontalMove(direction) ? rect.y() : rect.x();
}

static inline int middle(FocusDirection direction, const IntRect& rect)
{
    IntPoint center(rect.center());
    return isHorizontalMove(direction) ? center.y(): center.x();
}

static inline int end(FocusDirection direction, const IntRect& rect)
{
    return isHorizontalMove(direction) ? rect.maxY() : rect.maxX();
}

// This method checks if rects |a| and |b| are fully aligned either vertically or
// horizontally. In general, rects whose central point falls between the top or
// bottom of each other are considered fully aligned.
// Rects that match this criteria are preferable target nodes in move focus changing
// operations.
// * a = Current focused node's rect.
// * b = Focus candidate node's rect.
static bool areRectsFullyAligned(FocusDirection direction, const IntRect& a, const IntRect& b)
{
    int aStart, bStart, aEnd, bEnd;

    switch (direction) {
    case FocusDirectionLeft:
        aStart = a.x();
        bEnd = b.maxX();
        break;
    case FocusDirectionRight:
        aStart = b.x();
        bEnd = a.maxX();
        break;
    case FocusDirectionUp:
        aStart = a.y();
        bEnd = b.y();
        break;
    case FocusDirectionDown:
        aStart = b.y();
        bEnd = a.y();
        break;
    default:
        ASSERT_NOT_REACHED();
        return false;
    }

    if (aStart < bEnd)
        return false;

    aStart = start(direction, a);
    bStart = start(direction, b);

    int aMiddle = middle(direction, a);
    int bMiddle = middle(direction, b);

    aEnd = end(direction, a);
    bEnd = end(direction, b);

    // Picture of the totally aligned logic:
    //
    //     Horizontal    Vertical        Horizontal     Vertical
    //  ****************************  *****************************
    //  *  _          *   _ _ _ _  *  *         _   *      _ _    *
    //  * |_|     _   *  |_|_|_|_| *  *  _     |_|  *     |_|_|   *
    //  * |_|....|_|  *      .     *  * |_|....|_|  *       .     *
    //  * |_|    |_| (1)     .     *  * |_|    |_| (2)      .     *
    //  * |_|         *     _._    *  *        |_|  *    _ _._ _  *
    //  *             *    |_|_|   *  *             *   |_|_|_|_| *
    //  *             *            *  *             *             *
    //  ****************************  *****************************

    //     Horizontal    Vertical        Horizontal     Vertical
    //  ****************************  *****************************
    //  *  _......_   *   _ _ _ _  *  *  _          *    _ _ _ _  *
    //  * |_|    |_|  *  |_|_|_|_| *  * |_|     _   *   |_|_|_|_| *
    //  * |_|    |_|  *  .         *  * |_|    |_|  *           . *
    //  * |_|        (3) .         *  * |_|....|_| (4)          . *
    //  *             *  ._ _      *  *             *        _ _. *
    //  *             *  |_|_|     *  *             *       |_|_| *
    //  *             *            *  *             *             *
    //  ****************************  *****************************

    return ((bMiddle >= aStart && bMiddle <= aEnd) // (1)
            || (aMiddle >= bStart && aMiddle <= bEnd) // (2)
            || (bStart == aStart) // (3)
            || (bEnd == aEnd)); // (4)
}

// This method checks if |start| and |dest| have a partial intersection, either
// horizontally or vertically.
// * a = Current focused node's rect.
// * b = Focus candidate node's rect.
static bool areRectsPartiallyAligned(FocusDirection direction, const IntRect& a, const IntRect& b)
{
    int aStart  = start(direction, a);
    int bStart  = start(direction, b);
    int bMiddle = middle(direction, b);
    int aEnd = end(direction, a);
    int bEnd = end(direction, b);

    // Picture of the partially aligned logic:
    //
    //    Horizontal       Vertical
    // ********************************
    // *  _            *   _ _ _      *
    // * |_|           *  |_|_|_|     *
    // * |_|.... _     *      . .     *
    // * |_|    |_|    *      . .     *
    // * |_|....|_|    *      ._._ _  *
    // *        |_|    *      |_|_|_| *
    // *        |_|    *              *
    // *               *              *
    // ********************************
    //
    // ... and variants of the above cases.
    return ((bStart >= aStart && bStart <= aEnd)
            || (bStart >= aStart && bStart <= aEnd)
            || (bEnd >= aStart && bEnd <= aEnd)
            || (bMiddle >= aStart && bMiddle <= aEnd)
            || (bEnd >= aStart && bEnd <= aEnd));
}

static bool areRectsMoreThanFullScreenApart(FocusDirection direction, const IntRect& curRect, const IntRect& targetRect, const IntSize& viewSize)
{
    ASSERT(isRectInDirection(direction, curRect, targetRect));

    switch (direction) {
    case FocusDirectionLeft:
        return curRect.x() - targetRect.maxX() > viewSize.width();
    case FocusDirectionRight:
        return targetRect.x() - curRect.maxX() > viewSize.width();
    case FocusDirectionUp:
        return curRect.y() - targetRect.maxY() > viewSize.height();
    case FocusDirectionDown:
        return targetRect.y() - curRect.maxY() > viewSize.height();
    default:
        ASSERT_NOT_REACHED();
        return true;
    }
}

// Return true if rect |a| is below |b|. False otherwise.
static inline bool below(const IntRect& a, const IntRect& b)
{
    return a.y() > b.maxY();
}

// Return true if rect |a| is on the right of |b|. False otherwise.
static inline bool rightOf(const IntRect& a, const IntRect& b)
{
    return a.x() > b.maxX();
}

static bool isRectInDirection(FocusDirection direction, const IntRect& curRect, const IntRect& targetRect)
{
    switch (direction) {
    case FocusDirectionLeft:
        return targetRect.maxX() <= curRect.x();
    case FocusDirectionRight:
        return targetRect.x() >= curRect.maxX();
    case FocusDirectionUp:
        return targetRect.maxY() <= curRect.y();
    case FocusDirectionDown:
        return targetRect.y() >= curRect.maxY();
    default:
        ASSERT_NOT_REACHED();
        return false;
    }
}

// Checks if |node| is offscreen the visible area (viewport) of its container
// document. In case it is, one can scroll in direction or take any different
// desired action later on.
bool hasOffscreenRect(Node* node, FocusDirection direction)
{
    // Get the FrameView in which |node| is (which means the current viewport if |node|
    // is not in an inner document), so we can check if its content rect is visible
    // before we actually move the focus to it.
    FrameView* frameView = node->document()->view();
    if (!frameView)
        return true;

    ASSERT(!frameView->needsLayout());

    IntRect containerViewportRect = frameView->visibleContentRect();
    // We want to select a node if it is currently off screen, but will be
    // exposed after we scroll. Adjust the viewport to post-scrolling position.
    // If the container has overflow:hidden, we cannot scroll, so we do not pass direction
    // and we do not adjust for scrolling.
    switch (direction) {
    case FocusDirectionLeft:
        containerViewportRect.setX(containerViewportRect.x() - Scrollbar::pixelsPerLineStep());
        containerViewportRect.setWidth(containerViewportRect.width() + Scrollbar::pixelsPerLineStep());
        break;
    case FocusDirectionRight:
        containerViewportRect.setWidth(containerViewportRect.width() + Scrollbar::pixelsPerLineStep());
        break;
    case FocusDirectionUp:
        containerViewportRect.setY(containerViewportRect.y() - Scrollbar::pixelsPerLineStep());
        containerViewportRect.setHeight(containerViewportRect.height() + Scrollbar::pixelsPerLineStep());
        break;
    case FocusDirectionDown:
        containerViewportRect.setHeight(containerViewportRect.height() + Scrollbar::pixelsPerLineStep());
        break;
    default:
        break;
    }

    RenderObject* render = node->renderer();
    if (!render)
        return true;

    IntRect rect(render->absoluteClippedOverflowRect());
    if (rect.isEmpty())
        return true;

    return !containerViewportRect.intersects(rect);
}

bool scrollInDirection(Frame* frame, FocusDirection direction)
{
    ASSERT(frame);

    if (frame && canScrollInDirection(frame->document(), direction)) {
        int dx = 0;
        int dy = 0;
        switch (direction) {
        case FocusDirectionLeft:
            dx = - Scrollbar::pixelsPerLineStep();
            break;
        case FocusDirectionRight:
            dx = Scrollbar::pixelsPerLineStep();
            break;
        case FocusDirectionUp:
            dy = - Scrollbar::pixelsPerLineStep();
            break;
        case FocusDirectionDown:
            dy = Scrollbar::pixelsPerLineStep();
            break;
        default:
            ASSERT_NOT_REACHED();
            return false;
        }

        frame->view()->scrollBy(IntSize(dx, dy));
        return true;
    }
    return false;
}

bool scrollInDirection(Node* container, FocusDirection direction)
{
    ASSERT(container);
    if (container->isDocumentNode())
        return scrollInDirection(static_cast<Document*>(container)->frame(), direction);

    if (!container->renderBox())
        return false;

    if (canScrollInDirection(container, direction)) {
        int dx = 0;
        int dy = 0;
        switch (direction) {
        case FocusDirectionLeft:
            dx = - min(Scrollbar::pixelsPerLineStep(), container->renderBox()->scrollLeft());
            break;
        case FocusDirectionRight:
            ASSERT(container->renderBox()->scrollWidth() > (container->renderBox()->scrollLeft() + container->renderBox()->clientWidth()));
            dx = min(Scrollbar::pixelsPerLineStep(), container->renderBox()->scrollWidth() - (container->renderBox()->scrollLeft() + container->renderBox()->clientWidth()));
            break;
        case FocusDirectionUp:
            dy = - min(Scrollbar::pixelsPerLineStep(), container->renderBox()->scrollTop());
            break;
        case FocusDirectionDown:
            ASSERT(container->renderBox()->scrollHeight() - (container->renderBox()->scrollTop() + container->renderBox()->clientHeight()));
            dy = min(Scrollbar::pixelsPerLineStep(), container->renderBox()->scrollHeight() - (container->renderBox()->scrollTop() + container->renderBox()->clientHeight()));
            break;
        default:
            ASSERT_NOT_REACHED();
            return false;
        }

        container->renderBox()->enclosingLayer()->scrollByRecursively(dx, dy);
        return true;
    }

    return false;
}

static void deflateIfOverlapped(IntRect& a, IntRect& b)
{
    if (!a.intersects(b) || a.contains(b) || b.contains(a))
        return;

    int deflateFactor = -fudgeFactor();

    // Avoid negative width or height values.
    if ((a.width() + 2 * deflateFactor > 0) && (a.height() + 2 * deflateFactor > 0))
        a.inflate(deflateFactor);

    if ((b.width() + 2 * deflateFactor > 0) && (b.height() + 2 * deflateFactor > 0))
        b.inflate(deflateFactor);
}

bool isScrollableNode(const Node* node)
{
    ASSERT(!node->isDocumentNode());

    if (!node)
        return false;

    if (RenderObject* renderer = node->renderer())
        return renderer->isBox() && toRenderBox(renderer)->canBeScrolledAndHasScrollableArea() && node->hasChildNodes();

    return false;
}

Node* scrollableEnclosingBoxOrParentFrameForNodeInDirection(FocusDirection direction, Node* node)
{
    ASSERT(node);
    Node* parent = node;
    do {
        if (parent->isDocumentNode())
            parent = static_cast<Document*>(parent)->document()->frame()->ownerElement();
        else
            parent = parent->parentNode();
    } while (parent && !canScrollInDirection(parent, direction) && !parent->isDocumentNode());

    return parent;
}

bool canScrollInDirection(const Node* container, FocusDirection direction)
{
    ASSERT(container);
    if (container->isDocumentNode())
        return canScrollInDirection(static_cast<const Document*>(container)->frame(), direction);

    if (!isScrollableNode(container))
        return false;

    switch (direction) {
    case FocusDirectionLeft:
        return (container->renderer()->style()->overflowX() != OHIDDEN && container->renderBox()->scrollLeft() > 0);
    case FocusDirectionUp:
        return (container->renderer()->style()->overflowY() != OHIDDEN && container->renderBox()->scrollTop() > 0);
    case FocusDirectionRight:
        return (container->renderer()->style()->overflowX() != OHIDDEN && container->renderBox()->scrollLeft() + container->renderBox()->clientWidth() < container->renderBox()->scrollWidth());
    case FocusDirectionDown:
        return (container->renderer()->style()->overflowY() != OHIDDEN && container->renderBox()->scrollTop() + container->renderBox()->clientHeight() < container->renderBox()->scrollHeight());
    default:
        ASSERT_NOT_REACHED();
        return false;
    }
}

bool canScrollInDirection(const Frame* frame, FocusDirection direction)
{
    if (!frame->view())
        return false;
    ScrollbarMode verticalMode;
    ScrollbarMode horizontalMode;
    frame->view()->calculateScrollbarModesForLayout(horizontalMode, verticalMode);
    if ((direction == FocusDirectionLeft || direction == FocusDirectionRight) && ScrollbarAlwaysOff == horizontalMode)
        return false;
    if ((direction == FocusDirectionUp || direction == FocusDirectionDown) &&  ScrollbarAlwaysOff == verticalMode)
        return false;
    IntSize size = frame->view()->contentsSize();
    IntSize offset = frame->view()->scrollOffset();
    IntRect rect = frame->view()->visibleContentRect(true);

    switch (direction) {
    case FocusDirectionLeft:
        return offset.width() > 0;
    case FocusDirectionUp:
        return offset.height() > 0;
    case FocusDirectionRight:
        return rect.width() + offset.width() < size.width();
    case FocusDirectionDown:
        return rect.height() + offset.height() < size.height();
    default:
        ASSERT_NOT_REACHED();
        return false;
    }
}

static IntRect rectToAbsoluteCoordinates(Frame* initialFrame, const IntRect& initialRect)
{
    IntRect rect = initialRect;
    for (Frame* frame = initialFrame; frame; frame = frame->tree()->parent()) {
        if (Element* element = static_cast<Element*>(frame->ownerElement())) {
            do {
                rect.move(element->offsetLeft(), element->offsetTop());
            } while ((element = element->offsetParent()));
            rect.move((-frame->view()->scrollOffset()));
        }
    }
    return rect;
}

IntRect nodeRectInAbsoluteCoordinates(Node* node, bool ignoreBorder)
{
    ASSERT(node && node->renderer() && !node->document()->view()->needsLayout());

    if (node->isDocumentNode())
        return frameRectInAbsoluteCoordinates(static_cast<Document*>(node)->frame());
    IntRect rect = rectToAbsoluteCoordinates(node->document()->frame(), node->getRect());

    // For authors that use border instead of outline in their CSS, we compensate by ignoring the border when calculating
    // the rect of the focused element.
    if (ignoreBorder) {
        rect.move(node->renderer()->style()->borderLeftWidth(), node->renderer()->style()->borderTopWidth());
        rect.setWidth(rect.width() - node->renderer()->style()->borderLeftWidth() - node->renderer()->style()->borderRightWidth());
        rect.setHeight(rect.height() - node->renderer()->style()->borderTopWidth() - node->renderer()->style()->borderBottomWidth());
    }
    return rect;
}

IntRect frameRectInAbsoluteCoordinates(Frame* frame)
{
    return rectToAbsoluteCoordinates(frame, frame->view()->visibleContentRect());
}

// This method calculates the exitPoint from the startingRect and the entryPoint into the candidate rect.
// The line between those 2 points is the closest distance between the 2 rects.
void entryAndExitPointsForDirection(FocusDirection direction, const IntRect& startingRect, const IntRect& potentialRect, IntPoint& exitPoint, IntPoint& entryPoint)
{
    switch (direction) {
    case FocusDirectionLeft:
        exitPoint.setX(startingRect.x());
        entryPoint.setX(potentialRect.maxX());
        break;
    case FocusDirectionUp:
        exitPoint.setY(startingRect.y());
        entryPoint.setY(potentialRect.maxY());
        break;
    case FocusDirectionRight:
        exitPoint.setX(startingRect.maxX());
        entryPoint.setX(potentialRect.x());
        break;
    case FocusDirectionDown:
        exitPoint.setY(startingRect.maxY());
        entryPoint.setY(potentialRect.y());
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    switch (direction) {
    case FocusDirectionLeft:
    case FocusDirectionRight:
        if (below(startingRect, potentialRect)) {
            exitPoint.setY(startingRect.y());
            entryPoint.setY(potentialRect.maxY());
        } else if (below(potentialRect, startingRect)) {
            exitPoint.setY(startingRect.maxY());
            entryPoint.setY(potentialRect.y());
        } else {
            exitPoint.setY(max(startingRect.y(), potentialRect.y()));
            entryPoint.setY(exitPoint.y());
        }
        break;
    case FocusDirectionUp:
    case FocusDirectionDown:
        if (rightOf(startingRect, potentialRect)) {
            exitPoint.setX(startingRect.x());
            entryPoint.setX(potentialRect.maxX());
        } else if (rightOf(potentialRect, startingRect)) {
            exitPoint.setX(startingRect.maxX());
            entryPoint.setX(potentialRect.x());
        } else {
            exitPoint.setX(max(startingRect.x(), potentialRect.x()));
            entryPoint.setX(exitPoint.x());
        }
        break;
    default:
        ASSERT_NOT_REACHED();
    }
}

bool areElementsOnSameLine(const FocusCandidate& firstCandidate, const FocusCandidate& secondCandidate)
{
    if (firstCandidate.isNull() || secondCandidate.isNull())
        return false;

    if (!firstCandidate.visibleNode->renderer() || !secondCandidate.visibleNode->renderer())
        return false;

    if (!firstCandidate.rect.intersects(secondCandidate.rect))
        return false;

    if (firstCandidate.focusableNode->hasTagName(HTMLNames::areaTag) || secondCandidate.focusableNode->hasTagName(HTMLNames::areaTag))
        return false;

    if (!firstCandidate.visibleNode->renderer()->isRenderInline() || !secondCandidate.visibleNode->renderer()->isRenderInline())
        return false;

    if (firstCandidate.visibleNode->renderer()->containingBlock() != secondCandidate.visibleNode->renderer()->containingBlock())
        return false;

    return true;
}

void distanceDataForNode(FocusDirection direction, const FocusCandidate& current, FocusCandidate& candidate)
{
    if (areElementsOnSameLine(current, candidate)) {
        if ((direction == FocusDirectionUp && current.rect.y() > candidate.rect.y()) || (direction == FocusDirectionDown && candidate.rect.y() > current.rect.y())) {
            candidate.distance = 0;
            candidate.alignment = Full;
            return;
        }
    }

    IntRect nodeRect = candidate.rect;
    IntRect currentRect = current.rect;
    deflateIfOverlapped(currentRect, nodeRect);

    if (!isRectInDirection(direction, currentRect, nodeRect))
        return;

    IntPoint exitPoint;
    IntPoint entryPoint;
    int sameAxisDistance = 0;
    int otherAxisDistance = 0;
    entryAndExitPointsForDirection(direction, currentRect, nodeRect, exitPoint, entryPoint);

    switch (direction) {
    case FocusDirectionLeft:
        sameAxisDistance = exitPoint.x() - entryPoint.x();
        otherAxisDistance = abs(exitPoint.y() - entryPoint.y());
        break;
    case FocusDirectionUp:
        sameAxisDistance = exitPoint.y() - entryPoint.y();
        otherAxisDistance = abs(exitPoint.x() - entryPoint.x());
        break;
    case FocusDirectionRight:
        sameAxisDistance = entryPoint.x() - exitPoint.x();
        otherAxisDistance = abs(entryPoint.y() - exitPoint.y());
        break;
    case FocusDirectionDown:
        sameAxisDistance = entryPoint.y() - exitPoint.y();
        otherAxisDistance = abs(entryPoint.x() - exitPoint.x());
        break;
    default:
        ASSERT_NOT_REACHED();
        return;
    }

    int x = (entryPoint.x() - exitPoint.x()) * (entryPoint.x() - exitPoint.x());
    int y = (entryPoint.y() - exitPoint.y()) * (entryPoint.y() - exitPoint.y());

    float euclidianDistance = sqrt((x + y) * 1.0f);

    // Loosely based on http://www.w3.org/TR/WICD/#focus-handling
    // df = dotDist + dx + dy + 2 * (xdisplacement + ydisplacement) - sqrt(Overlap)

    float distance = euclidianDistance + sameAxisDistance + 2 * otherAxisDistance;
    candidate.distance = roundf(distance);
    IntSize viewSize = candidate.visibleNode->document()->page()->mainFrame()->view()->visibleContentRect().size();
    candidate.alignment = alignmentForRects(direction, currentRect, nodeRect, viewSize);
}

bool canBeScrolledIntoView(FocusDirection direction, const FocusCandidate& candidate)
{
    ASSERT(candidate.visibleNode && candidate.isOffscreen);
    IntRect candidateRect = candidate.rect;
    for (Node* parentNode = candidate.visibleNode->parentNode(); parentNode; parentNode = parentNode->parentNode()) {
        IntRect parentRect = nodeRectInAbsoluteCoordinates(parentNode);
        if (!candidateRect.intersects(parentRect)) {
            if (((direction == FocusDirectionLeft || direction == FocusDirectionRight) && parentNode->renderer()->style()->overflowX() == OHIDDEN)
                || ((direction == FocusDirectionUp || direction == FocusDirectionDown) && parentNode->renderer()->style()->overflowY() == OHIDDEN))
                return false;
        }
        if (parentNode == candidate.enclosingScrollableBox)
            return canScrollInDirection(parentNode, direction);
    }
    return true;
}

// The starting rect is the rect of the focused node, in document coordinates.
// Compose a virtual starting rect if there is no focused node or if it is off screen.
// The virtual rect is the edge of the container or frame. We select which
// edge depending on the direction of the navigation.
IntRect virtualRectForDirection(FocusDirection direction, const IntRect& startingRect, int width)
{
    IntRect virtualStartingRect = startingRect;
    switch (direction) {
    case FocusDirectionLeft:
        virtualStartingRect.setX(virtualStartingRect.maxX() - width);
        virtualStartingRect.setWidth(width);
        break;
    case FocusDirectionUp:
        virtualStartingRect.setY(virtualStartingRect.maxY() - width);
        virtualStartingRect.setHeight(width);
        break;
    case FocusDirectionRight:
        virtualStartingRect.setWidth(width);
        break;
    case FocusDirectionDown:
        virtualStartingRect.setHeight(width);
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    return virtualStartingRect;
}

IntRect virtualRectForAreaElementAndDirection(HTMLAreaElement* area, FocusDirection direction)
{
    ASSERT(area);
    ASSERT(area->imageElement());
    // Area elements tend to overlap more than other focusable elements. We flatten the rect of the area elements
    // to minimize the effect of overlapping areas.
    IntRect rect = virtualRectForDirection(direction, rectToAbsoluteCoordinates(area->document()->frame(), area->computeRect(area->imageElement()->renderer())), 1);
    return rect;
}

HTMLFrameOwnerElement* frameOwnerElement(FocusCandidate& candidate)
{
    return candidate.isFrameOwnerElement() ? static_cast<HTMLFrameOwnerElement*>(candidate.visibleNode) : 0;
};

} // namespace WebCore
