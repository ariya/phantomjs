/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "StyleScopeResolver.h"

#if ENABLE(STYLE_SCOPED) || ENABLE(SHADOW_DOM)

#include "CSSStyleRule.h"
#include "CSSStyleSheet.h"
#include "ContentDistributor.h"
#include "ContextFeatures.h"
#include "ElementShadow.h"
#include "HTMLNames.h"
#include "HTMLStyleElement.h"
#include "RuleFeature.h"
#include "RuleSet.h"
#include "ShadowRoot.h"

namespace WebCore {

StyleScopeResolver::StyleScopeResolver()
    : m_stackParent(0)
    , m_stackParentBoundsIndex(0)
{
}

StyleScopeResolver::~StyleScopeResolver()
{
}

const ContainerNode* StyleScopeResolver::scopeFor(const CSSStyleSheet* sheet)
{
    ASSERT(sheet);

    Document* document = sheet->ownerDocument();
    if (!document)
        return 0;
    Node* ownerNode = sheet->ownerNode();
    if (!ownerNode || !ownerNode->isHTMLElement() || !isHTMLStyleElement(ownerNode))
        return 0;

    HTMLStyleElement* styleElement = toHTMLStyleElement(ownerNode);
    if (!styleElement->scoped())
        return styleElement->isInShadowTree() ? styleElement->containingShadowRoot() : 0;

    ContainerNode* parent = styleElement->parentNode();
    if (!parent)
        return 0;

    return (parent->isElementNode() || parent->isShadowRoot()) ? parent : 0;
}

inline RuleSet* StyleScopeResolver::ruleSetFor(const ContainerNode* scope) const
{
    if (!scope->hasScopedHTMLStyleChild())
        return 0;
    ScopedRuleSetMap::const_iterator it = m_authorStyles.find(scope);
    return it != m_authorStyles.end() ? it->value.get() : 0; 
}

RuleSet* StyleScopeResolver::ensureRuleSetFor(const ContainerNode* scope)
{
    ScopedRuleSetMap::AddResult addResult = m_authorStyles.add(scope, nullptr);
    if (addResult.isNewEntry)
        addResult.iterator->value = RuleSet::create();
    return addResult.iterator->value.get();
}

void StyleScopeResolver::setupStack(const ContainerNode* parent)
{
    // The scoping element stack shouldn't be used if <style scoped> isn't used anywhere.
    ASSERT(!m_authorStyles.isEmpty());

    m_stack.shrink(0);
    int authorStyleBoundsIndex = 0;
    for (const ContainerNode* scope = parent; scope; scope = scope->parentOrShadowHostNode()) {
        RuleSet* ruleSet = ruleSetFor(scope);
        if (ruleSet)
            m_stack.append(StackFrame(scope, authorStyleBoundsIndex, ruleSet));
        if (scope->isShadowRoot() && !toShadowRoot(scope)->applyAuthorStyles())
            --authorStyleBoundsIndex;
    }

    m_stack.reverse();
    m_stackParent = parent;
    m_stackParentBoundsIndex = 0;
}

void StyleScopeResolver::push(const ContainerNode* scope, const ContainerNode* scopeParent)
{
    // Shortcut: Don't bother with the scoping element stack if <style scoped> isn't used anywhere.
    if (m_authorStyles.isEmpty()) {
        ASSERT(!m_stackParent);
        ASSERT(m_stack.isEmpty());
        return;
    }

    // In some wacky cases during style resolve we may get invoked for random elements.
    // Recreate the whole scoping element stack in such cases.
    if (!stackIsConsistent(scopeParent)) {
        setupStack(scope);
        return;
    }

    if (scope->isShadowRoot() && !toShadowRoot(scope)->applyAuthorStyles())
        ++m_stackParentBoundsIndex;
    // Otherwise just push the parent onto the stack.
    RuleSet* ruleSet = ruleSetFor(scope);
    if (ruleSet)
        m_stack.append(StackFrame(scope, m_stackParentBoundsIndex, ruleSet));
    m_stackParent = scope;
}

void StyleScopeResolver::pop(const ContainerNode* scope)
{
    // Only bother to update the scoping element stack if it is consistent.
    if (stackIsConsistent(scope)) {
        if (!m_stack.isEmpty() && m_stack.last().m_scope == scope)
            m_stack.removeLast();
        if (scope->isShadowRoot() && !toShadowRoot(scope)->applyAuthorStyles())
            --m_stackParentBoundsIndex;
        m_stackParent = scope->parentOrShadowHostNode();
    }
}

void StyleScopeResolver::collectFeaturesTo(RuleFeatureSet& features)
{
    for (ScopedRuleSetMap::iterator it = m_authorStyles.begin(); it != m_authorStyles.end(); ++it)
        features.add(it->value->features());
#if ENABLE(SHADOW_DOM)
    for (ScopedRuleSetMap::iterator it = m_atHostRules.begin(); it != m_atHostRules.end(); ++it)
        features.add(it->value->features());
#endif
}

inline RuleSet* StyleScopeResolver::ensureAtHostRuleSetFor(const ShadowRoot* shadowRoot)
{
    ScopedRuleSetMap::AddResult addResult = m_atHostRules.add(shadowRoot, nullptr);
    if (addResult.isNewEntry)
        addResult.iterator->value = RuleSet::create();
    return addResult.iterator->value.get();
}

inline RuleSet* StyleScopeResolver::atHostRuleSetFor(const ShadowRoot* shadowRoot) const
{
    ScopedRuleSetMap::const_iterator it = m_atHostRules.find(shadowRoot);
    return it != m_atHostRules.end() ? it->value.get() : 0;
}

#if ENABLE(SHADOW_DOM)
void StyleScopeResolver::addHostRule(StyleRuleHost* hostRule, bool hasDocumentSecurityOrigin, const ContainerNode* scope)
{
    if (!scope || !scope->isInShadowTree())
        return;

    ShadowRoot* shadowRoot = scope->containingShadowRoot();
    if (!shadowRoot || !shadowRoot->host())
        return;

    RuleSet* rule = ensureAtHostRuleSetFor(shadowRoot);

    const Vector<RefPtr<StyleRuleBase> >& childRules = hostRule->childRules();
    AddRuleFlags addRuleFlags = hasDocumentSecurityOrigin ? RuleHasDocumentSecurityOrigin : RuleHasNoSpecialState;
    addRuleFlags = static_cast<AddRuleFlags>(addRuleFlags | RuleCanUseFastCheckSelector);
    for (unsigned i = 0; i < childRules.size(); ++i) {
        StyleRuleBase* hostStylingRule = childRules[i].get();
        if (hostStylingRule->isStyleRule())
            rule->addStyleRule(static_cast<StyleRule*>(hostStylingRule), addRuleFlags);
    }
}
#endif

bool StyleScopeResolver::styleSharingCandidateMatchesHostRules(const Element* element)
{
    if (m_atHostRules.isEmpty())
        return false;

    ElementShadow* shadow = element->shadow();
    if (!shadow)
        return false;

    // FIXME(99827): https://bugs.webkit.org/show_bug.cgi?id=99827
    // add a new flag to ElementShadow and cache whether any@host @-rules are
    // applied to the element or not. So we can avoid always traversing
    // shadow roots.
    if (ShadowRoot* shadowRoot = shadow->shadowRoot()) {
        if (atHostRuleSetFor(shadowRoot))
            return true;
    }
    return false;
}

void StyleScopeResolver::matchHostRules(const Element* element, Vector<RuleSet*>& matchedRules)
{
    if (m_atHostRules.isEmpty())
        return;

    ElementShadow* shadow = element->shadow();
    if (!shadow)
        return;

    // FIXME(99827): https://bugs.webkit.org/show_bug.cgi?id=99827
    // add a new flag to ElementShadow and cache whether any @host @-rules are
    // applied to the element or not. So we can quickly exit this method
    // by using the flag.
    if (ShadowRoot* shadowRoot = shadow->shadowRoot()) {
        if (RuleSet* ruleSet = atHostRuleSetFor(shadowRoot))
            matchedRules.append(ruleSet);
    }
}

}

#endif // ENABLE(STYLE_SCOPED)
