/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
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
#include "JSHTMLCanvasElement.h"

#include "CanvasContextAttributes.h"
#include "HTMLCanvasElement.h"
#include "JSCanvasRenderingContext2D.h"
#if ENABLE(WEBGL)
#include "JSWebGLRenderingContext.h"
#include "WebGLContextAttributes.h"
#endif
#include <wtf/GetPtr.h>

using namespace JSC;

namespace WebCore {

JSValue JSHTMLCanvasElement::getContext(ExecState* exec)
{
    HTMLCanvasElement* canvas = static_cast<HTMLCanvasElement*>(impl());
    const UString& contextId = exec->argument(0).toString(exec);
    RefPtr<CanvasContextAttributes> attrs;
#if ENABLE(WEBGL)
    if (contextId == "experimental-webgl" || contextId == "webkit-3d") {
        attrs = WebGLContextAttributes::create();
        WebGLContextAttributes* webGLAttrs = static_cast<WebGLContextAttributes*>(attrs.get());
        if (exec->argumentCount() > 1 && exec->argument(1).isObject()) {
            JSObject* jsAttrs = exec->argument(1).getObject();
            Identifier alpha(exec, "alpha");
            if (jsAttrs->hasProperty(exec, alpha))
                webGLAttrs->setAlpha(jsAttrs->get(exec, alpha).toBoolean(exec));
            Identifier depth(exec, "depth");
            if (jsAttrs->hasProperty(exec, depth))
                webGLAttrs->setDepth(jsAttrs->get(exec, depth).toBoolean(exec));
            Identifier stencil(exec, "stencil");
            if (jsAttrs->hasProperty(exec, stencil))
                webGLAttrs->setStencil(jsAttrs->get(exec, stencil).toBoolean(exec));
            Identifier antialias(exec, "antialias");
            if (jsAttrs->hasProperty(exec, antialias))
                webGLAttrs->setAntialias(jsAttrs->get(exec, antialias).toBoolean(exec));
            Identifier premultipliedAlpha(exec, "premultipliedAlpha");
            if (jsAttrs->hasProperty(exec, premultipliedAlpha))
                webGLAttrs->setPremultipliedAlpha(jsAttrs->get(exec, premultipliedAlpha).toBoolean(exec));
            Identifier preserveDrawingBuffer(exec, "preserveDrawingBuffer");
            if (jsAttrs->hasProperty(exec, preserveDrawingBuffer))
                webGLAttrs->setPreserveDrawingBuffer(jsAttrs->get(exec, preserveDrawingBuffer).toBoolean(exec));
        }
    }
#endif
    CanvasRenderingContext* context = canvas->getContext(ustringToString(contextId), attrs.get());
    if (!context)
        return jsNull();
    return toJS(exec, globalObject(), WTF::getPtr(context));
}

JSValue JSHTMLCanvasElement::toDataURL(ExecState* exec)
{
    const String& type = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(0));
    HTMLCanvasElement* canvas = static_cast<HTMLCanvasElement*>(impl());
    ExceptionCode ec = 0;
    
    JSC::JSValue result;
    double quality;
    double* qualityPtr = 0;
    if (exec->argumentCount() > 1) {
        JSValue v = exec->argument(1);
        if (v.isNumber()) {
            quality = v.toNumber(exec);
            qualityPtr = &quality;
        }
    }
    
    result = jsString(exec, canvas->toDataURL(type, qualityPtr, ec));
    setDOMException(exec, ec);
    return result;
}

} // namespace WebCore
