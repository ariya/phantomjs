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

#if ENABLE(BLOB)

#include "DOMURL.h"

#include "KURL.h"
#include "ScriptExecutionContext.h"

namespace WebCore {

DOMURL::DOMURL(ScriptExecutionContext* scriptExecutionContext)
    : m_scriptExecutionContext(scriptExecutionContext)
{
    if (m_scriptExecutionContext)
        m_scriptExecutionContext->createdDomUrl(this);
}

DOMURL::~DOMURL()
{
    if (m_scriptExecutionContext)
        m_scriptExecutionContext->destroyedDomUrl(this);
}

void DOMURL::contextDestroyed()
{
    ASSERT(m_scriptExecutionContext);
    m_scriptExecutionContext = 0;
}

String DOMURL::createObjectURL(Blob* blob)
{
    if (!m_scriptExecutionContext)
        return String();
    return m_scriptExecutionContext->createPublicBlobURL(blob).string();
}

void DOMURL::revokeObjectURL(const String& urlString)
{
    if (!m_scriptExecutionContext)
        return;
    m_scriptExecutionContext->revokePublicBlobURL(KURL(KURL(), urlString));
}

} // namespace WebCore

#endif // ENABLE(BLOB)
