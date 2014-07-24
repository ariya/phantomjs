/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007, 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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
#include "SelectorCheckerFastPath.h"

#include "HTMLDocument.h"
#include "HTMLNames.h"
#include "StyledElement.h"

namespace WebCore {

using namespace HTMLNames;

namespace {

template <bool checkValue(const Element*, const CSSSelector*)>
inline bool fastCheckSingleSelector(const CSSSelector*& selector, const Element*& element, const CSSSelector*& topChildOrSubselector, const Element*& topChildOrSubselectorMatchElement)
{
    for (; element; element = element->parentElement()) {
        if (checkValue(element, selector)) {
            if (selector->relation() == CSSSelector::Descendant)
                topChildOrSubselector = 0;
            else if (!topChildOrSubselector) {
                ASSERT(selector->relation() == CSSSelector::Child || selector->relation() == CSSSelector::SubSelector);
                topChildOrSubselector = selector;
                topChildOrSubselectorMatchElement = element;
            }
            if (selector->relation() != CSSSelector::SubSelector)
                element = element->parentElement();
            selector = selector->tagHistory();
            return true;
        }
        if (topChildOrSubselector) {
            // Child or subselector check failed.
            // If the match element is null, topChildOrSubselector was also the very topmost selector and had to match
            // the original element we were checking.
            if (!topChildOrSubselectorMatchElement)
                return false;
            // There may be other matches down the ancestor chain.
            // Rewind to the topmost child or subselector and the element it matched, continue checking ancestors.
            selector = topChildOrSubselector;
            element = topChildOrSubselectorMatchElement->parentElement();
            topChildOrSubselector = 0;
            return true;
        }
    }
    return false;
}

inline bool checkClassValue(const Element* element, const CSSSelector* selector)
{
    return element->hasClass() && element->classNames().contains(selector->value());
}

inline bool checkIDValue(const Element* element, const CSSSelector* selector)
{
    return element->hasID() && element->idForStyleResolution().impl() == selector->value().impl();
}

inline bool checkExactAttributeValue(const Element* element, const CSSSelector* selector)
{
    return SelectorChecker::checkExactAttribute(element, selector, selector->attribute(), selector->value().impl());
}

inline bool checkTagValue(const Element* element, const CSSSelector* selector)
{
    return SelectorChecker::tagMatches(element, selector->tagQName());
}

}

SelectorCheckerFastPath::SelectorCheckerFastPath(const CSSSelector* selector, const Element* element)
    : m_selector(selector)
    , m_element(element)
{
}

bool SelectorCheckerFastPath::matchesRightmostSelector(SelectorChecker::VisitedMatchType visitedMatchType) const
{
    ASSERT(SelectorCheckerFastPath::canUse(m_selector));

    switch (m_selector->m_match) {
    case CSSSelector::Tag:
        return checkTagValue(m_element, m_selector);
    case CSSSelector::Class:
        return checkClassValue(m_element, m_selector);
    case CSSSelector::Id:
        return checkIDValue(m_element, m_selector);
    case CSSSelector::Exact:
    case CSSSelector::Set:
        return checkExactAttributeValue(m_element, m_selector);
    case CSSSelector::PseudoClass:
        return commonPseudoClassSelectorMatches(visitedMatchType);
    default:
        ASSERT_NOT_REACHED();
    }
    return false;
}

bool SelectorCheckerFastPath::matches() const
{
    ASSERT(matchesRightmostSelector(SelectorChecker::VisitedMatchEnabled));
    const CSSSelector* selector = m_selector;
    const Element* element = m_element;

    const CSSSelector* topChildOrSubselector = 0;
    const Element* topChildOrSubselectorMatchElement = 0;
    if (selector->relation() == CSSSelector::Child || selector->relation() == CSSSelector::SubSelector)
        topChildOrSubselector = selector;

    if (selector->relation() != CSSSelector::SubSelector)
        element = element->parentElement();

    selector = selector->tagHistory();

    // We know this compound selector has descendant, child and subselector combinators only and all components are simple.
    while (selector) {
        switch (selector->m_match) {
        case CSSSelector::Class:
            if (!fastCheckSingleSelector<checkClassValue>(selector, element, topChildOrSubselector, topChildOrSubselectorMatchElement))
                return false;
            break;
        case CSSSelector::Id:
            if (!fastCheckSingleSelector<checkIDValue>(selector, element, topChildOrSubselector, topChildOrSubselectorMatchElement))
                return false;
            break;
        case CSSSelector::Tag:
            if (!fastCheckSingleSelector<checkTagValue>(selector, element, topChildOrSubselector, topChildOrSubselectorMatchElement))
                return false;
            break;
        case CSSSelector::Set:
        case CSSSelector::Exact:
            if (!fastCheckSingleSelector<checkExactAttributeValue>(selector, element, topChildOrSubselector, topChildOrSubselectorMatchElement))
                return false;
            break;
        default:
            ASSERT_NOT_REACHED();
        }
    }
    return true;
}

static inline bool isFastCheckableRelation(CSSSelector::Relation relation)
{
    return relation == CSSSelector::Descendant || relation == CSSSelector::Child || relation == CSSSelector::SubSelector;
}

static inline bool isFastCheckableMatch(const CSSSelector* selector)
{
    if (selector->m_match == CSSSelector::Set) {
        // Style attribute is generated lazily but the fast path doesn't trigger it.
        // Disallow them here rather than making the fast path more branchy.
        return selector->attribute() != styleAttr;
    }
    if (selector->m_match == CSSSelector::Exact)
        return selector->attribute() != styleAttr && HTMLDocument::isCaseSensitiveAttribute(selector->attribute());
    return selector->m_match == CSSSelector::Tag || selector->m_match == CSSSelector::Id || selector->m_match == CSSSelector::Class;
}

static inline bool isFastCheckableRightmostSelector(const CSSSelector* selector)
{
    if (!isFastCheckableRelation(selector->relation()))
        return false;
    return isFastCheckableMatch(selector) || SelectorChecker::isCommonPseudoClassSelector(selector);
}

bool SelectorCheckerFastPath::canUse(const CSSSelector* selector)
{
    if (!isFastCheckableRightmostSelector(selector))
        return false;
    for (selector = selector->tagHistory(); selector; selector = selector->tagHistory()) {
        if (!isFastCheckableRelation(selector->relation()))
            return false;
        if (!isFastCheckableMatch(selector))
            return false;
    }
    return true;
}

bool SelectorCheckerFastPath::commonPseudoClassSelectorMatches(SelectorChecker::VisitedMatchType visitedMatchType) const
{
    ASSERT(SelectorChecker::isCommonPseudoClassSelector(m_selector));
    switch (m_selector->pseudoType()) {
    case CSSSelector::PseudoLink:
    case CSSSelector::PseudoAnyLink:
        return m_element->isLink();
    case CSSSelector::PseudoVisited:
        return m_element->isLink() && visitedMatchType == SelectorChecker::VisitedMatchEnabled;
    case CSSSelector::PseudoFocus:
        return SelectorChecker::matchesFocusPseudoClass(m_element);
    default:
        ASSERT_NOT_REACHED();
    }
    return true;
}


}
