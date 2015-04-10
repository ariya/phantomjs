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

#ifndef Download_h
#define Download_h

#include "MessageSender.h"
#include <WebCore/ResourceRequest.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>

#if PLATFORM(MAC)
#include <wtf/RetainPtr.h>

OBJC_CLASS NSURLDownload;
OBJC_CLASS WKDownloadAsDelegate;
#endif

#if PLATFORM(GTK) || PLATFORM(EFL)
#include <WebCore/ResourceHandle.h>
#include <WebCore/ResourceHandleClient.h>
#endif

#if USE(CFNETWORK)
#include <CFNetwork/CFURLDownloadPriv.h>
#endif

namespace CoreIPC {
    class DataReference;
}

namespace WebCore {
    class AuthenticationChallenge;
    class Credential;
    class ResourceError;
    class ResourceHandle;
    class ResourceResponse;
}

namespace WebKit {

class DownloadAuthenticationClient;
class DownloadManager;
class SandboxExtension;
class WebPage;

#if PLATFORM(QT)
class QtFileDownloader;
#endif

class Download : public CoreIPC::MessageSender {
    WTF_MAKE_NONCOPYABLE(Download);
public:
    static PassOwnPtr<Download> create(DownloadManager&, uint64_t downloadID, const WebCore::ResourceRequest&);
    ~Download();

    void start();
    void startWithHandle(WebCore::ResourceHandle*, const WebCore::ResourceResponse&);
    void cancel();

    uint64_t downloadID() const { return m_downloadID; }

    void didStart();
    void didReceiveAuthenticationChallenge(const WebCore::AuthenticationChallenge&);
    void didReceiveResponse(const WebCore::ResourceResponse&);
    void didReceiveData(uint64_t length);
    bool shouldDecodeSourceDataOfMIMEType(const String& mimeType);
    String decideDestinationWithSuggestedFilename(const String& filename, bool& allowOverwrite);
    void didCreateDestination(const String& path);
    void didFinish();
    void platformDidFinish();
    void didFail(const WebCore::ResourceError&, const CoreIPC::DataReference& resumeData);
    void didCancel(const CoreIPC::DataReference& resumeData);
    void didDecideDestination(const String&, bool allowOverwrite);

#if PLATFORM(QT)
    void startTransfer(const String& destination);
#endif

#if USE(CFNETWORK)
    const String& destination() const { return m_destination; }
    DownloadAuthenticationClient* authenticationClient();
#endif

    // Authentication
    static void receivedCredential(const WebCore::AuthenticationChallenge&, const WebCore::Credential&);
    static void receivedRequestToContinueWithoutCredential(const WebCore::AuthenticationChallenge&);
    static void receivedCancellation(const WebCore::AuthenticationChallenge&);

    void useCredential(const WebCore::AuthenticationChallenge&, const WebCore::Credential&);
    void continueWithoutCredential(const WebCore::AuthenticationChallenge&);
    void cancelAuthenticationChallenge(const WebCore::AuthenticationChallenge&);

private:
    Download(DownloadManager&, uint64_t downloadID, const WebCore::ResourceRequest&);

    // CoreIPC::MessageSender
    virtual CoreIPC::Connection* messageSenderConnection() OVERRIDE;
    virtual uint64_t messageSenderDestinationID() OVERRIDE;

    void platformInvalidate();

    String retrieveDestinationWithSuggestedFilename(const String& filename, bool& allowOverwrite);

    DownloadManager& m_downloadManager;
    uint64_t m_downloadID;
    WebCore::ResourceRequest m_request;

    RefPtr<SandboxExtension> m_sandboxExtension;

#if PLATFORM(MAC)
    RetainPtr<NSURLDownload> m_nsURLDownload;
    RetainPtr<WKDownloadAsDelegate> m_delegate;
#endif
    bool m_allowOverwrite;
    String m_destination;
    String m_bundlePath;
#if USE(CFNETWORK)
    RetainPtr<CFURLDownloadRef> m_download;
    RefPtr<DownloadAuthenticationClient> m_authenticationClient;
#endif
#if PLATFORM(QT)
    QtFileDownloader* m_qtDownloader;
#endif
#if PLATFORM(GTK) || PLATFORM(EFL)
    OwnPtr<WebCore::ResourceHandleClient> m_downloadClient;
    RefPtr<WebCore::ResourceHandle> m_resourceHandle;
#endif
};

} // namespace WebKit

#endif // Download_h
