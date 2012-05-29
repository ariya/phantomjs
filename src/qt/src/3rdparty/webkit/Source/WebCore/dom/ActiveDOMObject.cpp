/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 *
 */

#include "config.h"
#include "ActiveDOMObject.h"

#include "ScriptExecutionContext.h"
#include "WorkerContext.h"
#include "WorkerThread.h"

namespace WebCore {

ActiveDOMObject::ActiveDOMObject(ScriptExecutionContext* scriptExecutionContext, void* upcastPointer)
    : m_scriptExecutionContext(scriptExecutionContext)
    , m_pendingActivityCount(0)
{
    if (m_scriptExecutionContext) {
        ASSERT(m_scriptExecutionContext->isContextThread());
        m_scriptExecutionContext->createdActiveDOMObject(this, upcastPointer);
    }
}

ActiveDOMObject::~ActiveDOMObject()
{
    if (m_scriptExecutionContext) {
        ASSERT(m_scriptExecutionContext->isContextThread());
        m_scriptExecutionContext->destroyedActiveDOMObject(this);
    }
}

bool ActiveDOMObject::hasPendingActivity() const
{
    return m_pendingActivityCount;
}

void ActiveDOMObject::contextDestroyed()
{
    m_scriptExecutionContext = 0;
}

bool ActiveDOMObject::canSuspend() const
{
    return false;
}

void ActiveDOMObject::suspend(ReasonForSuspension)
{
}

void ActiveDOMObject::resume()
{
}

void ActiveDOMObject::stop()
{
}

} // namespace WebCore
