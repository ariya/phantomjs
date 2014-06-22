/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSNPMethod.h"

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "JSNPObject.h"
#include <JavaScriptCore/Error.h>
#include <JavaScriptCore/FunctionPrototype.h>
#include <JavaScriptCore/JSGlobalObject.h>
#include <JavaScriptCore/JSObject.h>
#include <WebCore/JSHTMLElement.h>
#include <WebCore/JSPluginElementFunctions.h>

using namespace JSC;
using namespace WebCore;

namespace WebKit {

ASSERT_HAS_TRIVIAL_DESTRUCTOR(JSNPMethod);

const ClassInfo JSNPMethod::s_info = { "NPMethod", &InternalFunction::s_info, 0, 0, CREATE_METHOD_TABLE(JSNPMethod) };

JSNPMethod::JSNPMethod(JSGlobalObject* globalObject, Structure* structure, NPIdentifier npIdentifier)
    : InternalFunction(globalObject, structure)
    , m_npIdentifier(npIdentifier)
{
}

void JSNPMethod::finishCreation(VM& vm, const String& name)
{
    Base::finishCreation(vm, name);
    ASSERT(inherits(&s_info));
}

static EncodedJSValue JSC_HOST_CALL callMethod(ExecState* exec)
{
    JSNPMethod* jsNPMethod = static_cast<JSNPMethod*>(exec->callee());

    JSValue thisValue = exec->hostThisValue();

    // Check if we're calling a method on the plug-in script object.
    if (thisValue.inherits(&JSHTMLElement::s_info)) {
        JSHTMLElement* element = static_cast<JSHTMLElement*>(asObject(thisValue));

        // Try to get the script object from the element
        if (JSObject* scriptObject = pluginScriptObject(exec, element))
            thisValue = scriptObject;
    }

    if (thisValue.inherits(&JSNPObject::s_info)) {
        JSNPObject* jsNPObject = static_cast<JSNPObject*>(asObject(thisValue));

        return JSValue::encode(jsNPObject->callMethod(exec, jsNPMethod->npIdentifier()));
    }

    return throwVMTypeError(exec);
}

CallType JSNPMethod::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = callMethod;
    return CallTypeHost;
}

} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)
