/*
 * Copyright (C) 2009 Apple Inc.  All rights reserved.
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

#ifndef HistoryDelegate_h
#define HistoryDelegate_h

#include <WebKit/WebKit.h>
#include <wtf/OwnPtr.h>

class HistoryDelegate : public IWebHistoryDelegate {
public:
    HistoryDelegate();
    virtual ~HistoryDelegate();

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IWebHistoryDelegate
    virtual HRESULT STDMETHODCALLTYPE didNavigateWithNavigationData(
        /* [in] */ IWebView* webView, 
        /* [in] */ IWebNavigationData* navigationData, 
        /* [in] */ IWebFrame* webFrame);

    virtual HRESULT STDMETHODCALLTYPE didPerformClientRedirectFromURL(
        /* [in] */ IWebView* webView, 
        /* [in] */ BSTR sourceURL, 
        /* [in] */ BSTR destinationURL, 
        /* [in] */ IWebFrame* webFrame);
    
    virtual HRESULT STDMETHODCALLTYPE didPerformServerRedirectFromURL(
        /* [in] */ IWebView* webView, 
        /* [in] */ BSTR sourceURL, 
        /* [in] */ BSTR destinationURL, 
        /* [in] */ IWebFrame* webFrame);
    
    virtual HRESULT STDMETHODCALLTYPE updateHistoryTitle(
        /* [in] */ IWebView* webView, 
        /* [in] */ BSTR title, 
        /* [in] */ BSTR url);
    
    virtual HRESULT STDMETHODCALLTYPE populateVisitedLinksForWebView(
        /* [in] */ IWebView* webView);

private:
    ULONG m_refCount;
};

#endif // HistoryDelegate_h
