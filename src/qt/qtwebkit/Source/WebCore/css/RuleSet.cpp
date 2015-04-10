/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
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
#include "RuleSet.h"

#include "CSSFontSelector.h"
#include "CSSSelector.h"
#include "CSSSelectorList.h"
#include "HTMLNames.h"
#include "MediaQueryEvaluator.h"
#include "SecurityOrigin.h"
#include "SelectorChecker.h"
#include "SelectorCheckerFastPath.h"
#include "SelectorFilter.h"
#include "StyleResolver.h"
#include "StyleRule.h"
#include "StyleRuleImport.h"
#include "StyleSheetContents.h"
#include "WebKitCSSKeyframesRule.h"

#if ENABLE(VIDEO_TRACK)
#include "TextTrackCue.h"
#endif

namespace WebCore {

using namespace HTMLNames;

// -----------------------------------------------------------------

static inline bool isSelectorMatchingHTMLBasedOnRuleHash(const CSSSelector* selector)
{
    ASSERT(selector);
    if (selector->m_match == CSSSelector::Tag) {
        const AtomicString& selectorNamespace = selector->tagQName().namespaceURI();
        if (selectorNamespace != starAtom && selectorNamespace != xhtmlNamespaceURI)
            return false;
        if (selector->relation() == CSSSelector::SubSelector)
            return isSelectorMatchingHTMLBasedOnRuleHash(selector->tagHistory());
        return true;
    }
    if (SelectorChecker::isCommonPseudoClassSelector(selector))
        return true;
    return selector->m_match == CSSSelector::Id || selector->m_match == CSSSelector::Class;
}

static inline bool selectorListContainsUncommonAttributeSelector(const CSSSelector* selector)
{
    const CSSSelectorList* selectorList = selector->selectorList();
    if (!selectorList)
        return false;
    for (const CSSSelector* selector = selectorList->first(); selector; selector = CSSSelectorList::next(selector)) {
        for (const CSSSelector* component = selector; component; component = component->tagHistory()) {
            if (component->isAttributeSelector())
                return true;
        }
    }
    return false;
}

static inline bool isCommonAttributeSelectorAttribute(const QualifiedName& attribute)
{
    // These are explicitly tested for equality in canShareStyleWithElement.
    return attribute == typeAttr || attribute == readonlyAttr;
}

static inline bool containsUncommonAttributeSelector(const CSSSelector* selector)
{
    for (; selector; selector = selector->tagHistory()) {
        // Allow certain common attributes (used in the default style) in the selectors that match the current element.
        if (selector->isAttributeSelector() && !isCommonAttributeSelectorAttribute(selector->attribute()))
            return true;
        if (selectorListContainsUncommonAttributeSelector(selector))
            return true;
        if (selector->relation() != CSSSelector::SubSelector) {
            selector = selector->tagHistory();
            break;
        }
    }

    for (; selector; selector = selector->tagHistory()) {
        if (selector->isAttributeSelector())
            return true;
        if (selectorListContainsUncommonAttributeSelector(selector))
            return true;
    }
    return false;
}

static inline PropertyWhitelistType determinePropertyWhitelistType(const AddRuleFlags addRuleFlags, const CSSSelector* selector)
{
    if (addRuleFlags & RuleIsInRegionRule)
        return PropertyWhitelistRegion;
#if ENABLE(VIDEO_TRACK)
    for (const CSSSelector* component = selector; component; component = component->tagHistory()) {
        if (component->pseudoType() == CSSSelector::PseudoCue || (component->m_match == CSSSelector::PseudoElement && component->value() == TextTrackCue::cueShadowPseudoId()))
            return PropertyWhitelistCue;
    }
#else
    UNUSED_PARAM(selector);
#endif
    return PropertyWhitelistNone;
}

RuleData::RuleData(StyleRule* rule, unsigned selectorIndex, unsigned position, AddRuleFlags addRuleFlags)
    : m_rule(rule)
    , m_selectorIndex(selectorIndex)
    , m_position(position)
    , m_hasFastCheckableSelector((addRuleFlags & RuleCanUseFastCheckSelector) && SelectorCheckerFastPath::canUse(selector()))
    , m_specificity(selector()->specificity())
    , m_hasMultipartSelector(!!selector()->tagHistory())
    , m_hasRightmostSelectorMatchingHTMLBasedOnRuleHash(isSelectorMatchingHTMLBasedOnRuleHash(selector()))
    , m_containsUncommonAttributeSelector(WebCore::containsUncommonAttributeSelector(selector()))
    , m_linkMatchType(SelectorChecker::determineLinkMatchType(selector()))
    , m_hasDocumentSecurityOrigin(addRuleFlags & RuleHasDocumentSecurityOrigin)
    , m_propertyWhitelistType(determinePropertyWhitelistType(addRuleFlags, selector()))
{
    ASSERT(m_position == position);
    ASSERT(m_selectorIndex == selectorIndex);
    SelectorFilter::collectIdentifierHashes(selector(), m_descendantSelectorIdentifierHashes, maximumIdentifierCount);
}

static void collectFeaturesFromRuleData(RuleFeatureSet& features, const RuleData& ruleData)
{
    bool foundSiblingSelector = false;
    for (const CSSSelector* selector = ruleData.selector(); selector; selector = selector->tagHistory()) {
        features.collectFeaturesFromSelector(selector);
        
        if (const CSSSelectorList* selectorList = selector->selectorList()) {
            for (const CSSSelector* subSelector = selectorList->first(); subSelector; subSelector = CSSSelectorList::next(subSelector)) {
                if (!foundSiblingSelector && selector->isSiblingSelector())
                    foundSiblingSelector = true;
                features.collectFeaturesFromSelector(subSelector);
            }
        } else if (!foundSiblingSelector && selector->isSiblingSelector())
            foundSiblingSelector = true;
    }
    if (foundSiblingSelector)
        features.siblingRules.append(RuleFeature(ruleData.rule(), ruleData.selectorIndex(), ruleData.hasDocumentSecurityOrigin()));
    if (ruleData.containsUncommonAttributeSelector())
        features.uncommonAttributeRules.append(RuleFeature(ruleData.rule(), ruleData.selectorIndex(), ruleData.hasDocumentSecurityOrigin()));
}
    
void RuleSet::addToRuleSet(AtomicStringImpl* key, AtomRuleMap& map, const RuleData& ruleData)
{
    if (!key)
        return;
    OwnPtr<Vector<RuleData> >& rules = map.add(key, nullptr).iterator->value;
    if (!rules)
        rules = adoptPtr(new Vector<RuleData>);
    rules->append(ruleData);
}

bool RuleSet::findBestRuleSetAndAdd(const CSSSelector* component, RuleData& ruleData)
{
    if (component->m_match == CSSSelector::Id) {
        addToRuleSet(component->value().impl(), m_idRules, ruleData);
        return true;
    }
    if (component->m_match == CSSSelector::Class) {
        addToRuleSet(component->value().impl(), m_classRules, ruleData);
        return true;
    }
    if (component->isCustomPseudoElement()) {
        addToRuleSet(component->value().impl(), m_shadowPseudoElementRules, ruleData);
        return true;
    }
#if ENABLE(VIDEO_TRACK)
    if (component->pseudoType() == CSSSelector::PseudoCue) {
        m_cuePseudoRules.append(ruleData);
        return true;
    }
#endif
    if (SelectorChecker::isCommonPseudoClassSelector(component)) {
        switch (component->pseudoType()) {
        case CSSSelector::PseudoLink:
        case CSSSelector::PseudoVisited:
        case CSSSelector::PseudoAnyLink:
            m_linkPseudoClassRules.append(ruleData);
            return true;
        case CSSSelector::PseudoFocus:
            m_focusPseudoClassRules.append(ruleData);
            return true;
        default:
            ASSERT_NOT_REACHED();
            return true;
        }
    }

    if (component->m_match == CSSSelector::Tag) {
        // If this is part of a subselector chain, recurse ahead to find a narrower set (ID/class/:pseudo)
        if (component->relation() == CSSSelector::SubSelector) {
            const CSSSelector* nextComponent = component->tagHistory();
            if (nextComponent->m_match == CSSSelector::Class || nextComponent->m_match == CSSSelector::Id || SelectorChecker::isCommonPseudoClassSelector(nextComponent)) {
                if (findBestRuleSetAndAdd(nextComponent, ruleData))
                    return true;
            }
        }
        if (component->tagQName().localName() != starAtom) {
            addToRuleSet(component->tagQName().localName().impl(), m_tagRules, ruleData);
            return true;
        }
    }
    return false;
}

void RuleSet::addRule(StyleRule* rule, unsigned selectorIndex, AddRuleFlags addRuleFlags)
{
    RuleData ruleData(rule, selectorIndex, m_ruleCount++, addRuleFlags);
    collectFeaturesFromRuleData(m_features, ruleData);

    if (!findBestRuleSetAndAdd(ruleData.selector(), ruleData)) {
        // If we didn't find a specialized map to stick it in, file under universal rules.
        m_universalRules.append(ruleData);
    }
}

void RuleSet::addPageRule(StyleRulePage* rule)
{
    m_pageRules.append(rule);
}

void RuleSet::addRegionRule(StyleRuleRegion* regionRule, bool hasDocumentSecurityOrigin)
{
    OwnPtr<RuleSet> regionRuleSet = RuleSet::create();
    // The region rule set should take into account the position inside the parent rule set.
    // Otherwise, the rules inside region block might be incorrectly positioned before other similar rules from
    // the stylesheet that contains the region block.
    regionRuleSet->m_ruleCount = m_ruleCount;

    // Collect the region rules into a rule set
    // FIXME: Should this add other types of rules? (i.e. use addChildRules() directly?)
    const Vector<RefPtr<StyleRuleBase> >& childRules = regionRule->childRules();
    AddRuleFlags addRuleFlags = hasDocumentSecurityOrigin ? RuleHasDocumentSecurityOrigin : RuleHasNoSpecialState;
    addRuleFlags = static_cast<AddRuleFlags>(addRuleFlags | RuleCanUseFastCheckSelector | RuleIsInRegionRule);
    for (unsigned i = 0; i < childRules.size(); ++i) {
        StyleRuleBase* regionStylingRule = childRules[i].get();
        if (regionStylingRule->isStyleRule())
            regionRuleSet->addStyleRule(static_cast<StyleRule*>(regionStylingRule), addRuleFlags);
    }
    // Update the "global" rule count so that proper order is maintained
    m_ruleCount = regionRuleSet->m_ruleCount;

    m_regionSelectorsAndRuleSets.append(RuleSetSelectorPair(regionRule->selectorList().first(), regionRuleSet.release()));
}

void RuleSet::addChildRules(const Vector<RefPtr<StyleRuleBase> >& rules, const MediaQueryEvaluator& medium, StyleResolver* resolver, const ContainerNode* scope, bool hasDocumentSecurityOrigin, AddRuleFlags addRuleFlags)
{
    for (unsigned i = 0; i < rules.size(); ++i) {
        StyleRuleBase* rule = rules[i].get();

        if (rule->isStyleRule()) {
            StyleRule* styleRule = static_cast<StyleRule*>(rule);
            addStyleRule(styleRule, addRuleFlags);
        } else if (rule->isPageRule())
            addPageRule(static_cast<StyleRulePage*>(rule));
        else if (rule->isMediaRule()) {
            StyleRuleMedia* mediaRule = static_cast<StyleRuleMedia*>(rule);
            if ((!mediaRule->mediaQueries() || medium.eval(mediaRule->mediaQueries(), resolver)))
                addChildRules(mediaRule->childRules(), medium, resolver, scope, hasDocumentSecurityOrigin, addRuleFlags);
        } else if (rule->isFontFaceRule() && resolver) {
            // Add this font face to our set.
            // FIXME(BUG 72461): We don't add @font-face rules of scoped style sheets for the moment.
            if (scope)
                continue;
            const StyleRuleFontFace* fontFaceRule = static_cast<StyleRuleFontFace*>(rule);
            resolver->fontSelector()->addFontFaceRule(fontFaceRule);
            resolver->invalidateMatchedPropertiesCache();
        } else if (rule->isKeyframesRule() && resolver) {
            // FIXME (BUG 72462): We don't add @keyframe rules of scoped style sheets for the moment.
            if (scope)
                continue;
            resolver->addKeyframeStyle(static_cast<StyleRuleKeyframes*>(rule));
        }
#if ENABLE(CSS_REGIONS)
        else if (rule->isRegionRule() && resolver) {
            // FIXME (BUG 72472): We don't add @-webkit-region rules of scoped style sheets for the moment.
            if (scope)
                continue;
            addRegionRule(static_cast<StyleRuleRegion*>(rule), hasDocumentSecurityOrigin);
        }
#endif
#if ENABLE(SHADOW_DOM)
        else if (rule->isHostRule())
            resolver->addHostRule(static_cast<StyleRuleHost*>(rule), hasDocumentSecurityOrigin, scope);
#endif
#if ENABLE(CSS_DEVICE_ADAPTATION)
        else if (rule->isViewportRule() && resolver) {
            // @viewport should not be scoped.
            if (scope)
                continue;
            resolver->viewportStyleResolver()->addViewportRule(static_cast<StyleRuleViewport*>(rule));
        }
#endif
#if ENABLE(CSS3_CONDITIONAL_RULES)
        else if (rule->isSupportsRule() && static_cast<StyleRuleSupports*>(rule)->conditionIsSupported())
            addChildRules(static_cast<StyleRuleSupports*>(rule)->childRules(), medium, resolver, scope, hasDocumentSecurityOrigin, addRuleFlags);
#endif
    }
}

void RuleSet::addRulesFromSheet(StyleSheetContents* sheet, const MediaQueryEvaluator& medium, StyleResolver* resolver, const ContainerNode* scope)
{
    ASSERT(sheet);

    const Vector<RefPtr<StyleRuleImport> >& importRules = sheet->importRules();
    for (unsigned i = 0; i < importRules.size(); ++i) {
        StyleRuleImport* importRule = importRules[i].get();
        if (importRule->styleSheet() && (!importRule->mediaQueries() || medium.eval(importRule->mediaQueries(), resolver)))
            addRulesFromSheet(importRule->styleSheet(), medium, resolver, scope);
    }

    bool hasDocumentSecurityOrigin = resolver && resolver->document()->securityOrigin()->canRequest(sheet->baseURL());
    AddRuleFlags addRuleFlags = static_cast<AddRuleFlags>((hasDocumentSecurityOrigin ? RuleHasDocumentSecurityOrigin : 0) | (!scope ? RuleCanUseFastCheckSelector : 0));

    addChildRules(sheet->childRules(), medium, resolver, scope, hasDocumentSecurityOrigin, addRuleFlags);

    if (m_autoShrinkToFitEnabled)
        shrinkToFit();
}

void RuleSet::addStyleRule(StyleRule* rule, AddRuleFlags addRuleFlags)
{
    for (size_t selectorIndex = 0; selectorIndex != notFound; selectorIndex = rule->selectorList().indexOfNextSelectorAfter(selectorIndex))
        addRule(rule, selectorIndex, addRuleFlags);
}

static inline void shrinkMapVectorsToFit(RuleSet::AtomRuleMap& map)
{
    RuleSet::AtomRuleMap::iterator end = map.end();
    for (RuleSet::AtomRuleMap::iterator it = map.begin(); it != end; ++it)
        it->value->shrinkToFit();
}

void RuleSet::shrinkToFit()
{
    shrinkMapVectorsToFit(m_idRules);
    shrinkMapVectorsToFit(m_classRules);
    shrinkMapVectorsToFit(m_tagRules);
    shrinkMapVectorsToFit(m_shadowPseudoElementRules);
    m_linkPseudoClassRules.shrinkToFit();
#if ENABLE(VIDEO_TRACK)
    m_cuePseudoRules.shrinkToFit();
#endif
    m_focusPseudoClassRules.shrinkToFit();
    m_universalRules.shrinkToFit();
    m_pageRules.shrinkToFit();
}

} // namespace WebCore
