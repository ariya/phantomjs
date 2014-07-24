/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "StyleInvalidationAnalysis.h"

#include "CSSSelectorList.h"
#include "Document.h"
#include "NodeTraversal.h"
#include "StyleRuleImport.h"
#include "StyleSheetContents.h"
#include "StyledElement.h"

namespace WebCore {

StyleInvalidationAnalysis::StyleInvalidationAnalysis(const Vector<StyleSheetContents*>& sheets)
    : m_dirtiesAllStyle(false)
{
    for (unsigned i = 0; i < sheets.size() && !m_dirtiesAllStyle; ++i)
        analyzeStyleSheet(sheets[i]);
}

static bool determineSelectorScopes(const CSSSelectorList& selectorList, HashSet<AtomicStringImpl*>& idScopes, HashSet<AtomicStringImpl*>& classScopes)
{
    for (const CSSSelector* selector = selectorList.first(); selector; selector = CSSSelectorList::next(selector)) {
        const CSSSelector* scopeSelector = 0;
        // This picks the widest scope, not the narrowest, to minimize the number of found scopes.
        for (const CSSSelector* current = selector; current; current = current->tagHistory()) {
            // Prefer ids over classes.
            if (current->m_match == CSSSelector::Id)
                scopeSelector = current;
            else if (current->m_match == CSSSelector::Class && (!scopeSelector || scopeSelector->m_match != CSSSelector::Id))
                scopeSelector = current;
            CSSSelector::Relation relation = current->relation();
            if (relation != CSSSelector::Descendant && relation != CSSSelector::Child && relation != CSSSelector::SubSelector)
                break;
        }
        if (!scopeSelector)
            return false;
        ASSERT(scopeSelector->m_match == CSSSelector::Class || scopeSelector->m_match == CSSSelector::Id);
        if (scopeSelector->m_match == CSSSelector::Id)
            idScopes.add(scopeSelector->value().impl());
        else
            classScopes.add(scopeSelector->value().impl());
    }
    return true;
}

void StyleInvalidationAnalysis::analyzeStyleSheet(StyleSheetContents* styleSheetContents)
{
    ASSERT(!styleSheetContents->isLoading());

    // See if all rules on the sheet are scoped to some specific ids or classes.
    // Then test if we actually have any of those in the tree at the moment.
    const Vector<RefPtr<StyleRuleImport> >& importRules = styleSheetContents->importRules();
    for (unsigned i = 0; i < importRules.size(); ++i) {
        if (!importRules[i]->styleSheet())
            continue;
        analyzeStyleSheet(importRules[i]->styleSheet());
        if (m_dirtiesAllStyle)
            return;
    }
    const Vector<RefPtr<StyleRuleBase> >& rules = styleSheetContents->childRules();
    for (unsigned i = 0; i < rules.size(); i++) {
        StyleRuleBase* rule = rules[i].get();
        if (!rule->isStyleRule()) {
            // FIXME: Media rules and maybe some others could be allowed.
            m_dirtiesAllStyle = true;
            return;
        }
        StyleRule* styleRule = static_cast<StyleRule*>(rule);
        if (!determineSelectorScopes(styleRule->selectorList(), m_idScopes, m_classScopes)) {
            m_dirtiesAllStyle = true;
            return;
        }
    }
}

static bool elementMatchesSelectorScopes(const Element* element, const HashSet<AtomicStringImpl*>& idScopes, const HashSet<AtomicStringImpl*>& classScopes)
{
    if (!idScopes.isEmpty() && element->hasID() && idScopes.contains(element->idForStyleResolution().impl()))
        return true;
    if (classScopes.isEmpty() || !element->hasClass())
        return false;
    const SpaceSplitString& classNames = element->classNames();
    for (unsigned i = 0; i < classNames.size(); ++i) {
        if (classScopes.contains(classNames[i].impl()))
            return true;
    }
    return false;
}

void StyleInvalidationAnalysis::invalidateStyle(Document* document)
{
    ASSERT(!m_dirtiesAllStyle);
    if (m_idScopes.isEmpty() && m_classScopes.isEmpty())
        return;
    Element* element = ElementTraversal::firstWithin(document);
    while (element) {
        if (elementMatchesSelectorScopes(element, m_idScopes, m_classScopes)) {
            element->setNeedsStyleRecalc();
            // The whole subtree is now invalidated, we can skip to the next sibling.
            element = ElementTraversal::nextSkippingChildren(element);
            continue;
        }
        element = ElementTraversal::next(element);
    }
}

}
