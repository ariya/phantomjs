/*
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2005, 2006, 2008 Apple Inc. All rights reserved.
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
#include "MouseRelatedEvent.h"

#include "DOMWindow.h"
#include "Document.h"
#include "Frame.h"
#include "FrameView.h"
#include "RenderLayer.h"
#include "RenderObject.h"

namespace WebCore {

MouseRelatedEvent::MouseRelatedEvent()
    : m_screenX(0)
    , m_screenY(0)
    , m_clientX(0)
    , m_clientY(0)
    , m_pageX(0)
    , m_pageY(0)
    , m_layerX(0)
    , m_layerY(0)
    , m_offsetX(0)
    , m_offsetY(0)
    , m_isSimulated(false)
    , m_hasCachedRelativePosition(false)
{
}

static int contentsX(AbstractView* abstractView)
{
    if (!abstractView)
        return 0;
    Frame* frame = abstractView->frame();
    if (!frame)
        return 0;
    FrameView* frameView = frame->view();
    if (!frameView)
        return 0;
    return frameView->scrollX() / frame->pageZoomFactor();
}

static int contentsY(AbstractView* abstractView)
{
    if (!abstractView)
        return 0;
    Frame* frame = abstractView->frame();
    if (!frame)
        return 0;
    FrameView* frameView = frame->view();
    if (!frameView)
        return 0;
    return frameView->scrollY() / frame->pageZoomFactor();
}

MouseRelatedEvent::MouseRelatedEvent(const AtomicString& eventType, bool canBubble, bool cancelable, PassRefPtr<AbstractView> abstractView,
                                     int detail, int screenX, int screenY, int windowX, int windowY,
                                     bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, bool isSimulated)
    : UIEventWithKeyState(eventType, canBubble, cancelable, abstractView, detail, ctrlKey, altKey, shiftKey, metaKey)
    , m_screenX(screenX)
    , m_screenY(screenY)
    , m_clientX(0)
    , m_clientY(0)
    , m_pageX(0)
    , m_pageY(0)
    , m_isSimulated(isSimulated)
{
    IntPoint adjustedPageLocation;
    IntPoint scrollPosition;

    Frame* frame = view() ? view()->frame() : 0;
    if (frame && !isSimulated) {
        if (FrameView* frameView = frame->view()) {
            scrollPosition = frameView->scrollPosition();
            adjustedPageLocation = frameView->windowToContents(IntPoint(windowX, windowY));
            float pageZoom = frame->pageZoomFactor();
            if (pageZoom != 1.0f) {
                // Adjust our pageX and pageY to account for the page zoom.
                adjustedPageLocation.setX(lroundf(adjustedPageLocation.x() / pageZoom));
                adjustedPageLocation.setY(lroundf(adjustedPageLocation.y() / pageZoom));
                scrollPosition.setX(scrollPosition.x() / pageZoom);
                scrollPosition.setY(scrollPosition.y() / pageZoom);
            }
        }
    }

    IntPoint clientLocation(adjustedPageLocation - scrollPosition);
    m_clientX = clientLocation.x();
    m_clientY = clientLocation.y();
    m_pageX = adjustedPageLocation.x();
    m_pageY = adjustedPageLocation.y();

    initCoordinates();
}

void MouseRelatedEvent::initCoordinates()
{
    // Set up initial values for coordinates.
    // Correct values are computed lazily, see computeRelativePosition.
    m_layerX = m_pageX;
    m_layerY = m_pageY;
    m_offsetX = m_pageX;
    m_offsetY = m_pageY;

    computePageLocation();
    m_hasCachedRelativePosition = false;
}

void MouseRelatedEvent::initCoordinates(int clientX, int clientY)
{
    // Set up initial values for coordinates.
    // Correct values are computed lazily, see computeRelativePosition.
    m_clientX = clientX;
    m_clientY = clientY;
    m_pageX = clientX + contentsX(view());
    m_pageY = clientY + contentsY(view());
    m_layerX = m_pageX;
    m_layerY = m_pageY;
    m_offsetX = m_pageX;
    m_offsetY = m_pageY;

    computePageLocation();
    m_hasCachedRelativePosition = false;
}

static float pageZoomFactor(const UIEvent* event)
{
    DOMWindow* window = event->view();
    if (!window)
        return 1;
    Frame* frame = window->frame();
    if (!frame)
        return 1;
    return frame->pageZoomFactor();
}

void MouseRelatedEvent::computePageLocation()
{
    float zoomFactor = pageZoomFactor(this);
    setAbsoluteLocation(roundedIntPoint(FloatPoint(pageX() * zoomFactor, pageY() * zoomFactor)));
}

void MouseRelatedEvent::receivedTarget()
{
    m_hasCachedRelativePosition = false;
}

void MouseRelatedEvent::computeRelativePosition()
{
    Node* targetNode = target() ? target()->toNode() : 0;
    if (!targetNode)
        return;

    // Compute coordinates that are based on the target.
    m_layerX = m_pageX;
    m_layerY = m_pageY;
    m_offsetX = m_pageX;
    m_offsetY = m_pageY;

    // Must have an updated render tree for this math to work correctly.
    targetNode->document()->updateStyleIfNeeded();

    // Adjust offsetX/Y to be relative to the target's position.
    if (!isSimulated()) {
        if (RenderObject* r = targetNode->renderer()) {
            FloatPoint localPos = r->absoluteToLocal(absoluteLocation(), false, true);
            float zoomFactor = pageZoomFactor(this);
            m_offsetX = lroundf(localPos.x() / zoomFactor);
            m_offsetY = lroundf(localPos.y() / zoomFactor);
        }
    }

    // Adjust layerX/Y to be relative to the layer.
    // FIXME: We're pretty sure this is the wrong definition of "layer."
    // Our RenderLayer is a more modern concept, and layerX/Y is some
    // other notion about groups of elements (left over from the Netscape 4 days?);
    // we should test and fix this.
    Node* n = targetNode;
    while (n && !n->renderer())
        n = n->parentNode();

    RenderLayer* layer;
    if (n && (layer = n->renderer()->enclosingLayer())) {
        layer->updateLayerPosition();
        for (; layer; layer = layer->parent()) {
            m_layerX -= layer->x();
            m_layerY -= layer->y();
        }
    }

    m_hasCachedRelativePosition = true;
}

int MouseRelatedEvent::layerX()
{
    if (!m_hasCachedRelativePosition)
        computeRelativePosition();
    return m_layerX;
}

int MouseRelatedEvent::layerY()
{
    if (!m_hasCachedRelativePosition)
        computeRelativePosition();
    return m_layerY;
}

int MouseRelatedEvent::offsetX()
{
    if (!m_hasCachedRelativePosition)
        computeRelativePosition();
    return m_offsetX;
}

int MouseRelatedEvent::offsetY()
{
    if (!m_hasCachedRelativePosition)
        computeRelativePosition();
    return m_offsetY;
}

int MouseRelatedEvent::pageX() const
{
    return m_pageX;
}

int MouseRelatedEvent::pageY() const
{
    return m_pageY;
}

int MouseRelatedEvent::x() const
{
    // FIXME: This is not correct.
    // See Microsoft documentation and <http://www.quirksmode.org/dom/w3c_events.html>.
    return m_clientX;
}

int MouseRelatedEvent::y() const
{
    // FIXME: This is not correct.
    // See Microsoft documentation and <http://www.quirksmode.org/dom/w3c_events.html>.
    return m_clientY;
}

} // namespace WebCore
