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

#ifndef StyleScopeResolver_h
#define StyleScopeResolver_h

#include <wtf/Assertions.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class ContainerNode;
class CSSStyleSheet;
class Element;
class RuleSet;
class ShadowRoot;
class StyleRuleHost;
struct RuleFeatureSet;

#if ENABLE(STYLE_SCOPED) || ENABLE(SHADOW_DOM)

class StyleScopeResolver {
public:
    typedef HashMap<const ContainerNode*, OwnPtr<RuleSet> > ScopedRuleSetMap;

    struct StackFrame {
        StackFrame() : m_scope(0), m_authorStyleBoundsIndex(0), m_ruleSet(0) { }
        StackFrame(const ContainerNode* scope, int authorStyleBoundsIndex, RuleSet* ruleSet)
            : m_scope(scope), m_authorStyleBoundsIndex(authorStyleBoundsIndex), m_ruleSet(ruleSet)
        { }

        const ContainerNode* m_scope;
        int m_authorStyleBoundsIndex;
        RuleSet* m_ruleSet;
    };

    StyleScopeResolver();
    ~StyleScopeResolver();

    static const ContainerNode* scopeFor(const CSSStyleSheet*);

    void push(const ContainerNode* scope, const ContainerNode* scopeParent);
    void pop(const ContainerNode* scope);
    bool hasScopedStyles() const { return !m_authorStyles.isEmpty(); }
    RuleSet* ensureRuleSetFor(const ContainerNode* scope);
    bool ensureStackConsistency(ContainerNode*);
    unsigned stackSize() const { return m_stack.size(); }
    const StackFrame& stackFrameAt(unsigned index) const { return m_stack.at(index); }
    bool matchesStyleBounds(const StackFrame& frame) const { return frame.m_authorStyleBoundsIndex == m_stackParentBoundsIndex; }
    void collectFeaturesTo(RuleFeatureSet&);

    void addHostRule(StyleRuleHost*, bool hasDocumentSecurityOrigin, const ContainerNode* scope);
    bool styleSharingCandidateMatchesHostRules(const Element*);
    void matchHostRules(const Element*, Vector<RuleSet*>& matchedRules);

private:
    RuleSet* ruleSetFor(const ContainerNode* scope) const;
    void setupStack(const ContainerNode*);
    bool stackIsConsistent(const ContainerNode* parent) const { return parent && parent == m_stackParent; }
    RuleSet* ensureAtHostRuleSetFor(const ShadowRoot*);
    RuleSet* atHostRuleSetFor(const ShadowRoot*) const;

    ScopedRuleSetMap m_authorStyles;

    // Vector (used as stack) that keeps track of scoping elements (i.e., elements with a <style scoped> child)
    // encountered during tree iteration for style resolution.
    Vector<StackFrame> m_stack;
    // Element last seen as parent element when updating m_scopingElementStack.
    // This is used to decide whether m_scopingElementStack is consistent, separately from SelectorChecker::m_parentStack.
    const ContainerNode* m_stackParent;
    int m_stackParentBoundsIndex;

    ScopedRuleSetMap m_atHostRules;
};

inline bool StyleScopeResolver::ensureStackConsistency(ContainerNode* parent)
{
    // Match scoped author rules by traversing the scoped element stack (rebuild it if it got inconsistent).
    if (!stackIsConsistent(parent))
        setupStack(parent);
    return !m_stack.isEmpty();
}

#else

class StyleScopeResolver {
public:
    static const ContainerNode* scopeFor(const CSSStyleSheet*) { return 0; }
    void push(const ContainerNode*, const ContainerNode*) { }
    void pop(const ContainerNode*) { }
    void collectFeaturesTo(RuleFeatureSet&) { }
    RuleSet* ensureRuleSetFor(const ContainerNode*) { return 0; }
};

#endif // ENABLE(STYLE_SCOPED)

} // namespace WebCore

#endif // StyleScopeResolver_h
