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
#include "DocumentRuleSets.h"

#include "CSSDefaultStyleSheets.h"
#include "CSSStyleSheet.h"
#include "DocumentStyleSheetCollection.h"
#include "MediaQueryEvaluator.h"
#include "StyleResolver.h"
#include "StyleSheetContents.h"

namespace WebCore {

DocumentRuleSets::DocumentRuleSets()
{
}

DocumentRuleSets::~DocumentRuleSets()
{
}

void DocumentRuleSets::initUserStyle(DocumentStyleSheetCollection* styleSheetCollection, const MediaQueryEvaluator& medium, StyleResolver& resolver)
{
    OwnPtr<RuleSet> tempUserStyle = RuleSet::create();
    if (CSSStyleSheet* pageUserSheet = styleSheetCollection->pageUserSheet())
        tempUserStyle->addRulesFromSheet(pageUserSheet->contents(), medium, &resolver);
    collectRulesFromUserStyleSheets(styleSheetCollection->injectedUserStyleSheets(), *tempUserStyle, medium, resolver);
    collectRulesFromUserStyleSheets(styleSheetCollection->documentUserStyleSheets(), *tempUserStyle, medium, resolver);
    if (tempUserStyle->m_ruleCount > 0 || tempUserStyle->m_pageRules.size() > 0)
        m_userStyle = tempUserStyle.release();
}

void DocumentRuleSets::collectRulesFromUserStyleSheets(const Vector<RefPtr<CSSStyleSheet> >& userSheets, RuleSet& userStyle, const MediaQueryEvaluator& medium, StyleResolver& resolver)
{
    for (unsigned i = 0; i < userSheets.size(); ++i) {
        ASSERT(userSheets[i]->contents()->isUserStyleSheet());
        userStyle.addRulesFromSheet(userSheets[i]->contents(), medium, &resolver);
    }
}

static PassOwnPtr<RuleSet> makeRuleSet(const Vector<RuleFeature>& rules)
{
    size_t size = rules.size();
    if (!size)
        return nullptr;
    OwnPtr<RuleSet> ruleSet = RuleSet::create();
    for (size_t i = 0; i < size; ++i)
        ruleSet->addRule(rules[i].rule, rules[i].selectorIndex, rules[i].hasDocumentSecurityOrigin ? RuleHasDocumentSecurityOrigin : RuleHasNoSpecialState);
    ruleSet->shrinkToFit();
    return ruleSet.release();
}

void DocumentRuleSets::resetAuthorStyle()
{
    m_authorStyle = RuleSet::create();
    m_authorStyle->disableAutoShrinkToFit();
}

void DocumentRuleSets::appendAuthorStyleSheets(unsigned firstNew, const Vector<RefPtr<CSSStyleSheet> >& styleSheets, MediaQueryEvaluator* medium, InspectorCSSOMWrappers& inspectorCSSOMWrappers, bool isViewSource, StyleResolver* resolver)
{
    // This handles sheets added to the end of the stylesheet list only. In other cases the style resolver
    // needs to be reconstructed. To handle insertions too the rule order numbers would need to be updated.
    unsigned size = styleSheets.size();
    for (unsigned i = firstNew; i < size; ++i) {
        CSSStyleSheet* cssSheet = styleSheets[i].get();
        ASSERT(!cssSheet->disabled());
        if (cssSheet->mediaQueries() && !medium->eval(cssSheet->mediaQueries(), resolver))
            continue;
        StyleSheetContents* sheet = cssSheet->contents();
#if ENABLE(STYLE_SCOPED) || ENABLE(SHADOW_DOM)
        if (const ContainerNode* scope = StyleScopeResolver::scopeFor(cssSheet)) {
            // FIXME: Remove a dependency to calling a StyleResolver's member function.
            // If we can avoid calling resolver->ensureScopeResolver() here, we don't have to include "StyleResolver.h".
            // https://bugs.webkit.org/show_bug.cgi?id=108890
            resolver->ensureScopeResolver()->ensureRuleSetFor(scope)->addRulesFromSheet(sheet, *medium, resolver, scope);
            continue;
        }
#endif
        m_authorStyle->addRulesFromSheet(sheet, *medium, resolver);
        inspectorCSSOMWrappers.collectFromStyleSheetIfNeeded(cssSheet);
    }
    m_authorStyle->shrinkToFit();
    collectFeatures(isViewSource, resolver->scopeResolver());
}

void DocumentRuleSets::collectFeatures(bool isViewSource, StyleScopeResolver* scopeResolver)
{
    m_features.clear();
    // Collect all ids and rules using sibling selectors (:first-child and similar)
    // in the current set of stylesheets. Style sharing code uses this information to reject
    // sharing candidates.
    if (CSSDefaultStyleSheets::defaultStyle)
        m_features.add(CSSDefaultStyleSheets::defaultStyle->features());
    if (m_authorStyle)
        m_features.add(m_authorStyle->features());
    if (isViewSource)
        m_features.add(CSSDefaultStyleSheets::viewSourceStyle()->features());

    if (scopeResolver)
        scopeResolver->collectFeaturesTo(m_features);
    if (m_userStyle)
        m_features.add(m_userStyle->features());

    m_siblingRuleSet = makeRuleSet(m_features.siblingRules);
    m_uncommonAttributeRuleSet = makeRuleSet(m_features.uncommonAttributeRules);
}

} // namespace WebCore
