/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef ElementRuleCollector_h
#define ElementRuleCollector_h

#include "MediaQueryEvaluator.h"
#include "SelectorChecker.h"
#include "StyleResolver.h"
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class DocumentRuleSets;
class RenderRegion;
class RuleData;
class RuleSet;
class SelectorFilter;
class StyleScopeResolver;

class ElementRuleCollector {
public:
    ElementRuleCollector(StyleResolver* styleResolver, const StyleResolver::State& state)
        : m_state(state)
        , m_ruleSets(styleResolver->ruleSets())
        , m_selectorFilter(styleResolver->selectorFilter())
        , m_inspectorCSSOMWrappers(styleResolver->inspectorCSSOMWrappers())
        , m_scopeResolver(styleResolver->scopeResolver())
        , m_isPrintStyle(false)
        , m_regionForStyling(0)
        , m_pseudoStyleRequest(NOPSEUDO)
        , m_sameOriginOnly(false)
        , m_mode(SelectorChecker::ResolvingStyle)
        , m_canUseFastReject(m_selectorFilter.parentStackIsConsistent(state.parentNode()))
        , m_behaviorAtBoundary(SelectorChecker::DoesNotCrossBoundary) { }

    void matchAllRules(bool matchAuthorAndUserStyles, bool includeSMILProperties);
    void matchUARules();
    void matchAuthorRules(bool includeEmptyRules);
    void matchUserRules(bool includeEmptyRules);

    void setMode(SelectorChecker::Mode mode) { m_mode = mode; }
    void setPseudoStyleRequest(const PseudoStyleRequest& request) { m_pseudoStyleRequest = request; }
    void setSameOriginOnly(bool f) { m_sameOriginOnly = f; } 
    void setRegionForStyling(RenderRegion* regionForStyling) { m_regionForStyling = regionForStyling; }
    void setMedium(const MediaQueryEvaluator* medium) { m_isPrintStyle = medium->mediaTypeMatchSpecific("print"); }

    bool hasAnyMatchingRules(RuleSet*);

    StyleResolver::MatchResult& matchedResult();
    const Vector<RefPtr<StyleRuleBase> >& matchedRuleList() const;

private:
    Document* document() { return m_state.document(); }
    void addElementStyleProperties(const StylePropertySet*, bool isCacheable = true);

    void matchUARules(RuleSet*);
    void matchScopedAuthorRules(bool includeEmptyRules);
    void matchHostRules(bool includeEmptyRules);

    void collectMatchingRules(const MatchRequest&, StyleResolver::RuleRange&);
    void collectMatchingRulesForRegion(const MatchRequest&, StyleResolver::RuleRange&);
    void collectMatchingRulesForList(const Vector<RuleData>*, const MatchRequest&, StyleResolver::RuleRange&);
    bool ruleMatches(const RuleData&, const ContainerNode* scope, PseudoId&);

    void sortMatchedRules();
    void sortAndTransferMatchedRules();

    void addMatchedRule(const RuleData*);
    void clearMatchedRules();
        
private:
    template<bool hasInspectorFrontends>
    void doCollectMatchingRulesForList(const Vector<RuleData>*, const MatchRequest&, StyleResolver::RuleRange&);

    const StyleResolver::State& m_state;
    DocumentRuleSets& m_ruleSets;
    SelectorFilter& m_selectorFilter;
    InspectorCSSOMWrappers& m_inspectorCSSOMWrappers;
    StyleScopeResolver* m_scopeResolver;

    bool m_isPrintStyle;
    RenderRegion* m_regionForStyling;
    PseudoStyleRequest m_pseudoStyleRequest;
    bool m_sameOriginOnly;
    SelectorChecker::Mode m_mode;
    bool m_canUseFastReject;
    SelectorChecker::BehaviorAtBoundary m_behaviorAtBoundary;

    OwnPtr<Vector<const RuleData*, 32> > m_matchedRules;

    // Output.
    Vector<RefPtr<StyleRuleBase> > m_matchedRuleList;
    StyleResolver::MatchResult m_result;
};

} // namespace WebCore

#endif // ElementRuleCollector_h
