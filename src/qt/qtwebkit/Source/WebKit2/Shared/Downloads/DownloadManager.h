/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef DownloadManager_h
#define DownloadManager_h

#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>

namespace WebCore {
class ResourceHandle;
class ResourceRequest;
class ResourceResponse;
}

namespace CoreIPC {
class Connection;
}

namespace WebKit {

class AuthenticationManager;
class Download;
class WebPage;

class DownloadManager {
    WTF_MAKE_NONCOPYABLE(DownloadManager);

public:
    class Client {
    public:
        virtual ~Client() { }

        virtual void didCreateDownload() = 0;
        virtual void didDestroyDownload() = 0;
        virtual CoreIPC::Connection* downloadProxyConnection() = 0;
        virtual AuthenticationManager& downloadsAuthenticationManager() = 0;
    };

    explicit DownloadManager(Client*);

    void startDownload(uint64_t downloadID, const WebCore::ResourceRequest&);
    void convertHandleToDownload(uint64_t downloadID, WebCore::ResourceHandle*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&);

    void cancelDownload(uint64_t downloadID);

    void downloadFinished(Download*);
    bool isDownloading() const { return !m_downloads.isEmpty(); }
    uint64_t activeDownloadCount() const { return m_downloads.size(); }

    void didCreateDownload();
    void didDestroyDownload();

    CoreIPC::Connection* downloadProxyConnection();
    AuthenticationManager& downloadsAuthenticationManager();

#if PLATFORM(QT)
    void startTransfer(uint64_t downloadID, const String& destination);
#endif

private:
    Client* m_client;
    HashMap<uint64_t, Download*> m_downloads;
};

} // namespace WebKit

#endif // DownloadManager_h
