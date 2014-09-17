/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSXMLHttpRequest.h"

#include "ArrayBuffer.h"
#include "Blob.h"
#include "DOMFormData.h"
#include "DOMWindow.h"
#include "Document.h"
#include "Event.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "HTMLDocument.h"
#include "InspectorInstrumentation.h"
#include "JSArrayBuffer.h"
#include "JSBlob.h"
#include "JSDOMFormData.h"
#include "JSDOMWindowCustom.h"
#include "JSDocument.h"
#include "JSEvent.h"
#include "JSEventListener.h"
#include "XMLHttpRequest.h"
#include <runtime/Error.h>
#include <interpreter/Interpreter.h>

using namespace JSC;

namespace WebCore {

void JSXMLHttpRequest::visitChildren(SlotVisitor& visitor)
{
    Base::visitChildren(visitor);

    if (XMLHttpRequestUpload* upload = m_impl->optionalUpload())
        visitor.addOpaqueRoot(upload);

    if (Document* responseDocument = m_impl->optionalResponseXML())
        visitor.addOpaqueRoot(responseDocument);

    if (ArrayBuffer* responseArrayBuffer = m_impl->optionalResponseArrayBuffer())
        visitor.addOpaqueRoot(responseArrayBuffer);

#if ENABLE(XHR_RESPONSE_BLOB)
    if (Blob* responseBlob = m_impl->optionalResponseBlob())
        visitor.addOpaqueRoot(responseBlob);
#endif

    m_impl->visitJSEventListeners(visitor);
}

// Custom functions
JSValue JSXMLHttpRequest::open(ExecState* exec)
{
    if (exec->argumentCount() < 2)
        return throwError(exec, createSyntaxError(exec, "Not enough arguments"));

    const KURL& url = impl()->scriptExecutionContext()->completeURL(ustringToString(exec->argument(1).toString(exec)));
    String method = ustringToString(exec->argument(0).toString(exec));

    ExceptionCode ec = 0;
    if (exec->argumentCount() >= 3) {
        bool async = exec->argument(2).toBoolean(exec);

        if (exec->argumentCount() >= 4 && !exec->argument(3).isUndefined()) {
            String user = valueToStringWithNullCheck(exec, exec->argument(3));

            if (exec->argumentCount() >= 5 && !exec->argument(4).isUndefined()) {
                String password = valueToStringWithNullCheck(exec, exec->argument(4));
                impl()->open(method, url, async, user, password, ec);
            } else
                impl()->open(method, url, async, user, ec);
        } else
            impl()->open(method, url, async, ec);
    } else
        impl()->open(method, url, ec);

    setDOMException(exec, ec);
    return jsUndefined();
}

JSValue JSXMLHttpRequest::send(ExecState* exec)
{
    InspectorInstrumentation::willSendXMLHttpRequest(impl()->scriptExecutionContext(), impl()->url());

    ExceptionCode ec = 0;
    if (!exec->argumentCount())
        impl()->send(ec);
    else {
        JSValue val = exec->argument(0);
        if (val.isUndefinedOrNull())
            impl()->send(ec);
        else if (val.inherits(&JSDocument::s_info))
            impl()->send(toDocument(val), ec);
        else if (val.inherits(&JSBlob::s_info))
            impl()->send(toBlob(val), ec);
        else if (val.inherits(&JSDOMFormData::s_info))
            impl()->send(toDOMFormData(val), ec);
        else if (val.inherits(&JSArrayBuffer::s_info))
            impl()->send(toArrayBuffer(val), ec);
        else
            impl()->send(ustringToString(val.toString(exec)), ec);
    }

    int signedLineNumber;
    intptr_t sourceID;
    UString sourceURL;
    JSValue function;
    exec->interpreter()->retrieveLastCaller(exec, signedLineNumber, sourceID, sourceURL, function);
    impl()->setLastSendLineNumber(signedLineNumber >= 0 ? signedLineNumber : 0);
    impl()->setLastSendURL(ustringToString(sourceURL));

    setDOMException(exec, ec);
    return jsUndefined();
}

JSValue JSXMLHttpRequest::responseText(ExecState* exec) const
{
    ExceptionCode ec = 0;
    String text = impl()->responseText(ec);
    if (ec) {
        setDOMException(exec, ec);
        return jsUndefined();
    }
    return jsOwnedStringOrNull(exec, text);
}

JSValue JSXMLHttpRequest::response(ExecState* exec) const
{
    switch (impl()->responseTypeCode()) {
    case XMLHttpRequest::ResponseTypeDefault:
    case XMLHttpRequest::ResponseTypeText:
        return responseText(exec);

    case XMLHttpRequest::ResponseTypeDocument:
        {
            ExceptionCode ec = 0;
            Document* document = impl()->responseXML(ec);
            if (ec) {
                setDOMException(exec, ec);
                return jsUndefined();
            }
            return toJS(exec, globalObject(), document);
        }

    case XMLHttpRequest::ResponseTypeBlob:
#if ENABLE(XHR_RESPONSE_BLOB)
        {
            ExceptionCode ec = 0;
            Blob* blob = impl()->responseBlob(ec);
            if (ec) {
                setDOMException(exec, ec);
                return jsUndefined();
            }
            return toJS(exec, globalObject(), blob);
        }
#else
        return jsUndefined();
#endif

    case XMLHttpRequest::ResponseTypeArrayBuffer:
        {
            ExceptionCode ec = 0;
            ArrayBuffer* arrayBuffer = impl()->responseArrayBuffer(ec);
            if (ec) {
                setDOMException(exec, ec);
                return jsUndefined();
            }
            return toJS(exec, globalObject(), arrayBuffer);
        }
    }

    return jsUndefined();
}

EncodedJSValue JSC_HOST_CALL JSXMLHttpRequestConstructor::constructJSXMLHttpRequest(ExecState* exec)
{
    JSXMLHttpRequestConstructor* jsConstructor = static_cast<JSXMLHttpRequestConstructor*>(exec->callee());
    ScriptExecutionContext* context = jsConstructor->scriptExecutionContext();
    if (!context)
        return throwVMError(exec, createReferenceError(exec, "XMLHttpRequest constructor associated document is unavailable"));

    RefPtr<XMLHttpRequest> xmlHttpRequest = XMLHttpRequest::create(context);
    return JSValue::encode(CREATE_DOM_WRAPPER(exec, jsConstructor->globalObject(), XMLHttpRequest, xmlHttpRequest.get()));
}

} // namespace WebCore
