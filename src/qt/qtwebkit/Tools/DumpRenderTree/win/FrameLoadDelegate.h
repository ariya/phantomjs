/*
 * Copyright (C) 2005, 2006, 2007 Apple Inc.  All rights reserved.
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

#ifndef FrameLoadDelegate_h
#define FrameLoadDelegate_h

#include <WebKit/WebKit.h>
#include <wtf/OwnPtr.h>

class AccessibilityController;
class TextInputController;
class GCController;

class FrameLoadDelegate : public IWebFrameLoadDelegate, public IWebFrameLoadDelegatePrivate2 {
public:
    FrameLoadDelegate();
    virtual ~FrameLoadDelegate();

    void processWork();

    void resetToConsistentState();

    AccessibilityController* accessibilityController() const { return m_accessibilityController.get(); }

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IWebFrameLoadDelegate
    virtual HRESULT STDMETHODCALLTYPE didStartProvisionalLoadForFrame( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame); 

    virtual HRESULT STDMETHODCALLTYPE didReceiveServerRedirectForProvisionalLoadForFrame( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE didFailProvisionalLoadWithError( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebError *error,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE didCommitLoadForFrame( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE didReceiveTitle( 
        /* [in] */ IWebView *webView,
        /* [in] */ BSTR title,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE didChangeIcons(
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE didReceiveIcon( 
        /* [in] */ IWebView *webView,
        /* [in] */ OLE_HANDLE image,
        /* [in] */ IWebFrame *frame) { return E_NOTIMPL; } 

    virtual HRESULT STDMETHODCALLTYPE didFinishLoadForFrame( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE didFailLoadWithError( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebError *error,
        /* [in] */ IWebFrame *forFrame);

    virtual HRESULT STDMETHODCALLTYPE didChangeLocationWithinPageForFrame( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame) { return E_NOTIMPL; } 

    virtual HRESULT STDMETHODCALLTYPE willPerformClientRedirectToURL( 
        /* [in] */ IWebView *webView,
        /* [in] */ BSTR url,
        /* [in] */ double delaySeconds,
        /* [in] */ DATE fireDate,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE didCancelClientRedirectForFrame( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE willCloseFrame( 
        /* [in] */ IWebView *webView,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE windowScriptObjectAvailable( 
        /* [in] */ IWebView *sender,
        /* [in] */ JSContextRef context,
        /* [in] */ JSObjectRef windowObject) { return E_NOTIMPL; }

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE didClearWindowObject( 
        /* [in] */ IWebView* webView,
        /* [in] */ JSContextRef context,
        /* [in] */ JSObjectRef windowObject,
        /* [in] */ IWebFrame* frame);

    // IWebFrameLoadDelegatePrivate
    virtual HRESULT STDMETHODCALLTYPE didFinishDocumentLoadForFrame( 
        /* [in] */ IWebView *sender,
        /* [in] */ IWebFrame *frame);
        
    virtual HRESULT STDMETHODCALLTYPE didFirstLayoutInFrame( 
        /* [in] */ IWebView *sender,
        /* [in] */ IWebFrame *frame) { return E_NOTIMPL; } 
        
    virtual HRESULT STDMETHODCALLTYPE didHandleOnloadEventsForFrame( 
        /* [in] */ IWebView *sender,
        /* [in] */ IWebFrame *frame);

    virtual HRESULT STDMETHODCALLTYPE didFirstVisuallyNonEmptyLayoutInFrame( 
        /* [in] */ IWebView *sender,
        /* [in] */ IWebFrame *frame);

    // IWebFrameLoadDelegatePrivate2
    virtual HRESULT STDMETHODCALLTYPE didDisplayInsecureContent( 
        /* [in] */ IWebView *sender);

    virtual HRESULT STDMETHODCALLTYPE didRunInsecureContent( 
        /* [in] */ IWebView *sender,
        /* [in] */ IWebSecurityOrigin *origin);

    virtual HRESULT STDMETHODCALLTYPE didClearWindowObjectForFrameInScriptWorld(IWebView*, IWebFrame*, IWebScriptWorld*);

    virtual HRESULT STDMETHODCALLTYPE didPushStateWithinPageForFrame( 
        /* [in] */ IWebView *sender,
        /* [in] */ IWebFrame *frame) { return E_NOTIMPL; } 
    
    virtual HRESULT STDMETHODCALLTYPE didReplaceStateWithinPageForFrame( 
        /* [in] */ IWebView *sender,
        /* [in] */ IWebFrame *frame) { return E_NOTIMPL; } 

    virtual HRESULT STDMETHODCALLTYPE didPopStateWithinPageForFrame( 
        /* [in] */ IWebView *sender,
        /* [in] */ IWebFrame *frame) { return E_NOTIMPL; } 

private:
    void didClearWindowObjectForFrameInIsolatedWorld(IWebFrame*, IWebScriptWorld*);
    void didClearWindowObjectForFrameInStandardWorld(IWebFrame*);

    void locationChangeDone(IWebError*, IWebFrame*);

    ULONG m_refCount;
    OwnPtr<GCController> m_gcController;
    OwnPtr<AccessibilityController> m_accessibilityController;
    OwnPtr<TextInputController> m_textInputController;
};

#endif // FrameLoadDelegate_h
