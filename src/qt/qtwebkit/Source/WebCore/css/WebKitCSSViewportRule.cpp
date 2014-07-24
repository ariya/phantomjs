/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#include "WebKitCSSViewportRule.h"

#if ENABLE(CSS_DEVICE_ADAPTATION)

#include "PropertySetCSSStyleDeclaration.h"
#include "StylePropertySet.h"
#include "StyleRule.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

WebKitCSSViewportRule::WebKitCSSViewportRule(StyleRuleViewport* viewportRule, CSSStyleSheet* sheet)
    : CSSRule(sheet)
    , m_viewportRule(viewportRule)
{
}

WebKitCSSViewportRule::~WebKitCSSViewportRule()
{
    if (m_propertiesCSSOMWrapper)
        m_propertiesCSSOMWrapper->clearParentRule();
}

CSSStyleDeclaration* WebKitCSSViewportRule::style()
{
    if (!m_propertiesCSSOMWrapper)
        m_propertiesCSSOMWrapper = StyleRuleCSSStyleDeclaration::create(m_viewportRule->mutableProperties(), this);

    return m_propertiesCSSOMWrapper.get();
}

String WebKitCSSViewportRule::cssText() const
{
    StringBuilder result;
    result.appendLiteral("@-webkit-viewport { ");

    String decls = m_viewportRule->properties()->asText();
    result.append(decls);
    if (!decls.isEmpty())
        result.append(' ');

    result.append('}');

    return result.toString();
}

void WebKitCSSViewportRule::reattach(StyleRuleBase* rule)
{
    ASSERT(rule);
    ASSERT_WITH_SECURITY_IMPLICATION(rule->isViewportRule());
    m_viewportRule = static_cast<StyleRuleViewport*>(rule);

    if (m_propertiesCSSOMWrapper)
        m_propertiesCSSOMWrapper->reattach(m_viewportRule->mutableProperties());
}

} // namespace WebCore

#endif // ENABLE(CSS_DEVICE_ADAPTATION)
