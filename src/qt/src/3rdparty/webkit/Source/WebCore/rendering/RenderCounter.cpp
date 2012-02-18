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
#include "RenderListItem.h"
#include "RenderListMarker.h"
#include "RenderStyle.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

using namespace HTMLNames;

typedef HashMap<RefPtr<AtomicStringImpl>, RefPtr<CounterNode> > CounterMap;
typedef HashMap<const RenderObject*, CounterMap*> CounterMaps;

static CounterNode* makeCounterNode(RenderObject*, const AtomicString& identifier, bool alwaysCreateCounter);

static CounterMaps& counterMaps()
{
    DEFINE_STATIC_LOCAL(CounterMaps, staticCounterMaps, ());
    return staticCounterMaps;
}

// This function processes the renderer tree in the order of the DOM tree
// including pseudo elements as defined in CSS 2.1.
// Anonymous renderers are skipped except for those representing pseudo elements.
static RenderObject* previousInPreOrder(const RenderObject* object)
{
    Element* parent;
    Element* sibling;
    switch (object->style()->styleType()) {
    case NOPSEUDO:
        ASSERT(!object->isAnonymous());
        parent = toElement(object->node());
        sibling = parent->previousElementSibling();
        parent = parent->parentElement();
        break;
    case BEFORE:
        return object->generatingNode()->renderer(); // It is always the generating node's renderer
    case AFTER:
        parent = toElement(object->generatingNode());
        sibling = parent->lastElementChild();
        break;
    default:
        ASSERT_NOT_REACHED();
        return 0;
    }
    while (sibling) {
        if (RenderObject* renderer = sibling->renderer()) {
            if (RenderObject* after = renderer->afterPseudoElementRenderer())
                return after;
            parent = sibling;
            sibling = sibling->lastElementChild();
            if (!sibling) {
                if (RenderObject* before = renderer->beforePseudoElementRenderer())
                    return before;
                return renderer;
            }
        } else
            sibling = sibling->previousElementSibling();
    }
    if (!parent)
        return 0;
    RenderObject* renderer = parent->renderer(); // Should never be null
    if (RenderObject* before = renderer->beforePseudoElementRenderer())
        return before;
    return renderer;
}

// This function processes the renderer tree in the order of the DOM tree
// including pseudo elements as defined in CSS 2.1.
// Anonymous renderers are skipped except for those representing pseudo elements.
static RenderObject* previousSiblingOrParent(const RenderObject* object)
{
    Element* parent;
    Element* sibling;
    switch (object->style()->styleType()) {
    case NOPSEUDO:
        ASSERT(!object->isAnonymous());
        parent = toElement(object->node());
        sibling = parent->previousElementSibling();
        parent = parent->parentElement();
        break;
    case BEFORE:
        return object->generatingNode()->renderer(); // It is always the generating node's renderer
    case AFTER:
        parent = toElement(object->generatingNode());
        sibling = parent->lastElementChild();
        break;
    default:
        ASSERT_NOT_REACHED();
        return 0;
    }
    while (sibling) {
        if (RenderObject* renderer = sibling->renderer()) // This skips invisible nodes
            return renderer;
        sibling = sibling->previousElementSibling();
    }
    if (parent) {
        RenderObject* renderer = parent->renderer();
        if (RenderObject* before = renderer->virtualChildren()->beforePseudoElementRenderer(renderer))
            return before;
        return renderer;
    }
    return 0;
}

static Element* parentElement(RenderObject* object)
{
    switch (object->style()->styleType()) {
    case NOPSEUDO:
        ASSERT(!object->isAnonymous());
        return toElement(object->node())->parentElement();
    case BEFORE:
    case AFTER:
        return toElement(object->generatingNode());
    default:
        ASSERT_NOT_REACHED();
        return 0;
    }
}

static inline bool areRenderersElementsSiblings(RenderObject* first, RenderObject* second)
{
    return parentElement(first) == parentElement(second);
}

// This function processes the renderer tree in the order of the DOM tree
// including pseudo elements as defined in CSS 2.1.
// Anonymous renderers are skipped except for those representing pseudo elements.
static RenderObject* nextInPreOrder(const RenderObject* object, const Element* stayWithin, bool skipDescendants = false)
{
    Element* self;
    Element* child;
    RenderObject* result;
    self = toElement(object->generatingNode());
    if (skipDescendants)
        goto nextsibling;
    switch (object->style()->styleType()) {
    case NOPSEUDO:
        ASSERT(!object->isAnonymous());
        result = object->beforePseudoElementRenderer();
        if (result)
            return result;
        break;
    case BEFORE:
        break;
    case AFTER:
        goto nextsibling;
    default:
        ASSERT_NOT_REACHED();
        return 0;
    }
    child = self->firstElementChild();
    while (true) {
        while (child) {
            result = child->renderer();
            if (result)
                return result;
            child = child->nextElementSibling();
        }
        result = self->renderer()->afterPseudoElementRenderer();
        if (result)
            return result;
nextsibling:
        if (self == stayWithin)
            return 0;
        child = self->nextElementSibling();
        self = self->parentElement();
        if (!self) {
            ASSERT(!child); // We can only reach this if we are searching beyond the root element
            return 0; //  which cannot have siblings
        }
    }
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

    if (const CounterDirectiveMap* directivesMap = style->counterDirectives()) {
        CounterDirectives directives = directivesMap->get(identifier.impl());
        if (directives.m_reset) {
            value = directives.m_resetValue;
            if (directives.m_increment)
                value += directives.m_incrementValue;
            isReset = true;
            return true;
        }
        if (directives.m_increment) {
            value = directives.m_incrementValue;
            isReset = false;
            return true;
        }
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

static bool findPlaceForCounter(RenderObject* counterOwner, const AtomicString& identifier, bool isReset, CounterNode*& parent, CounterNode*& previousSibling)
{
    // We cannot stop searching for counters with the same identifier before we also
    // check this renderer, because it may affect the positioning in the tree of our counter.
    RenderObject* searchEndRenderer = previousSiblingOrParent(counterOwner);
    // We check renderers in preOrder from the renderer that our counter is attached to
    // towards the begining of the document for counters with the same identifier as the one
    // we are trying to find a place for. This is the next renderer to be checked.
    RenderObject* currentRenderer = previousInPreOrder(counterOwner);
    previousSibling = 0;
    while (currentRenderer) {
        CounterNode* currentCounter = makeCounterNode(currentRenderer, identifier, false);
        if (searchEndRenderer == currentRenderer) {
            // We may be at the end of our search.
            if (currentCounter) {
                // We have a suitable counter on the EndSearchRenderer.
                if (previousSibling) { // But we already found another counter that we come after.
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
                        ASSERT(previousSibling->parent() == currentCounter);
                        return true;
                    }
                    // CurrentCounter, the counter at the EndSearchRenderer, is not reset.
                    if (!isReset || !areRenderersElementsSiblings(currentRenderer, counterOwner)) {
                        // If the node we are placing is not reset or we have found a counter that is attached
                        // to an ancestor of the placed counter's owner renderer we know we are a sibling of that node.
                        ASSERT(currentCounter->parent() == previousSibling->parent());
                        parent = currentCounter->parent();
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
                        return true;
                    }
                    if (!isReset || !areRenderersElementsSiblings(currentRenderer, counterOwner)) {
                        parent = currentCounter->parent();
                        previousSibling = currentCounter;
                        return true;
                    }
                    previousSibling = currentCounter;
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
                if (previousSibling) {
                    // Since we had a suitable previous counter before, we should only consider this one as our 
                    // previousSibling if it is a reset counter and hence the current previousSibling is its child.
                    if (currentCounter->actsAsReset()) {
                        previousSibling = currentCounter;
                        // We are no longer interested in previous siblings of the currentRenderer or their children
                        // as counters they may have attached cannot be the previous sibling of the counter we are placing.
                        currentRenderer = parentElement(currentRenderer)->renderer();
                        continue;
                    }
                } else
                    previousSibling = currentCounter;
                currentRenderer = previousSiblingOrParent(currentRenderer);
                continue;
            }
        }
        // This function is designed so that the same test is not done twice in an iteration, except for this one
        // which may be done twice in some cases. Rearranging the decision points though, to accommodate this 
        // performance improvement would create more code duplication than is worthwhile in my oppinion and may further
        // impede the readability of this already complex algorithm.
        if (previousSibling)
            currentRenderer = previousSiblingOrParent(currentRenderer);
        else
            currentRenderer = previousInPreOrder(currentRenderer);
    }
    return false;
}

static CounterNode* makeCounterNode(RenderObject* object, const AtomicString& identifier, bool alwaysCreateCounter)
{
    ASSERT(object);

    if (object->m_hasCounterNodeMap) {
        if (CounterMap* nodeMap = counterMaps().get(object)) {
            if (CounterNode* node = nodeMap->get(identifier.impl()).get())
                return node;
        }
    }

    bool isReset = false;
    int value = 0;
    if (!planCounter(object, identifier, isReset, value) && !alwaysCreateCounter)
        return 0;

    CounterNode* newParent = 0;
    CounterNode* newPreviousSibling = 0;
    RefPtr<CounterNode> newNode = CounterNode::create(object, isReset, value);
    if (findPlaceForCounter(object, identifier, isReset, newParent, newPreviousSibling))
        newParent->insertAfter(newNode.get(), newPreviousSibling, identifier);
    CounterMap* nodeMap;
    if (object->m_hasCounterNodeMap)
        nodeMap = counterMaps().get(object);
    else {
        nodeMap = new CounterMap;
        counterMaps().set(object, nodeMap);
        object->m_hasCounterNodeMap = true;
    }
    nodeMap->set(identifier.impl(), newNode);
    if (newNode->parent())
        return newNode.get();
    // Checking if some nodes that were previously counter tree root nodes
    // should become children of this node now.
    CounterMaps& maps = counterMaps();
    Element* stayWithin = parentElement(object);
    bool skipDescendants;
    for (RenderObject* currentRenderer = nextInPreOrder(object, stayWithin); currentRenderer; currentRenderer = nextInPreOrder(currentRenderer, stayWithin, skipDescendants)) {
        skipDescendants = false;
        if (!currentRenderer->m_hasCounterNodeMap)
            continue;
        CounterNode* currentCounter = maps.get(currentRenderer)->get(identifier.impl()).get();
        if (!currentCounter)
            continue;
        skipDescendants = true;
        if (currentCounter->parent()) {
            ASSERT(newNode->firstChild());
            continue;
        }
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
}

RenderCounter::~RenderCounter()
{
    if (m_counterNode) {
        m_counterNode->removeRenderer(this);
        ASSERT(!m_counterNode);
    }
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
            if (!beforeAfterContainer->isAnonymous())
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

void RenderCounter::computePreferredLogicalWidths(float lead)
{
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
        ASSERT(counterMaps().get(child->owner())->get(identifier.impl()) == child);
        counterMaps().get(child->owner())->remove(identifier.impl());
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
    CounterMap* map = mapsIterator->second;
    CounterMap::const_iterator end = map->end();
    for (CounterMap::const_iterator it = map->begin(); it != end; ++it) {
        AtomicString identifier(it->first.get());
        destroyCounterNodeWithoutMapRemoval(identifier, it->second.get());
    }
    maps.remove(mapsIterator);
    delete map;
    owner->m_hasCounterNodeMap = false;
}

void RenderCounter::destroyCounterNode(RenderObject* owner, const AtomicString& identifier)
{
    CounterMap* map = counterMaps().get(owner);
    if (!map)
        return;
    CounterMap::iterator mapIterator = map->find(identifier.impl());
    if (mapIterator == map->end())
        return;
    destroyCounterNodeWithoutMapRemoval(identifier, mapIterator->second.get());
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

void RenderCounter::rendererRemovedFromTree(RenderObject* removedRenderer)
{
    RenderObject* currentRenderer = removedRenderer->lastLeafChild();
    if (!currentRenderer)
        currentRenderer = removedRenderer;
    while (true) {
        destroyCounterNodes(currentRenderer);
        if (currentRenderer == removedRenderer)
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
    if (!renderer->m_hasCounterNodeMap) {
        for (CounterDirectiveMap::const_iterator it = directiveMap->begin(); it != end; ++it)
            makeCounterNode(renderer, AtomicString(it->first.get()), false);
        return;
    }
    CounterMap* counterMap = counterMaps().get(renderer);
    ASSERT(counterMap);
    for (CounterDirectiveMap::const_iterator it = directiveMap->begin(); it != end; ++it) {
        RefPtr<CounterNode> node = counterMap->get(it->first.get());
        if (!node) {
            makeCounterNode(renderer, AtomicString(it->first.get()), false);
            continue;
        }
        CounterNode* newParent = 0;
        CounterNode* newPreviousSibling;
        
        findPlaceForCounter(renderer, AtomicString(it->first.get()), node->hasResetType(), newParent, newPreviousSibling);
        if (node != counterMap->get(it->first.get()))
            continue;
        CounterNode* parent = node->parent();
        if (newParent == parent && newPreviousSibling == node->previousSibling())
            continue;
        if (parent)
            parent->removeChild(node.get());
        if (newParent)
            newParent->insertAfter(node.get(), newPreviousSibling, it->first.get());
    }
}

void RenderCounter::rendererSubtreeAttached(RenderObject* renderer)
{
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
                CounterDirectiveMap::const_iterator oldMapIt = oldCounterDirectives->find(it->first);
                if (oldMapIt != oldMapEnd) {
                    if (oldMapIt->second == it->second)
                        continue;
                    RenderCounter::destroyCounterNode(renderer, it->first.get());
                }
                // We must create this node here, because the changed node may be a node with no display such as
                // as those created by the increment or reset directives and the re-layout that will happen will
                // not catch the change if the node had no children.
                makeCounterNode(renderer, it->first.get(), false);
            }
            // Destroying old counters that do not exist in the new counterDirective map.
            for (CounterDirectiveMap::const_iterator it = oldCounterDirectives->begin(); it !=oldMapEnd; ++it) {
                if (!newCounterDirectives->contains(it->first))
                    RenderCounter::destroyCounterNode(renderer, it->first.get());
            }
        } else {
            if (renderer->m_hasCounterNodeMap)
                RenderCounter::destroyCounterNodes(renderer);
        }
    } else if (newStyle && (newCounterDirectives = newStyle->counterDirectives())) {
        CounterDirectiveMap::const_iterator newMapEnd = newCounterDirectives->end();
        for (CounterDirectiveMap::const_iterator it = newCounterDirectives->begin(); it != newMapEnd; ++it) {
            // We must create this node here, because the added node may be a node with no display such as
            // as those created by the increment or reset directives and the re-layout that will happen will
            // not catch the change if the node had no children.
            makeCounterNode(renderer, it->first.get(), false);
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
            current->nextSibling(), current->m_hasCounterNodeMap?
            counterName ? WebCore::counterMaps().get(current)->get(identifier.impl()).get() : (WebCore::CounterNode*)1 : (WebCore::CounterNode*)0);
    }
    fflush(stderr);
}

#endif // NDEBUG
