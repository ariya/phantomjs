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

#ifndef SelectorChecker_h
#define SelectorChecker_h

#include "CSSSelector.h"
#include "InspectorInstrumentation.h"
#include "SpaceSplitString.h"
#include <wtf/HashSet.h>
#include <wtf/Vector.h>

namespace WebCore {

class CSSSelector;
class Element;
class RenderScrollbar;
class RenderStyle;

class SelectorChecker {
    WTF_MAKE_NONCOPYABLE(SelectorChecker);
public:
    enum Match { SelectorMatches, SelectorFailsLocally, SelectorFailsAllSiblings, SelectorFailsCompletely };
    enum VisitedMatchType { VisitedMatchDisabled, VisitedMatchEnabled };
    enum Mode { ResolvingStyle = 0, CollectingRules, QueryingRules, SharingRules };
    explicit SelectorChecker(Document*, Mode);
    enum BehaviorAtBoundary { DoesNotCrossBoundary, CrossesBoundary, StaysWithinTreeScope };

    struct SelectorCheckingContext {
        // Initial selector constructor
        SelectorCheckingContext(const CSSSelector* selector, Element* element, VisitedMatchType visitedMatchType)
            : selector(selector)
            , element(element)
            , scope(0)
            , visitedMatchType(visitedMatchType)
            , pseudoId(NOPSEUDO)
            , elementStyle(0)
            , scrollbar(0)
            , scrollbarPart(NoPart)
            , isSubSelector(false)
            , hasScrollbarPseudo(false)
            , hasSelectionPseudo(false)
            , behaviorAtBoundary(DoesNotCrossBoundary)
        { }

        const CSSSelector* selector;
        Element* element;
        const ContainerNode* scope;
        VisitedMatchType visitedMatchType;
        PseudoId pseudoId;
        RenderStyle* elementStyle;
        RenderScrollbar* scrollbar;
        ScrollbarPart scrollbarPart;
        bool isSubSelector;
        bool hasScrollbarPseudo;
        bool hasSelectionPseudo;
        BehaviorAtBoundary behaviorAtBoundary;
    };

    Match match(const SelectorCheckingContext&, PseudoId&) const;
    bool checkOne(const SelectorCheckingContext&) const;

    bool strictParsing() const { return m_strictParsing; }

    Mode mode() const { return m_mode; }

    static bool tagMatches(const Element*, const QualifiedName&);
    static bool isCommonPseudoClassSelector(const CSSSelector*);
    static bool matchesFocusPseudoClass(const Element*);
    static bool checkExactAttribute(const Element*, const CSSSelector*, const QualifiedName& selectorAttributeName, const AtomicStringImpl* value);

    enum LinkMatchMask { MatchLink = 1, MatchVisited = 2, MatchAll = MatchLink | MatchVisited };
    static unsigned determineLinkMatchType(const CSSSelector*);

private:
    bool checkScrollbarPseudoClass(const SelectorCheckingContext&, Document*, const CSSSelector*) const;

    static bool isFrameFocused(const Element*);

    bool m_strictParsing;
    bool m_documentIsHTML;
    Mode m_mode;
};

inline bool SelectorChecker::isCommonPseudoClassSelector(const CSSSelector* selector)
{
    if (selector->m_match != CSSSelector::PseudoClass)
        return false;
    CSSSelector::PseudoType pseudoType = selector->pseudoType();
    return pseudoType == CSSSelector::PseudoLink
        || pseudoType == CSSSelector::PseudoAnyLink
        || pseudoType == CSSSelector::PseudoVisited
        || pseudoType == CSSSelector::PseudoFocus;
}

inline bool SelectorChecker::tagMatches(const Element* element, const QualifiedName& tagQName)
{
    if (tagQName == anyQName())
        return true;
    const AtomicString& localName = tagQName.localName();
    if (localName != starAtom && localName != element->localName())
        return false;
    const AtomicString& namespaceURI = tagQName.namespaceURI();
    return namespaceURI == starAtom || namespaceURI == element->namespaceURI();
}

inline bool SelectorChecker::checkExactAttribute(const Element* element, const CSSSelector* selector, const QualifiedName& selectorAttributeName, const AtomicStringImpl* value)
{
    if (!element->hasAttributesWithoutUpdate())
        return false;
    const AtomicString& localName = element->isHTMLElement() ? selector->attributeCanonicalLocalName() : selectorAttributeName.localName();
    unsigned size = element->attributeCount();
    for (unsigned i = 0; i < size; ++i) {
        const Attribute* attribute = element->attributeItem(i);
        if (attribute->matches(selectorAttributeName.prefix(), localName, selectorAttributeName.namespaceURI()) && (!value || attribute->value().impl() == value))
            return true;
    }
    return false;
}

}

#endif
