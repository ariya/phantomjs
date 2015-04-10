/*
 * Copyright (C) 2006, 2007, 2010, 2011 Apple Inc. All rights reserved.
 *           (C) 2007 Graham Dennis (graham.dennis@gmail.com)
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
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
#include "ResourceLoader.h"

#include "ApplicationCacheHost.h"
#include "AsyncFileStream.h"
#include "AuthenticationChallenge.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "InspectorInstrumentation.h"
#include "LoaderStrategy.h"
#include "Page.h"
#include "PlatformStrategies.h"
#include "ProgressTracker.h"
#include "ResourceBuffer.h"
#include "ResourceError.h"
#include "ResourceHandle.h"
#include "ResourceLoadScheduler.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "SharedBuffer.h"

namespace WebCore {

ResourceLoader::ResourceLoader(Frame* frame, ResourceLoaderOptions options)
    : m_frame(frame)
    , m_documentLoader(frame->loader()->activeDocumentLoader())
    , m_identifier(0)
    , m_reachedTerminalState(false)
    , m_notifiedLoadComplete(false)
    , m_cancellationStatus(NotCancelled)
    , m_defersLoading(frame->page()->defersLoading())
    , m_options(options)
{
}

ResourceLoader::~ResourceLoader()
{
    ASSERT(m_reachedTerminalState);
}

void ResourceLoader::releaseResources()
{
    ASSERT(!m_reachedTerminalState);
    
    // It's possible that when we release the handle, it will be
    // deallocated and release the last reference to this object.
    // We need to retain to avoid accessing the object after it
    // has been deallocated and also to avoid reentering this method.
    RefPtr<ResourceLoader> protector(this);

    m_frame = 0;
    m_documentLoader = 0;
    
    // We need to set reachedTerminalState to true before we release
    // the resources to prevent a double dealloc of WebView <rdar://problem/4372628>
    m_reachedTerminalState = true;

    platformStrategies()->loaderStrategy()->resourceLoadScheduler()->remove(this);
    m_identifier = 0;

    if (m_handle) {
        // Clear out the ResourceHandle's client so that it doesn't try to call
        // us back after we release it, unless it has been replaced by someone else.
        if (m_handle->client() == this)
            m_handle->setClient(0);
        m_handle = 0;
    }

    m_resourceData = 0;
    m_deferredRequest = ResourceRequest();
}

bool ResourceLoader::init(const ResourceRequest& r)
{
    ASSERT(!m_handle);
    ASSERT(m_request.isNull());
    ASSERT(m_deferredRequest.isNull());
    ASSERT(!m_documentLoader->isSubstituteLoadPending(this));
    
    ResourceRequest clientRequest(r);
    
    m_defersLoading = m_frame->page()->defersLoading();
    if (m_options.securityCheck == DoSecurityCheck && !m_frame->document()->securityOrigin()->canDisplay(clientRequest.url())) {
        FrameLoader::reportLocalLoadFailed(m_frame.get(), clientRequest.url().string());
        releaseResources();
        return false;
    }
    
    // https://bugs.webkit.org/show_bug.cgi?id=26391
    // The various plug-in implementations call directly to ResourceLoader::load() instead of piping requests
    // through FrameLoader. As a result, they miss the FrameLoader::addExtraFieldsToRequest() step which sets
    // up the 1st party for cookies URL. Until plug-in implementations can be reigned in to pipe through that
    // method, we need to make sure there is always a 1st party for cookies set.
    if (clientRequest.firstPartyForCookies().isNull()) {
        if (Document* document = m_frame->document())
            clientRequest.setFirstPartyForCookies(document->firstPartyForCookies());
    }

    willSendRequest(clientRequest, ResourceResponse());
    if (clientRequest.isNull()) {
        cancel();
        return false;
    }

    m_originalRequest = m_request = clientRequest;
    return true;
}

void ResourceLoader::start()
{
    ASSERT(!m_handle);
    ASSERT(!m_request.isNull());
    ASSERT(m_deferredRequest.isNull());

#if ENABLE(WEB_ARCHIVE) || ENABLE(MHTML)
    if (m_documentLoader->scheduleArchiveLoad(this, m_request))
        return;
#endif

    if (m_documentLoader->applicationCacheHost()->maybeLoadResource(this, m_request, m_request.url()))
        return;

    if (m_defersLoading) {
        m_deferredRequest = m_request;
        return;
    }

    if (!m_reachedTerminalState)
        m_handle = ResourceHandle::create(m_frame->loader()->networkingContext(), m_request, this, m_defersLoading, m_options.sniffContent == SniffContent);
}

void ResourceLoader::setDefersLoading(bool defers)
{
    m_defersLoading = defers;
    if (m_handle)
        m_handle->setDefersLoading(defers);
    if (!defers && !m_deferredRequest.isNull()) {
        m_request = m_deferredRequest;
        m_deferredRequest = ResourceRequest();
        start();
    }
}

FrameLoader* ResourceLoader::frameLoader() const
{
    if (!m_frame)
        return 0;
    return m_frame->loader();
}

void ResourceLoader::setDataBufferingPolicy(DataBufferingPolicy dataBufferingPolicy)
{ 
    m_options.dataBufferingPolicy = dataBufferingPolicy; 

    // Reset any already buffered data
    if (dataBufferingPolicy == DoNotBufferData)
        m_resourceData = 0;
}
    

void ResourceLoader::addDataOrBuffer(const char* data, int length, SharedBuffer* buffer, DataPayloadType dataPayloadType)
{
    if (m_options.dataBufferingPolicy == DoNotBufferData)
        return;

    if (dataPayloadType == DataPayloadWholeResource) {
        m_resourceData = buffer ? ResourceBuffer::adoptSharedBuffer(buffer) : ResourceBuffer::create(data, length);
        return;
    }
        
    if (!m_resourceData)
        m_resourceData = buffer ? ResourceBuffer::adoptSharedBuffer(buffer) : ResourceBuffer::create(data, length);
    else {
        if (buffer)
            m_resourceData->append(buffer);
        else
            m_resourceData->append(data, length);
    }
}

void ResourceLoader::clearResourceData()
{
    if (m_resourceData)
        m_resourceData->clear();
}

bool ResourceLoader::isSubresourceLoader()
{
    return false;
}

void ResourceLoader::willSendRequest(ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    // Protect this in this delegate method since the additional processing can do
    // anything including possibly derefing this; one example of this is Radar 3266216.
    RefPtr<ResourceLoader> protector(this);

    ASSERT(!m_reachedTerminalState);

    // We need a resource identifier for all requests, even if FrameLoader is never going to see it (such as with CORS preflight requests).
    bool createdResourceIdentifier = false;
    if (!m_identifier) {
        m_identifier = m_frame->page()->progress()->createUniqueIdentifier();
        createdResourceIdentifier = true;
    }

    if (m_options.sendLoadCallbacks == SendCallbacks) {
        if (createdResourceIdentifier)
            frameLoader()->notifier()->assignIdentifierToInitialRequest(m_identifier, documentLoader(), request);

        frameLoader()->notifier()->willSendRequest(this, request, redirectResponse);
    }
#if ENABLE(INSPECTOR)
    else
        InspectorInstrumentation::willSendRequest(m_frame.get(), m_identifier, m_frame->loader()->documentLoader(), request, redirectResponse);
#endif

    if (!redirectResponse.isNull())
        platformStrategies()->loaderStrategy()->resourceLoadScheduler()->crossOriginRedirectReceived(this, request.url());

    m_request = request;

    if (!redirectResponse.isNull() && !m_documentLoader->isCommitted())
        frameLoader()->client()->dispatchDidReceiveServerRedirectForProvisionalLoad();
}

void ResourceLoader::didSendData(unsigned long long, unsigned long long)
{
}

void ResourceLoader::didReceiveResponse(const ResourceResponse& r)
{
    ASSERT(!m_reachedTerminalState);

    // Protect this in this delegate method since the additional processing can do
    // anything including possibly derefing this; one example of this is Radar 3266216.
    RefPtr<ResourceLoader> protector(this);

    m_response = r;

    if (FormData* data = m_request.httpBody())
        data->removeGeneratedFilesIfNeeded();
        
    if (m_options.sendLoadCallbacks == SendCallbacks)
        frameLoader()->notifier()->didReceiveResponse(this, m_response);
}

void ResourceLoader::didReceiveData(const char* data, int length, long long encodedDataLength, DataPayloadType dataPayloadType)
{
    // The following assertions are not quite valid here, since a subclass
    // might override didReceiveData in a way that invalidates them. This
    // happens with the steps listed in 3266216
    // ASSERT(con == connection);
    // ASSERT(!m_reachedTerminalState);

    didReceiveDataOrBuffer(data, length, 0, encodedDataLength, dataPayloadType);
}

void ResourceLoader::didReceiveBuffer(PassRefPtr<SharedBuffer> buffer, long long encodedDataLength, DataPayloadType dataPayloadType)
{
    didReceiveDataOrBuffer(0, 0, buffer, encodedDataLength, dataPayloadType);
}

void ResourceLoader::didReceiveDataOrBuffer(const char* data, int length, PassRefPtr<SharedBuffer> prpBuffer, long long encodedDataLength, DataPayloadType dataPayloadType)
{
    // This method should only get data+length *OR* a SharedBuffer.
    ASSERT(!prpBuffer || (!data && !length));

    // Protect this in this delegate method since the additional processing can do
    // anything including possibly derefing this; one example of this is Radar 3266216.
    RefPtr<ResourceLoader> protector(this);
    RefPtr<SharedBuffer> buffer = prpBuffer;

    addDataOrBuffer(data, length, buffer.get(), dataPayloadType);
    
    // FIXME: If we get a resource with more than 2B bytes, this code won't do the right thing.
    // However, with today's computers and networking speeds, this won't happen in practice.
    // Could be an issue with a giant local file.
    if (m_options.sendLoadCallbacks == SendCallbacks && m_frame)
        frameLoader()->notifier()->didReceiveData(this, buffer ? buffer->data() : data, buffer ? buffer->size() : length, static_cast<int>(encodedDataLength));
}

void ResourceLoader::willStopBufferingData(const char* data, int length)
{
    if (m_options.dataBufferingPolicy == DoNotBufferData)
        return;

    ASSERT(!m_resourceData);
    m_resourceData = ResourceBuffer::create(data, length);
}

void ResourceLoader::didFinishLoading(double finishTime)
{
    didFinishLoadingOnePart(finishTime);

    // If the load has been cancelled by a delegate in response to didFinishLoad(), do not release
    // the resources a second time, they have been released by cancel.
    if (wasCancelled())
        return;
    releaseResources();
}

void ResourceLoader::didFinishLoadingOnePart(double finishTime)
{
    // If load has been cancelled after finishing (which could happen with a
    // JavaScript that changes the window location), do nothing.
    if (wasCancelled())
        return;
    ASSERT(!m_reachedTerminalState);

    if (m_notifiedLoadComplete)
        return;
    m_notifiedLoadComplete = true;
    if (m_options.sendLoadCallbacks == SendCallbacks)
        frameLoader()->notifier()->didFinishLoad(this, finishTime);
}

void ResourceLoader::didFail(const ResourceError& error)
{
    if (wasCancelled())
        return;
    ASSERT(!m_reachedTerminalState);

    // Protect this in this delegate method since the additional processing can do
    // anything including possibly derefing this; one example of this is Radar 3266216.
    RefPtr<ResourceLoader> protector(this);

    cleanupForError(error);
    releaseResources();
}

void ResourceLoader::cleanupForError(const ResourceError& error)
{
    if (FormData* data = m_request.httpBody())
        data->removeGeneratedFilesIfNeeded();

    if (m_notifiedLoadComplete)
        return;
    m_notifiedLoadComplete = true;
    if (m_options.sendLoadCallbacks == SendCallbacks && m_identifier)
        frameLoader()->notifier()->didFailToLoad(this, error);
}

void ResourceLoader::didChangePriority(ResourceLoadPriority loadPriority)
{
    if (handle()) {
        frameLoader()->client()->dispatchDidChangeResourcePriority(identifier(), loadPriority);
        handle()->didChangePriority(loadPriority);
    }
}

void ResourceLoader::cancel()
{
    cancel(ResourceError());
}

void ResourceLoader::cancel(const ResourceError& error)
{
    // If the load has already completed - succeeded, failed, or previously cancelled - do nothing.
    if (m_reachedTerminalState)
        return;
       
    ResourceError nonNullError = error.isNull() ? cancelledError() : error;
    
    // willCancel() and didFailToLoad() both call out to clients that might do 
    // something causing the last reference to this object to go away.
    RefPtr<ResourceLoader> protector(this);
    
    // If we re-enter cancel() from inside willCancel(), we want to pick up from where we left 
    // off without re-running willCancel()
    if (m_cancellationStatus == NotCancelled) {
        m_cancellationStatus = CalledWillCancel;
        
        willCancel(nonNullError);
    }

    // If we re-enter cancel() from inside didFailToLoad(), we want to pick up from where we 
    // left off without redoing any of this work.
    if (m_cancellationStatus == CalledWillCancel) {
        m_cancellationStatus = Cancelled;

        if (m_handle)
            m_handle->clearAuthentication();

        m_documentLoader->cancelPendingSubstituteLoad(this);
        if (m_handle) {
            m_handle->cancel();
            m_handle = 0;
        }
        cleanupForError(nonNullError);
    }

    // If cancel() completed from within the call to willCancel() or didFailToLoad(),
    // we don't want to redo didCancel() or releasesResources().
    if (m_reachedTerminalState)
        return;

    didCancel(nonNullError);

    if (m_cancellationStatus == FinishedCancel)
        return;
    m_cancellationStatus = FinishedCancel;

    releaseResources();
}

ResourceError ResourceLoader::cancelledError()
{
    return frameLoader()->cancelledError(m_request);
}

ResourceError ResourceLoader::blockedError()
{
    return frameLoader()->client()->blockedError(m_request);
}

ResourceError ResourceLoader::cannotShowURLError()
{
    return frameLoader()->client()->cannotShowURLError(m_request);
}

void ResourceLoader::willSendRequest(ResourceHandle*, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    if (documentLoader()->applicationCacheHost()->maybeLoadFallbackForRedirect(this, request, redirectResponse))
        return;
    willSendRequest(request, redirectResponse);
}

void ResourceLoader::didSendData(ResourceHandle*, unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    didSendData(bytesSent, totalBytesToBeSent);
}

void ResourceLoader::didReceiveResponse(ResourceHandle*, const ResourceResponse& response)
{
    if (documentLoader()->applicationCacheHost()->maybeLoadFallbackForResponse(this, response))
        return;
    didReceiveResponse(response);
}

void ResourceLoader::didReceiveData(ResourceHandle*, const char* data, int length, int encodedDataLength)
{
    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willReceiveResourceData(m_frame.get(), identifier(), encodedDataLength);
    didReceiveData(data, length, encodedDataLength, DataPayloadBytes);
    InspectorInstrumentation::didReceiveResourceData(cookie);
}

void ResourceLoader::didReceiveBuffer(ResourceHandle*, PassRefPtr<SharedBuffer> buffer, int encodedDataLength)
{
    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willReceiveResourceData(m_frame.get(), identifier(), encodedDataLength);
    didReceiveBuffer(buffer, encodedDataLength, DataPayloadBytes);
    InspectorInstrumentation::didReceiveResourceData(cookie);
}

void ResourceLoader::didFinishLoading(ResourceHandle*, double finishTime)
{
    didFinishLoading(finishTime);
}

void ResourceLoader::didFail(ResourceHandle*, const ResourceError& error)
{
    if (documentLoader()->applicationCacheHost()->maybeLoadFallbackForError(this, error))
        return;
    didFail(error);
}

void ResourceLoader::wasBlocked(ResourceHandle*)
{
    didFail(blockedError());
}

void ResourceLoader::cannotShowURL(ResourceHandle*)
{
    didFail(cannotShowURLError());
}

bool ResourceLoader::shouldUseCredentialStorage()
{
    if (m_options.allowCredentials == DoNotAllowStoredCredentials)
        return false;
    
    RefPtr<ResourceLoader> protector(this);
    return frameLoader()->client()->shouldUseCredentialStorage(documentLoader(), identifier());
}

void ResourceLoader::didReceiveAuthenticationChallenge(const AuthenticationChallenge& challenge)
{
    ASSERT(handle()->hasAuthenticationChallenge());

    // Protect this in this delegate method since the additional processing can do
    // anything including possibly derefing this; one example of this is Radar 3266216.
    RefPtr<ResourceLoader> protector(this);

    if (m_options.allowCredentials == AllowStoredCredentials) {
        if (m_options.clientCredentialPolicy == AskClientForAllCredentials || (m_options.clientCredentialPolicy == DoNotAskClientForCrossOriginCredentials && m_frame->document()->securityOrigin()->canRequest(originalRequest().url()))) {
            frameLoader()->notifier()->didReceiveAuthenticationChallenge(this, challenge);
            return;
        }
    }
    // Only these platforms provide a way to continue without credentials.
    // If we can't continue with credentials, we need to cancel the load altogether.
#if PLATFORM(MAC) || USE(CFNETWORK) || USE(CURL) || PLATFORM(GTK) || PLATFORM(EFL)
    challenge.authenticationClient()->receivedRequestToContinueWithoutCredential(challenge);
    ASSERT(!handle() || !handle()->hasAuthenticationChallenge());
#else
    didFail(blockedError());
#endif
}

void ResourceLoader::didCancelAuthenticationChallenge(const AuthenticationChallenge& challenge)
{
    // Protect this in this delegate method since the additional processing can do
    // anything including possibly derefing this; one example of this is Radar 3266216.
    RefPtr<ResourceLoader> protector(this);
    frameLoader()->notifier()->didCancelAuthenticationChallenge(this, challenge);
}

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
bool ResourceLoader::canAuthenticateAgainstProtectionSpace(const ProtectionSpace& protectionSpace)
{
    RefPtr<ResourceLoader> protector(this);
    return frameLoader()->client()->canAuthenticateAgainstProtectionSpace(documentLoader(), identifier(), protectionSpace);
}
#endif

void ResourceLoader::receivedCancellation(const AuthenticationChallenge&)
{
    cancel();
}

}
