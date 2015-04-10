/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
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
#include "SelectorQuery.h"

#include "CSSParser.h"
#include "CSSSelectorList.h"
#include "Document.h"
#include "NodeTraversal.h"
#include "SelectorChecker.h"
#include "SelectorCheckerFastPath.h"
#include "StaticNodeList.h"
#include "StyledElement.h"

namespace WebCore {

void SelectorDataList::initialize(const CSSSelectorList& selectorList)
{
    ASSERT(m_selectors.isEmpty());

    unsigned selectorCount = 0;
    for (const CSSSelector* selector = selectorList.first(); selector; selector = CSSSelectorList::next(selector))
        selectorCount++;

    m_selectors.reserveInitialCapacity(selectorCount);
    for (const CSSSelector* selector = selectorList.first(); selector; selector = CSSSelectorList::next(selector))
        m_selectors.uncheckedAppend(SelectorData(selector, SelectorCheckerFastPath::canUse(selector)));
}

inline bool SelectorDataList::selectorMatches(const SelectorData& selectorData, Element* element, const Node* rootNode) const
{
    if (selectorData.isFastCheckable && !element->isSVGElement()) {
        SelectorCheckerFastPath selectorCheckerFastPath(selectorData.selector, element);
        if (!selectorCheckerFastPath.matchesRightmostSelector(SelectorChecker::VisitedMatchDisabled))
            return false;
        return selectorCheckerFastPath.matches();
    }

    SelectorChecker selectorChecker(element->document(), SelectorChecker::QueryingRules);
    SelectorChecker::SelectorCheckingContext selectorCheckingContext(selectorData.selector, element, SelectorChecker::VisitedMatchDisabled);
    selectorCheckingContext.behaviorAtBoundary = SelectorChecker::StaysWithinTreeScope;
    selectorCheckingContext.scope = !rootNode->isDocumentNode() && rootNode->isContainerNode() ? toContainerNode(rootNode) : 0;
    PseudoId ignoreDynamicPseudo = NOPSEUDO;
    return selectorChecker.match(selectorCheckingContext, ignoreDynamicPseudo) == SelectorChecker::SelectorMatches;
}

bool SelectorDataList::matches(Element* targetElement) const
{
    ASSERT(targetElement);

    unsigned selectorCount = m_selectors.size();
    for (unsigned i = 0; i < selectorCount; ++i) {
        if (selectorMatches(m_selectors[i], targetElement, targetElement))
            return true;
    }

    return false;
}

PassRefPtr<NodeList> SelectorDataList::queryAll(Node* rootNode) const
{
    Vector<RefPtr<Node> > result;
    execute<false>(rootNode, result);
    return StaticNodeList::adopt(result);
}

PassRefPtr<Element> SelectorDataList::queryFirst(Node* rootNode) const
{
    Vector<RefPtr<Node> > result;
    execute<true>(rootNode, result);
    if (result.isEmpty())
        return 0;
    ASSERT(result.size() == 1);
    ASSERT(result.first()->isElementNode());
    return toElement(result.first().get());
}

static const CSSSelector* selectorForIdLookup(const Node* rootNode, const CSSSelector* selector)
{
    if (!rootNode->inDocument())
        return 0;
    if (rootNode->document()->inQuirksMode())
        return 0;

    do {
        if (selector->m_match == CSSSelector::Id)
            return selector;
        if (selector->relation() != CSSSelector::SubSelector)
            break;
        selector = selector->tagHistory();
    } while (selector);

    return 0;
}

static inline bool isTreeScopeRoot(const Node* node)
{
    ASSERT(node);
    return node->isDocumentNode() || node->isShadowRoot();
}

template <bool firstMatchOnly>
ALWAYS_INLINE void SelectorDataList::executeFastPathForIdSelector(const Node* rootNode, const SelectorData& selectorData, const CSSSelector* idSelector, Vector<RefPtr<Node> >& matchedElements) const
{
    ASSERT(m_selectors.size() == 1);
    ASSERT(idSelector);
    const AtomicString& idToMatch = idSelector->value();
    if (UNLIKELY(rootNode->treeScope()->containsMultipleElementsWithId(idToMatch))) {
        const Vector<Element*>* elements = rootNode->treeScope()->getAllElementsById(idToMatch);
        ASSERT(elements);
        size_t count = elements->size();
        bool rootNodeIsTreeScopeRoot = isTreeScopeRoot(rootNode);
        for (size_t i = 0; i < count; ++i) {
            Element* element = elements->at(i);
            if ((rootNodeIsTreeScopeRoot || element->isDescendantOf(rootNode)) && selectorMatches(selectorData, element, rootNode)) {
                matchedElements.append(element);
                if (firstMatchOnly)
                    return;
            }
        }
        return;
    }
    Element* element = rootNode->treeScope()->getElementById(idToMatch);
    if (!element || !(isTreeScopeRoot(rootNode) || element->isDescendantOf(rootNode)))
        return;
    if (selectorMatches(selectorData, element, rootNode))
        matchedElements.append(element);
}

static bool isSingleTagNameSelector(const CSSSelector* selector)
{
    return selector->isLastInTagHistory() && selector->m_match == CSSSelector::Tag;
}

template <bool firstMatchOnly>
static inline void elementsForLocalName(const Node* rootNode, const AtomicString& localName, Vector<RefPtr<Node> >& matchedElements)
{
    for (Element* element = ElementTraversal::firstWithin(rootNode); element; element = ElementTraversal::next(element, rootNode)) {
        if (element->localName() == localName) {
            matchedElements.append(element);
            if (firstMatchOnly)
                return;
        }
    }
}

template <bool firstMatchOnly>
static inline void anyElement(const Node* rootNode, Vector<RefPtr<Node> >& matchedElements)
{
    for (Element* element = ElementTraversal::firstWithin(rootNode); element; element = ElementTraversal::next(element, rootNode)) {
        matchedElements.append(element);
        if (firstMatchOnly)
            return;
    }
}


template <bool firstMatchOnly>
ALWAYS_INLINE void SelectorDataList::executeSingleTagNameSelectorData(const Node* rootNode, const SelectorData& selectorData, Vector<RefPtr<Node> >& matchedElements) const
{
    ASSERT(m_selectors.size() == 1);
    ASSERT(isSingleTagNameSelector(selectorData.selector));

    const QualifiedName& tagQualifiedName = selectorData.selector->tagQName();
    const AtomicString& selectorLocalName = tagQualifiedName.localName();
    const AtomicString& selectorNamespaceURI = tagQualifiedName.namespaceURI();

    if (selectorNamespaceURI == starAtom) {
        if (selectorLocalName != starAtom) {
            // Common case: name defined, selectorNamespaceURI is a wildcard.
            elementsForLocalName<firstMatchOnly>(rootNode, selectorLocalName, matchedElements);
        } else {
            // Other fairly common case: both are wildcards.
            anyElement<firstMatchOnly>(rootNode, matchedElements);
        }
    } else {
        // Fallback: NamespaceURI is set, selectorLocalName may be starAtom.
        for (Element* element = ElementTraversal::firstWithin(rootNode); element; element = ElementTraversal::next(element, rootNode)) {
            if (element->namespaceURI() == selectorNamespaceURI && (selectorLocalName == starAtom || element->localName() == selectorLocalName)) {
                matchedElements.append(element);
                if (firstMatchOnly)
                    break;
            }
        }
    }
#if !ASSERT_DISABLED
    for (size_t i = 0; i < matchedElements.size(); ++i) {
        ASSERT(matchedElements[i]->isElementNode());
        ASSERT(SelectorChecker::tagMatches(static_cast<const Element*>(matchedElements[i].get()), tagQualifiedName));
    }
#endif
}

static bool isSingleClassNameSelector(const CSSSelector* selector)
{
    return selector->isLastInTagHistory() && selector->m_match == CSSSelector::Class;
}

template <bool firstMatchOnly>
ALWAYS_INLINE void SelectorDataList::executeSingleClassNameSelectorData(const Node* rootNode, const SelectorData& selectorData, Vector<RefPtr<Node> >& matchedElements) const
{
    ASSERT(m_selectors.size() == 1);
    ASSERT(isSingleClassNameSelector(selectorData.selector));

    const AtomicString& className = selectorData.selector->value();
    for (Element* element = ElementTraversal::firstWithin(rootNode); element; element = ElementTraversal::next(element, rootNode)) {
        if (element->hasClass() && element->classNames().contains(className)) {
            matchedElements.append(element);
            if (firstMatchOnly)
                return;
        }
    }
}

template <bool firstMatchOnly>
ALWAYS_INLINE void SelectorDataList::executeSingleSelectorData(const Node* rootNode, const SelectorData& selectorData, Vector<RefPtr<Node> >& matchedElements) const
{
    ASSERT(m_selectors.size() == 1);

    for (Element* element = ElementTraversal::firstWithin(rootNode); element; element = ElementTraversal::next(element, rootNode)) {
        if (selectorMatches(selectorData, element, rootNode)) {
            matchedElements.append(element);
            if (firstMatchOnly)
                return;
        }
    }
}

template <bool firstMatchOnly>
ALWAYS_INLINE void SelectorDataList::executeSingleMultiSelectorData(const Node* rootNode, Vector<RefPtr<Node> >& matchedElements) const
{
    unsigned selectorCount = m_selectors.size();
    for (Element* element = ElementTraversal::firstWithin(rootNode); element; element = ElementTraversal::next(element, rootNode)) {
        for (unsigned i = 0; i < selectorCount; ++i) {
            if (selectorMatches(m_selectors[i], element, rootNode)) {
                matchedElements.append(element);
                if (firstMatchOnly)
                    return;
                break;
            }
        }
    }
}

template <bool firstMatchOnly>
ALWAYS_INLINE void SelectorDataList::execute(Node* rootNode, Vector<RefPtr<Node> >& matchedElements) const
{
    if (m_selectors.size() == 1) {
        const SelectorData& selectorData = m_selectors[0];
        if (const CSSSelector* idSelector = selectorForIdLookup(rootNode, selectorData.selector))
            executeFastPathForIdSelector<firstMatchOnly>(rootNode, selectorData, idSelector, matchedElements);
        else if (isSingleTagNameSelector(selectorData.selector))
            executeSingleTagNameSelectorData<firstMatchOnly>(rootNode, selectorData, matchedElements);
        else if (isSingleClassNameSelector(selectorData.selector))
            executeSingleClassNameSelectorData<firstMatchOnly>(rootNode, selectorData, matchedElements);
        else
            executeSingleSelectorData<firstMatchOnly>(rootNode, selectorData, matchedElements);
        return;
    }
    executeSingleMultiSelectorData<firstMatchOnly>(rootNode, matchedElements);
}

SelectorQuery::SelectorQuery(const CSSSelectorList& selectorList)
    : m_selectorList(selectorList)
{
    m_selectors.initialize(m_selectorList);
}

SelectorQuery* SelectorQueryCache::add(const AtomicString& selectors, Document* document, ExceptionCode& ec)
{
    HashMap<AtomicString, OwnPtr<SelectorQuery> >::iterator it = m_entries.find(selectors);
    if (it != m_entries.end())
        return it->value.get();

    CSSParser parser(document);
    CSSSelectorList selectorList;
    parser.parseSelector(selectors, selectorList);

    if (!selectorList.first() || selectorList.hasInvalidSelector()) {
        ec = SYNTAX_ERR;
        return 0;
    }

    // throw a NAMESPACE_ERR if the selector includes any namespace prefixes.
    if (selectorList.selectorsNeedNamespaceResolution()) {
        ec = NAMESPACE_ERR;
        return 0;
    }

    const int maximumSelectorQueryCacheSize = 256;
    if (m_entries.size() == maximumSelectorQueryCacheSize)
        m_entries.remove(m_entries.begin());
    
    OwnPtr<SelectorQuery> selectorQuery = adoptPtr(new SelectorQuery(selectorList));
    SelectorQuery* rawSelectorQuery = selectorQuery.get();
    m_entries.add(selectors, selectorQuery.release());
    return rawSelectorQuery;
}

void SelectorQueryCache::invalidate()
{
    m_entries.clear();
}

}
