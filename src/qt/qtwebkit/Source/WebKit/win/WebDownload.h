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

#ifndef WebDownload_h
#define WebDownload_h

#include "WebKit.h"
#include <WebCore/COMPtr.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/WTFString.h>

#if USE(CFNETWORK)
#include <CFNetwork/CFURLDownloadPriv.h>
#elif USE(CURL)
#include <WebCore/CurlDownload.h>
#endif

namespace WebCore {
    class KURL;
    class ResourceHandle;
    class ResourceRequest;
    class ResourceResponse;
}

class WebDownload
: public IWebDownload
, public IWebURLAuthenticationChallengeSender
#if USE(CURL)
, public WebCore::CurlDownloadListener
#endif
{
public:
    static WebDownload* createInstance(const WebCore::KURL&, IWebDownloadDelegate*);
    static WebDownload* createInstance(WebCore::ResourceHandle*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&, IWebDownloadDelegate*);
    static WebDownload* createInstance();
private:
    WebDownload();
    void init(WebCore::ResourceHandle*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&, IWebDownloadDelegate*);
    void init(const WebCore::KURL&, IWebDownloadDelegate*);
    ~WebDownload();
public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IWebDownload
    virtual HRESULT STDMETHODCALLTYPE initWithRequest(
        /* [in] */ IWebURLRequest* request, 
        /* [in] */ IWebDownloadDelegate* delegate);

    virtual HRESULT STDMETHODCALLTYPE initToResumeWithBundle(
        /* [in] */ BSTR bundlePath, 
        /* [in] */ IWebDownloadDelegate* delegate);

    virtual HRESULT STDMETHODCALLTYPE canResumeDownloadDecodedWithEncodingMIMEType(
        /* [in] */ BSTR mimeType, 
        /* [out, retval] */ BOOL* result);

    virtual HRESULT STDMETHODCALLTYPE start();

    virtual HRESULT STDMETHODCALLTYPE cancel();

    virtual HRESULT STDMETHODCALLTYPE cancelForResume();

    virtual HRESULT STDMETHODCALLTYPE deletesFileUponFailure(
        /* [out, retval] */ BOOL* result);

    virtual HRESULT STDMETHODCALLTYPE bundlePathForTargetPath(
        /* [in] */ BSTR target, 
        /* [out, retval] */ BSTR* bundle);

    virtual HRESULT STDMETHODCALLTYPE request(
        /* [out, retval] */ IWebURLRequest** request);

    virtual HRESULT STDMETHODCALLTYPE setDeletesFileUponFailure(
        /* [in] */ BOOL deletesFileUponFailure);

    virtual HRESULT STDMETHODCALLTYPE setDestination(
        /* [in] */ BSTR path, 
        /* [in] */ BOOL allowOverwrite);

    // IWebURLAuthenticationChallengeSender
    virtual HRESULT STDMETHODCALLTYPE cancelAuthenticationChallenge(
        /* [in] */ IWebURLAuthenticationChallenge* challenge);

    virtual HRESULT STDMETHODCALLTYPE continueWithoutCredentialForAuthenticationChallenge(
        /* [in] */ IWebURLAuthenticationChallenge* challenge);

    virtual HRESULT STDMETHODCALLTYPE useCredential(
        /* [in] */ IWebURLCredential* credential, 
        /* [in] */ IWebURLAuthenticationChallenge* challenge);

#if USE(CFNETWORK)
    // CFURLDownload Callbacks
    void didStart();
    CFURLRequestRef willSendRequest(CFURLRequestRef, CFURLResponseRef);
    void didReceiveAuthenticationChallenge(CFURLAuthChallengeRef);
    void didReceiveResponse(CFURLResponseRef);
    void willResumeWithResponse(CFURLResponseRef, UInt64);
    void didReceiveData(CFIndex);
    bool shouldDecodeDataOfMIMEType(CFStringRef);
    void decideDestinationWithSuggestedObjectName(CFStringRef);
    void didCreateDestination(CFURLRef);
    void didFinish();
    void didFail(CFErrorRef);
#elif USE(CURL)
    virtual void didReceiveResponse();
    virtual void didReceiveDataOfLength(int size);
    virtual void didFinish();
    virtual void didFail();
#endif

protected:
    ULONG m_refCount;

    WTF::String m_destination;
    WTF::String m_bundlePath;
#if USE(CFNETWORK)
    RetainPtr<CFURLDownloadRef> m_download;
#elif USE(CURL)
    WebCore::CurlDownload m_download;
#endif
    COMPtr<IWebMutableURLRequest> m_request;
    COMPtr<IWebDownloadDelegate> m_delegate;

#ifndef NDEBUG
    double m_startTime;
    double m_dataTime;
    int m_received;
#endif
};


#endif
