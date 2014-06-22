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

#ifndef WebArchive_h
#define WebArchive_h

#include "WebKit.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {
    class LegacyWebArchive;
}

class WebArchive : public IWebArchive
{
public:
    static WebArchive* createInstance();
    static WebArchive* createInstance(PassRefPtr<WebCore::LegacyWebArchive>);
protected:
    WebArchive(PassRefPtr<WebCore::LegacyWebArchive>);
    ~WebArchive();

public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // IWebArchive
    virtual HRESULT STDMETHODCALLTYPE initWithMainResource(
        /* [in] */ IWebResource* mainResource, 
        /* [in, size_is(cSubResources)] */ IWebResource** subResources, 
        /* [in] */ int cSubResources, 
        /* in, size_is(cSubFrameArchives)] */ IWebArchive** subFrameArchives, 
        /* [in] */ int cSubFrameArchives);

    virtual HRESULT STDMETHODCALLTYPE  initWithData(
        /* [in] */ IStream*);

    virtual HRESULT STDMETHODCALLTYPE  initWithNode(
        /* [in] */ IDOMNode*);

    virtual HRESULT STDMETHODCALLTYPE  mainResource(
        /* [out, retval] */ IWebResource**);

    virtual HRESULT STDMETHODCALLTYPE  subResources(
        /* [out, retval] */ IEnumVARIANT**);

    virtual HRESULT STDMETHODCALLTYPE  subframeArchives(
        /* [out, retval] */ IEnumVARIANT**);

    virtual HRESULT STDMETHODCALLTYPE  data(
        /* [out, retval] */ IStream**);

protected:
    ULONG m_refCount;
    RefPtr<WebCore::LegacyWebArchive> m_archive;
};

#endif // WebArchive_h
