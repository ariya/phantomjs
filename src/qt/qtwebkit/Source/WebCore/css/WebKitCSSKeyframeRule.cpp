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

#include "config.h"
#include "WebKitCSSKeyframeRule.h"

#include "PropertySetCSSStyleDeclaration.h"
#include "StylePropertySet.h"
#include "WebKitCSSKeyframesRule.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

StyleKeyframe::StyleKeyframe()
{
}

StyleKeyframe::~StyleKeyframe()
{
}

MutableStylePropertySet* StyleKeyframe::mutableProperties()
{
    if (!m_properties->isMutable())
        m_properties = m_properties->mutableCopy();
    return static_cast<MutableStylePropertySet*>(m_properties.get());
}
    
void StyleKeyframe::setProperties(PassRefPtr<StylePropertySet> properties)
{
    m_properties = properties;
}

/* static */
void StyleKeyframe::parseKeyString(const String& s, Vector<float>& keys)
{
    keys.clear();
    Vector<String> strings;
    s.split(',', strings);

    for (size_t i = 0; i < strings.size(); ++i) {
        float key = -1;
        String cur = strings[i].stripWhiteSpace();
        
        // For now the syntax MUST be 'xxx%' or 'from' or 'to', where xxx is a legal floating point number
        if (cur == "from")
            key = 0;
        else if (cur == "to")
            key = 1;
        else if (cur.endsWith('%')) {
            float k = cur.substring(0, cur.length() - 1).toFloat();
            if (k >= 0 && k <= 100)
                key = k/100;
        }
        if (key < 0) {
            keys.clear();
            return;
        }
        else
            keys.append(key);
    }
}

String StyleKeyframe::cssText() const
{
    StringBuilder result;
    result.append(keyText());
    result.appendLiteral(" { ");
    String decls = m_properties->asText();
    result.append(decls);
    if (!decls.isEmpty())
        result.append(' ');
    result.append('}');
    return result.toString();
}

WebKitCSSKeyframeRule::WebKitCSSKeyframeRule(StyleKeyframe* keyframe, WebKitCSSKeyframesRule* parent)
    : CSSRule(0)
    , m_keyframe(keyframe)
{
    setParentRule(parent);
}

WebKitCSSKeyframeRule::~WebKitCSSKeyframeRule()
{
    if (m_propertiesCSSOMWrapper)
        m_propertiesCSSOMWrapper->clearParentRule();
}

CSSStyleDeclaration* WebKitCSSKeyframeRule::style()
{
    if (!m_propertiesCSSOMWrapper)
        m_propertiesCSSOMWrapper = StyleRuleCSSStyleDeclaration::create(m_keyframe->mutableProperties(), this);
    return m_propertiesCSSOMWrapper.get();
}

void WebKitCSSKeyframeRule::reattach(StyleRuleBase*)
{
    // No need to reattach, the underlying data is shareable on mutation.
    ASSERT_NOT_REACHED();
}

} // namespace WebCore
