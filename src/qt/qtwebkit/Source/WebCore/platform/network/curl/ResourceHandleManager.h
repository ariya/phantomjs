/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * All rights reserved.
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

#ifndef ResourceHandleManager_h
#define ResourceHandleManager_h

#include "Frame.h"
#include "Timer.h"
#include "ResourceHandleClient.h"

#if PLATFORM(WIN)
#include <winsock2.h>
#include <windows.h>
#endif

#include <curl/curl.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class ResourceHandleManager {
public:
    enum ProxyType {
        HTTP = CURLPROXY_HTTP,
        Socks4 = CURLPROXY_SOCKS4,
        Socks4A = CURLPROXY_SOCKS4A,
        Socks5 = CURLPROXY_SOCKS5,
        Socks5Hostname = CURLPROXY_SOCKS5_HOSTNAME
    };
    static ResourceHandleManager* sharedInstance();
    void add(ResourceHandle*);
    void cancel(ResourceHandle*);

    CURLSH* getCurlShareHandle() const;

    void setCookieJarFileName(const char* cookieJarFileName);
    const char* getCookieJarFileName() const;

    void dispatchSynchronousJob(ResourceHandle*);

    void setupPOST(ResourceHandle*, struct curl_slist**);
    void setupPUT(ResourceHandle*, struct curl_slist**);

    void setProxyInfo(const String& host = "",
                      unsigned long port = 0,
                      ProxyType type = HTTP,
                      const String& username = "",
                      const String& password = "");

private:
    ResourceHandleManager();
    ~ResourceHandleManager();
    void downloadTimerCallback(Timer<ResourceHandleManager>*);
    void removeFromCurl(ResourceHandle*);
    bool removeScheduledJob(ResourceHandle*);
    void startJob(ResourceHandle*);
    bool startScheduledJobs();
    void applyAuthenticationToRequest(ResourceHandle*, ResourceRequest&);

    void initializeHandle(ResourceHandle*);

    void initCookieSession();

    Timer<ResourceHandleManager> m_downloadTimer;
    CURLM* m_curlMultiHandle;
    CURLSH* m_curlShareHandle;
    char* m_cookieJarFileName;
    char m_curlErrorBuffer[CURL_ERROR_SIZE];
    Vector<ResourceHandle*> m_resourceHandleList;
    const CString m_certificatePath;
    int m_runningJobs;
    
    String m_proxy;
    ProxyType m_proxyType;
};

}

#endif
