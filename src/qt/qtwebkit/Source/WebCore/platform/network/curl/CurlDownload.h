/*
 * Copyright (C) 2013 Apple Inc.  All rights reserved.
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

#ifndef CurlDownload_h
#define CurlDownload_h

#include <WebCore/FileSystem.h>
#include <WebCore/ResourceHandle.h>
#include <WebCore/ResourceResponse.h>

#if PLATFORM(WIN)
#include <windows.h>
#include <winsock2.h>
#endif

#include <curl/curl.h>

namespace WebCore {

class CurlDownloadManager {
public:
    CurlDownloadManager();
    ~CurlDownloadManager();

    bool add(CURL* curlHandle);
    bool remove(CURL* curlHandle);

    int getActiveDownloadCount() const;
    int getPendingDownloadCount() const;

private:
    void startThreadIfNeeded();
    void stopThread();
    void stopThreadIfIdle();

    void updateHandleList();

    CURLM* getMultiHandle() const { return m_curlMultiHandle; }

    bool runThread() const { return m_runThread; }
    void setRunThread(bool runThread) { m_runThread = runThread; }

    bool addToCurl(CURL* curlHandle);
    bool removeFromCurl(CURL* curlHandle);

    static void downloadThread(void* data);

    ThreadIdentifier m_threadId;
    CURLM* m_curlMultiHandle;
    int m_activeDownloadCount;
    Vector<CURL*> m_pendingHandleList;
    Vector<CURL*> m_removedHandleList;
    mutable Mutex m_mutex;
    bool m_runThread;
};

class CurlDownloadListener {
public:
    virtual void didReceiveResponse() { }
    virtual void didReceiveDataOfLength(int size) { }
    virtual void didFinish() { }
    virtual void didFail() { }
};

class CurlDownload {
public:
    CurlDownload();
    ~CurlDownload();

    void init(CurlDownloadListener*, const WebCore::KURL&);
    void init(CurlDownloadListener*, ResourceHandle*, const ResourceRequest&, const ResourceResponse&);

    bool start();
    bool cancel();

    String getTempPath() const;
    String getUrl() const;
    WebCore::ResourceResponse getResponse() const;

    bool deletesFileUponFailure() const { return m_deletesFileUponFailure; }
    void setDeletesFileUponFailure(bool deletesFileUponFailure) { m_deletesFileUponFailure = deletesFileUponFailure; }

    void setDestination(const String& destination) { m_destination = destination; }

private:
    void closeFile();
    void moveFileToDestination();
    void writeDataToFile(const char* data, int size);

    void addHeaders(const ResourceRequest&);

    // Called on download thread.
    void didReceiveHeader(const String& header);
    void didReceiveData(void* data, int size);

    // Called on main thread.
    void didReceiveResponse();
    void didReceiveDataOfLength(int size);
    void didFinish();
    void didFail();

    static size_t writeCallback(void* ptr, size_t, size_t nmemb, void* data);
    static size_t headerCallback(char* ptr, size_t, size_t nmemb, void* data);

    static void downloadFinishedCallback(CurlDownload*);
    static void downloadFailedCallback(CurlDownload*);
    static void receivedDataCallback(CurlDownload*, int size);
    static void receivedResponseCallback(CurlDownload*);

    CURL* m_curlHandle;
    struct curl_slist* m_customHeaders;
    char* m_url;
    String m_tempPath;
    String m_destination;
    WebCore::PlatformFileHandle m_tempHandle;
    WebCore::ResourceResponse m_response;
    bool m_deletesFileUponFailure;
    mutable Mutex m_mutex;
    CurlDownloadListener *m_listener;

    static CurlDownloadManager m_downloadManager;

    friend class CurlDownloadManager;
};

}

#endif
