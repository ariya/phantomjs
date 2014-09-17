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

#if ENABLE(XPATH)

#include "Document.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "JSDOMWindowCustom.h"
#include "SecurityOrigin.h"
#include <runtime/JSLock.h>

namespace WebCore {

using namespace JSC;

PassRefPtr<JSCustomXPathNSResolver> JSCustomXPathNSResolver::create(JSC::ExecState* exec, JSC::JSValue value)
{
    if (value.isUndefinedOrNull())
        return 0;

    JSObject* resolverObject = value.getObject();
    if (!resolverObject) {
        setDOMException(exec, TYPE_MISMATCH_ERR);
        return 0;
    }

    return adoptRef(new JSCustomXPathNSResolver(resolverObject, asJSDOMWindow(exec->dynamicGlobalObject())));
}

JSCustomXPathNSResolver::JSCustomXPathNSResolver(JSObject* customResolver, JSDOMWindow* globalObject)
    : m_customResolver(customResolver)
    , m_globalObject(globalObject)
{
}

JSCustomXPathNSResolver::~JSCustomXPathNSResolver()
{
}

String JSCustomXPathNSResolver::lookupNamespaceURI(const String& prefix)
{
    ASSERT(m_customResolver);

    JSLock lock(SilenceAssertionsOnly);

    ExecState* exec = m_globalObject->globalExec();
        
    JSValue function = m_customResolver->get(exec, Identifier(exec, "lookupNamespaceURI"));
    CallData callData;
    CallType callType = getCallData(function, callData);
    if (callType == CallTypeNone) {
        callType = m_customResolver->getCallData(callData);
        if (callType == CallTypeNone) {
            // FIXME: Pass actual line number and source URL.
            m_globalObject->impl()->console()->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "XPathNSResolver does not have a lookupNamespaceURI method.", 0, String());
            return String();
        }
        function = m_customResolver;
    }

    RefPtr<JSCustomXPathNSResolver> selfProtector(this);

    MarkedArgumentBuffer args;
    args.append(jsString(exec, prefix));

    m_globalObject->globalData().timeoutChecker.start();
    JSValue retval = JSC::call(exec, function, callType, callData, m_customResolver, args);
    m_globalObject->globalData().timeoutChecker.stop();

    String result;
    if (exec->hadException())
        reportCurrentException(exec);
    else {
        if (!retval.isUndefinedOrNull())
            result = ustringToString(retval.toString(exec));
    }

    Document::updateStyleForAllDocuments();

    return result;
}

} // namespace WebCore

#endif // ENABLE(XPATH)
