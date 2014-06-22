/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
#include "JSCanvasRenderingContext.h"

#include "CanvasRenderingContext2D.h"
#include "HTMLCanvasElement.h"
#include "JSCanvasRenderingContext2D.h"
#include "JSNode.h"
#if ENABLE(WEBGL)
#include "WebGLRenderingContext.h"
#include "JSWebGLRenderingContext.h"
#endif

using namespace JSC;

namespace WebCore {

void JSCanvasRenderingContext::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSCanvasRenderingContext* thisObject = jsCast<JSCanvasRenderingContext*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());
    Base::visitChildren(thisObject, visitor);

    visitor.addOpaqueRoot(root(thisObject->impl()->canvas()));
}

JSC::JSValue toJS(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, CanvasRenderingContext* object)
{
    if (!object)
        return jsNull();

#if ENABLE(WEBGL)
    if (object->is3d())
        return wrap<JSWebGLRenderingContext>(exec, globalObject, static_cast<WebGLRenderingContext*>(object));
#endif
    ASSERT(object->is2d());
    return wrap<JSCanvasRenderingContext2D>(exec, globalObject, static_cast<CanvasRenderingContext2D*>(object));
}

} // namespace WebCore
