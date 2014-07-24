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
#ifndef DefaultDownloadDelegate_h
#define DefaultDownloadDelegate_h

#include "WebKit.h"
#include <wtf/HashSet.h>

#if USE(CFNETWORK)
#include <CFNetwork/CFURLDownloadPriv.h>
#endif

class DefaultDownloadDelegate : public IWebDownloadDelegate
{
public:
    static DefaultDownloadDelegate* sharedInstance();
    static DefaultDownloadDelegate* createInstance();
private:
    DefaultDownloadDelegate();
    ~DefaultDownloadDelegate();
public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IWebDownloadDelegate
    virtual HRESULT STDMETHODCALLTYPE decideDestinationWithSuggestedFilename(IWebDownload *download, BSTR filename);
    virtual HRESULT STDMETHODCALLTYPE didCancelAuthenticationChallenge(IWebDownload* download,  IWebURLAuthenticationChallenge* challenge);
    virtual HRESULT STDMETHODCALLTYPE didCreateDestination(IWebDownload* download,  BSTR destination);
    virtual HRESULT STDMETHODCALLTYPE didFailWithError(IWebDownload* download,  IWebError* error);
    virtual HRESULT STDMETHODCALLTYPE didReceiveAuthenticationChallenge(IWebDownload* download,  IWebURLAuthenticationChallenge* challenge);
    virtual HRESULT STDMETHODCALLTYPE didReceiveDataOfLength(IWebDownload* download,  unsigned length);
    virtual HRESULT STDMETHODCALLTYPE didReceiveResponse(IWebDownload* download,  IWebURLResponse* response);
    virtual HRESULT STDMETHODCALLTYPE shouldDecodeSourceDataOfMIMEType(IWebDownload* download,  BSTR encodingType, BOOL* shouldDecode);
    virtual HRESULT STDMETHODCALLTYPE willResumeWithResponse(IWebDownload* download,  IWebURLResponse* response,  long long fromByte);
    virtual HRESULT STDMETHODCALLTYPE willSendRequest(IWebDownload* download, IWebMutableURLRequest* request,  IWebURLResponse* redirectResponse, IWebMutableURLRequest** finalRequest);
    virtual HRESULT STDMETHODCALLTYPE didBegin(IWebDownload* download);
    virtual HRESULT STDMETHODCALLTYPE didFinish(IWebDownload* download);

    // DefaultDownloadDelegate
    void registerDownload(IWebDownload*);
    void unregisterDownload(IWebDownload*);
protected:
    ULONG m_refCount;

    HashSet<IWebDownload*> m_downloads;
};

#endif
