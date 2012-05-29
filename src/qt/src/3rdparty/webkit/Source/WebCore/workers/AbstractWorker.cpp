/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WORKERS)

#include "AbstractWorker.h"

#include "ErrorEvent.h"
#include "Event.h"
#include "EventException.h"
#include "EventNames.h"
#include "InspectorInstrumentation.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"

namespace WebCore {

AbstractWorker::AbstractWorker(ScriptExecutionContext* context)
    : ActiveDOMObject(context, this)
{
}

AbstractWorker::~AbstractWorker()
{
    onDestroyWorker();
}

void AbstractWorker::onDestroyWorker()
{
    InspectorInstrumentation::didDestroyWorker(scriptExecutionContext(), asID());
}

void AbstractWorker::contextDestroyed()
{
    onDestroyWorker();
    ActiveDOMObject::contextDestroyed(); 
}

KURL AbstractWorker::resolveURL(const String& url, ExceptionCode& ec)
{
    if (url.isEmpty()) {
        ec = SYNTAX_ERR;
        return KURL();
    }

    // FIXME: This should use the dynamic global scope (bug #27887)
    KURL scriptURL = scriptExecutionContext()->completeURL(url);
    if (!scriptURL.isValid()) {
        ec = SYNTAX_ERR;
        return KURL();
    }

    if (!scriptExecutionContext()->securityOrigin()->canAccess(SecurityOrigin::create(scriptURL).get())) {
        ec = SECURITY_ERR;
        return KURL();
    }
    return scriptURL;
}

EventTargetData* AbstractWorker::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* AbstractWorker::ensureEventTargetData()
{
    return &m_eventTargetData;
}

} // namespace WebCore

#endif // ENABLE(WORKERS)
