/*
 * Copyright (C) 2013 Adobe Systems Incorporated. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "WebKitCSSMatFunctionValue.h"

#include <wtf/text/StringBuilder.h>

#if ENABLE(CSS_SHADERS)

namespace WebCore {

WebKitCSSMatFunctionValue::WebKitCSSMatFunctionValue()
    : CSSValueList(WebKitCSSMatFunctionValueClass, CommaSeparator)
{
}

WebKitCSSMatFunctionValue::WebKitCSSMatFunctionValue(const WebKitCSSMatFunctionValue& cloneFrom)
    : CSSValueList(cloneFrom)
{
}

String WebKitCSSMatFunctionValue::customCssText() const
{
    StringBuilder builder;
    if (length() == 4)
        builder.appendLiteral("mat2(");
    else if (length() == 9)
        builder.appendLiteral("mat3(");
    else if (length() == 16)
        builder.appendLiteral("mat4(");
    else {
        ASSERT_NOT_REACHED();
        return String();
    }

    builder.append(CSSValueList::customCssText());
    builder.append(')');
    return builder.toString();
}

PassRefPtr<WebKitCSSMatFunctionValue> WebKitCSSMatFunctionValue::cloneForCSSOM() const
{
    return adoptRef(new WebKitCSSMatFunctionValue(*this));
}

bool WebKitCSSMatFunctionValue::equals(const WebKitCSSMatFunctionValue& other) const
{
    return CSSValueList::equals(other);
}

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS)
