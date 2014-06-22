/**
 * Copyright (C) 2004 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
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

#include "config.h"
#include "RenderCounter.h"

#include "CounterNode.h"
#include "Document.h"
#include "Element.h"
#include "HTMLNames.h"
#include "HTMLOListElement.h"
#include "NodeTraversal.h"
#include "RenderListItem.h"
#include "RenderListMarker.h"
#include "RenderStyle.h"
#include "RenderView.h"
#include <wtf/StdLibExtras.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

namespace WebCore {

using namespace HTMLNames;

typedef HashMap<AtomicString, RefPtr<CounterNode> > CounterMap;
typedef HashMap<const RenderObject*, OwnPtr<CounterMap> > CounterMaps;

static CounterNode* makeCounterNode(RenderObject*, const AtomicString& identifier, bool alwaysCreateCounter);

static CounterMaps& counterMaps()
{
    DEFINE_STATIC_LOCAL(CounterMaps, staticCounterMaps, ());
    return staticCounterMaps;
}

// This function processes the renderer tree in the order of the DOM tree
// including pseudo elements as defined in CSS 2.1.
static RenderObject* previousInPreOrder(const RenderObject* object)
{
    Element* self = toElement(object->node());
    Element* previous = ElementTraversal::previousIncludingPseudo(self);
    while (previous && !previous->renderer())
        previous = ElementTraversal::previousIncludingPseudo(previous);
    return previous ? previous->renderer() : 0;
}

// This function processes the renderer tree in the order of the DOM tree
// including pseudo elements as defined in CSS 2.1.
static RenderObject* previousSiblingOrParent(const RenderObject* object)
{
    Element* self = toElement(object->node());
    Element* previous = ElementTraversal::pseudoAwarePreviousSibling(self);
    while (previous && !previous->renderer())
        previous = ElementTraversal::pseudoAwarePreviousSibling(previous);
    if (previous)
        return previous->renderer();
    previous = self->parentElement();
    return previous ? previous->renderer() : 0;
}

static inline Element* parentElement(RenderObject* object)
{
    return toElement(object->node())->parentElement();
}

static inline bool areRenderersElementsSiblings(RenderObject* first, RenderObject* second)
{
    return parentElement(first) == parentElement(second);
}

// This function processes the renderer tree in the order of the DOM tree
// including pseudo elements as defined in CSS 2.1.
static RenderObject* nextInPreOrder(const RenderObject* object, const Element* stayWithin, bool skipDescendants = false)
{
    Element* self = toElement(object->node());
    Element* next = skipDescendants ? ElementTraversal::nextIncludingPseudoSkippingChildren(self, stayWithin) : ElementTraversal::nextIncludingPseudo(self, stayWithin);
    while (next && !next->renderer())
        next = skipDescendants ? ElementTraversal::nextIncludingPseudoSkippingChildren(next, stayWithin) : ElementTraversal::nextIncludingPseudo(next, stayWithin);
    return next ? next->renderer() : 0;
}

static bool planCounter(RenderObject* object, const AtomicString& identifier, bool& isReset, int& value)
{
    ASSERT(object);

    // Real text nodes don't have their own style so they can't have counters.
    // We can't even look at their styles or we'll see extra resets and increments!
    if (object->isText() && !object->isBR())
        return false;
    Node* generatingNode = object->generatingNode();
    // We must have a generating node or else we cannot have a counter.
    if (!generatingNode)
        return false;
    RenderStyle* style = object->style();
    ASSERT(style);

    switch (style->styleType()) {
    case NOPSEUDO:
        // Sometimes nodes have more then one renderer. Only the first one gets the counter
        // LayoutTests/http/tests/css/counter-crash.html
        if (generatingNode->renderer() != object)
            return false;
        break;
    case BEFORE:
    case AFTER:
        break;
    default:
        return false; // Counters are forbidden from all other pseudo elements.
    }

    const CounterDirectives directives = style->getCounterDirectives(identifier);
    if (directives.isDefined()) {
        value = directives.combinedValue();
        isReset = directives.isReset();
        return true;
    }

    if (identifier == "list-item") {
        if (object->isListItem()) {
            if (toRenderListItem(object)->hasExplicitValue()) {
                value = toRenderListItem(object)->explicitValue();
                isReset = true;
                return true;
            }
            value = 1;
            isReset = false;
            return true;
        }
        if (Node* e = object->node()) {
            if (e->hasTagName(olTag)) {
                value = static_cast<HTMLOListElement*>(e)->start();
                isReset = true;
                return true;
            }
            if (e->hasTagName(ulTag) || e->hasTagName(menuTag) || e->hasTagName(dirTag)) {
                value = 0;
                isReset = true;
                return true;
            }
        }
    }

    return false;
}

// - Finds the insertion point for the counter described by counterOwner, isReset and 
// identifier in the CounterNode tree for identifier and sets parent and
// previousSibling accordingly.
// - The function returns true if the counter whose insertion point is searched is NOT
// the root of the tree.
// - The root of the tree is a counter reference that is not in the scope of any other
// counter with the same identifier.
// - All the counter references with the same identifier as this one that are in
// children or subsequent siblings of the renderer that owns the root of the tree
// form the rest of of the nodes of the tree.
// - The root of the tree is always a reset type reference.
// - A subtree rooted at any reset node in the tree is equivalent to all counter 
// references that are in the scope of the counter or nested counter defined by that
// reset node.
// - Non-reset CounterNodes cannot have descendants.

static bool findPlaceForCounter(RenderObject* counterOwner, const AtomicString& identifier, bool isReset, RefPtr<CounterNode>& parent, RefPtr<CounterNode>& previousSibling)
{
    // We cannot stop searching for counters with the same identifier before we also
    // check this renderer, because it may affect the positioning in the tree of our counter.
    RenderObject* searchEndRenderer = previousSiblingOrParent(counterOwner);
    // We check renderers in preOrder from the renderer that our counter is attached to
    // towards the begining of the document for counters with the same identifier as the one
    // we are trying to find a place for. This is the next renderer to be checked.
    RenderObject* currentRenderer = previousInPreOrder(counterOwner);
    previousSibling = 0;
    RefPtr<CounterNode> previousSiblingProtector = 0;

    while (currentRenderer) {
        CounterNode* currentCounter = makeCounterNode(currentRenderer, identifier, false);
        if (searchEndRenderer == currentRenderer) {
            // We may be at the end of our search.
            if (currentCounter) {
                // We have a suitable counter on the EndSearchRenderer.
                if (previousSiblingProtector) { // But we already found another counter that we come after.
                    if (currentCounter->actsAsReset()) {
                        // We found a reset counter that is on a renderer that is a sibling of ours or a parent.
                        if (isReset && areRenderersElementsSiblings(currentRenderer, counterOwner)) {
                            // We are also a reset counter and the previous reset was on a sibling renderer
                            // hence we are the next sibling of that counter if that reset is not a root or
                            // we are a root node if that reset is a root.
                            parent = currentCounter->parent();
                            previousSibling = parent ? currentCounter : 0;
                            return parent;
                        }
                        // We are not a reset node or the previous reset must be on an ancestor of our owner renderer
                        // hence we must be a child of that reset counter.
                        parent = currentCounter;
                        // In some cases renders can be reparented (ex. nodes inside a table but not in a column or row).
                        // In these cases the identified previousSibling will be invalid as its parent is different from
                        // our identified parent.
                        if (previousSiblingProtector->parent() != currentCounter)
                            previousSiblingProtector = 0;

                        previousSibling = previousSiblingProtector.get();
                        return true;
                    }
                    // CurrentCounter, the counter at the EndSearchRenderer, is not reset.
                    if (!isReset || !areRenderersElementsSiblings(currentRenderer, counterOwner)) {
                        // If the node we are placing is not reset or we have found a counter that is attached
                        // to an ancestor of the placed counter's owner renderer we know we are a sibling of that node.
                        if (currentCounter->parent() != previousSiblingProtector->parent())
                            return false;

                        parent = currentCounter->parent();
                        previousSibling = previousSiblingProtector.get();
                        return true;
                    }
                } else { 
                    // We are at the potential end of the search, but we had no previous sibling candidate
                    // In this case we follow pretty much the same logic as above but no ASSERTs about 
                    // previousSibling, and when we are a sibling of the end counter we must set previousSibling
                    // to currentCounter.
                    if (currentCounter->actsAsReset()) {
                        if (isReset && areRenderersElementsSiblings(currentRenderer, counterOwner)) {
                            parent = currentCounter->parent();
                            previousSibling = currentCounter;
                            return parent;
                        }
                        parent = currentCounter;
                        previousSibling = previousSiblingProtector.get();
                        return true;
                    }
                    if (!isReset || !areRenderersElementsSiblings(currentRenderer, counterOwner)) {
                        parent = currentCounter->parent();
                        previousSibling = currentCounter;
                        return true;
                    }
                    previousSiblingProtector = currentCounter;
                }
            }
            // We come here if the previous sibling or parent of our owner renderer had no
            // good counter, or we are a reset node and the counter on the previous sibling
            // of our owner renderer was not a reset counter.
            // Set a new goal for the end of the search.
            searchEndRenderer = previousSiblingOrParent(currentRenderer);
        } else {
            // We are searching descendants of a previous sibling of the renderer that the
            // counter being placed is attached to.
            if (currentCounter) {
                // We found a suitable counter.
                if (previousSiblingProtector) {
                    // Since we had a suitable previous counter before, we should only consider this one as our 
                    // previousSibling if it is a reset counter and hence the current previousSibling is its child.
                    if (currentCounter->actsAsReset()) {
                        previousSiblingProtector = currentCounter;
                        // We are no longer interested in previous siblings of the currentRenderer or their children
                        // as counters they may have attached cannot be the previous sibling of the counter we are placing.
                        currentRenderer = parentElement(currentRenderer)->renderer();
                        continue;
                    }
                } else
                    previousSiblingProtector = currentCounter;
                currentRenderer = previousSiblingOrParent(currentRenderer);
                continue;
            }
        }
        // This function is designed so that the same test is not done twice in an iteration, except for this one
        // which may be done twice in some cases. Rearranging the decision points though, to accommodate this 
        // performance improvement would create more code duplication than is worthwhile in my oppinion and may further
        // impede the readability of this already complex algorithm.
        if (previousSiblingProtector)
            currentRenderer = previousSiblingOrParent(currentRenderer);
        else
            currentRenderer = previousInPreOrder(currentRenderer);
    }
    return false;
}

static CounterNode* makeCounterNode(RenderObject* object, const AtomicString& identifier, bool alwaysCreateCounter)
{
    ASSERT(object);

    if (object->hasCounterNodeMap()) {
        if (CounterMap* nodeMap = counterMaps().get(object)) {
            if (CounterNode* node = nodeMap->get(identifier))
                return node;
        }
    }

    bool isReset = false;
    int value = 0;
    if (!planCounter(object, identifier, isReset, value) && !alwaysCreateCounter)
        return 0;

    RefPtr<CounterNode> newParent = 0;
    RefPtr<CounterNode> newPreviousSibling = 0;
    RefPtr<CounterNode> newNode = CounterNode::create(object, isReset, value);
    if (findPlaceForCounter(object, identifier, isReset, newParent, newPreviousSibling))
        newParent->insertAfter(newNode.get(), newPreviousSibling.get(), identifier);
    CounterMap* nodeMap;
    if (object->hasCounterNodeMap())
        nodeMap = counterMaps().get(object);
    else {
        nodeMap = new CounterMap;
        counterMaps().set(object, adoptPtr(nodeMap));
        object->setHasCounterNodeMap(true);
    }
    nodeMap->set(identifier, newNode);
    if (newNode->parent())
        return newNode.get();
    // Checking if some nodes that were previously counter tree root nodes
    // should become children of this node now.
    CounterMaps& maps = counterMaps();
    Element* stayWithin = parentElement(object);
    bool skipDescendants;
    for (RenderObject* currentRenderer = nextInPreOrder(object, stayWithin); currentRenderer; currentRenderer = nextInPreOrder(currentRenderer, stayWithin, skipDescendants)) {
        skipDescendants = false;
        if (!currentRenderer->hasCounterNodeMap())
            continue;
        CounterNode* currentCounter = maps.get(currentRenderer)->get(identifier);
        if (!currentCounter)
            continue;
        skipDescendants = true;
        if (currentCounter->parent())
            continue;
        if (stayWithin == parentElement(currentRenderer) && currentCounter->hasResetType())
            break;
        newNode->insertAfter(currentCounter, newNode->lastChild(), identifier);
    }
    return newNode.get();
}

RenderCounter::RenderCounter(Document* node, const CounterContent& counter)
    : RenderText(node, StringImpl::empty())
    , m_counter(counter)
    , m_counterNode(0)
    , m_nextForSameCounter(0)
{
    view()->addRenderCounter();
}

RenderCounter::~RenderCounter()
{
    if (m_counterNode) {
        m_counterNode->removeRenderer(this);
        ASSERT(!m_counterNode);
    }
}

void RenderCounter::willBeDestroyed()
{
    if (view())
        view()->removeRenderCounter();
    RenderText::willBeDestroyed();
}

const char* RenderCounter::renderName() const
{
    return "RenderCounter";
}

bool RenderCounter::isCounter() const
{
    return true;
}

PassRefPtr<StringImpl> RenderCounter::originalText() const
{
    if (!m_counterNode) {
        RenderObject* beforeAfterContainer = parent();
        while (true) {
            if (!beforeAfterContainer)
                return 0;
            if (!beforeAfterContainer->isAnonymous() && !beforeAfterContainer->isPseudoElement())
                return 0; // RenderCounters are restricted to before and after pseudo elements
            PseudoId containerStyle = beforeAfterContainer->style()->styleType();
            if ((containerStyle == BEFORE) || (containerStyle == AFTER))
                break;
            beforeAfterContainer = beforeAfterContainer->parent();
        }
        makeCounterNode(beforeAfterContainer, m_counter.identifier(), true)->addRenderer(const_cast<RenderCounter*>(this));
        ASSERT(m_counterNode);
    }
    CounterNode* child = m_counterNode;
    int value = child->actsAsReset() ? child->value() : child->countInParent();

    String text = listMarkerText(m_counter.listStyle(), value);

    if (!m_counter.separator().isNull()) {
        if (!child->actsAsReset())
            child = child->parent();
        while (CounterNode* parent = child->parent()) {
            text = listMarkerText(m_counter.listStyle(), child->countInParent())
                + m_counter.separator() + text;
            child = parent;
        }
    }

    return text.impl();
}

void RenderCounter::updateCounter()
{
    computePreferredLogicalWidths(0);
}

void RenderCounter::computePreferredLogicalWidths(float lead)
{
#ifndef NDEBUG
    // FIXME: We shouldn't be modifying the tree in computePreferredLogicalWidths.
    // Instead, we should properly hook the appropriate changes in the DOM and modify
    // the render tree then. When that's done, we also won't need to override
    // computePreferredLogicalWidths at all.
    // https://bugs.webkit.org/show_bug.cgi?id=104829
    SetLayoutNeededForbiddenScope layoutForbiddenScope(this, false);
#endif

    setTextInternal(originalText());

    RenderText::computePreferredLogicalWidths(lead);
}

void RenderCounter::invalidate()
{
    m_counterNode->removeRenderer(this);
    ASSERT(!m_counterNode);
    if (documentBeingDestroyed())
        return;
    setNeedsLayoutAndPrefWidthsRecalc();
}

static void destroyCounterNodeWithoutMapRemoval(const AtomicString& identifier, CounterNode* node)
{
    CounterNode* previous;
    for (RefPtr<CounterNode> child = node->lastDescendant(); child && child != node; child = previous) {
        previous = child->previousInPreOrder();
        child->parent()->removeChild(child.get());
        ASSERT(counterMaps().get(child->owner())->get(identifier) == child);
        counterMaps().get(child->owner())->remove(identifier);
    }
    if (CounterNode* parent = node->parent())
        parent->removeChild(node);
}

void RenderCounter::destroyCounterNodes(RenderObject* owner)
{
    CounterMaps& maps = counterMaps();
    CounterMaps::iterator mapsIterator = maps.find(owner);
    if (mapsIterator == maps.end())
        return;
    CounterMap* map = mapsIterator->value.get();
    CounterMap::const_iterator end = map->end();
    for (CounterMap::const_iterator it = map->begin(); it != end; ++it) {
        destroyCounterNodeWithoutMapRemoval(it->key, it->value.get());
    }
    maps.remove(mapsIterator);
    owner->setHasCounterNodeMap(false);
}

void RenderCounter::destroyCounterNode(RenderObject* owner, const AtomicString& identifier)
{
    CounterMap* map = counterMaps().get(owner);
    if (!map)
        return;
    CounterMap::iterator mapIterator = map->find(identifier);
    if (mapIterator == map->end())
        return;
    destroyCounterNodeWithoutMapRemoval(identifier, mapIterator->value.get());
    map->remove(mapIterator);
    // We do not delete "map" here even if empty because we expect to reuse
    // it soon. In order for a renderer to lose all its counters permanently,
    // a style change for the renderer involving removal of all counter
    // directives must occur, in which case, RenderCounter::destroyCounterNodes()
    // must be called.
    // The destruction of the Renderer (possibly caused by the removal of its 
    // associated DOM node) is the other case that leads to the permanent
    // destruction of all counters attached to a Renderer. In this case
    // RenderCounter::destroyCounterNodes() must be and is now called, too.
    // RenderCounter::destroyCounterNodes() handles destruction of the counter
    // map associated with a renderer, so there is no risk in leaking the map.
}

void RenderCounter::rendererRemovedFromTree(RenderObject* renderer)
{
    ASSERT(renderer->view());
    if (!renderer->view()->hasRenderCounters())
        return;
    RenderObject* currentRenderer = renderer->lastLeafChild();
    if (!currentRenderer)
        currentRenderer = renderer;
    while (true) {
        destroyCounterNodes(currentRenderer);
        if (currentRenderer == renderer)
            break;
        currentRenderer = currentRenderer->previousInPreOrder();
    }
}

static void updateCounters(RenderObject* renderer)
{
    ASSERT(renderer->style());
    const CounterDirectiveMap* directiveMap = renderer->style()->counterDirectives();
    if (!directiveMap)
        return;
    CounterDirectiveMap::const_iterator end = directiveMap->end();
    if (!renderer->hasCounterNodeMap()) {
        for (CounterDirectiveMap::const_iterator it = directiveMap->begin(); it != end; ++it)
            makeCounterNode(renderer, it->key, false);
        return;
    }
    CounterMap* counterMap = counterMaps().get(renderer);
    ASSERT(counterMap);
    for (CounterDirectiveMap::const_iterator it = directiveMap->begin(); it != end; ++it) {
        RefPtr<CounterNode> node = counterMap->get(it->key);
        if (!node) {
            makeCounterNode(renderer, it->key, false);
            continue;
        }
        RefPtr<CounterNode> newParent = 0;
        RefPtr<CounterNode> newPreviousSibling = 0;
        
        findPlaceForCounter(renderer, it->key, node->hasResetType(), newParent, newPreviousSibling);
        if (node != counterMap->get(it->key))
            continue;
        CounterNode* parent = node->parent();
        if (newParent == parent && newPreviousSibling == node->previousSibling())
            continue;
        if (parent)
            parent->removeChild(node.get());
        if (newParent)
            newParent->insertAfter(node.get(), newPreviousSibling.get(), it->key);
    }
}

void RenderCounter::rendererSubtreeAttached(RenderObject* renderer)
{
    ASSERT(renderer->view());
    if (!renderer->view()->hasRenderCounters())
        return;
    Node* node = renderer->node();
    if (node)
        node = node->parentNode();
    else
        node = renderer->generatingNode();
    if (node && !node->attached())
        return; // No need to update if the parent is not attached yet
    for (RenderObject* descendant = renderer; descendant; descendant = descendant->nextInPreOrder(renderer))
        updateCounters(descendant);
}

void RenderCounter::rendererStyleChanged(RenderObject* renderer, const RenderStyle* oldStyle, const RenderStyle* newStyle)
{
    Node* node = renderer->generatingNode();
    if (!node || !node->attached())
        return; // cannot have generated content or if it can have, it will be handled during attaching
    const CounterDirectiveMap* newCounterDirectives;
    const CounterDirectiveMap* oldCounterDirectives;
    if (oldStyle && (oldCounterDirectives = oldStyle->counterDirectives())) {
        if (newStyle && (newCounterDirectives = newStyle->counterDirectives())) {
            CounterDirectiveMap::const_iterator newMapEnd = newCounterDirectives->end();
            CounterDirectiveMap::const_iterator oldMapEnd = oldCounterDirectives->end();
            for (CounterDirectiveMap::const_iterator it = newCounterDirectives->begin(); it != newMapEnd; ++it) {
                CounterDirectiveMap::const_iterator oldMapIt = oldCounterDirectives->find(it->key);
                if (oldMapIt != oldMapEnd) {
                    if (oldMapIt->value == it->value)
                        continue;
                    RenderCounter::destroyCounterNode(renderer, it->key);
                }
                // We must create this node here, because the changed node may be a node with no display such as
                // as those created by the increment or reset directives and the re-layout that will happen will
                // not catch the change if the node had no children.
                makeCounterNode(renderer, it->key, false);
            }
            // Destroying old counters that do not exist in the new counterDirective map.
            for (CounterDirectiveMap::const_iterator it = oldCounterDirectives->begin(); it !=oldMapEnd; ++it) {
                if (!newCounterDirectives->contains(it->key))
                    RenderCounter::destroyCounterNode(renderer, it->key);
            }
        } else {
            if (renderer->hasCounterNodeMap())
                RenderCounter::destroyCounterNodes(renderer);
        }
    } else if (newStyle && (newCounterDirectives = newStyle->counterDirectives())) {
        CounterDirectiveMap::const_iterator newMapEnd = newCounterDirectives->end();
        for (CounterDirectiveMap::const_iterator it = newCounterDirectives->begin(); it != newMapEnd; ++it) {
            // We must create this node here, because the added node may be a node with no display such as
            // as those created by the increment or reset directives and the re-layout that will happen will
            // not catch the change if the node had no children.
            makeCounterNode(renderer, it->key, false);
        }
    }
}

} // namespace WebCore

#ifndef NDEBUG

void showCounterRendererTree(const WebCore::RenderObject* renderer, const char* counterName)
{
    if (!renderer)
        return;
    const WebCore::RenderObject* root = renderer;
    while (root->parent())
        root = root->parent();

    AtomicString identifier(counterName);
    for (const WebCore::RenderObject* current = root; current; current = current->nextInPreOrder()) {
        fprintf(stderr, "%c", (current == renderer) ? '*' : ' ');
        for (const WebCore::RenderObject* parent = current; parent && parent != root; parent = parent->parent())
            fprintf(stderr, "    ");
        fprintf(stderr, "%p N:%p P:%p PS:%p NS:%p C:%p\n",
            current, current->node(), current->parent(), current->previousSibling(),
            current->nextSibling(), current->hasCounterNodeMap() ?
            counterName ? WebCore::counterMaps().get(current)->get(identifier) : (WebCore::CounterNode*)1 : (WebCore::CounterNode*)0);
    }
    fflush(stderr);
}

#endif // NDEBUG
