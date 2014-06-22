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

#ifndef RuleSet_h
#define RuleSet_h

#include "RuleFeature.h"
#include "StyleRule.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

enum AddRuleFlags {
    RuleHasNoSpecialState         = 0,
    RuleHasDocumentSecurityOrigin = 1,
    RuleCanUseFastCheckSelector   = 1 << 1,
    RuleIsInRegionRule            = 1 << 2,
};
    
enum PropertyWhitelistType {
    PropertyWhitelistNone   = 0,
    PropertyWhitelistRegion,
#if ENABLE(VIDEO_TRACK)
    PropertyWhitelistCue
#endif
};

class CSSSelector;
class ContainerNode;
class MediaQueryEvaluator;
class StyleResolver;
class StyleRuleRegion;
class StyleSheetContents;

class RuleData {
public:
    static const unsigned maximumSelectorComponentCount = 8192;

    RuleData(StyleRule*, unsigned selectorIndex, unsigned position, AddRuleFlags);

    unsigned position() const { return m_position; }
    StyleRule* rule() const { return m_rule; }
    const CSSSelector* selector() const { return m_rule->selectorList().selectorAt(m_selectorIndex); }
    unsigned selectorIndex() const { return m_selectorIndex; }

    bool hasFastCheckableSelector() const { return m_hasFastCheckableSelector; }
    bool hasMultipartSelector() const { return m_hasMultipartSelector; }
    bool hasRightmostSelectorMatchingHTMLBasedOnRuleHash() const { return m_hasRightmostSelectorMatchingHTMLBasedOnRuleHash; }
    bool containsUncommonAttributeSelector() const { return m_containsUncommonAttributeSelector; }
    unsigned specificity() const { return m_specificity; }
    unsigned linkMatchType() const { return m_linkMatchType; }
    bool hasDocumentSecurityOrigin() const { return m_hasDocumentSecurityOrigin; }
    PropertyWhitelistType propertyWhitelistType(bool isMatchingUARules = false) const { return isMatchingUARules ? PropertyWhitelistNone : static_cast<PropertyWhitelistType>(m_propertyWhitelistType); }
    // Try to balance between memory usage (there can be lots of RuleData objects) and good filtering performance.
    static const unsigned maximumIdentifierCount = 4;
    const unsigned* descendantSelectorIdentifierHashes() const { return m_descendantSelectorIdentifierHashes; }

private:
    StyleRule* m_rule;
    unsigned m_selectorIndex : 13;
    // This number was picked fairly arbitrarily. We can probably lower it if we need to.
    // Some simple testing showed <100,000 RuleData's on large sites.
    unsigned m_position : 18;
    unsigned m_hasFastCheckableSelector : 1;
    unsigned m_specificity : 24;
    unsigned m_hasMultipartSelector : 1;
    unsigned m_hasRightmostSelectorMatchingHTMLBasedOnRuleHash : 1;
    unsigned m_containsUncommonAttributeSelector : 1;
    unsigned m_linkMatchType : 2; //  SelectorChecker::LinkMatchMask
    unsigned m_hasDocumentSecurityOrigin : 1;
    unsigned m_propertyWhitelistType : 2;
    // Use plain array instead of a Vector to minimize memory overhead.
    unsigned m_descendantSelectorIdentifierHashes[maximumIdentifierCount];
};
    
struct SameSizeAsRuleData {
    void* a;
    unsigned b;
    unsigned c;
    unsigned d[4];
};

COMPILE_ASSERT(sizeof(RuleData) == sizeof(SameSizeAsRuleData), RuleData_should_stay_small);

class RuleSet {
    WTF_MAKE_NONCOPYABLE(RuleSet); WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<RuleSet> create() { return adoptPtr(new RuleSet); }

    typedef HashMap<AtomicStringImpl*, OwnPtr<Vector<RuleData> > > AtomRuleMap;

    void addRulesFromSheet(StyleSheetContents*, const MediaQueryEvaluator&, StyleResolver* = 0, const ContainerNode* = 0);

    void addStyleRule(StyleRule*, AddRuleFlags);
    void addRule(StyleRule*, unsigned selectorIndex, AddRuleFlags);
    void addPageRule(StyleRulePage*);
    void addToRuleSet(AtomicStringImpl* key, AtomRuleMap&, const RuleData&);
    void addRegionRule(StyleRuleRegion*, bool hasDocumentSecurityOrigin);
    void shrinkToFit();
    void disableAutoShrinkToFit() { m_autoShrinkToFitEnabled = false; }

    const RuleFeatureSet& features() const { return m_features; }

    const Vector<RuleData>* idRules(AtomicStringImpl* key) const { return m_idRules.get(key); }
    const Vector<RuleData>* classRules(AtomicStringImpl* key) const { return m_classRules.get(key); }
    const Vector<RuleData>* tagRules(AtomicStringImpl* key) const { return m_tagRules.get(key); }
    const Vector<RuleData>* shadowPseudoElementRules(AtomicStringImpl* key) const { return m_shadowPseudoElementRules.get(key); }
    const Vector<RuleData>* linkPseudoClassRules() const { return &m_linkPseudoClassRules; }
#if ENABLE(VIDEO_TRACK)
    const Vector<RuleData>* cuePseudoRules() const { return &m_cuePseudoRules; }
#endif
    const Vector<RuleData>* focusPseudoClassRules() const { return &m_focusPseudoClassRules; }
    const Vector<RuleData>* universalRules() const { return &m_universalRules; }
    const Vector<StyleRulePage*>& pageRules() const { return m_pageRules; }

private:
    void addChildRules(const Vector<RefPtr<StyleRuleBase> >&, const MediaQueryEvaluator& medium, StyleResolver*, const ContainerNode* scope, bool hasDocumentSecurityOrigin, AddRuleFlags);
    bool findBestRuleSetAndAdd(const CSSSelector*, RuleData&);

public:
    RuleSet();

    AtomRuleMap m_idRules;
    AtomRuleMap m_classRules;
    AtomRuleMap m_tagRules;
    AtomRuleMap m_shadowPseudoElementRules;
    Vector<RuleData> m_linkPseudoClassRules;
#if ENABLE(VIDEO_TRACK)
    Vector<RuleData> m_cuePseudoRules;
#endif
    Vector<RuleData> m_focusPseudoClassRules;
    Vector<RuleData> m_universalRules;
    Vector<StyleRulePage*> m_pageRules;
    unsigned m_ruleCount;
    bool m_autoShrinkToFitEnabled;
    RuleFeatureSet m_features;

    struct RuleSetSelectorPair {
        RuleSetSelectorPair(const CSSSelector* selector, PassOwnPtr<RuleSet> ruleSet) : selector(selector), ruleSet(ruleSet) { }
        RuleSetSelectorPair(const RuleSetSelectorPair& rs) : selector(rs.selector), ruleSet(const_cast<RuleSetSelectorPair*>(&rs)->ruleSet.release()) { }

        const CSSSelector* selector;
        OwnPtr<RuleSet> ruleSet;
    };

    Vector<RuleSetSelectorPair> m_regionSelectorsAndRuleSets;
};

inline RuleSet::RuleSet()
    : m_ruleCount(0)
    , m_autoShrinkToFitEnabled(true)
{
}

} // namespace WebCore

#endif // RuleSet_h
