/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
#include "EventRetargeter.h"

#include "ContainerNode.h"
#include "EventContext.h"
#include "EventPathWalker.h"
#include "FocusEvent.h"
#include "MouseEvent.h"
#include "ShadowRoot.h"
#include "Touch.h"
#include "TouchEvent.h"
#include "TouchList.h"
#include "TreeScope.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

static inline bool inTheSameScope(ShadowRoot* shadowRoot, EventTarget* target)
{
    return target->toNode() && target->toNode()->treeScope()->rootNode() == shadowRoot;
}

static inline EventDispatchBehavior determineDispatchBehavior(Event* event, ShadowRoot* shadowRoot, EventTarget* target)
{
#if ENABLE(FULLSCREEN_API) && ENABLE(VIDEO)
    // Video-only full screen is a mode where we use the shadow DOM as an implementation
    // detail that should not be detectable by the web content.
    if (Element* element = target->toNode()->document()->webkitCurrentFullScreenElement()) {
        // FIXME: We assume that if the full screen element is a media element that it's
        // the video-only full screen. Both here and elsewhere. But that is probably wrong.
        if (element->isMediaElement() && shadowRoot && shadowRoot->host() == element)
            return StayInsideShadowDOM;
    }
#else
    UNUSED_PARAM(shadowRoot);
#endif

    // WebKit never allowed selectstart event to cross the the shadow DOM boundary.
    // Changing this breaks existing sites.
    // See https://bugs.webkit.org/show_bug.cgi?id=52195 for details.
    const AtomicString eventType = event->type();
    if (inTheSameScope(shadowRoot, target)
        && (eventType == eventNames().abortEvent
            || eventType == eventNames().changeEvent
            || eventType == eventNames().errorEvent
            || eventType == eventNames().loadEvent
            || eventType == eventNames().resetEvent
            || eventType == eventNames().resizeEvent
            || eventType == eventNames().scrollEvent
            || eventType == eventNames().selectEvent
            || eventType == eventNames().selectstartEvent))
        return StayInsideShadowDOM;

    return RetargetEvent;
}

void EventRetargeter::calculateEventPath(Node* node, Event* event, EventPath& eventPath)
{
    bool inDocument = node->inDocument();
    bool isSVGElement = node->isSVGElement();
    bool isMouseOrFocusEvent = event->isMouseEvent() || event->isFocusEvent();
#if ENABLE(TOUCH_EVENTS)
    bool isTouchEvent = event->isTouchEvent();
#endif
    Vector<EventTarget*, 32> targetStack;
    for (EventPathWalker walker(node); walker.node(); walker.moveToParent()) {
        Node* node = walker.node();
        if (targetStack.isEmpty())
            targetStack.append(eventTargetRespectingTargetRules(node));
        else if (walker.isVisitingInsertionPointInReprojection())
            targetStack.append(targetStack.last());
        if (isMouseOrFocusEvent)
            eventPath.append(adoptPtr(new MouseOrFocusEventContext(node, eventTargetRespectingTargetRules(node), targetStack.last())));
#if ENABLE(TOUCH_EVENTS)
        else if (isTouchEvent)
            eventPath.append(adoptPtr(new TouchEventContext(node, eventTargetRespectingTargetRules(node), targetStack.last())));
#endif
        else
            eventPath.append(adoptPtr(new EventContext(node, eventTargetRespectingTargetRules(node), targetStack.last())));
        if (!inDocument)
            return;
        if (!node->isShadowRoot())
            continue;
        if (determineDispatchBehavior(event, toShadowRoot(node), targetStack.last()) == StayInsideShadowDOM)
            return;
        if (!isSVGElement) {
            ASSERT(!targetStack.isEmpty());
            targetStack.removeLast();
        }
    }
}

void EventRetargeter::adjustForMouseEvent(Node* node, const MouseEvent& mouseEvent, EventPath& eventPath)
{
    adjustForRelatedTarget(node, mouseEvent.relatedTarget(), eventPath);
}

void EventRetargeter::adjustForFocusEvent(Node* node, const FocusEvent& focusEvent, EventPath& eventPath)
{
    adjustForRelatedTarget(node, focusEvent.relatedTarget(), eventPath);
}

#if ENABLE(TOUCH_EVENTS)
void EventRetargeter::adjustForTouchEvent(Node* node, const TouchEvent& touchEvent, EventPath& eventPath)
{
    size_t eventPathSize = eventPath.size();

    EventPathTouchLists eventPathTouches(eventPathSize);
    EventPathTouchLists eventPathTargetTouches(eventPathSize);
    EventPathTouchLists eventPathChangedTouches(eventPathSize);

    for (size_t i = 0; i < eventPathSize; ++i) {
        ASSERT(eventPath[i]->isTouchEventContext());
        TouchEventContext* touchEventContext = toTouchEventContext(eventPath[i].get());
        eventPathTouches[i] = touchEventContext->touches();
        eventPathTargetTouches[i] = touchEventContext->targetTouches();
        eventPathChangedTouches[i] = touchEventContext->changedTouches();
    }

    adjustTouchList(node, touchEvent.touches(), eventPath, eventPathTouches);
    adjustTouchList(node, touchEvent.targetTouches(), eventPath, eventPathTargetTouches);
    adjustTouchList(node, touchEvent.changedTouches(), eventPath, eventPathChangedTouches);
}

void EventRetargeter::adjustTouchList(const Node* node, const TouchList* touchList, const EventPath& eventPath, EventPathTouchLists& eventPathTouchLists)
{
    if (!touchList)
        return;
    size_t eventPathSize = eventPath.size();
    ASSERT(eventPathTouchLists.size() == eventPathSize);
    for (size_t i = 0; i < touchList->length(); ++i) {
        const Touch& touch = *touchList->item(i);
        AdjustedNodes adjustedNodes;
        calculateAdjustedNodes(node, touch.target()->toNode(), DoesNotStopAtBoundary, const_cast<EventPath&>(eventPath), adjustedNodes);
        ASSERT(adjustedNodes.size() == eventPathSize);
        for (size_t j = 0; j < eventPathSize; ++j)
            eventPathTouchLists[j]->append(touch.cloneWithNewTarget(adjustedNodes[j].get()));
    }
}
#endif

void EventRetargeter::adjustForRelatedTarget(const Node* node, EventTarget* relatedTarget, EventPath& eventPath)
{
    if (!node)
        return;
    if (!relatedTarget)
        return;
    Node* relatedNode = relatedTarget->toNode();
    if (!relatedNode)
        return;
    AdjustedNodes adjustedNodes;
    calculateAdjustedNodes(node, relatedNode, StopAtBoundaryIfNeeded, eventPath, adjustedNodes);
    ASSERT(adjustedNodes.size() <= eventPath.size());
    for (size_t i = 0; i < adjustedNodes.size(); ++i) {
        ASSERT(eventPath[i]->isMouseOrFocusEventContext());
        MouseOrFocusEventContext* mouseOrFocusEventContext = static_cast<MouseOrFocusEventContext*>(eventPath[i].get());
        mouseOrFocusEventContext->setRelatedTarget(adjustedNodes[i]);
    }
}

void EventRetargeter::calculateAdjustedNodes(const Node* node, const Node* relatedNode, EventWithRelatedTargetDispatchBehavior eventWithRelatedTargetDispatchBehavior, EventPath& eventPath, AdjustedNodes& adjustedNodes)
{
    RelatedNodeMap relatedNodeMap;
    buildRelatedNodeMap(relatedNode, relatedNodeMap);

    // Synthetic mouse events can have a relatedTarget which is identical to the target.
    bool targetIsIdenticalToToRelatedTarget = (node == relatedNode);

    TreeScope* lastTreeScope = 0;
    Node* adjustedNode = 0;
    for (EventPath::const_iterator iter = eventPath.begin(); iter < eventPath.end(); ++iter) {
        TreeScope* scope = (*iter)->node()->treeScope();
        if (scope == lastTreeScope) {
            // Re-use the previous adjustedRelatedTarget if treeScope does not change. Just for the performance optimization.
            adjustedNodes.append(adjustedNode);
        } else {
            adjustedNode = findRelatedNode(scope, relatedNodeMap);
            adjustedNodes.append(adjustedNode);
        }
        lastTreeScope = scope;
        if (eventWithRelatedTargetDispatchBehavior == DoesNotStopAtBoundary)
            continue;
        if (targetIsIdenticalToToRelatedTarget) {
            if (node->treeScope()->rootNode() == (*iter)->node()) {
                eventPath.shrink(iter + 1 - eventPath.begin());
                break;
            }
        } else if ((*iter)->target() == adjustedNode) {
            // Event dispatching should be stopped here.
            eventPath.shrink(iter - eventPath.begin());
            adjustedNodes.shrink(adjustedNodes.size() - 1);
            break;
        }
    }
}

void EventRetargeter::buildRelatedNodeMap(const Node* relatedNode, RelatedNodeMap& relatedNodeMap)
{
    Vector<Node*, 32> relatedNodeStack;
    TreeScope* lastTreeScope = 0;
    for (EventPathWalker walker(relatedNode); walker.node(); walker.moveToParent()) {
        Node* node = walker.node();
        if (relatedNodeStack.isEmpty())
            relatedNodeStack.append(node);
        else if (walker.isVisitingInsertionPointInReprojection())
            relatedNodeStack.append(relatedNodeStack.last());
        TreeScope* scope = node->treeScope();
        // Skips adding a node to the map if treeScope does not change. Just for the performance optimization.
        if (scope != lastTreeScope)
            relatedNodeMap.add(scope, relatedNodeStack.last());
        lastTreeScope = scope;
        if (node->isShadowRoot()) {
            ASSERT(!relatedNodeStack.isEmpty());
            relatedNodeStack.removeLast();
        }
    }
}

Node* EventRetargeter::findRelatedNode(TreeScope* scope, RelatedNodeMap& relatedNodeMap)
{
    Vector<TreeScope*, 32> parentTreeScopes;
    Node* relatedNode = 0;
    while (scope) {
        parentTreeScopes.append(scope);
        RelatedNodeMap::const_iterator found = relatedNodeMap.find(scope);
        if (found != relatedNodeMap.end()) {
            relatedNode = found->value;
            break;
        }
        scope = scope->parentTreeScope();
    }
    for (Vector<TreeScope*, 32>::iterator iter = parentTreeScopes.begin(); iter < parentTreeScopes.end(); ++iter)
        relatedNodeMap.add(*iter, relatedNode);
    return relatedNode;
}

}
