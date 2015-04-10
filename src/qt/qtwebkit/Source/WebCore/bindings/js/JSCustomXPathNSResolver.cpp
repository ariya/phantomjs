/*
 * Copyright (C) 2007 Alexey Proskuryakov (ap@nypop.com)
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
#include "JSCustomXPathNSResolver.h"

#include "Document.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "JSDOMWindowCustom.h"
#include "JSMainThreadExecState.h"
#include "Page.h"
#include "PageConsole.h"
#include "SecurityOrigin.h"
#include <runtime/JSLock.h>

namespace WebCore {

using namespace JSC;

PassRefPtr<JSCustomXPathNSResolver> JSCustomXPathNSResolver::create(ExecState* exec, JSValue value)
{
    if (value.isUndefinedOrNull())
        return 0;

    JSObject* resolverObject = value.getObject();
    if (!resolverObject) {
        setDOMException(exec, TYPE_MISMATCH_ERR);
        return 0;
    }

    return adoptRef(new JSCustomXPathNSResolver(exec, resolverObject, asJSDOMWindow(exec->dynamicGlobalObject())));
}

JSCustomXPathNSResolver::JSCustomXPathNSResolver(ExecState* exec, JSObject* customResolver, JSDOMWindow* globalObject)
    : m_customResolver(exec->vm(), customResolver)
    , m_globalObject(exec->vm(), globalObject)
{
}

JSCustomXPathNSResolver::~JSCustomXPathNSResolver()
{
}

String JSCustomXPathNSResolver::lookupNamespaceURI(const String& prefix)
{
    ASSERT(m_customResolver);

    JSLockHolder lock(JSDOMWindowBase::commonVM());

    ExecState* exec = m_globalObject->globalExec();
        
    JSValue function = m_customResolver->get(exec, Identifier(exec, "lookupNamespaceURI"));
    CallData callData;
    CallType callType = getCallData(function, callData);
    if (callType == CallTypeNone) {
        callType = m_customResolver->methodTable()->getCallData(m_customResolver.get(), callData);
        if (callType == CallTypeNone) {
            // FIXME: <http://webkit.org/b/114312> JSCustomXPathNSResolver::lookupNamespaceURI Console Message should include Line, Column, and SourceURL
            if (PageConsole* console = m_globalObject->impl()->pageConsole())
                console->addMessage(JSMessageSource, ErrorMessageLevel, "XPathNSResolver does not have a lookupNamespaceURI method.");
            return String();
        }
        function = m_customResolver.get();
    }

    RefPtr<JSCustomXPathNSResolver> selfProtector(this);

    MarkedArgumentBuffer args;
    args.append(jsStringWithCache(exec, prefix));

    JSValue retval = JSMainThreadExecState::call(exec, function, callType, callData, m_customResolver.get(), args);

    String result;
    if (exec->hadException())
        reportCurrentException(exec);
    else {
        if (!retval.isUndefinedOrNull())
            result = retval.toString(exec)->value(exec);
    }

    Document::updateStyleForAllDocuments();

    return result;
}

} // namespace WebCore
