/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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
#include "Download.h"

#include "DataReference.h"
#include "DownloadAuthenticationClient.h"

#pragma warning(push, 0)
#include <WebCore/AuthenticationCF.h>
#include <WebCore/AuthenticationChallenge.h>
#include <WebCore/DownloadBundle.h>
#include <WebCore/LoaderRunLoopCF.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceHandle.h>
#include <WebCore/ResourceResponse.h>
#pragma warning(pop)

using namespace WebCore;

namespace WebKit {

// CFURLDownload Callbacks ----------------------------------------------------------------
static void didStartCallback(CFURLDownloadRef download, const void* clientInfo);
static CFURLRequestRef willSendRequestCallback(CFURLDownloadRef download, CFURLRequestRef request, CFURLResponseRef redirectionResponse, const void* clientInfo);
static void didReceiveAuthenticationChallengeCallback(CFURLDownloadRef download, CFURLAuthChallengeRef challenge, const void* clientInfo);
static void didReceiveResponseCallback(CFURLDownloadRef download, CFURLResponseRef response, const void* clientInfo);
static void willResumeWithResponseCallback(CFURLDownloadRef download, CFURLResponseRef response, UInt64 startingByte, const void* clientInfo);
static void didReceiveDataCallback(CFURLDownloadRef download, CFIndex length, const void* clientInfo);
static Boolean shouldDecodeDataOfMIMETypeCallback(CFURLDownloadRef download, CFStringRef encodingType, const void* clientInfo);
static void decideDestinationWithSuggestedObjectNameCallback(CFURLDownloadRef download, CFStringRef objectName, const void* clientInfo);
static void didCreateDestinationCallback(CFURLDownloadRef download, CFURLRef path, const void* clientInfo);
static void didFinishCallback(CFURLDownloadRef download, const void* clientInfo);
static void didFailCallback(CFURLDownloadRef download, CFErrorRef error, const void* clientInfo);

void Download::useCredential(const WebCore::AuthenticationChallenge& challenge, const WebCore::Credential& credential)
{
    if (!m_download)
        return;
    RetainPtr<CFURLCredentialRef> cfCredential = adoptCF(createCF(credential));
    CFURLDownloadUseCredential(m_download.get(), cfCredential.get(), challenge.cfURLAuthChallengeRef());
}

void Download::continueWithoutCredential(const WebCore::AuthenticationChallenge& challenge)
{
    if (!m_download)
        return;
    CFURLDownloadUseCredential(m_download.get(), 0, challenge.cfURLAuthChallengeRef());
}

void Download::cancelAuthenticationChallenge(const WebCore::AuthenticationChallenge&)
{
    if (!m_download)
        return;
    CFURLDownloadCancel(m_download.get());
    m_download = 0;
}

DownloadAuthenticationClient* Download::authenticationClient()
{
    if (!m_authenticationClient)
        m_authenticationClient = DownloadAuthenticationClient::create(this);
    return m_authenticationClient.get();
}

void Download::start()
{
    ASSERT(!m_download);

    CFURLRequestRef cfRequest = m_request.cfURLRequest();

    CFURLDownloadClient client = {0, this, 0, 0, 0, didStartCallback, willSendRequestCallback, didReceiveAuthenticationChallengeCallback, 
                                  didReceiveResponseCallback, willResumeWithResponseCallback, didReceiveDataCallback, shouldDecodeDataOfMIMETypeCallback, 
                                  decideDestinationWithSuggestedObjectNameCallback, didCreateDestinationCallback, didFinishCallback, didFailCallback};
    m_download = adoptCF(CFURLDownloadCreate(0, cfRequest, &client));

    // FIXME: Allow this to be changed by the client.
    CFURLDownloadSetDeletesUponFailure(m_download.get(), false);

#if OS(WINDOWS)
    CFURLDownloadScheduleWithCurrentMessageQueue(m_download.get());
#endif
    CFURLDownloadScheduleDownloadWithRunLoop(m_download.get(), loaderRunLoop(), kCFRunLoopDefaultMode);

    CFURLDownloadStart(m_download.get());
}

void Download::startWithHandle(ResourceHandle* handle, const ResourceResponse& response)
{
    ASSERT(!m_download);

    CFURLConnectionRef connection = handle->connection();
    if (!connection)
        return;

    CFURLDownloadClient client = {0, this, 0, 0, 0, didStartCallback, willSendRequestCallback, didReceiveAuthenticationChallengeCallback, 
                                  didReceiveResponseCallback, willResumeWithResponseCallback, didReceiveDataCallback, shouldDecodeDataOfMIMETypeCallback,
                                  decideDestinationWithSuggestedObjectNameCallback, didCreateDestinationCallback, didFinishCallback, didFailCallback};

    m_download = adoptCF(CFURLDownloadCreateAndStartWithLoadingConnection(0, connection, handle->firstRequest().cfURLRequest(), response.cfURLResponse(), &client));

    // It is possible for CFURLDownloadCreateAndStartWithLoadingConnection() to fail if the passed in CFURLConnection is not in a "downloadable state"
    // However, we should never hit that case
    if (!m_download)
        ASSERT_NOT_REACHED();

    // The CFURLDownload either starts successfully and retains the CFURLConnection, 
    // or it fails to creating and we have a now-useless connection with a dangling ref. 
    // Either way, we need to release the connection to balance out ref counts
    handle->releaseConnectionForDownload();
    CFRelease(connection);
}

void Download::cancel()
{
    ASSERT(m_download);
    if (!m_download)
        return;

    CFURLDownloadSetDeletesUponFailure(m_download.get(), false);
    CFURLDownloadCancel(m_download.get());

    RetainPtr<CFDataRef> resumeData = adoptCF(CFURLDownloadCopyResumeData(m_download.get()));
    if (resumeData)
        DownloadBundle::appendResumeData(resumeData.get(), m_bundlePath);

    didCancel(CoreIPC::DataReference());
}

void Download::platformInvalidate()
{
    m_download = nullptr;
    if (m_authenticationClient)
        m_authenticationClient->detach();
}

void Download::didDecideDestination(const String& destination, bool allowOverwrite)
{
    ASSERT(!destination.isEmpty());
    if (destination.isEmpty())
        return;

    m_allowOverwrite = allowOverwrite;
    m_destination = destination;
    m_bundlePath = destination + DownloadBundle::fileExtension();

    RetainPtr<CFStringRef> bundlePath = adoptCF(CFStringCreateWithCharactersNoCopy(0, reinterpret_cast<const UniChar*>(m_bundlePath.characters()), m_bundlePath.length(), kCFAllocatorNull));
    RetainPtr<CFURLRef> bundlePathURL = adoptCF(CFURLCreateWithFileSystemPath(0, bundlePath.get(), kCFURLWindowsPathStyle, false));
    CFURLDownloadSetDestination(m_download.get(), bundlePathURL.get(), allowOverwrite);
}

// CFURLDownload Callbacks ----------------------------------------------------------------
static Download* downloadFromClientInfo(const void* clientInfo)
{
    return reinterpret_cast<Download*>(const_cast<void*>(clientInfo));
}

void didStartCallback(CFURLDownloadRef, const void* clientInfo)
{ 
    downloadFromClientInfo(clientInfo)->didStart(); 
}

CFURLRequestRef willSendRequestCallback(CFURLDownloadRef, CFURLRequestRef request, CFURLResponseRef redirectionResponse, const void* clientInfo)
{ 
    // CFNetwork requires us to return a retained request.
    CFRetain(request);
    return request;
}

void didReceiveAuthenticationChallengeCallback(CFURLDownloadRef, CFURLAuthChallengeRef challenge, const void* clientInfo)
{
    Download* download = downloadFromClientInfo(clientInfo);
    download->didReceiveAuthenticationChallenge(AuthenticationChallenge(challenge, download->authenticationClient()));
}

void didReceiveResponseCallback(CFURLDownloadRef, CFURLResponseRef response, const void* clientInfo)
{
    downloadFromClientInfo(clientInfo)->didReceiveResponse(ResourceResponse(response)); 
}

void willResumeWithResponseCallback(CFURLDownloadRef, CFURLResponseRef response, UInt64 startingByte, const void* clientInfo)
{
    // FIXME: implement.
    notImplemented();
}

void didReceiveDataCallback(CFURLDownloadRef, CFIndex length, const void* clientInfo)
{ 
    downloadFromClientInfo(clientInfo)->didReceiveData(length);
}

Boolean shouldDecodeDataOfMIMETypeCallback(CFURLDownloadRef, CFStringRef encodingType, const void* clientInfo)
{ 
    return downloadFromClientInfo(clientInfo)->shouldDecodeSourceDataOfMIMEType(encodingType);
}

void decideDestinationWithSuggestedObjectNameCallback(CFURLDownloadRef, CFStringRef objectName, const void* clientInfo)
{ 
    Download* download = downloadFromClientInfo(clientInfo);
    bool allowOverwrite;
    download->decideDestinationWithSuggestedFilename(objectName, allowOverwrite);
}

void didCreateDestinationCallback(CFURLDownloadRef, CFURLRef, const void* clientInfo)
{
    // The concept of the ".download bundle" is internal to the Download, so we try to hide its
    // existence by reporting the final destination was created, when in reality the bundle was created.

    Download* download = downloadFromClientInfo(clientInfo);
    download->didCreateDestination(download->destination());
}

void didFinishCallback(CFURLDownloadRef, const void* clientInfo)
{ 
    downloadFromClientInfo(clientInfo)->didFinish();
}

void didFailCallback(CFURLDownloadRef, CFErrorRef error, const void* clientInfo)
{ 
    CoreIPC::DataReference dataReference(0, 0);
    downloadFromClientInfo(clientInfo)->didFail(ResourceError(error), dataReference);
}

void Download::receivedCredential(const AuthenticationChallenge& authenticationChallenge, const Credential& credential)
{
    ASSERT(authenticationChallenge.authenticationClient());
    authenticationChallenge.authenticationClient()->receivedCredential(authenticationChallenge, credential);
}

void Download::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge& authenticationChallenge)
{
    ASSERT(authenticationChallenge.authenticationClient());
    authenticationChallenge.authenticationClient()->receivedRequestToContinueWithoutCredential(authenticationChallenge);
}

void Download::receivedCancellation(const AuthenticationChallenge& authenticationChallenge)
{
    ASSERT(authenticationChallenge.authenticationClient());
    authenticationChallenge.authenticationClient()->receivedCancellation(authenticationChallenge);
}

} // namespace WebKit
