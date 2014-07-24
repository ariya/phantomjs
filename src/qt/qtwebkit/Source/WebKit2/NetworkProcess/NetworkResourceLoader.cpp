/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "NetworkResourceLoader.h"

#if ENABLE(NETWORK_PROCESS)

#include "AuthenticationManager.h"
#include "DataReference.h"
#include "Logging.h"
#include "NetworkConnectionToWebProcess.h"
#include "NetworkProcess.h"
#include "NetworkProcessConnectionMessages.h"
#include "NetworkResourceLoadParameters.h"
#include "PlatformCertificateInfo.h"
#include "RemoteNetworkingContext.h"
#include "ShareableResource.h"
#include "SharedMemory.h"
#include "WebCoreArgumentCoders.h"
#include "WebErrors.h"
#include "WebResourceLoaderMessages.h"
#include <WebCore/NotImplemented.h>
#include <WebCore/ResourceBuffer.h>
#include <WebCore/ResourceHandle.h>
#include <wtf/CurrentTime.h>
#include <wtf/MainThread.h>

using namespace WebCore;

namespace WebKit {

NetworkResourceLoader::NetworkResourceLoader(const NetworkResourceLoadParameters& loadParameters, NetworkConnectionToWebProcess* connection)
    : SchedulableLoader(loadParameters, connection)
    , m_bytesReceived(0)
    , m_handleConvertedToDownload(false)
{
    ASSERT(isMainThread());
}

NetworkResourceLoader::~NetworkResourceLoader()
{
    ASSERT(isMainThread());
    ASSERT(!m_handle);
}

void NetworkResourceLoader::start()
{
    ASSERT(isMainThread());

    // Explicit ref() balanced by a deref() in NetworkResourceLoader::resourceHandleStopped()
    ref();
    
    // FIXME (NetworkProcess): Create RemoteNetworkingContext with actual settings.
    m_networkingContext = RemoteNetworkingContext::create(false, false, inPrivateBrowsingMode(), shouldClearReferrerOnHTTPSToHTTPRedirect());

    consumeSandboxExtensions();

    // FIXME (NetworkProcess): Pass an actual value for defersLoading
    m_handle = ResourceHandle::create(m_networkingContext.get(), request(), this, false /* defersLoading */, contentSniffingPolicy() == SniffContent);
}

void NetworkResourceLoader::cleanup()
{
    ASSERT(isMainThread());

    invalidateSandboxExtensions();

    if (FormData* formData = request().httpBody())
        formData->removeGeneratedFilesIfNeeded();

    // Tell the scheduler about this finished loader soon so it can start more network requests.
    NetworkProcess::shared().networkResourceLoadScheduler().scheduleRemoveLoader(this);

    if (m_handle) {
        // Explicit deref() balanced by a ref() in NetworkResourceLoader::start()
        // This might cause the NetworkResourceLoader to be destroyed and therefore we do it last.
        m_handle = 0;
        deref();
    }
}

template<typename U> bool NetworkResourceLoader::sendAbortingOnFailure(const U& message, unsigned messageSendFlags)
{
    bool result = messageSenderConnection()->send(message, messageSenderDestinationID(), messageSendFlags);
    if (!result)
        abort();
    return result;
}

void NetworkResourceLoader::didConvertHandleToDownload()
{
    ASSERT(m_handle);
    m_handleConvertedToDownload = true;
}

void NetworkResourceLoader::abort()
{
    ASSERT(isMainThread());

    if (m_handle && !m_handleConvertedToDownload)
        m_handle->cancel();

    cleanup();
}

void NetworkResourceLoader::didReceiveResponseAsync(ResourceHandle* handle, const ResourceResponse& response)
{
    ASSERT_UNUSED(handle, handle == m_handle);

    // FIXME (NetworkProcess): Cache the response.
    if (FormData* formData = request().httpBody())
        formData->removeGeneratedFilesIfNeeded();

    sendAbortingOnFailure(Messages::WebResourceLoader::DidReceiveResponseWithCertificateInfo(response, PlatformCertificateInfo(response), isLoadingMainResource()));

    // m_handle will be 0 if the request got aborted above.
    if (!m_handle)
        return;

    if (!isLoadingMainResource()) {
        // For main resources, the web process is responsible for sending back a NetworkResourceLoader::ContinueDidReceiveResponse message.
        m_handle->continueDidReceiveResponse();
    }
}

void NetworkResourceLoader::didReceiveData(ResourceHandle*, const char* data, int length, int encodedDataLength)
{
    // The NetworkProcess should never get a didReceiveData callback.
    // We should always be using didReceiveBuffer.
    ASSERT_NOT_REACHED();
}

void NetworkResourceLoader::didReceiveBuffer(ResourceHandle* handle, PassRefPtr<SharedBuffer> buffer, int encodedDataLength)
{
    ASSERT_UNUSED(handle, handle == m_handle);

    // FIXME (NetworkProcess): For the memory cache we'll also need to cache the response data here.
    // Such buffering will need to be thread safe, as this callback is happening on a background thread.
    
    m_bytesReceived += buffer->size();
    
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    ShareableResource::Handle shareableResourceHandle;
    tryGetShareableHandleFromSharedBuffer(shareableResourceHandle, buffer.get());
    if (!shareableResourceHandle.isNull()) {
        // Since we're delivering this resource by ourselves all at once, we'll abort the resource handle since we don't need anymore callbacks from ResourceHandle.
        abort();
        send(Messages::WebResourceLoader::DidReceiveResource(shareableResourceHandle, currentTime()));
        return;
    }
#endif // __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090

    CoreIPC::DataReference dataReference(reinterpret_cast<const uint8_t*>(buffer->data()), buffer->size());
    sendAbortingOnFailure(Messages::WebResourceLoader::DidReceiveData(dataReference, encodedDataLength));
}

void NetworkResourceLoader::didFinishLoading(ResourceHandle* handle, double finishTime)
{
    ASSERT_UNUSED(handle, handle == m_handle);

    // FIXME (NetworkProcess): For the memory cache we'll need to update the finished status of the cached resource here.
    // Such bookkeeping will need to be thread safe, as this callback is happening on a background thread.
    send(Messages::WebResourceLoader::DidFinishResourceLoad(finishTime));
    
    cleanup();
}

void NetworkResourceLoader::didFail(ResourceHandle* handle, const ResourceError& error)
{
    ASSERT_UNUSED(handle, handle == m_handle);

    // FIXME (NetworkProcess): For the memory cache we'll need to update the finished status of the cached resource here.
    // Such bookkeeping will need to be thread safe, as this callback is happening on a background thread.
    send(Messages::WebResourceLoader::DidFailResourceLoad(error));
    cleanup();
}

void NetworkResourceLoader::willSendRequestAsync(ResourceHandle* handle, const ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    ASSERT_UNUSED(handle, handle == m_handle);

    // We only expect to get the willSendRequest callback from ResourceHandle as the result of a redirect.
    ASSERT(!redirectResponse.isNull());
    ASSERT(isMainThread());

    m_suggestedRequestForWillSendRequest = request;

    // This message is DispatchMessageEvenWhenWaitingForSyncReply to avoid a situation where the NetworkProcess is deadlocked waiting for 6 connections
    // to complete while the WebProcess is waiting for a 7th to complete.
    sendAbortingOnFailure(Messages::WebResourceLoader::WillSendRequest(request, redirectResponse), CoreIPC::DispatchMessageEvenWhenWaitingForSyncReply);
}

void NetworkResourceLoader::continueWillSendRequest(const ResourceRequest& newRequest)
{
    m_suggestedRequestForWillSendRequest.updateFromDelegatePreservingOldHTTPBody(newRequest.nsURLRequest(DoNotUpdateHTTPBody));

    RunLoop::main()->dispatch(bind(&NetworkResourceLoadScheduler::receivedRedirect, &NetworkProcess::shared().networkResourceLoadScheduler(), this, m_suggestedRequestForWillSendRequest.url()));
    m_handle->continueWillSendRequest(m_suggestedRequestForWillSendRequest);

    m_suggestedRequestForWillSendRequest = ResourceRequest();
}

void NetworkResourceLoader::continueDidReceiveResponse()
{
    // FIXME: Remove this check once BlobResourceHandle implements didReceiveResponseAsync correctly.
    // Currently, it does not wait for response, so the load is likely to finish before continueDidReceiveResponse.
    if (!m_handle)
        return;

    m_handle->continueDidReceiveResponse();
}

void NetworkResourceLoader::didSendData(ResourceHandle* handle, unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    ASSERT_UNUSED(handle, handle == m_handle);

    send(Messages::WebResourceLoader::DidSendData(bytesSent, totalBytesToBeSent));
}

void NetworkResourceLoader::wasBlocked(ResourceHandle* handle)
{
    ASSERT_UNUSED(handle, handle == m_handle);

    didFail(handle, WebKit::blockedError(request()));
}

void NetworkResourceLoader::cannotShowURL(ResourceHandle* handle)
{
    ASSERT_UNUSED(handle, handle == m_handle);

    didFail(handle, WebKit::cannotShowURLError(request()));
}

bool NetworkResourceLoader::shouldUseCredentialStorage(ResourceHandle* handle)
{
    ASSERT_UNUSED(handle, handle == m_handle || !m_handle); // m_handle will be 0 if called from ResourceHandle::start().

    // When the WebProcess is handling loading a client is consulted each time this shouldUseCredentialStorage question is asked.
    // In NetworkProcess mode we ask the WebProcess client up front once and then reuse the cached answer.

    // We still need this sync version, because ResourceHandle itself uses it internally, even when the delegate uses an async one.

    return allowStoredCredentials() == AllowStoredCredentials;
}

void NetworkResourceLoader::shouldUseCredentialStorageAsync(ResourceHandle* handle)
{
    ASSERT_UNUSED(handle, handle == m_handle);

    handle->continueShouldUseCredentialStorage(shouldUseCredentialStorage(handle));
}

void NetworkResourceLoader::didReceiveAuthenticationChallenge(ResourceHandle* handle, const AuthenticationChallenge& challenge)
{
    ASSERT_UNUSED(handle, handle == m_handle);

    // FIXME (http://webkit.org/b/115291): Since we go straight to the UI process for authentication we don't get WebCore's
    // cross-origin check before asking the client for credentials.
    // Therefore we are too permissive in the case where the ClientCredentialPolicy is DoNotAskClientForCrossOriginCredentials.
    if (clientCredentialPolicy() == DoNotAskClientForAnyCredentials) {
        challenge.authenticationClient()->receivedRequestToContinueWithoutCredential(challenge);
        return;
    }

    NetworkProcess::shared().authenticationManager().didReceiveAuthenticationChallenge(webPageID(), webFrameID(), challenge);
}

void NetworkResourceLoader::didCancelAuthenticationChallenge(ResourceHandle* handle, const AuthenticationChallenge& challenge)
{
    ASSERT_UNUSED(handle, handle == m_handle);

    // This function is probably not needed (see <rdar://problem/8960124>).
    notImplemented();
}

CoreIPC::Connection* NetworkResourceLoader::messageSenderConnection()
{
    return connectionToWebProcess()->connection();
}

uint64_t NetworkResourceLoader::messageSenderDestinationID()
{
    return identifier();
}

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
void NetworkResourceLoader::canAuthenticateAgainstProtectionSpaceAsync(ResourceHandle* handle, const ProtectionSpace& protectionSpace)
{
    ASSERT(isMainThread());
    ASSERT_UNUSED(handle, handle == m_handle);

    // This message is DispatchMessageEvenWhenWaitingForSyncReply to avoid a situation where the NetworkProcess is deadlocked
    // waiting for 6 connections to complete while the WebProcess is waiting for a 7th to complete.
    sendAbortingOnFailure(Messages::WebResourceLoader::CanAuthenticateAgainstProtectionSpace(protectionSpace), CoreIPC::DispatchMessageEvenWhenWaitingForSyncReply);
}

void NetworkResourceLoader::continueCanAuthenticateAgainstProtectionSpace(bool result)
{
    m_handle->continueCanAuthenticateAgainstProtectionSpace(result);
}

#endif

#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)
bool NetworkResourceLoader::supportsDataArray()
{
    notImplemented();
    return false;
}

void NetworkResourceLoader::didReceiveDataArray(ResourceHandle*, CFArrayRef)
{
    ASSERT_NOT_REACHED();
    notImplemented();
}
#endif

#if PLATFORM(MAC)
void NetworkResourceLoader::willStopBufferingData(ResourceHandle*, const char*, int)
{
    notImplemented();
}
#endif // PLATFORM(MAC)

} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)
