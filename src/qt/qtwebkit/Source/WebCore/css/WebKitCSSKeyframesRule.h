/*
 * Copyright (C) 2007, 2008, 2012 Apple Inc. All rights reserved.
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

#ifndef WebKitCSSKeyframesRule_h
#define WebKitCSSKeyframesRule_h

#include "CSSRule.h"
#include "StyleRule.h"
#include <wtf/Forward.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

class CSSRuleList;
class StyleKeyframe;
class WebKitCSSKeyframeRule;

class StyleRuleKeyframes : public StyleRuleBase {
public:
    static PassRefPtr<StyleRuleKeyframes> create() { return adoptRef(new StyleRuleKeyframes()); }
    
    ~StyleRuleKeyframes();
    
    const Vector<RefPtr<StyleKeyframe> >& keyframes() const { return m_keyframes; }
    
    void parserAppendKeyframe(PassRefPtr<StyleKeyframe>);
    void wrapperAppendKeyframe(PassRefPtr<StyleKeyframe>);
    void wrapperRemoveKeyframe(unsigned);

    String name() const { return m_name; }    
    void setName(const String& name) { m_name = AtomicString(name); }
    
    int findKeyframeIndex(const String& key) const;

    PassRefPtr<StyleRuleKeyframes> copy() const { return adoptRef(new StyleRuleKeyframes(*this)); }

private:
    StyleRuleKeyframes();
    StyleRuleKeyframes(const StyleRuleKeyframes&);

    Vector<RefPtr<StyleKeyframe> > m_keyframes;
    AtomicString m_name;
};

class WebKitCSSKeyframesRule : public CSSRule {
public:
    static PassRefPtr<WebKitCSSKeyframesRule> create(StyleRuleKeyframes* rule, CSSStyleSheet* sheet) { return adoptRef(new WebKitCSSKeyframesRule(rule, sheet)); }

    virtual ~WebKitCSSKeyframesRule();

    virtual CSSRule::Type type() const OVERRIDE { return WEBKIT_KEYFRAMES_RULE; }
    virtual String cssText() const OVERRIDE;
    virtual void reattach(StyleRuleBase*) OVERRIDE;

    String name() const { return m_keyframesRule->name(); }
    void setName(const String&);

    CSSRuleList* cssRules();

    void insertRule(const String& rule);
    void deleteRule(const String& key);
    WebKitCSSKeyframeRule* findRule(const String& key);

    // For IndexedGetter and CSSRuleList.
    unsigned length() const;
    WebKitCSSKeyframeRule* item(unsigned index) const;

private:
    WebKitCSSKeyframesRule(StyleRuleKeyframes*, CSSStyleSheet* parent);

    RefPtr<StyleRuleKeyframes> m_keyframesRule;
    mutable Vector<RefPtr<WebKitCSSKeyframeRule> > m_childRuleCSSOMWrappers;
    mutable OwnPtr<CSSRuleList> m_ruleListCSSOMWrapper;
};

} // namespace WebCore

#endif // WebKitCSSKeyframesRule_h
