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
#include "SchedulableLoader.h"

#if ENABLE(NETWORK_PROCESS)

#include "NetworkBlobRegistry.h"
#include "NetworkConnectionToWebProcess.h"
#include "NetworkResourceLoadParameters.h"
#include <WebCore/FormData.h>

using namespace WebCore;

namespace WebKit {

SchedulableLoader::SchedulableLoader(const NetworkResourceLoadParameters& parameters, NetworkConnectionToWebProcess* connection)
    : m_identifier(parameters.identifier)
    , m_webPageID(parameters.webPageID)
    , m_webFrameID(parameters.webFrameID)
    , m_request(parameters.request)
    , m_priority(parameters.priority)
    , m_contentSniffingPolicy(parameters.contentSniffingPolicy)
    , m_allowStoredCredentials(parameters.allowStoredCredentials)
    , m_clientCredentialPolicy(parameters.clientCredentialPolicy)
    , m_inPrivateBrowsingMode(parameters.inPrivateBrowsingMode)
    , m_shouldClearReferrerOnHTTPSToHTTPRedirect(parameters.shouldClearReferrerOnHTTPSToHTTPRedirect)
    , m_isLoadingMainResource(parameters.isMainResource)
    , m_sandboxExtensionsAreConsumed(false)
    , m_connection(connection)
{
    // Either this loader has both a webPageID and webFrameID, or it is not allowed to ask the client for authentication credentials.
    // FIXME: This is necessary because of the existence of EmptyFrameLoaderClient in WebCore.
    //        Once bug 116233 is resolved, this ASSERT can just be "m_webPageID && m_webFrameID"
    ASSERT((m_webPageID && m_webFrameID) || m_clientCredentialPolicy == DoNotAskClientForAnyCredentials);

    for (size_t i = 0, count = parameters.requestBodySandboxExtensions.size(); i < count; ++i) {
        if (RefPtr<SandboxExtension> extension = SandboxExtension::create(parameters.requestBodySandboxExtensions[i]))
            m_requestBodySandboxExtensions.append(extension);
    }

#if ENABLE(BLOB)
    if (m_request.httpBody()) {
        const Vector<FormDataElement>& elements = m_request.httpBody()->elements();
        for (size_t i = 0, count = elements.size(); i < count; ++i) {
            if (elements[i].m_type == FormDataElement::encodedBlob) {
                Vector<RefPtr<SandboxExtension>> blobElementExtensions = NetworkBlobRegistry::shared().sandboxExtensions(elements[i].m_url);
                m_requestBodySandboxExtensions.appendVector(blobElementExtensions);
            }
        }
    }

    if (m_request.url().protocolIs("blob")) {
        ASSERT(!SandboxExtension::create(parameters.resourceSandboxExtension));
        m_resourceSandboxExtensions = NetworkBlobRegistry::shared().sandboxExtensions(m_request.url());
    } else
#endif
    if (RefPtr<SandboxExtension> resourceSandboxExtension = SandboxExtension::create(parameters.resourceSandboxExtension))
        m_resourceSandboxExtensions.append(resourceSandboxExtension);
}

SchedulableLoader::~SchedulableLoader()
{
    ASSERT(!m_hostRecord);
}

void SchedulableLoader::consumeSandboxExtensions()
{
    for (size_t i = 0, count = m_requestBodySandboxExtensions.size(); i < count; ++i)
        m_requestBodySandboxExtensions[i]->consume();

    for (size_t i = 0, count = m_resourceSandboxExtensions.size(); i < count; ++i)
        m_resourceSandboxExtensions[i]->consume();

    m_sandboxExtensionsAreConsumed = true;
}

void SchedulableLoader::invalidateSandboxExtensions()
{
    if (m_sandboxExtensionsAreConsumed) {
        for (size_t i = 0, count = m_requestBodySandboxExtensions.size(); i < count; ++i)
            m_requestBodySandboxExtensions[i]->revoke();
        for (size_t i = 0, count = m_resourceSandboxExtensions.size(); i < count; ++i)
            m_resourceSandboxExtensions[i]->revoke();
    }

    m_requestBodySandboxExtensions.clear();
    m_resourceSandboxExtensions.clear();

    m_sandboxExtensionsAreConsumed = false;
}

} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)
