/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009, 2011 Google Inc. All Rights Reserved.
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

#if ENABLE(WORKERS)

#include "WorkerScriptLoader.h"

#include "CrossThreadTask.h"
#include "ResourceRequest.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"
#include "WorkerContext.h"
#include "WorkerScriptLoaderClient.h"
#include "WorkerThreadableLoader.h"
#include <wtf/OwnPtr.h>
#include <wtf/UnusedParam.h>

namespace WebCore {

WorkerScriptLoader::WorkerScriptLoader(ResourceRequestBase::TargetType targetType)
    : m_client(0)
    , m_failed(false)
    , m_identifier(0)
    , m_targetType(targetType)
{
}

void WorkerScriptLoader::loadSynchronously(ScriptExecutionContext* scriptExecutionContext, const KURL& url, CrossOriginRequestPolicy crossOriginRequestPolicy)
{
    m_url = url;

    OwnPtr<ResourceRequest> request(createResourceRequest());
    if (!request)
        return;

    ASSERT(scriptExecutionContext->isWorkerContext());

    ThreadableLoaderOptions options;
    options.allowCredentials = true;
    options.crossOriginRequestPolicy = crossOriginRequestPolicy;
    options.sendLoadCallbacks = true;

    WorkerThreadableLoader::loadResourceSynchronously(static_cast<WorkerContext*>(scriptExecutionContext), *request, *this, options);
}
    
void WorkerScriptLoader::loadAsynchronously(ScriptExecutionContext* scriptExecutionContext, const KURL& url, CrossOriginRequestPolicy crossOriginRequestPolicy, WorkerScriptLoaderClient* client)
{
    ASSERT(client);
    m_client = client;
    m_url = url;

    OwnPtr<ResourceRequest> request(createResourceRequest());
    if (!request)
        return;

    ThreadableLoaderOptions options;
    options.allowCredentials = true;
    options.crossOriginRequestPolicy = crossOriginRequestPolicy;
    options.sendLoadCallbacks = true;

    m_threadableLoader = ThreadableLoader::create(scriptExecutionContext, this, *request, options);
}

const KURL& WorkerScriptLoader::responseURL() const
{
    ASSERT(!failed());
    return m_responseURL;
}

PassOwnPtr<ResourceRequest> WorkerScriptLoader::createResourceRequest()
{
    OwnPtr<ResourceRequest> request = adoptPtr(new ResourceRequest(m_url));
    request->setHTTPMethod("GET");
    request->setTargetType(m_targetType);
    return request.release();
}
    
void WorkerScriptLoader::didReceiveResponse(const ResourceResponse& response)
{
    if (response.httpStatusCode() / 100 != 2 && response.httpStatusCode()) {
        m_failed = true;
        return;
    }
    m_responseURL = response.url();
    m_responseEncoding = response.textEncodingName();
    if (m_client)
        m_client->didReceiveResponse(response);
}

void WorkerScriptLoader::didReceiveData(const char* data, int len)
{
    if (m_failed)
        return;

    if (!m_decoder) {
        if (!m_responseEncoding.isEmpty())
            m_decoder = TextResourceDecoder::create("text/javascript", m_responseEncoding);
        else
            m_decoder = TextResourceDecoder::create("text/javascript", "UTF-8");
    }

    if (!len)
        return;
    
    if (len == -1)
        len = strlen(data);
    
    m_script += m_decoder->decode(data, len);
}

void WorkerScriptLoader::didFinishLoading(unsigned long identifier, double)
{
    if (m_failed)
        return;

    if (m_decoder)
        m_script += m_decoder->flush();

    m_identifier = identifier;
    notifyFinished();
}

void WorkerScriptLoader::didFail(const ResourceError&)
{
    notifyError();
}

void WorkerScriptLoader::didFailRedirectCheck()
{
    notifyError();
}

void WorkerScriptLoader::didReceiveAuthenticationCancellation(const ResourceResponse&)
{
    notifyError();
}

void WorkerScriptLoader::notifyError()
{
    m_failed = true;
    notifyFinished();
}
    
void WorkerScriptLoader::notifyFinished()
{
    if (m_client)
        m_client->notifyFinished();
}

} // namespace WebCore

#endif // ENABLE(WORKERS)
