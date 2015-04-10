/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "SynchronousLoaderClient.h"

#include "AuthenticationChallenge.h"
#include "ResourceHandle.h"
#include "ResourceRequest.h"

#if USE(CFNETWORK)
#include <CFNetwork/CFURLConnectionPriv.h>
#endif

namespace WebCore {

SynchronousLoaderClient::~SynchronousLoaderClient()
{
}

void SynchronousLoaderClient::willSendRequest(ResourceHandle* handle, ResourceRequest& request, const ResourceResponse& /*redirectResponse*/)
{
    // FIXME: This needs to be fixed to follow the redirect correctly even for cross-domain requests.
    if (protocolHostAndPortAreEqual(handle->firstRequest().url(), request.url()))
        return;

    ASSERT(m_error.isNull());
    m_error = platformBadResponseError();
    m_isDone = true;
    request = 0;
}

bool SynchronousLoaderClient::shouldUseCredentialStorage(ResourceHandle*)
{
    // FIXME: We should ask FrameLoaderClient whether using credential storage is globally forbidden.
    return m_allowStoredCredentials;
}

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
bool SynchronousLoaderClient::canAuthenticateAgainstProtectionSpace(ResourceHandle*, const ProtectionSpace&)
{
    // FIXME: We should ask FrameLoaderClient. <http://webkit.org/b/65196>
    return true;
}
#endif

#if USE(CFNETWORK)
void SynchronousLoaderClient::didReceiveAuthenticationChallenge(ResourceHandle* handle, const AuthenticationChallenge& challenge)
{
    // FIXME: The user should be asked for credentials, as in async case.
    CFURLConnectionUseCredential(handle->connection(), 0, challenge.cfURLAuthChallengeRef());
}
#endif

void SynchronousLoaderClient::didReceiveResponse(ResourceHandle*, const ResourceResponse& response)
{
    m_response = response;
}

void SynchronousLoaderClient::didReceiveData(ResourceHandle*, const char* data, int length, int /*encodedDataLength*/)
{
    m_data.append(data, length);
}

void SynchronousLoaderClient::didFinishLoading(ResourceHandle*, double)
{
    m_isDone = true;
}

void SynchronousLoaderClient::didFail(ResourceHandle*, const ResourceError& error)
{
    ASSERT(m_error.isNull());

    m_error = error;
    m_isDone = true;
}

#if USE(CFNETWORK)
ResourceError SynchronousLoaderClient::platformBadResponseError()
{
    RetainPtr<CFErrorRef> cfError = adoptCF(CFErrorCreate(kCFAllocatorDefault, kCFErrorDomainCFNetwork, kCFURLErrorBadServerResponse, 0));
    return cfError.get();
}
#endif

}
