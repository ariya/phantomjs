/*
 * Copyright (C) 2007, 2008, 2012, 2013 Apple Inc. All rights reserved.
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
#include "WebKitCSSKeyframesRule.h"

#include "CSSParser.h"
#include "CSSRuleList.h"
#include "CSSStyleSheet.h"
#include "StylePropertySet.h"
#include "StyleSheet.h"
#include "WebKitCSSKeyframeRule.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

StyleRuleKeyframes::StyleRuleKeyframes()
    : StyleRuleBase(Keyframes, 0)
{
}

StyleRuleKeyframes::StyleRuleKeyframes(const StyleRuleKeyframes& o)
    : StyleRuleBase(o)
    , m_keyframes(o.m_keyframes)
    , m_name(o.m_name)
{
}

StyleRuleKeyframes::~StyleRuleKeyframes()
{
}

void StyleRuleKeyframes::parserAppendKeyframe(PassRefPtr<StyleKeyframe> keyframe)
{
    if (!keyframe)
        return;
    m_keyframes.append(keyframe);
}

void StyleRuleKeyframes::wrapperAppendKeyframe(PassRefPtr<StyleKeyframe> keyframe)
{
    m_keyframes.append(keyframe);
}

void StyleRuleKeyframes::wrapperRemoveKeyframe(unsigned index)
{
    m_keyframes.remove(index);
}

int StyleRuleKeyframes::findKeyframeIndex(const String& key) const
{
    String percentageString;
    if (equalIgnoringCase(key, "from"))
        percentageString = ASCIILiteral("0%");
    else if (equalIgnoringCase(key, "to"))
        percentageString = ASCIILiteral("100%");
    else
        percentageString = key;
    
    for (unsigned i = 0; i < m_keyframes.size(); ++i) {
        if (m_keyframes[i]->keyText() == percentageString)
            return i;
    }
    return -1;
}

WebKitCSSKeyframesRule::WebKitCSSKeyframesRule(StyleRuleKeyframes* keyframesRule, CSSStyleSheet* parent)
    : CSSRule(parent)
    , m_keyframesRule(keyframesRule)
    , m_childRuleCSSOMWrappers(keyframesRule->keyframes().size())
{
}

WebKitCSSKeyframesRule::~WebKitCSSKeyframesRule()
{
    ASSERT(m_childRuleCSSOMWrappers.size() == m_keyframesRule->keyframes().size());

    for (unsigned i = 0; i < m_childRuleCSSOMWrappers.size(); ++i) {
        if (m_childRuleCSSOMWrappers[i])
            m_childRuleCSSOMWrappers[i]->setParentRule(0);
    }
}

void WebKitCSSKeyframesRule::setName(const String& name)
{
    CSSStyleSheet::RuleMutationScope mutationScope(this);

    m_keyframesRule->setName(name);
}

void WebKitCSSKeyframesRule::insertRule(const String& ruleText)
{
    ASSERT(m_childRuleCSSOMWrappers.size() == m_keyframesRule->keyframes().size());

    CSSParser parser(parserContext());
    CSSStyleSheet* styleSheet = parentStyleSheet();
    RefPtr<StyleKeyframe> keyframe = parser.parseKeyframeRule(styleSheet ? styleSheet->contents() : 0, ruleText);
    if (!keyframe)
        return;

    CSSStyleSheet::RuleMutationScope mutationScope(this);

    m_keyframesRule->wrapperAppendKeyframe(keyframe);

    m_childRuleCSSOMWrappers.grow(length());
}

void WebKitCSSKeyframesRule::deleteRule(const String& s)
{
    ASSERT(m_childRuleCSSOMWrappers.size() == m_keyframesRule->keyframes().size());

    int i = m_keyframesRule->findKeyframeIndex(s);
    if (i < 0)
        return;

    CSSStyleSheet::RuleMutationScope mutationScope(this);

    m_keyframesRule->wrapperRemoveKeyframe(i);

    if (m_childRuleCSSOMWrappers[i])
        m_childRuleCSSOMWrappers[i]->setParentRule(0);
    m_childRuleCSSOMWrappers.remove(i);
}

WebKitCSSKeyframeRule* WebKitCSSKeyframesRule::findRule(const String& s)
{
    int i = m_keyframesRule->findKeyframeIndex(s);
    return (i >= 0) ? item(i) : 0;
}

String WebKitCSSKeyframesRule::cssText() const
{
    StringBuilder result;
    result.appendLiteral("@-webkit-keyframes ");
    result.append(name());
    result.appendLiteral(" { \n");

    unsigned size = length();
    for (unsigned i = 0; i < size; ++i) {
        result.appendLiteral("  ");
        result.append(m_keyframesRule->keyframes()[i]->cssText());
        result.append('\n');
    }
    result.append('}');
    return result.toString();
}

unsigned WebKitCSSKeyframesRule::length() const
{ 
    return m_keyframesRule->keyframes().size(); 
}

WebKitCSSKeyframeRule* WebKitCSSKeyframesRule::item(unsigned index) const
{ 
    if (index >= length())
        return 0;

    ASSERT(m_childRuleCSSOMWrappers.size() == m_keyframesRule->keyframes().size());
    RefPtr<WebKitCSSKeyframeRule>& rule = m_childRuleCSSOMWrappers[index];
    if (!rule)
        rule = adoptRef(new WebKitCSSKeyframeRule(m_keyframesRule->keyframes()[index].get(), const_cast<WebKitCSSKeyframesRule*>(this)));

    return rule.get(); 
}

CSSRuleList* WebKitCSSKeyframesRule::cssRules()
{
    if (!m_ruleListCSSOMWrapper)
        m_ruleListCSSOMWrapper = adoptPtr(new LiveCSSRuleList<WebKitCSSKeyframesRule>(this));
    return m_ruleListCSSOMWrapper.get();
}

void WebKitCSSKeyframesRule::reattach(StyleRuleBase* rule)
{
    ASSERT(rule);
    ASSERT_WITH_SECURITY_IMPLICATION(rule->isKeyframesRule());
    m_keyframesRule = static_cast<StyleRuleKeyframes*>(rule);
}

} // namespace WebCore
