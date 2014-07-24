/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef WebCoreStatistics_h
#define WebCoreStatistics_h

#include "WebKit.h"

class WebCoreStatistics : public IWebCoreStatistics {
public:
    static WebCoreStatistics* createInstance();
protected:
    WebCoreStatistics();
    ~WebCoreStatistics();

public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IWebCoreStatistics
    virtual HRESULT STDMETHODCALLTYPE javaScriptObjectsCount( 
        /* [retval][out] */ UINT *count);
    virtual HRESULT STDMETHODCALLTYPE javaScriptGlobalObjectsCount( 
        /* [retval][out] */ UINT *count);
    virtual HRESULT STDMETHODCALLTYPE javaScriptProtectedObjectsCount( 
        /* [retval][out] */ UINT *count);
    virtual HRESULT STDMETHODCALLTYPE javaScriptProtectedGlobalObjectsCount( 
        /* [retval][out] */ UINT *count);
    virtual HRESULT STDMETHODCALLTYPE javaScriptProtectedObjectTypeCounts( 
        /* [retval][out] */ IPropertyBag2** typeNamesAndCounts);
    virtual HRESULT STDMETHODCALLTYPE iconPageURLMappingCount( 
        /* [retval][out] */ UINT *count);
    virtual HRESULT STDMETHODCALLTYPE iconRetainedPageURLCount( 
        /* [retval][out] */ UINT *count);
    virtual HRESULT STDMETHODCALLTYPE iconRecordCount( 
        /* [retval][out] */ UINT *count);
    virtual HRESULT STDMETHODCALLTYPE iconsWithDataCount( 
        /* [retval][out] */ UINT *count);
    virtual HRESULT STDMETHODCALLTYPE cachedFontDataCount( 
        /* [retval][out] */ UINT *count);
    virtual HRESULT STDMETHODCALLTYPE cachedFontDataInactiveCount( 
        /* [retval][out] */ UINT *count);
    virtual HRESULT STDMETHODCALLTYPE purgeInactiveFontData(void);
    virtual HRESULT STDMETHODCALLTYPE glyphPageCount( 
        /* [retval][out] */ UINT *count);

protected:
    ULONG m_refCount;
};

#endif
