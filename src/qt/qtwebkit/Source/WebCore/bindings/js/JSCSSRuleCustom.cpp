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
#include "CSSSupportsRule.h"
#include "JSCSSCharsetRule.h"
#include "JSCSSFontFaceRule.h"
#include "JSCSSHostRule.h"
#include "JSCSSImportRule.h"
#include "JSCSSMediaRule.h"
#include "JSCSSPageRule.h"
#include "JSCSSStyleRule.h"
#include "JSCSSSupportsRule.h"
#include "JSNode.h"
#include "JSStyleSheetCustom.h"
#include "JSWebKitCSSFilterRule.h"
#include "JSWebKitCSSKeyframeRule.h"
#include "JSWebKitCSSKeyframesRule.h"
#include "JSWebKitCSSRegionRule.h"
#include "JSWebKitCSSViewportRule.h"
#include "WebKitCSSFilterRule.h"
#include "WebKitCSSKeyframeRule.h"
#include "WebKitCSSKeyframesRule.h"
#include "WebKitCSSRegionRule.h"
#include "WebKitCSSViewportRule.h"

using namespace JSC;

namespace WebCore {

void JSCSSRule::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSCSSRule* thisObject = jsCast<JSCSSRule*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());
    Base::visitChildren(thisObject, visitor);
    visitor.addOpaqueRoot(root(thisObject->impl()));
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
#if ENABLE(CSS3_CONDITIONAL_RULES)
        case CSSRule::SUPPORTS_RULE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, CSSSupportsRule, rule);
            break;
#endif
#if ENABLE(CSS_DEVICE_ADAPTATION)
        case CSSRule::WEBKIT_VIEWPORT_RULE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, WebKitCSSViewportRule, rule);
            break;
#endif
#if ENABLE(CSS_REGIONS)
        case CSSRule::WEBKIT_REGION_RULE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, WebKitCSSRegionRule, rule);
            break;
#endif
#if ENABLE(SHADOW_DOM)
        case CSSRule::HOST_RULE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, CSSHostRule, rule);
            break;
#endif
#if ENABLE(CSS_SHADERS)
        case CSSRule::WEBKIT_FILTER_RULE:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, WebKitCSSFilterRule, rule);
            break;
#endif
        default:
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, CSSRule, rule);
    }

    return wrapper;
}

} // namespace WebCore
