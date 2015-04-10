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

#ifndef WebKitCSSKeyframeRule_h
#define WebKitCSSKeyframeRule_h

#include "CSSRule.h"

namespace WebCore {

class CSSStyleDeclaration;
class MutableStylePropertySet;
class StylePropertySet;
class StyleRuleCSSStyleDeclaration;
class WebKitCSSKeyframesRule;

class StyleKeyframe : public RefCounted<StyleKeyframe> {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<StyleKeyframe> create()
    {
        return adoptRef(new StyleKeyframe());
    }
    ~StyleKeyframe();

    String keyText() const { return m_key; }
    void setKeyText(const String& s) { m_key = s; }

    void getKeys(Vector<float>& keys) const   { parseKeyString(m_key, keys); }
    
    const StylePropertySet* properties() const { return m_properties.get(); }
    MutableStylePropertySet* mutableProperties();
    void setProperties(PassRefPtr<StylePropertySet>);
    
    String cssText() const;

private:
    StyleKeyframe();
    
    static void parseKeyString(const String&, Vector<float>& keys);
    
    RefPtr<StylePropertySet> m_properties;
    // FIXME: This should be a parsed vector of floats.
    // comma separated list of keys
    String m_key;
};

class WebKitCSSKeyframeRule : public CSSRule {
public:
    virtual ~WebKitCSSKeyframeRule();

    virtual CSSRule::Type type() const OVERRIDE { return WEBKIT_KEYFRAME_RULE; }
    virtual String cssText() const OVERRIDE { return m_keyframe->cssText(); }
    virtual void reattach(StyleRuleBase*) OVERRIDE;

    String keyText() const { return m_keyframe->keyText(); }
    void setKeyText(const String& s) { m_keyframe->setKeyText(s); }

    CSSStyleDeclaration* style();

private:
    WebKitCSSKeyframeRule(StyleKeyframe*, WebKitCSSKeyframesRule* parent);

    RefPtr<StyleKeyframe> m_keyframe;
    mutable RefPtr<StyleRuleCSSStyleDeclaration> m_propertiesCSSOMWrapper;
    
    friend class WebKitCSSKeyframesRule;
};

} // namespace WebCore

#endif // WebKitCSSKeyframeRule_h
