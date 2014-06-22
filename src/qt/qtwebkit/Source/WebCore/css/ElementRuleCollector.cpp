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
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "ElementRuleCollector.h"

#include "CSSDefaultStyleSheets.h"
#include "CSSRule.h"
#include "CSSRuleList.h"
#include "CSSSelector.h"
#include "CSSSelectorList.h"
#include "CSSValueKeywords.h"
#include "HTMLElement.h"
#include "RenderRegion.h"
#include "SVGElement.h"
#include "SelectorCheckerFastPath.h"
#include "StylePropertySet.h"
#include "StyledElement.h"

#include <wtf/TemporaryChange.h>

namespace WebCore {

static StylePropertySet* leftToRightDeclaration()
{
    DEFINE_STATIC_LOCAL(RefPtr<MutableStylePropertySet>, leftToRightDecl, (MutableStylePropertySet::create()));
    if (leftToRightDecl->isEmpty())
        leftToRightDecl->setProperty(CSSPropertyDirection, CSSValueLtr);
    return leftToRightDecl.get();
}

static StylePropertySet* rightToLeftDeclaration()
{
    DEFINE_STATIC_LOCAL(RefPtr<MutableStylePropertySet>, rightToLeftDecl, (MutableStylePropertySet::create()));
    if (rightToLeftDecl->isEmpty())
        rightToLeftDecl->setProperty(CSSPropertyDirection, CSSValueRtl);
    return rightToLeftDecl.get();
}

StyleResolver::MatchResult& ElementRuleCollector::matchedResult()
{
    ASSERT(m_mode == SelectorChecker::ResolvingStyle);
    return m_result;
}

const Vector<RefPtr<StyleRuleBase> >& ElementRuleCollector::matchedRuleList() const
{
    ASSERT(m_mode == SelectorChecker::CollectingRules);
    return m_matchedRuleList;
}

inline void ElementRuleCollector::addMatchedRule(const RuleData* rule)
{
    if (!m_matchedRules)
        m_matchedRules = adoptPtr(new Vector<const RuleData*, 32>);
    m_matchedRules->append(rule);
}

inline void ElementRuleCollector::clearMatchedRules()
{
    if (!m_matchedRules)
        return;
    m_matchedRules->clear();
}

inline void ElementRuleCollector::addElementStyleProperties(const StylePropertySet* propertySet, bool isCacheable)
{
    if (!propertySet)
        return;
    m_result.ranges.lastAuthorRule = m_result.matchedProperties.size();
    if (m_result.ranges.firstAuthorRule == -1)
        m_result.ranges.firstAuthorRule = m_result.ranges.lastAuthorRule;
    m_result.addMatchedProperties(propertySet);
    if (!isCacheable)
        m_result.isCacheable = false;
}

class MatchingUARulesScope {
public:
    MatchingUARulesScope();
    ~MatchingUARulesScope();

    static bool isMatchingUARules();

private:
    static bool m_matchingUARules;
};

MatchingUARulesScope::MatchingUARulesScope()
{
    ASSERT(!m_matchingUARules);
    m_matchingUARules = true;
}

MatchingUARulesScope::~MatchingUARulesScope()
{
    m_matchingUARules = false;
}

inline bool MatchingUARulesScope::isMatchingUARules()
{
    return m_matchingUARules;
}

bool MatchingUARulesScope::m_matchingUARules = false;

void ElementRuleCollector::collectMatchingRules(const MatchRequest& matchRequest, StyleResolver::RuleRange& ruleRange)
{
    ASSERT(matchRequest.ruleSet);
    ASSERT(m_state.element());

    const StyleResolver::State& state = m_state;
    Element* element = state.element();
    const StyledElement* styledElement = state.styledElement();
    const AtomicString& pseudoId = element->shadowPseudoId();
    if (!pseudoId.isEmpty()) {
        ASSERT(styledElement);
        collectMatchingRulesForList(matchRequest.ruleSet->shadowPseudoElementRules(pseudoId.impl()), matchRequest, ruleRange);
    }

#if ENABLE(VIDEO_TRACK)
    if (element->isWebVTTElement())
        collectMatchingRulesForList(matchRequest.ruleSet->cuePseudoRules(), matchRequest, ruleRange);
#endif
    // Check whether other types of rules are applicable in the current tree scope. Criteria for this:
    // a) it's a UA rule
    // b) the tree scope allows author rules
    // c) the rules comes from a scoped style sheet within the same tree scope
    TreeScope* treeScope = element->treeScope();
    if (!MatchingUARulesScope::isMatchingUARules()
        && !treeScope->applyAuthorStyles()
        && (!matchRequest.scope || matchRequest.scope->treeScope() != treeScope)
        && m_behaviorAtBoundary == SelectorChecker::DoesNotCrossBoundary)
        return;

    // We need to collect the rules for id, class, tag, and everything else into a buffer and
    // then sort the buffer.
    if (element->hasID())
        collectMatchingRulesForList(matchRequest.ruleSet->idRules(element->idForStyleResolution().impl()), matchRequest, ruleRange);
    if (styledElement && styledElement->hasClass()) {
        for (size_t i = 0; i < styledElement->classNames().size(); ++i)
            collectMatchingRulesForList(matchRequest.ruleSet->classRules(styledElement->classNames()[i].impl()), matchRequest, ruleRange);
    }

    if (element->isLink())
        collectMatchingRulesForList(matchRequest.ruleSet->linkPseudoClassRules(), matchRequest, ruleRange);
    if (SelectorChecker::matchesFocusPseudoClass(element))
        collectMatchingRulesForList(matchRequest.ruleSet->focusPseudoClassRules(), matchRequest, ruleRange);
    collectMatchingRulesForList(matchRequest.ruleSet->tagRules(element->localName().impl()), matchRequest, ruleRange);
    collectMatchingRulesForList(matchRequest.ruleSet->universalRules(), matchRequest, ruleRange);
}

void ElementRuleCollector::collectMatchingRulesForRegion(const MatchRequest& matchRequest, StyleResolver::RuleRange& ruleRange)
{
    if (!m_regionForStyling)
        return;

    unsigned size = matchRequest.ruleSet->m_regionSelectorsAndRuleSets.size();
    for (unsigned i = 0; i < size; ++i) {
        const CSSSelector* regionSelector = matchRequest.ruleSet->m_regionSelectorsAndRuleSets.at(i).selector;
        if (checkRegionSelector(regionSelector, toElement(m_regionForStyling->node()))) {
            RuleSet* regionRules = matchRequest.ruleSet->m_regionSelectorsAndRuleSets.at(i).ruleSet.get();
            ASSERT(regionRules);
            collectMatchingRules(MatchRequest(regionRules, matchRequest.includeEmptyRules, matchRequest.scope), ruleRange);
        }
    }
}

void ElementRuleCollector::sortAndTransferMatchedRules()
{
    const StyleResolver::State& state = m_state;

    if (!m_matchedRules || m_matchedRules->isEmpty())
        return;

    sortMatchedRules();

    Vector<const RuleData*, 32>& matchedRules = *m_matchedRules;
    if (m_mode == SelectorChecker::CollectingRules) {
        for (unsigned i = 0; i < matchedRules.size(); ++i)
            m_matchedRuleList.append(matchedRules[i]->rule());
        return;
    }

    // Now transfer the set of matched rules over to our list of declarations.
    for (unsigned i = 0; i < matchedRules.size(); i++) {
        if (state.style() && matchedRules[i]->containsUncommonAttributeSelector())
            state.style()->setUnique();
        m_result.addMatchedProperties(matchedRules[i]->rule()->properties(), matchedRules[i]->rule(), matchedRules[i]->linkMatchType(), matchedRules[i]->propertyWhitelistType(MatchingUARulesScope::isMatchingUARules()));
    }
}

void ElementRuleCollector::matchScopedAuthorRules(bool includeEmptyRules)
{
#if ENABLE(STYLE_SCOPED) || ENABLE(SHADOW_DOM)
    if (!m_scopeResolver)
        return;

    // Match scoped author rules by traversing the scoped element stack (rebuild it if it got inconsistent).
    if (m_scopeResolver->hasScopedStyles() && m_scopeResolver->ensureStackConsistency(m_state.element())) {
        bool applyAuthorStyles = m_state.element()->treeScope()->applyAuthorStyles();
        bool documentScope = true;
        unsigned scopeSize = m_scopeResolver->stackSize();
        for (unsigned i = 0; i < scopeSize; ++i) {
            clearMatchedRules();
            m_result.ranges.lastAuthorRule = m_result.matchedProperties.size() - 1;

            const StyleScopeResolver::StackFrame& frame = m_scopeResolver->stackFrameAt(i);
            documentScope = documentScope && !frame.m_scope->isInShadowTree();
            if (documentScope) {
                if (!applyAuthorStyles)
                    continue;
            } else {
                if (!m_scopeResolver->matchesStyleBounds(frame))
                    continue;
            }

            MatchRequest matchRequest(frame.m_ruleSet, includeEmptyRules, frame.m_scope);
            StyleResolver::RuleRange ruleRange = m_result.ranges.authorRuleRange();
            collectMatchingRules(matchRequest, ruleRange);
            collectMatchingRulesForRegion(matchRequest, ruleRange);
            sortAndTransferMatchedRules();
        }
    }

    matchHostRules(includeEmptyRules);
#else
    UNUSED_PARAM(includeEmptyRules);
#endif
}

void ElementRuleCollector::matchHostRules(bool includeEmptyRules)
{
#if ENABLE(SHADOW_DOM)
    ASSERT(m_scopeResolver);

    clearMatchedRules();
    m_result.ranges.lastAuthorRule = m_result.matchedProperties.size() - 1;

    Vector<RuleSet*> matchedRules;
    m_scopeResolver->matchHostRules(m_state.element(), matchedRules);
    if (matchedRules.isEmpty())
        return;

    for (unsigned i = matchedRules.size(); i > 0; --i) {
        StyleResolver::RuleRange ruleRange = m_result.ranges.authorRuleRange();
        collectMatchingRules(MatchRequest(matchedRules.at(i-1), includeEmptyRules, m_state.element()), ruleRange);
    }
    sortAndTransferMatchedRules();
#else
    UNUSED_PARAM(includeEmptyRules);
#endif
}

void ElementRuleCollector::matchAuthorRules(bool includeEmptyRules)
{
    clearMatchedRules();
    m_result.ranges.lastAuthorRule = m_result.matchedProperties.size() - 1;

    if (!m_state.element())
        return;

    // Match global author rules.
    MatchRequest matchRequest(m_ruleSets.authorStyle(), includeEmptyRules);
    StyleResolver::RuleRange ruleRange = m_result.ranges.authorRuleRange();
    collectMatchingRules(matchRequest, ruleRange);
    collectMatchingRulesForRegion(matchRequest, ruleRange);

    sortAndTransferMatchedRules();

    matchScopedAuthorRules(includeEmptyRules);
}

void ElementRuleCollector::matchUserRules(bool includeEmptyRules)
{
    if (!m_ruleSets.userStyle())
        return;
    
    clearMatchedRules();

    m_result.ranges.lastUserRule = m_result.matchedProperties.size() - 1;
    MatchRequest matchRequest(m_ruleSets.userStyle(), includeEmptyRules);
    StyleResolver::RuleRange ruleRange = m_result.ranges.userRuleRange();
    collectMatchingRules(matchRequest, ruleRange);
    collectMatchingRulesForRegion(matchRequest, ruleRange);

    sortAndTransferMatchedRules();
}

void ElementRuleCollector::matchUARules()
{
    MatchingUARulesScope scope;

    // First we match rules from the user agent sheet.
    if (CSSDefaultStyleSheets::simpleDefaultStyleSheet)
        m_result.isCacheable = false;
    RuleSet* userAgentStyleSheet = m_isPrintStyle
        ? CSSDefaultStyleSheets::defaultPrintStyle : CSSDefaultStyleSheets::defaultStyle;
    matchUARules(userAgentStyleSheet);

    // In quirks mode, we match rules from the quirks user agent sheet.
    if (document()->inQuirksMode())
        matchUARules(CSSDefaultStyleSheets::defaultQuirksStyle);

    // If document uses view source styles (in view source mode or in xml viewer mode), then we match rules from the view source style sheet.
    if (document()->isViewSource())
        matchUARules(CSSDefaultStyleSheets::viewSourceStyle());
}

void ElementRuleCollector::matchUARules(RuleSet* rules)
{
    clearMatchedRules();
    
    m_result.ranges.lastUARule = m_result.matchedProperties.size() - 1;
    StyleResolver::RuleRange ruleRange = m_result.ranges.UARuleRange();
    collectMatchingRules(MatchRequest(rules), ruleRange);

    sortAndTransferMatchedRules();
}

inline bool ElementRuleCollector::ruleMatches(const RuleData& ruleData, const ContainerNode* scope, PseudoId& dynamicPseudo)
{
    const StyleResolver::State& state = m_state;

    if (ruleData.hasFastCheckableSelector()) {
        // We know this selector does not include any pseudo elements.
        if (m_pseudoStyleRequest.pseudoId != NOPSEUDO)
            return false;
        // We know a sufficiently simple single part selector matches simply because we found it from the rule hash.
        // This is limited to HTML only so we don't need to check the namespace.
        if (ruleData.hasRightmostSelectorMatchingHTMLBasedOnRuleHash() && state.element()->isHTMLElement()) {
            if (!ruleData.hasMultipartSelector())
                return true;
        }
        if (ruleData.selector()->m_match == CSSSelector::Tag && !SelectorChecker::tagMatches(state.element(), ruleData.selector()->tagQName()))
            return false;
        SelectorCheckerFastPath selectorCheckerFastPath(ruleData.selector(), state.element());
        if (!selectorCheckerFastPath.matchesRightmostAttributeSelector())
            return false;

        return selectorCheckerFastPath.matches();
    }

    // Slow path.
    SelectorChecker selectorChecker(document(), m_mode);
    SelectorChecker::SelectorCheckingContext context(ruleData.selector(), state.element(), SelectorChecker::VisitedMatchEnabled);
    context.elementStyle = state.style();
    context.scope = scope;
    context.pseudoId = m_pseudoStyleRequest.pseudoId;
    context.scrollbar = m_pseudoStyleRequest.scrollbar;
    context.scrollbarPart = m_pseudoStyleRequest.scrollbarPart;
    context.behaviorAtBoundary = m_behaviorAtBoundary;
    SelectorChecker::Match match = selectorChecker.match(context, dynamicPseudo);
    if (match != SelectorChecker::SelectorMatches)
        return false;
    if (m_pseudoStyleRequest.pseudoId != NOPSEUDO && m_pseudoStyleRequest.pseudoId != dynamicPseudo)
        return false;
    return true;
}

void ElementRuleCollector::collectMatchingRulesForList(const Vector<RuleData>* rules, const MatchRequest& matchRequest, StyleResolver::RuleRange& ruleRange)
{
    if (UNLIKELY(InspectorInstrumentation::hasFrontends())) {
        doCollectMatchingRulesForList<true>(rules, matchRequest, ruleRange);
        return;
    }
    doCollectMatchingRulesForList<false>(rules, matchRequest, ruleRange);
}

template<bool hasInspectorFrontends>
void ElementRuleCollector::doCollectMatchingRulesForList(const Vector<RuleData>* rules, const MatchRequest& matchRequest, StyleResolver::RuleRange& ruleRange)
{
    if (!rules)
        return;

    const StyleResolver::State& state = m_state;

    unsigned size = rules->size();
    for (unsigned i = 0; i < size; ++i) {
        const RuleData& ruleData = rules->at(i);
        if (m_canUseFastReject && m_selectorFilter.fastRejectSelector<RuleData::maximumIdentifierCount>(ruleData.descendantSelectorIdentifierHashes()))
            continue;

        StyleRule* rule = ruleData.rule();
        InspectorInstrumentationCookie cookie;
        if (hasInspectorFrontends)
            cookie = InspectorInstrumentation::willMatchRule(document(), rule, m_inspectorCSSOMWrappers, document()->styleSheetCollection());
        PseudoId dynamicPseudo = NOPSEUDO;
        if (ruleMatches(ruleData, matchRequest.scope, dynamicPseudo)) {
            // If the rule has no properties to apply, then ignore it in the non-debug mode.
            const StylePropertySet* properties = rule->properties();
            if (!properties || (properties->isEmpty() && !matchRequest.includeEmptyRules)) {
                if (hasInspectorFrontends)
                    InspectorInstrumentation::didMatchRule(cookie, false);
                continue;
            }
            // FIXME: Exposing the non-standard getMatchedCSSRules API to web is the only reason this is needed.
            if (m_sameOriginOnly && !ruleData.hasDocumentSecurityOrigin()) {
                if (hasInspectorFrontends)
                    InspectorInstrumentation::didMatchRule(cookie, false);
                continue;
            }
            // If we're matching normal rules, set a pseudo bit if
            // we really just matched a pseudo-element.
            if (dynamicPseudo != NOPSEUDO && m_pseudoStyleRequest.pseudoId == NOPSEUDO) {
                if (m_mode == SelectorChecker::CollectingRules) {
                    if (hasInspectorFrontends)
                        InspectorInstrumentation::didMatchRule(cookie, false);
                    continue;
                }
                if (dynamicPseudo < FIRST_INTERNAL_PSEUDOID)
                    state.style()->setHasPseudoStyle(dynamicPseudo);
            } else {
                // Update our first/last rule indices in the matched rules array.
                ++ruleRange.lastRuleIndex;
                if (ruleRange.firstRuleIndex == -1)
                    ruleRange.firstRuleIndex = ruleRange.lastRuleIndex;

                // Add this rule to our list of matched rules.
                addMatchedRule(&ruleData);
                if (hasInspectorFrontends)
                    InspectorInstrumentation::didMatchRule(cookie, true);
                continue;
            }
        }
        if (hasInspectorFrontends)
            InspectorInstrumentation::didMatchRule(cookie, false);
    }
}

static inline bool compareRules(const RuleData* r1, const RuleData* r2)
{
    unsigned specificity1 = r1->specificity();
    unsigned specificity2 = r2->specificity();
    return (specificity1 == specificity2) ? r1->position() < r2->position() : specificity1 < specificity2;
}

void ElementRuleCollector::sortMatchedRules()
{
    ASSERT(m_matchedRules);
    std::sort(m_matchedRules->begin(), m_matchedRules->end(), compareRules);
}

void ElementRuleCollector::matchAllRules(bool matchAuthorAndUserStyles, bool includeSMILProperties)
{
    matchUARules();

    // Now we check user sheet rules.
    if (matchAuthorAndUserStyles)
        matchUserRules(false);

    // Now check author rules, beginning first with presentational attributes mapped from HTML.
    if (m_state.styledElement()) {
        addElementStyleProperties(m_state.styledElement()->presentationAttributeStyle());

        // Now we check additional mapped declarations.
        // Tables and table cells share an additional mapped rule that must be applied
        // after all attributes, since their mapped style depends on the values of multiple attributes.
        addElementStyleProperties(m_state.styledElement()->additionalPresentationAttributeStyle());

        if (m_state.styledElement()->isHTMLElement()) {
            bool isAuto;
            TextDirection textDirection = toHTMLElement(m_state.styledElement())->directionalityIfhasDirAutoAttribute(isAuto);
            if (isAuto)
                m_result.addMatchedProperties(textDirection == LTR ? leftToRightDeclaration() : rightToLeftDeclaration());
        }
    }
    
    // Check the rules in author sheets next.
    if (matchAuthorAndUserStyles)
        matchAuthorRules(false);

    // Now check our inline style attribute.
    if (matchAuthorAndUserStyles && m_state.styledElement() && m_state.styledElement()->inlineStyle()) {
        // Inline style is immutable as long as there is no CSSOM wrapper.
        // FIXME: Media control shadow trees seem to have problems with caching.
        bool isInlineStyleCacheable = !m_state.styledElement()->inlineStyle()->isMutable() && !m_state.styledElement()->isInShadowTree();
        // FIXME: Constify.
        addElementStyleProperties(m_state.styledElement()->inlineStyle(), isInlineStyleCacheable);
    }

#if ENABLE(SVG)
    // Now check SMIL animation override style.
    if (includeSMILProperties && matchAuthorAndUserStyles && m_state.styledElement() && m_state.styledElement()->isSVGElement())
        addElementStyleProperties(toSVGElement(m_state.styledElement())->animatedSMILStyleProperties(), false /* isCacheable */);
#else
    UNUSED_PARAM(includeSMILProperties);
#endif
}

bool ElementRuleCollector::hasAnyMatchingRules(RuleSet* ruleSet)
{
    clearMatchedRules();

    m_mode = SelectorChecker::SharingRules;
    int firstRuleIndex = -1, lastRuleIndex = -1;
    StyleResolver::RuleRange ruleRange(firstRuleIndex, lastRuleIndex);
    collectMatchingRules(MatchRequest(ruleSet), ruleRange);

    return m_matchedRules && !m_matchedRules->isEmpty();
}

} // namespace WebCore
