/*
 * Copyright (C) 2011, 2012 Google Inc. All rights reserved.
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
#include "DocumentThreadableLoader.h"

#include "CachedRawResource.h"
#include "CachedResourceLoader.h"
#include "CachedResourceRequest.h"
#include "CrossOriginAccessControl.h"
#include "CrossOriginPreflightResultCache.h"
#include "Document.h"
#include "DocumentThreadableLoaderClient.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "InspectorInstrumentation.h"
#include "ResourceError.h"
#include "ResourceRequest.h"
#include "SchemeRegistry.h"
#include "SecurityOrigin.h"
#include "SubresourceLoader.h"
#include "ThreadableLoaderClient.h"
#include <wtf/Assertions.h>

#if ENABLE(INSPECTOR)
#include "ProgressTracker.h"
#endif

namespace WebCore {

void DocumentThreadableLoader::loadResourceSynchronously(Document* document, const ResourceRequest& request, ThreadableLoaderClient& client, const ThreadableLoaderOptions& options)
{
    // The loader will be deleted as soon as this function exits.
    RefPtr<DocumentThreadableLoader> loader = adoptRef(new DocumentThreadableLoader(document, &client, LoadSynchronously, request, options));
    ASSERT(loader->hasOneRef());
}

PassRefPtr<DocumentThreadableLoader> DocumentThreadableLoader::create(Document* document, ThreadableLoaderClient* client, const ResourceRequest& request, const ThreadableLoaderOptions& options)
{
    RefPtr<DocumentThreadableLoader> loader = adoptRef(new DocumentThreadableLoader(document, client, LoadAsynchronously, request, options));
    if (!loader->m_resource)
        loader = 0;
    return loader.release();
}

DocumentThreadableLoader::DocumentThreadableLoader(Document* document, ThreadableLoaderClient* client, BlockingBehavior blockingBehavior, const ResourceRequest& request, const ThreadableLoaderOptions& options)
    : m_client(client)
    , m_document(document)
    , m_options(options)
    , m_sameOriginRequest(securityOrigin()->canRequest(request.url()))
    , m_simpleRequest(true)
    , m_async(blockingBehavior == LoadAsynchronously)
{
    ASSERT(document);
    ASSERT(client);
    // Setting an outgoing referer is only supported in the async code path.
    ASSERT(m_async || request.httpReferrer().isEmpty());

    if (m_sameOriginRequest || m_options.crossOriginRequestPolicy == AllowCrossOriginRequests) {
        loadRequest(request, DoSecurityCheck);
        return;
    }

    if (m_options.crossOriginRequestPolicy == DenyCrossOriginRequests) {
        m_client->didFail(ResourceError(errorDomainWebKitInternal, 0, request.url().string(), "Cross origin requests are not supported."));
        return;
    }

    makeCrossOriginAccessRequest(request);
}

void DocumentThreadableLoader::makeCrossOriginAccessRequest(const ResourceRequest& request)
{
    ASSERT(m_options.crossOriginRequestPolicy == UseAccessControl);

    OwnPtr<ResourceRequest> crossOriginRequest = adoptPtr(new ResourceRequest(request));
    updateRequestForAccessControl(*crossOriginRequest, securityOrigin(), m_options.allowCredentials);

    if ((m_options.preflightPolicy == ConsiderPreflight && isSimpleCrossOriginAccessRequest(crossOriginRequest->httpMethod(), crossOriginRequest->httpHeaderFields())) || m_options.preflightPolicy == PreventPreflight)
        makeSimpleCrossOriginAccessRequest(*crossOriginRequest);
    else {
        m_simpleRequest = false;
        m_actualRequest = crossOriginRequest.release();

        if (CrossOriginPreflightResultCache::shared().canSkipPreflight(securityOrigin()->toString(), m_actualRequest->url(), m_options.allowCredentials, m_actualRequest->httpMethod(), m_actualRequest->httpHeaderFields()))
            preflightSuccess();
        else
            makeCrossOriginAccessRequestWithPreflight(*m_actualRequest);
    }
}

void DocumentThreadableLoader::makeSimpleCrossOriginAccessRequest(const ResourceRequest& request)
{
    ASSERT(m_options.preflightPolicy != ForcePreflight);
    ASSERT(m_options.preflightPolicy == PreventPreflight || isSimpleCrossOriginAccessRequest(request.httpMethod(), request.httpHeaderFields()));

    // Cross-origin requests are only allowed for HTTP and registered schemes. We would catch this when checking response headers later, but there is no reason to send a request that's guaranteed to be denied.
    if (!SchemeRegistry::shouldTreatURLSchemeAsCORSEnabled(request.url().protocol())) {
        m_client->didFailAccessControlCheck(ResourceError(errorDomainWebKitInternal, 0, request.url().string(), "Cross origin requests are only supported for HTTP."));
        return;
    }

    loadRequest(request, DoSecurityCheck);
}

void DocumentThreadableLoader::makeCrossOriginAccessRequestWithPreflight(const ResourceRequest& request)
{
    ResourceRequest preflightRequest = createAccessControlPreflightRequest(request, securityOrigin());
    loadRequest(preflightRequest, DoSecurityCheck);
}

DocumentThreadableLoader::~DocumentThreadableLoader()
{
    if (m_resource)
        m_resource->removeClient(this);
}

void DocumentThreadableLoader::cancel()
{
    RefPtr<DocumentThreadableLoader> protect(this);

    // Cancel can re-enter and m_resource might be null here as a result.
    if (m_client && m_resource) {
        // FIXME: This error is sent to the client in didFail(), so it should not be an internal one. Use FrameLoaderClient::cancelledError() instead.
        ResourceError error(errorDomainWebKitInternal, 0, m_resource->url(), "Load cancelled");
        error.setIsCancellation(true);
        didFail(m_resource->identifier(), error);
    }
    clearResource();
    m_client = 0;
}

void DocumentThreadableLoader::setDefersLoading(bool value)
{
    if (m_resource)
        m_resource->setDefersLoading(value);
}

void DocumentThreadableLoader::clearResource()
{
    // Script can cancel and restart a request reentrantly within removeClient(),
    // which could lead to calling CachedResource::removeClient() multiple times for
    // this DocumentThreadableLoader. Save off a copy of m_resource and clear it to
    // prevent the reentrancy.
    if (CachedResourceHandle<CachedRawResource> resource = m_resource) {
        m_resource = 0;
        resource->removeClient(this);
    }
}

void DocumentThreadableLoader::redirectReceived(CachedResource* resource, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    ASSERT(m_client);
    ASSERT_UNUSED(resource, resource == m_resource);

    RefPtr<DocumentThreadableLoader> protect(this);
    // Allow same origin requests to continue after allowing clients to audit the redirect.
    if (isAllowedRedirect(request.url())) {
        if (m_client->isDocumentThreadableLoaderClient())
            static_cast<DocumentThreadableLoaderClient*>(m_client)->willSendRequest(request, redirectResponse);
        return;
    }

    // When using access control, only simple cross origin requests are allowed to redirect. The new request URL must have a supported
    // scheme and not contain the userinfo production. In addition, the redirect response must pass the access control check.
    if (m_options.crossOriginRequestPolicy == UseAccessControl) {
        bool allowRedirect = false;
        if (m_simpleRequest) {
            String accessControlErrorDescription;
            allowRedirect = SchemeRegistry::shouldTreatURLSchemeAsCORSEnabled(request.url().protocol())
                            && request.url().user().isEmpty()
                            && request.url().pass().isEmpty()
                            && passesAccessControlCheck(redirectResponse, m_options.allowCredentials, securityOrigin(), accessControlErrorDescription);
        }

        if (allowRedirect) {
            if (m_resource)
                clearResource();

            RefPtr<SecurityOrigin> originalOrigin = SecurityOrigin::createFromString(redirectResponse.url());
            RefPtr<SecurityOrigin> requestOrigin = SecurityOrigin::createFromString(request.url());
            // If the request URL origin is not same origin with the original URL origin, set source origin to a globally unique identifier.
            if (!originalOrigin->isSameSchemeHostPort(requestOrigin.get()))
                m_options.securityOrigin = SecurityOrigin::createUnique();
            // Force any subsequent requests to use these checks.
            m_sameOriginRequest = false;

            // Remove any headers that may have been added by the network layer that cause access control to fail.
            request.clearHTTPContentType();
            request.clearHTTPReferrer();
            request.clearHTTPOrigin();
            request.clearHTTPUserAgent();
            request.clearHTTPAccept();
            makeCrossOriginAccessRequest(request);
            return;
        }
    }

    m_client->didFailRedirectCheck();
    request = ResourceRequest();
}

void DocumentThreadableLoader::dataSent(CachedResource* resource, unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    ASSERT(m_client);
    ASSERT_UNUSED(resource, resource == m_resource);
    m_client->didSendData(bytesSent, totalBytesToBeSent);
}

void DocumentThreadableLoader::responseReceived(CachedResource* resource, const ResourceResponse& response)
{
    ASSERT_UNUSED(resource, resource == m_resource);
    didReceiveResponse(m_resource->identifier(), response);
}

void DocumentThreadableLoader::didReceiveResponse(unsigned long identifier, const ResourceResponse& response)
{
    ASSERT(m_client);

    String accessControlErrorDescription;
    if (m_actualRequest) {
#if ENABLE(INSPECTOR)
        DocumentLoader* loader = m_document->frame()->loader()->documentLoader();
        InspectorInstrumentationCookie cookie = InspectorInstrumentation::willReceiveResourceResponse(m_document->frame(), identifier, response);
        InspectorInstrumentation::didReceiveResourceResponse(cookie, identifier, loader, response, 0);
#endif

        if (!passesAccessControlCheck(response, m_options.allowCredentials, securityOrigin(), accessControlErrorDescription)) {
            preflightFailure(identifier, response.url(), accessControlErrorDescription);
            return;
        }

        OwnPtr<CrossOriginPreflightResultCacheItem> preflightResult = adoptPtr(new CrossOriginPreflightResultCacheItem(m_options.allowCredentials));
        if (!preflightResult->parse(response, accessControlErrorDescription)
            || !preflightResult->allowsCrossOriginMethod(m_actualRequest->httpMethod(), accessControlErrorDescription)
            || !preflightResult->allowsCrossOriginHeaders(m_actualRequest->httpHeaderFields(), accessControlErrorDescription)) {
            preflightFailure(identifier, response.url(), accessControlErrorDescription);
            return;
        }

        CrossOriginPreflightResultCache::shared().appendEntry(securityOrigin()->toString(), m_actualRequest->url(), preflightResult.release());
    } else {
        if (!m_sameOriginRequest && m_options.crossOriginRequestPolicy == UseAccessControl) {
            if (!passesAccessControlCheck(response, m_options.allowCredentials, securityOrigin(), accessControlErrorDescription)) {
                m_client->didFailAccessControlCheck(ResourceError(errorDomainWebKitInternal, 0, response.url().string(), accessControlErrorDescription));
                return;
            }
        }

        m_client->didReceiveResponse(identifier, response);
    }
}

void DocumentThreadableLoader::dataReceived(CachedResource* resource, const char* data, int dataLength)
{
    ASSERT_UNUSED(resource, resource == m_resource);
    didReceiveData(m_resource->identifier(), data, dataLength);
}

void DocumentThreadableLoader::didReceiveData(unsigned long identifier, const char* data, int dataLength)
{
    ASSERT(m_client);

    // Preflight data should be invisible to clients.
    if (m_actualRequest) {
#if ENABLE(INSPECTOR)
        InspectorInstrumentation::didReceiveData(m_document->frame(), identifier, 0, 0, dataLength);
#endif
        return;
    }

    m_client->didReceiveData(data, dataLength);
}

void DocumentThreadableLoader::notifyFinished(CachedResource* resource)
{
    ASSERT(m_client);
    ASSERT_UNUSED(resource, resource == m_resource);
        
    if (m_resource->errorOccurred())
        didFail(m_resource->identifier(), m_resource->resourceError());
    else
        didFinishLoading(m_resource->identifier(), m_resource->loadFinishTime());
}

void DocumentThreadableLoader::didFinishLoading(unsigned long identifier, double finishTime)
{
    if (m_actualRequest) {
#if ENABLE(INSPECTOR)
        InspectorInstrumentation::didFinishLoading(m_document->frame(), m_document->frame()->loader()->documentLoader(), identifier, finishTime);
#endif
        ASSERT(!m_sameOriginRequest);
        ASSERT(m_options.crossOriginRequestPolicy == UseAccessControl);
        preflightSuccess();
    } else
        m_client->didFinishLoading(identifier, finishTime);
}

void DocumentThreadableLoader::didFail(unsigned long identifier, const ResourceError& error)
{
#if ENABLE(INSPECTOR)
    if (m_actualRequest)
        InspectorInstrumentation::didFailLoading(m_document->frame(), m_document->frame()->loader()->documentLoader(), identifier, error);
#endif

    m_client->didFail(error);
}

void DocumentThreadableLoader::preflightSuccess()
{
    OwnPtr<ResourceRequest> actualRequest;
    actualRequest.swap(m_actualRequest);

    actualRequest->setHTTPOrigin(securityOrigin()->toString());

    clearResource();

    // It should be ok to skip the security check since we already asked about the preflight request.
    loadRequest(*actualRequest, SkipSecurityCheck);
}

void DocumentThreadableLoader::preflightFailure(unsigned long identifier, const String& url, const String& errorDescription)
{
    ResourceError error(errorDomainWebKitInternal, 0, url, errorDescription);
#if ENABLE(INSPECTOR)
    if (m_actualRequest)
        InspectorInstrumentation::didFailLoading(m_document->frame(), m_document->frame()->loader()->documentLoader(), identifier, error);
#endif
    m_actualRequest = nullptr; // Prevent didFinishLoading() from bypassing access check.
    m_client->didFailAccessControlCheck(error);
}

void DocumentThreadableLoader::loadRequest(const ResourceRequest& request, SecurityCheckPolicy securityCheck)
{
    // Any credential should have been removed from the cross-site requests.
    const KURL& requestURL = request.url();
    m_options.securityCheck = securityCheck;
    ASSERT(m_sameOriginRequest || requestURL.user().isEmpty());
    ASSERT(m_sameOriginRequest || requestURL.pass().isEmpty());

    if (m_async) {
        ThreadableLoaderOptions options = m_options;
        options.clientCredentialPolicy = DoNotAskClientForCrossOriginCredentials;
        if (m_actualRequest) {
            // Don't sniff content or send load callbacks for the preflight request.
            options.sendLoadCallbacks = DoNotSendCallbacks;
            options.sniffContent = DoNotSniffContent;
            // Keep buffering the data for the preflight request.
            options.dataBufferingPolicy = BufferData;
        }

        CachedResourceRequest newRequest(request, options);
#if ENABLE(RESOURCE_TIMING)
        newRequest.setInitiator(m_options.initiator);
#endif
        ASSERT(!m_resource);
        m_resource = m_document->cachedResourceLoader()->requestRawResource(newRequest);
        if (m_resource) {
#if ENABLE(INSPECTOR)
            if (m_resource->loader()) {
                unsigned long identifier = m_resource->loader()->identifier();
                InspectorInstrumentation::documentThreadableLoaderStartedLoadingForClient(m_document, identifier, m_client);
            }
#endif
            m_resource->addClient(this);
        }
        return;
    }
    
    // FIXME: ThreadableLoaderOptions.sniffContent is not supported for synchronous requests.
    Vector<char> data;
    ResourceError error;
    ResourceResponse response;
    unsigned long identifier = std::numeric_limits<unsigned long>::max();
    if (m_document->frame())
        identifier = m_document->frame()->loader()->loadResourceSynchronously(request, m_options.allowCredentials, m_options.clientCredentialPolicy, error, response, data);

    InspectorInstrumentation::documentThreadableLoaderStartedLoadingForClient(m_document, identifier, m_client);

    // No exception for file:/// resources, see <rdar://problem/4962298>.
    // Also, if we have an HTTP response, then it wasn't a network error in fact.
    if (!error.isNull() && !requestURL.isLocalFile() && response.httpStatusCode() <= 0) {
        m_client->didFail(error);
        return;
    }

    // FIXME: FrameLoader::loadSynchronously() does not tell us whether a redirect happened or not, so we guess by comparing the
    // request and response URLs. This isn't a perfect test though, since a server can serve a redirect to the same URL that was
    // requested. Also comparing the request and response URLs as strings will fail if the requestURL still has its credentials.
    if (requestURL != response.url() && !isAllowedRedirect(response.url())) {
        m_client->didFailRedirectCheck();
        return;
    }

    didReceiveResponse(identifier, response);

    const char* bytes = static_cast<const char*>(data.data());
    int len = static_cast<int>(data.size());
    didReceiveData(identifier, bytes, len);

    didFinishLoading(identifier, 0.0);
}

bool DocumentThreadableLoader::isAllowedRedirect(const KURL& url)
{
    if (m_options.crossOriginRequestPolicy == AllowCrossOriginRequests)
        return true;

    return m_sameOriginRequest && securityOrigin()->canRequest(url);
}

SecurityOrigin* DocumentThreadableLoader::securityOrigin() const
{
    return m_options.securityOrigin ? m_options.securityOrigin.get() : m_document->securityOrigin();
}

} // namespace WebCore
