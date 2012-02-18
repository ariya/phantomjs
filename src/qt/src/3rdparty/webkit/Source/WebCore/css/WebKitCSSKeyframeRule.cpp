/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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

#include "CSSMutableStyleDeclaration.h"

namespace WebCore {

WebKitCSSKeyframeRule::WebKitCSSKeyframeRule(CSSStyleSheet* parent)
    : CSSRule(parent)
{
}

WebKitCSSKeyframeRule::~WebKitCSSKeyframeRule()
{
    if (m_style)
        m_style->setParent(0);
}

String WebKitCSSKeyframeRule::cssText() const
{
    String result = m_key;

    result += " { ";
    result += m_style->cssText();
    result += "}";

    return result;
}

bool WebKitCSSKeyframeRule::parseString(const String& /*string*/, bool /*strict*/)
{
    // FIXME
    return false;
}

void WebKitCSSKeyframeRule::setDeclaration(PassRefPtr<CSSMutableStyleDeclaration> style)
{
    m_style = style;
    m_style->setParent(parent());
}

/* static */
void WebKitCSSKeyframeRule::parseKeyString(const String& s, Vector<float>& keys)
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
        else if (cur.endsWith("%")) {
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

} // namespace WebCore
