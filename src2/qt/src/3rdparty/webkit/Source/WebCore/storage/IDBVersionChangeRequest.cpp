/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "IDBVersionChangeRequest.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBVersionChangeEvent.h"
#include "ScriptExecutionContext.h"

namespace WebCore {

PassRefPtr<IDBVersionChangeRequest> IDBVersionChangeRequest::create(ScriptExecutionContext* context, PassRefPtr<IDBAny> source, const String& version)
{
    return adoptRef(new IDBVersionChangeRequest(context, source, version));
}

IDBVersionChangeRequest::IDBVersionChangeRequest(ScriptExecutionContext* context, PassRefPtr<IDBAny> source, const String& version)
    : IDBRequest(context, source, 0)
    , m_version(version)
{
}

IDBVersionChangeRequest::~IDBVersionChangeRequest()
{
}

void IDBVersionChangeRequest::onBlocked()
{
    ASSERT(!m_errorCode && m_errorMessage.isNull() && !m_result);
    enqueueEvent(IDBVersionChangeEvent::create(m_version, eventNames().blockedEvent));
}

} // namespace WebCore

#endif
