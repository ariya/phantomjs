/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
 */

#include "config.h"
#include "WebKitDLL.h"
#include "WebDownload.h"

#include "DefaultDownloadDelegate.h"
#include "MarshallingHelpers.h"
#include "WebError.h"
#include "WebKit.h"
#include "WebKitLogging.h"
#include "WebMutableURLRequest.h"
#include "WebURLAuthenticationChallenge.h"
#include "WebURLCredential.h"
#include "WebURLResponse.h"

#include <wtf/platform.h>
#include <wtf/text/CString.h>

#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <WebCore/AuthenticationCF.h>
#include <WebCore/BString.h>
#include <WebCore/CredentialStorage.h>
#include <WebCore/DownloadBundle.h>
#include <WebCore/LoaderRunLoopCF.h>
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceHandle.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>
#include <wtf/CurrentTime.h>

using namespace WebCore;

// CFURLDownload Callbacks ----------------------------------------------------------------
static void didStartCallback(CFURLDownloadRef download, const void *clientInfo);
static CFURLRequestRef willSendRequestCallback(CFURLDownloadRef download, CFURLRequestRef request, CFURLResponseRef redirectionResponse, const void *clientInfo);
static void didReceiveAuthenticationChallengeCallback(CFURLDownloadRef download, CFURLAuthChallengeRef challenge, const void *clientInfo);
static void didReceiveResponseCallback(CFURLDownloadRef download, CFURLResponseRef response, const void *clientInfo);
static void willResumeWithResponseCallback(CFURLDownloadRef download, CFURLResponseRef response, UInt64 startingByte, const void *clientInfo);
static void didReceiveDataCallback(CFURLDownloadRef download, CFIndex length, const void *clientInfo);
static Boolean shouldDecodeDataOfMIMETypeCallback(CFURLDownloadRef download, CFStringRef encodingType, const void *clientInfo);
static void decideDestinationWithSuggestedObjectNameCallback(CFURLDownloadRef download, CFStringRef objectName, const void *clientInfo);
static void didCreateDestinationCallback(CFURLDownloadRef download, CFURLRef path, const void *clientInfo);
static void didFinishCallback(CFURLDownloadRef download, const void *clientInfo);
static void didFailCallback(CFURLDownloadRef download, CFErrorRef error, const void *clientInfo);

void WebDownload::init(ResourceHandle* handle, const ResourceRequest& request, const ResourceResponse& response, IWebDownloadDelegate* delegate)
{
    m_delegate = delegate ? delegate : DefaultDownloadDelegate::sharedInstance();
    CFURLConnectionRef connection = handle->connection();
    if (!connection) {
        LOG_ERROR("WebDownload::WebDownload(ResourceHandle*,...) called with an inactive ResourceHandle");    
        return;
    }

    CFURLDownloadClient client = {0, this, 0, 0, 0, didStartCallback, willSendRequestCallback, didReceiveAuthenticationChallengeCallback, 
        didReceiveResponseCallback, willResumeWithResponseCallback, didReceiveDataCallback, shouldDecodeDataOfMIMETypeCallback,
        decideDestinationWithSuggestedObjectNameCallback, didCreateDestinationCallback, didFinishCallback, didFailCallback};

    m_request.adoptRef(WebMutableURLRequest::createInstance(request));
    m_download = adoptCF(CFURLDownloadCreateAndStartWithLoadingConnection(0, connection, request.cfURLRequest(UpdateHTTPBody), response.cfURLResponse(), &client));

    // It is possible for CFURLDownloadCreateAndStartWithLoadingConnection() to fail if the passed in CFURLConnection is not in a "downloadable state"
    // However, we should never hit that case
    if (!m_download) {
        ASSERT_NOT_REACHED();
        LOG_ERROR("WebDownload - Failed to create WebDownload from existing connection (%s)", request.url().string().utf8().data());
    } else
        LOG(Download, "WebDownload - Created WebDownload %p from existing connection (%s)", this, request.url().string().utf8().data());

    // The CFURLDownload either starts successfully and retains the CFURLConnection, 
    // or it fails to creating and we have a now-useless connection with a dangling ref. 
    // Either way, we need to release the connection to balance out ref counts
    handle->releaseConnectionForDownload();
    CFRelease(connection);
}

void WebDownload::init(const KURL& url, IWebDownloadDelegate* delegate)
{
    m_delegate = delegate ? delegate : DefaultDownloadDelegate::sharedInstance();
    LOG_ERROR("Delegate is %p", m_delegate.get());

    ResourceRequest request(url);
    CFURLRequestRef cfRequest = request.cfURLRequest(UpdateHTTPBody);

    CFURLDownloadClient client = {0, this, 0, 0, 0, didStartCallback, willSendRequestCallback, didReceiveAuthenticationChallengeCallback, 
                                  didReceiveResponseCallback, willResumeWithResponseCallback, didReceiveDataCallback, shouldDecodeDataOfMIMETypeCallback, 
                                  decideDestinationWithSuggestedObjectNameCallback, didCreateDestinationCallback, didFinishCallback, didFailCallback};
    m_request.adoptRef(WebMutableURLRequest::createInstance(request));
    m_download = adoptCF(CFURLDownloadCreate(0, cfRequest, &client));

    CFURLDownloadScheduleWithCurrentMessageQueue(m_download.get());
    CFURLDownloadScheduleDownloadWithRunLoop(m_download.get(), loaderRunLoop(), kCFRunLoopDefaultMode);

    LOG(Download, "WebDownload - Initialized download of url %s in WebDownload %p", url.string().utf8().data(), this);
}

// IWebDownload -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebDownload::initWithRequest(
        /* [in] */ IWebURLRequest* request, 
        /* [in] */ IWebDownloadDelegate* delegate)
{
    COMPtr<WebMutableURLRequest> webRequest;
    if (!request || FAILED(request->QueryInterface(&webRequest))) {
        LOG(Download, "WebDownload - initWithRequest failed - not a WebMutableURLRequest");    
        return E_FAIL;
    }

    if (!delegate)
        return E_FAIL;
    m_delegate = delegate;
    LOG(Download, "Delegate is %p", m_delegate.get());

    RetainPtr<CFURLRequestRef> cfRequest = webRequest->resourceRequest().cfURLRequest(UpdateHTTPBody);

    CFURLDownloadClient client = {0, this, 0, 0, 0, didStartCallback, willSendRequestCallback, didReceiveAuthenticationChallengeCallback, 
                                  didReceiveResponseCallback, willResumeWithResponseCallback, didReceiveDataCallback, shouldDecodeDataOfMIMETypeCallback, 
                                  decideDestinationWithSuggestedObjectNameCallback, didCreateDestinationCallback, didFinishCallback, didFailCallback};
    m_request.adoptRef(WebMutableURLRequest::createInstance(webRequest.get()));
    m_download = adoptCF(CFURLDownloadCreate(0, cfRequest.get(), &client));

    // If for some reason the download failed to create, 
    // we have particular cleanup to do
    if (!m_download) {
        m_request = 0;    
        return E_FAIL;
    }

    CFURLDownloadScheduleWithCurrentMessageQueue(m_download.get());
    CFURLDownloadScheduleDownloadWithRunLoop(m_download.get(), loaderRunLoop(), kCFRunLoopDefaultMode);

    LOG(Download, "WebDownload - initWithRequest complete, started download of url %s", webRequest->resourceRequest().url().string().utf8().data());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDownload::initToResumeWithBundle(
        /* [in] */ BSTR bundlePath, 
        /* [in] */ IWebDownloadDelegate* delegate)
{
    LOG(Download, "Attempting resume of download bundle %s", String(bundlePath, SysStringLen(bundlePath)).ascii().data());

    RetainPtr<CFDataRef> resumeData = adoptCF(DownloadBundle::extractResumeData(String(bundlePath, SysStringLen(bundlePath))));

    if (!resumeData)
        return E_FAIL;

    if (!delegate)
        return E_FAIL;
    m_delegate = delegate;
    LOG(Download, "Delegate is %p", m_delegate.get());

    CFURLDownloadClient client = {0, this, 0, 0, 0, didStartCallback, willSendRequestCallback, didReceiveAuthenticationChallengeCallback, 
                                  didReceiveResponseCallback, willResumeWithResponseCallback, didReceiveDataCallback, shouldDecodeDataOfMIMETypeCallback, 
                                  decideDestinationWithSuggestedObjectNameCallback, didCreateDestinationCallback, didFinishCallback, didFailCallback};
    
    RetainPtr<CFURLRef> pathURL = adoptCF(MarshallingHelpers::PathStringToFileCFURLRef(String(bundlePath, SysStringLen(bundlePath))));
    ASSERT(pathURL);

    m_download = adoptCF(CFURLDownloadCreateWithResumeData(0, resumeData.get(), pathURL.get(), &client));

    if (!m_download) {
        LOG(Download, "Failed to create CFURLDownloadRef for resume");    
        return E_FAIL;
    }
    
    m_bundlePath = String(bundlePath, SysStringLen(bundlePath));
    // Attempt to remove the ".download" extension from the bundle for the final file destination
    // Failing that, we clear m_destination and will ask the delegate later once the download starts
    if (m_bundlePath.endsWith(DownloadBundle::fileExtension(), false)) {
        m_destination = m_bundlePath.isolatedCopy();
        m_destination.truncate(m_destination.length() - DownloadBundle::fileExtension().length());
    } else
        m_destination = String();
    
    CFURLDownloadScheduleWithCurrentMessageQueue(m_download.get());
    CFURLDownloadScheduleDownloadWithRunLoop(m_download.get(), loaderRunLoop(), kCFRunLoopDefaultMode);

    LOG(Download, "WebDownload - initWithRequest complete, resumed download of bundle %s", String(bundlePath, SysStringLen(bundlePath)).ascii().data());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDownload::start()
{
    LOG(Download, "WebDownload - Starting download (%p)", this);
    if (!m_download)
        return E_FAIL;

    CFURLDownloadStart(m_download.get());
    // FIXME: 4950477 - CFURLDownload neglects to make the didStart() client call upon starting the download.
    // This is a somewhat critical call, so we'll fake it for now!
    didStart();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDownload::cancel()
{
    LOG(Download, "WebDownload - Cancelling download (%p)", this);
    if (!m_download)
        return E_FAIL;

    CFURLDownloadCancel(m_download.get());
    m_download = 0;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDownload::cancelForResume()
{
    LOG(Download, "WebDownload - Cancelling download (%p), writing resume information to file if possible", this);
    ASSERT(m_download);
    if (!m_download)
        return E_FAIL;

    HRESULT hr = S_OK;
    RetainPtr<CFDataRef> resumeData;
    if (m_destination.isEmpty()) {
        CFURLDownloadCancel(m_download.get());
        goto exit;
    }

    CFURLDownloadSetDeletesUponFailure(m_download.get(), false);
    CFURLDownloadCancel(m_download.get());

    resumeData = adoptCF(CFURLDownloadCopyResumeData(m_download.get()));
    if (!resumeData) {
        LOG(Download, "WebDownload - Unable to create resume data for download (%p)", this);
        goto exit;
    }

    DownloadBundle::appendResumeData(resumeData.get(), m_bundlePath);

exit:
    m_download = 0;
    return hr;
}

HRESULT STDMETHODCALLTYPE WebDownload::deletesFileUponFailure(
        /* [out, retval] */ BOOL* result)
{
    if (!m_download)
        return E_FAIL;
    *result = CFURLDownloadDeletesUponFailure(m_download.get());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDownload::setDeletesFileUponFailure(
        /* [in] */ BOOL deletesFileUponFailure)
{
    if (!m_download)
        return E_FAIL;
    CFURLDownloadSetDeletesUponFailure(m_download.get(), !!deletesFileUponFailure);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDownload::setDestination(
        /* [in] */ BSTR path, 
        /* [in] */ BOOL allowOverwrite)
{
    if (!m_download)
        return E_FAIL;

    m_destination = String(path, SysStringLen(path));
    m_bundlePath = m_destination + DownloadBundle::fileExtension();

    CFURLRef pathURL = MarshallingHelpers::PathStringToFileCFURLRef(m_bundlePath);
    CFURLDownloadSetDestination(m_download.get(), pathURL, !!allowOverwrite);
    CFRelease(pathURL);

    LOG(Download, "WebDownload - Set destination to %s", m_bundlePath.ascii().data());

    return S_OK;
}

// IWebURLAuthenticationChallengeSender -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebDownload::cancelAuthenticationChallenge(
        /* [in] */ IWebURLAuthenticationChallenge*)
{
    if (m_download) {
        CFURLDownloadCancel(m_download.get());
        m_download = 0;
    }

    // FIXME: Do we need a URL or description for this error code?
    ResourceError error(String(WebURLErrorDomain), WebURLErrorUserCancelledAuthentication, "", "");
    COMPtr<WebError> webError(AdoptCOM, WebError::createInstance(error));
    m_delegate->didFailWithError(this, webError.get());

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDownload::continueWithoutCredentialForAuthenticationChallenge(
        /* [in] */ IWebURLAuthenticationChallenge* challenge)
{
    COMPtr<WebURLAuthenticationChallenge> webChallenge(Query, challenge);
    if (!webChallenge)
        return E_NOINTERFACE;

    if (m_download)
        CFURLDownloadUseCredential(m_download.get(), 0, webChallenge->authenticationChallenge().cfURLAuthChallengeRef());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDownload::useCredential(
        /* [in] */ IWebURLCredential* credential, 
        /* [in] */ IWebURLAuthenticationChallenge* challenge)
{
    COMPtr<WebURLAuthenticationChallenge> webChallenge(Query, challenge);
    if (!webChallenge)
        return E_NOINTERFACE;

    COMPtr<WebURLCredential> webCredential(Query, credential);
    if (!webCredential)
        return E_NOINTERFACE;

    RetainPtr<CFURLCredentialRef> cfCredential = adoptCF(createCF(webCredential->credential()));

    if (m_download)
        CFURLDownloadUseCredential(m_download.get(), cfCredential.get(), webChallenge->authenticationChallenge().cfURLAuthChallengeRef());
    return S_OK;
}

// CFURLDownload Callbacks -------------------------------------------------------------------
void WebDownload::didStart()
{
#ifndef NDEBUG
    m_startTime = m_dataTime = currentTime();
    m_received = 0;
    LOG(Download, "DOWNLOAD - Started %p at %.3f seconds", this, m_startTime);
#endif
    if (FAILED(m_delegate->didBegin(this)))
        LOG_ERROR("DownloadDelegate->didBegin failed");
}

CFURLRequestRef WebDownload::willSendRequest(CFURLRequestRef request, CFURLResponseRef response)
{
    COMPtr<WebMutableURLRequest> webRequest(AdoptCOM, WebMutableURLRequest::createInstance(ResourceRequest(request)));
    COMPtr<WebURLResponse> webResponse(AdoptCOM, WebURLResponse::createInstance(ResourceResponse(response)));
    COMPtr<IWebMutableURLRequest> finalRequest;

    if (FAILED(m_delegate->willSendRequest(this, webRequest.get(), webResponse.get(), &finalRequest)))
        LOG_ERROR("DownloadDelegate->willSendRequest failed");
    
    if (!finalRequest)
        return 0;

    COMPtr<WebMutableURLRequest> finalWebRequest(AdoptCOM, WebMutableURLRequest::createInstance(finalRequest.get()));
    m_request = finalWebRequest.get();
    CFURLRequestRef result = finalWebRequest->resourceRequest().cfURLRequest(UpdateHTTPBody);
    CFRetain(result);
    return result;
}

void WebDownload::didReceiveAuthenticationChallenge(CFURLAuthChallengeRef challenge)
{
    // Try previously stored credential first.
    if (!CFURLAuthChallengeGetPreviousFailureCount(challenge)) {
        Credential credential = CredentialStorage::get(core(CFURLAuthChallengeGetProtectionSpace(challenge)));
        if (!credential.isEmpty()) {
            RetainPtr<CFURLCredentialRef> cfCredential = adoptCF(createCF(credential));
            CFURLDownloadUseCredential(m_download.get(), cfCredential.get(), challenge);
            return;
        }
    }

    COMPtr<IWebURLAuthenticationChallenge> webChallenge(AdoptCOM,
        WebURLAuthenticationChallenge::createInstance(AuthenticationChallenge(challenge, 0), this));

    if (SUCCEEDED(m_delegate->didReceiveAuthenticationChallenge(this, webChallenge.get())))
        return;

    cancelAuthenticationChallenge(webChallenge.get());
}

void WebDownload::didReceiveResponse(CFURLResponseRef response)
{
    COMPtr<WebURLResponse> webResponse(AdoptCOM, WebURLResponse::createInstance(ResourceResponse(response)));
    if (FAILED(m_delegate->didReceiveResponse(this, webResponse.get())))
        LOG_ERROR("DownloadDelegate->didReceiveResponse failed");
}

void WebDownload::willResumeWithResponse(CFURLResponseRef response, UInt64 fromByte)
{
    COMPtr<WebURLResponse> webResponse(AdoptCOM, WebURLResponse::createInstance(ResourceResponse(response)));
    if (FAILED(m_delegate->willResumeWithResponse(this, webResponse.get(), fromByte)))
        LOG_ERROR("DownloadDelegate->willResumeWithResponse failed");
}

void WebDownload::didReceiveData(CFIndex length)
{
#ifndef NDEBUG
    m_received += length;
    double current = currentTime();
    if (current - m_dataTime > 2.0)
        LOG(Download, "DOWNLOAD - %p hanged for %.3f seconds - Received %i bytes for a total of %i", this, current - m_dataTime, length, m_received);
    m_dataTime = current;
#endif
    if (FAILED(m_delegate->didReceiveDataOfLength(this, length)))
        LOG_ERROR("DownloadDelegate->didReceiveData failed");
}

bool WebDownload::shouldDecodeDataOfMIMEType(CFStringRef mimeType)
{
    BOOL result;
    if (FAILED(m_delegate->shouldDecodeSourceDataOfMIMEType(this, BString(mimeType), &result))) {
        LOG_ERROR("DownloadDelegate->shouldDecodeSourceDataOfMIMEType failed");
        return false;
    }
    return !!result;
}

void WebDownload::decideDestinationWithSuggestedObjectName(CFStringRef name)
{    
    if (FAILED(m_delegate->decideDestinationWithSuggestedFilename(this, BString(name))))
        LOG_ERROR("DownloadDelegate->decideDestinationWithSuggestedObjectName failed");
}

void WebDownload::didCreateDestination(CFURLRef destination)
{
    // The concept of the ".download bundle" is internal to the WebDownload, so therefore
    // we try to mask the delegate from its existence as much as possible by telling it the final
    // destination was created, when in reality the bundle was created

    String createdDestination = MarshallingHelpers::FileCFURLRefToPathString(destination);

    // At this point in receiving CFURLDownload callbacks, we should definitely have the bundle path stored locally
    // and it should match with the file that CFURLDownload created
    ASSERT(createdDestination == m_bundlePath);
    // And we should also always have the final-destination stored
    ASSERT(!m_destination.isEmpty());

    BString path(m_destination);
    if (FAILED(m_delegate->didCreateDestination(this, path)))
        LOG_ERROR("DownloadDelegate->didCreateDestination failed");
}

void WebDownload::didFinish()
{
#ifndef NDEBUG
    LOG(Download, "DOWNLOAD - Finished %p after %i bytes and %.3f seconds", this, m_received, currentTime() - m_startTime);
#endif

    ASSERT(!m_bundlePath.isEmpty() && !m_destination.isEmpty());
    LOG(Download, "WebDownload - Moving file from bundle %s to destination %s", m_bundlePath.ascii().data(), m_destination.ascii().data());

    // We try to rename the bundle to the final file name.  If that fails, we give the delegate one more chance to chose
    // the final file name, then we just leave it
    if (!MoveFileEx(m_bundlePath.charactersWithNullTermination().data(), m_destination.charactersWithNullTermination().data(), 0)) {
        LOG_ERROR("Failed to move bundle %s to %s on completion\nError - %i", m_bundlePath.ascii().data(), m_destination.ascii().data(), GetLastError());
        
        bool reportBundlePathAsFinalPath = true;

        BString destinationBSTR(m_destination.characters(), m_destination.length());
        if (FAILED(m_delegate->decideDestinationWithSuggestedFilename(this, destinationBSTR)))
            LOG_ERROR("delegate->decideDestinationWithSuggestedFilename() failed");

        // The call to m_delegate->decideDestinationWithSuggestedFilename() should have changed our destination, so we'll try the move
        // one last time.
        if (!m_destination.isEmpty())
            if (MoveFileEx(m_bundlePath.charactersWithNullTermination().data(), m_destination.charactersWithNullTermination().data(), 0))
                reportBundlePathAsFinalPath = false;

        // We either need to tell the delegate our final filename is the bundle filename, or is the file name they just told us to use
        if (reportBundlePathAsFinalPath) {
            BString bundleBSTR(m_bundlePath);
            m_delegate->didCreateDestination(this, bundleBSTR);
        } else {
            BString finalDestinationBSTR = BString(m_destination);
            m_delegate->didCreateDestination(this, finalDestinationBSTR);
        }
    }

    // It's extremely likely the call to delegate->didFinish() will deref this, so lets not let that cause our destruction just yet
    COMPtr<WebDownload> protect = this;
    if (FAILED(m_delegate->didFinish(this)))
        LOG_ERROR("DownloadDelegate->didFinish failed");

    m_download = 0;
}

void WebDownload::didFail(CFErrorRef error)
{
    COMPtr<WebError> webError(AdoptCOM, WebError::createInstance(ResourceError(error)));
    if (FAILED(m_delegate->didFailWithError(this, webError.get())))
        LOG_ERROR("DownloadDelegate->didFailWithError failed");
}

// CFURLDownload Callbacks ----------------------------------------------------------------
void didStartCallback(CFURLDownloadRef, const void *clientInfo)
{ ((WebDownload*)clientInfo)->didStart(); }

CFURLRequestRef willSendRequestCallback(CFURLDownloadRef, CFURLRequestRef request, CFURLResponseRef redirectionResponse, const void *clientInfo)
{ return ((WebDownload*)clientInfo)->willSendRequest(request, redirectionResponse); }

void didReceiveAuthenticationChallengeCallback(CFURLDownloadRef, CFURLAuthChallengeRef challenge, const void *clientInfo)
{ ((WebDownload*)clientInfo)->didReceiveAuthenticationChallenge(challenge); }

void didReceiveResponseCallback(CFURLDownloadRef, CFURLResponseRef response, const void *clientInfo)
{ ((WebDownload*)clientInfo)->didReceiveResponse(response); }

void willResumeWithResponseCallback(CFURLDownloadRef, CFURLResponseRef response, UInt64 startingByte, const void *clientInfo)
{ ((WebDownload*)clientInfo)->willResumeWithResponse(response, startingByte); }

void didReceiveDataCallback(CFURLDownloadRef, CFIndex length, const void *clientInfo)
{ ((WebDownload*)clientInfo)->didReceiveData(length); }

Boolean shouldDecodeDataOfMIMETypeCallback(CFURLDownloadRef, CFStringRef encodingType, const void *clientInfo)
{ return ((WebDownload*)clientInfo)->shouldDecodeDataOfMIMEType(encodingType); }

void decideDestinationWithSuggestedObjectNameCallback(CFURLDownloadRef, CFStringRef objectName, const void *clientInfo)
{ ((WebDownload*)clientInfo)->decideDestinationWithSuggestedObjectName(objectName); }

void didCreateDestinationCallback(CFURLDownloadRef, CFURLRef path, const void *clientInfo)
{ ((WebDownload*)clientInfo)->didCreateDestination(path); }

void didFinishCallback(CFURLDownloadRef, const void *clientInfo)
{ ((WebDownload*)clientInfo)->didFinish(); }

void didFailCallback(CFURLDownloadRef, CFErrorRef error, const void *clientInfo)
{ ((WebDownload*)clientInfo)->didFail(error); }
