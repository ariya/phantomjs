/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "EventDispatcher.h"

#include "EventContext.h"
#include "FrameView.h"
#include "HTMLMediaElement.h"
#include "InspectorInstrumentation.h"
#include "MouseEvent.h"
#include "ScopedEventQueue.h"
#include "WindowEventContext.h"
#include <wtf/RefPtr.h>

#if ENABLE(SVG)
#include "SVGElementInstance.h"
#include "SVGNames.h"
#include "SVGUseElement.h"
#endif

namespace WebCore {

static HashSet<Node*>* gNodesDispatchingSimulatedClicks = 0;

bool EventDispatcher::dispatchEvent(Node* node, const EventDispatchMediator& mediator)
{
    ASSERT(!eventDispatchForbidden());

    EventDispatcher dispatcher(node);
    return mediator.dispatchEvent(&dispatcher);
}

static EventTarget* findElementInstance(Node* referenceNode)
{
#if ENABLE(SVG)
    // Spec: The event handling for the non-exposed tree works as if the referenced element had been textually included
    // as a deeply cloned child of the 'use' element, except that events are dispatched to the SVGElementInstance objects
    for (Node* n = referenceNode; n; n = n->parentNode()) {
        if (!n->isSVGShadowRoot() || !n->isSVGElement())
            continue;

        Element* shadowTreeParentElement = n->svgShadowHost();
        ASSERT(shadowTreeParentElement->hasTagName(SVGNames::useTag));

        if (SVGElementInstance* instance = static_cast<SVGUseElement*>(shadowTreeParentElement)->instanceForShadowTreeElement(referenceNode))
            return instance;
    }
#else
    // SVG elements with SVG disabled should not be possible.
    ASSERT_NOT_REACHED();
#endif

    return referenceNode;
}

inline static EventTarget* eventTargetRespectingSVGTargetRules(Node* referenceNode)
{
    ASSERT(referenceNode);

    return referenceNode->isSVGElement() ? findElementInstance(referenceNode) : referenceNode;
}

void EventDispatcher::dispatchScopedEvent(Node* node, PassRefPtr<Event> event)
{
    // We need to set the target here because it can go away by the time we actually fire the event.
    event->setTarget(eventTargetRespectingSVGTargetRules(node));

    ScopedEventQueue::instance()->enqueueEvent(event);
}

void EventDispatcher::dispatchSimulatedClick(Node* node, PassRefPtr<Event> underlyingEvent, bool sendMouseEvents, bool showPressedLook)
{
    if (node->disabled())
        return;

    EventDispatcher dispatcher(node);

    if (!gNodesDispatchingSimulatedClicks)
        gNodesDispatchingSimulatedClicks = new HashSet<Node*>;
    else if (gNodesDispatchingSimulatedClicks->contains(node))
        return;

    gNodesDispatchingSimulatedClicks->add(node);

    // send mousedown and mouseup before the click, if requested
    if (sendMouseEvents)
        dispatcher.dispatchEvent(SimulatedMouseEvent::create(eventNames().mousedownEvent, node->document()->defaultView(), underlyingEvent));
    node->setActive(true, showPressedLook);
    if (sendMouseEvents)
        dispatcher.dispatchEvent(SimulatedMouseEvent::create(eventNames().mouseupEvent, node->document()->defaultView(), underlyingEvent));
    node->setActive(false);

    // always send click
    dispatcher.dispatchEvent(SimulatedMouseEvent::create(eventNames().clickEvent, node->document()->defaultView(), underlyingEvent));

    gNodesDispatchingSimulatedClicks->remove(node);
}

static inline bool isShadowRootOrSVGShadowRoot(const Node* node)
{
    return node->isShadowRoot() || node->isSVGShadowRoot();
}

PassRefPtr<EventTarget> EventDispatcher::adjustToShadowBoundaries(PassRefPtr<Node> relatedTarget, const Vector<Node*> relatedTargetAncestors)
{
    Vector<EventContext>::const_iterator lowestCommonBoundary = m_ancestors.end();
    // Assume divergent boundary is the relatedTarget itself (in other words, related target ancestor chain does not cross any shadow DOM boundaries).
    Vector<Node*>::const_iterator firstDivergentBoundary = relatedTargetAncestors.begin();

    Vector<EventContext>::const_iterator targetAncestor = m_ancestors.end();
    // Walk down from the top, looking for lowest common ancestor, also monitoring shadow DOM boundaries.
    bool diverged = false;
    for (Vector<Node*>::const_iterator i = relatedTargetAncestors.end() - 1; i >= relatedTargetAncestors.begin(); --i) {
        if (diverged) {
            if (isShadowRootOrSVGShadowRoot(*i)) {
                firstDivergentBoundary = i + 1;
                break;
            }
            continue;
        }

        if (targetAncestor == m_ancestors.begin()) {
            diverged = true;
            continue;
        }

        targetAncestor--;

        if (isShadowRootOrSVGShadowRoot(*i))
            lowestCommonBoundary = targetAncestor;

        if ((*i) != (*targetAncestor).node())
            diverged = true;
    }

    if (!diverged) {
        // The relatedTarget is an ancestor or shadowHost of the target.
        if (m_node->shadowHost() == relatedTarget.get())
            lowestCommonBoundary = m_ancestors.begin();
    } else if ((*firstDivergentBoundary) == m_node.get()) {
        // Since ancestors does not contain target itself, we must account
        // for the possibility that target is a shadowHost of relatedTarget
        // and thus serves as the lowestCommonBoundary.
        // Luckily, in this case the firstDivergentBoundary is target.
        lowestCommonBoundary = m_ancestors.begin();
    }

    // Trim ancestors to lowestCommonBoundary to keep events inside of the common shadow DOM subtree.
    if (lowestCommonBoundary != m_ancestors.end())
        m_ancestors.shrink(lowestCommonBoundary - m_ancestors.begin());
    // Set event's related target to the first encountered shadow DOM boundary in the divergent subtree.
    return firstDivergentBoundary != relatedTargetAncestors.begin() ? *firstDivergentBoundary : relatedTarget;
}

inline static bool ancestorsCrossShadowBoundaries(const Vector<EventContext>& ancestors)
{
    return ancestors.isEmpty() || ancestors.first().node() == ancestors.last().node();
}

// FIXME: Once https://bugs.webkit.org/show_bug.cgi?id=52963 lands, this should
// be greatly improved. See https://bugs.webkit.org/show_bug.cgi?id=54025.
PassRefPtr<EventTarget> EventDispatcher::adjustRelatedTarget(Event* event, PassRefPtr<EventTarget> prpRelatedTarget)
{
    if (!prpRelatedTarget)
        return 0;

    RefPtr<Node> relatedTarget = prpRelatedTarget->toNode();
    if (!relatedTarget)
        return 0;

    Node* target = m_node.get();
    if (!target)
        return prpRelatedTarget;

    ensureEventAncestors(event);

    // Calculate early if the common boundary is even possible by looking at
    // ancestors size and if the retargeting has occured (indicating the presence of shadow DOM boundaries).
    // If there are no boundaries detected, the target and related target can't have a common boundary.
    bool noCommonBoundary = ancestorsCrossShadowBoundaries(m_ancestors);

    Vector<Node*> relatedTargetAncestors;
    Node* outermostShadowBoundary = relatedTarget.get();
    for (Node* n = outermostShadowBoundary; n; n = n->parentOrHostNode()) {
        if (isShadowRootOrSVGShadowRoot(n))
            outermostShadowBoundary = n->parentOrHostNode();
        if (!noCommonBoundary)
            relatedTargetAncestors.append(n);
    }

    // Short-circuit the fast case when we know there is no need to calculate a common boundary.
    if (noCommonBoundary)
        return outermostShadowBoundary;

    return adjustToShadowBoundaries(relatedTarget.release(), relatedTargetAncestors);
}

EventDispatcher::EventDispatcher(Node* node)
    : m_node(node)
    , m_ancestorsInitialized(false)
{
    ASSERT(node);
    m_view = node->document()->view();
}

void EventDispatcher::ensureEventAncestors(Event* event)
{
    if (!m_node->inDocument())
        return;

    if (m_ancestorsInitialized)
        return;

    m_ancestorsInitialized = true;

    Node* ancestor = m_node.get();
    EventTarget* target = eventTargetRespectingSVGTargetRules(ancestor);
    bool shouldSkipNextAncestor = false;
    while (true) {
        bool isSVGShadowRoot = ancestor->isSVGShadowRoot();
        if (isSVGShadowRoot || ancestor->isShadowRoot()) {
            if (determineDispatchBehavior(event, ancestor) == StayInsideShadowDOM)
                return;
#if ENABLE(SVG)
            ancestor = isSVGShadowRoot ? ancestor->svgShadowHost() : ancestor->shadowHost();
#else
            ancestor = ancestor->shadowHost();
#endif
            if (!shouldSkipNextAncestor)
                target = ancestor;
        } else
            ancestor = ancestor->parentNodeGuaranteedHostFree();

        if (!ancestor)
            return;

#if ENABLE(SVG)
        // Skip SVGShadowTreeRootElement.
        shouldSkipNextAncestor = ancestor->isSVGShadowRoot();
        if (shouldSkipNextAncestor)
            continue;
#endif
        // FIXME: Unroll the extra loop inside eventTargetRespectingSVGTargetRules into this loop.
        m_ancestors.append(EventContext(ancestor, eventTargetRespectingSVGTargetRules(ancestor), target));
    }
}

bool EventDispatcher::dispatchEvent(PassRefPtr<Event> event)
{
    event->setTarget(eventTargetRespectingSVGTargetRules(m_node.get()));

    ASSERT(!eventDispatchForbidden());
    ASSERT(event->target());
    ASSERT(!event->type().isNull()); // JavaScript code can create an event with an empty name, but not null.

    RefPtr<EventTarget> originalTarget = event->target();
    ensureEventAncestors(event.get());

    WindowEventContext windowContext(event.get(), m_node.get(), topEventContext());

    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willDispatchEvent(m_node->document(), *event, windowContext.window(), m_node.get(), m_ancestors);

    // Give the target node a chance to do some work before DOM event handlers get a crack.
    void* data = m_node->preDispatchEventHandler(event.get());
    if (event->propagationStopped())
        goto doneDispatching;

    // Trigger capturing event handlers, starting at the top and working our way down.
    event->setEventPhase(Event::CAPTURING_PHASE);

    if (windowContext.handleLocalEvents(event.get()) && event->propagationStopped())
        goto doneDispatching;

    for (size_t i = m_ancestors.size(); i; --i) {
        m_ancestors[i - 1].handleLocalEvents(event.get());
        if (event->propagationStopped())
            goto doneDispatching;
    }

    event->setEventPhase(Event::AT_TARGET);
    event->setTarget(originalTarget.get());
    event->setCurrentTarget(eventTargetRespectingSVGTargetRules(m_node.get()));
    m_node->handleLocalEvents(event.get());
    if (event->propagationStopped())
        goto doneDispatching;

    if (event->bubbles() && !event->cancelBubble()) {
        // Trigger bubbling event handlers, starting at the bottom and working our way up.
        event->setEventPhase(Event::BUBBLING_PHASE);

        size_t size = m_ancestors.size();
        for (size_t i = 0; i < size; ++i) {
            m_ancestors[i].handleLocalEvents(event.get());
            if (event->propagationStopped() || event->cancelBubble())
                goto doneDispatching;
        }
        windowContext.handleLocalEvents(event.get());
    }

doneDispatching:
    event->setTarget(originalTarget.get());
    event->setCurrentTarget(0);
    event->setEventPhase(0);

    // Pass the data from the preDispatchEventHandler to the postDispatchEventHandler.
    m_node->postDispatchEventHandler(event.get(), data);

    // Call default event handlers. While the DOM does have a concept of preventing
    // default handling, the detail of which handlers are called is an internal
    // implementation detail and not part of the DOM.
    if (!event->defaultPrevented() && !event->defaultHandled()) {
        // Non-bubbling events call only one default event handler, the one for the target.
        m_node->defaultEventHandler(event.get());
        ASSERT(!event->defaultPrevented());
        if (event->defaultHandled())
            goto doneWithDefault;
        // For bubbling events, call default event handlers on the same targets in the
        // same order as the bubbling phase.
        if (event->bubbles()) {
            size_t size = m_ancestors.size();
            for (size_t i = 0; i < size; ++i) {
                m_ancestors[i].node()->defaultEventHandler(event.get());
                ASSERT(!event->defaultPrevented());
                if (event->defaultHandled())
                    goto doneWithDefault;
            }
        }
    }

doneWithDefault:

    // Ensure that after event dispatch, the event's target object is the
    // outermost shadow DOM boundary.
    event->setTarget(windowContext.target());
    event->setCurrentTarget(0);
    InspectorInstrumentation::didDispatchEvent(cookie);

    return !event->defaultPrevented();
}

const EventContext* EventDispatcher::topEventContext()
{
    return m_ancestors.isEmpty() ? 0 : &m_ancestors.last();
}

EventDispatchBehavior EventDispatcher::determineDispatchBehavior(Event* event, Node* shadowRoot)
{
#if ENABLE(FULLSCREEN_API)
    // Video-only full screen is a mode where we use the shadow DOM as an implementation
    // detail that should not be detectable by the web content.
    if (Element* element = m_node->document()->webkitCurrentFullScreenElement()) {
        // FIXME: We assume that if the full screen element is a media element that it's
        // the video-only full screen. Both here and elsewhere. But that is probably wrong.
        if (element->isMediaElement() && shadowRoot && shadowRoot->shadowHost() == element)
            return StayInsideShadowDOM;
    }
#endif

    // Per XBL 2.0 spec, mutation events should never cross shadow DOM boundary:
    // http://dev.w3.org/2006/xbl2/#event-flow-and-targeting-across-shadow-s
    if (event->isMutationEvent())
        return StayInsideShadowDOM;

    // WebKit never allowed selectstart event to cross the the shadow DOM boundary.
    // Changing this breaks existing sites.
    // See https://bugs.webkit.org/show_bug.cgi?id=52195 for details.
    if (event->type() == eventNames().selectstartEvent)
        return StayInsideShadowDOM;

    return RetargetEvent;
}

}
