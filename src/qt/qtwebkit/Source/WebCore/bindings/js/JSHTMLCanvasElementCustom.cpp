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
#include "InspectorCanvasInstrumentation.h"
#include "JSCanvasRenderingContext2D.h"
#include "ScriptObject.h"
#if ENABLE(WEBGL)
#include "JSDictionary.h"
#include "JSWebGLRenderingContext.h"
#include "WebGLContextAttributes.h"
#endif
#include <wtf/GetPtr.h>

using namespace JSC;

namespace WebCore {

#if ENABLE(WEBGL)
static void get3DContextAttributes(ExecState* exec, RefPtr<CanvasContextAttributes>& attrs)
{
    JSValue initializerValue = exec->argument(1);
    if (initializerValue.isUndefinedOrNull())
        return;
    
    JSObject* initializerObject = initializerValue.toObject(exec);
    JSDictionary dictionary(exec, initializerObject);
    
    GraphicsContext3D::Attributes graphicsAttrs;
    
    dictionary.tryGetProperty("alpha", graphicsAttrs.alpha);
    dictionary.tryGetProperty("depth", graphicsAttrs.depth);
    dictionary.tryGetProperty("stencil", graphicsAttrs.stencil);
    dictionary.tryGetProperty("antialias", graphicsAttrs.antialias);
    dictionary.tryGetProperty("premultipliedAlpha", graphicsAttrs.premultipliedAlpha);
    dictionary.tryGetProperty("preserveDrawingBuffer", graphicsAttrs.preserveDrawingBuffer);
    
    attrs = WebGLContextAttributes::create(graphicsAttrs);
}
#endif

JSValue JSHTMLCanvasElement::getContext(ExecState* exec)
{
    HTMLCanvasElement* canvas = static_cast<HTMLCanvasElement*>(impl());
    const String& contextId = exec->argument(0).toString(exec)->value(exec);
    
    RefPtr<CanvasContextAttributes> attrs;
#if ENABLE(WEBGL)
    if (HTMLCanvasElement::is3dType(contextId)) {
        get3DContextAttributes(exec, attrs);
        if (exec->hadException())
            return jsUndefined();
    }
#endif
    
    CanvasRenderingContext* context = canvas->getContext(contextId, attrs.get());
    if (!context)
        return jsNull();
    JSValue jsValue = toJS(exec, globalObject(), WTF::getPtr(context));
    if (InspectorInstrumentation::canvasAgentEnabled(canvas->document())) {
        ScriptObject contextObject(exec, jsValue.getObject());
        ScriptObject wrapped;
        if (context->is2d())
            wrapped = InspectorInstrumentation::wrapCanvas2DRenderingContextForInstrumentation(canvas->document(), contextObject);
#if ENABLE(WEBGL)
        else if (context->is3d())
            wrapped = InspectorInstrumentation::wrapWebGLRenderingContextForInstrumentation(canvas->document(), contextObject);
#endif
        if (!wrapped.hasNoValue())
            return wrapped.jsValue();
    }
    return jsValue;
}

JSValue JSHTMLCanvasElement::supportsContext(ExecState* exec)
{
    HTMLCanvasElement* canvas = static_cast<HTMLCanvasElement*>(impl());
    if (!exec->argumentCount())
        return jsBoolean(false);
    const String& contextId = exec->argument(0).toString(exec)->value(exec);
    if (exec->hadException())
        return jsUndefined();
    
    RefPtr<CanvasContextAttributes> attrs;
#if ENABLE(WEBGL)
    if (HTMLCanvasElement::is3dType(contextId)) {
        get3DContextAttributes(exec, attrs);
        if (exec->hadException())
            return jsUndefined();
    }
#endif
    
    return jsBoolean(canvas->supportsContext(contextId, attrs.get()));
}

JSValue JSHTMLCanvasElement::toDataURL(ExecState* exec)
{
    HTMLCanvasElement* canvas = static_cast<HTMLCanvasElement*>(impl());
    ExceptionCode ec = 0;

    const String& type = valueToStringWithUndefinedOrNullCheck(exec, exec->argument(0));
    double quality;
    double* qualityPtr = 0;
    if (exec->argumentCount() > 1) {
        JSValue v = exec->argument(1);
        if (v.isNumber()) {
            quality = v.toNumber(exec);
            qualityPtr = &quality;
        }
    }

    JSValue result = JSC::jsString(exec, canvas->toDataURL(type, qualityPtr, ec));
    setDOMException(exec, ec);
    return result;
}

} // namespace WebCore
