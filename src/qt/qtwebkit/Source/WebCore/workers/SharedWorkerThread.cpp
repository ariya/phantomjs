/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#if ENABLE(SHARED_WORKERS)

#include "SecurityOrigin.h"
#include "SharedWorkerThread.h"
#include "SharedWorkerGlobalScope.h"

namespace WebCore {

PassRefPtr<SharedWorkerThread> SharedWorkerThread::create(const String& name, const KURL& scriptURL, const String& userAgent, const GroupSettings* settings, const String& sourceCode, WorkerLoaderProxy& workerLoaderProxy, WorkerReportingProxy& workerReportingProxy, WorkerThreadStartMode startMode, const String& contentSecurityPolicy, ContentSecurityPolicy::HeaderType contentSecurityPolicyType)
{
    return adoptRef(new SharedWorkerThread(name, scriptURL, userAgent, settings, sourceCode, workerLoaderProxy, workerReportingProxy, startMode, contentSecurityPolicy, contentSecurityPolicyType));
}

SharedWorkerThread::SharedWorkerThread(const String& name, const KURL& url, const String& userAgent, const GroupSettings* settings, const String& sourceCode, WorkerLoaderProxy& workerLoaderProxy, WorkerReportingProxy& workerReportingProxy, WorkerThreadStartMode startMode, const String& contentSecurityPolicy, ContentSecurityPolicy::HeaderType contentSecurityPolicyType)
    : WorkerThread(url, userAgent, settings, sourceCode, workerLoaderProxy, workerReportingProxy, startMode, contentSecurityPolicy, contentSecurityPolicyType, 0)
    , m_name(name.isolatedCopy())
{
}

SharedWorkerThread::~SharedWorkerThread()
{
}

PassRefPtr<WorkerGlobalScope> SharedWorkerThread::createWorkerGlobalScope(const KURL& url, const String& userAgent, PassOwnPtr<GroupSettings> settings, const String& contentSecurityPolicy, ContentSecurityPolicy::HeaderType contentSecurityPolicyType, PassRefPtr<SecurityOrigin>)
{
    return SharedWorkerGlobalScope::create(m_name, url, userAgent, settings, this, contentSecurityPolicy, contentSecurityPolicyType);
}

} // namespace WebCore

#endif // ENABLE(SHARED_WORKERS)
