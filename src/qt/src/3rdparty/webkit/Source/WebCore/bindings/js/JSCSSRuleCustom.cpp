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
#include "JSCSSRule.h"

#include "CSSCharsetRule.h"
#include "CSSFontFaceRule.h"
#include "CSSImportRule.h"
#include "CSSMediaRule.h"
#include "CSSPageRule.h"
#include "CSSStyleRule.h"
#include "JSCSSCharsetRule.h"
#include "JSCSSFontFaceRule.h"
#include "JSCSSImportRule.h"
#include "JSCSSMediaRule.h"
#include "JSCSSPageRule.h"
#include "JSCSSStyleRule.h"
#include "JSNode.h"
#include "JSWebKitCSSKeyframeRule.h"
#include "JSWebKitCSSKeyframesRule.h"
#include "WebKitCSSKeyframeRule.h"
#include "WebKitCSSKeyframesRule.h"

using namespace JSC;

namespace WebCore {

void JSCSSRule::visitChildren(SlotVisitor& visitor)
{
    Base::visitChildren(visitor);
    visitor.addOpaqueRoot(root(impl()));
}

JSValue toJS(ExecState* exec, JSDOMGlobalObject* globalObject, CSSRule* rule)
{
    if (!rule)
        return jsNull();

    JSDOMWrapper* wrapper = getCachedWrapper(currentWorld(exec), rule);
    if (wrapper)
        return wrapper;

    switch (rule->type()) {
        case CSSRule::STYLE_RULE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, CSSStyleRule, rule);
            break;
        case CSSRule::MEDIA_RULE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, CSSMediaRule, rule);
            break;
        case CSSRule::FONT_FACE_RULE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, CSSFontFaceRule, rule);
            break;
        case CSSRule::PAGE_RULE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, CSSPageRule, rule);
            break;
        case CSSRule::IMPORT_RULE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, CSSImportRule, rule);
            break;
        case CSSRule::CHARSET_RULE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, CSSCharsetRule, rule);
            break;
        case CSSRule::WEBKIT_KEYFRAME_RULE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, WebKitCSSKeyframeRule, rule);
            break;
        case CSSRule::WEBKIT_KEYFRAMES_RULE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, WebKitCSSKeyframesRule, rule);
            break;
        default:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, CSSRule, rule);
    }

    return wrapper;
}

} // namespace WebCore
