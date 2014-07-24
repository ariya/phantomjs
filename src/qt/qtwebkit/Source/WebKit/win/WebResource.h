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

#ifndef WebResource_h
#define WebResource_h

#include "WebKit.h"
#include <WebCore/COMPtr.h>
#include <WebCore/KURL.h>
#include <WebCore/ResourceResponse.h>
#include <WebCore/SharedBuffer.h>
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

class WebResource : public IWebResource {
public:
    static WebResource* createInstance(PassRefPtr<WebCore::SharedBuffer> data, const WebCore::ResourceResponse& response);
protected:
    WebResource(IStream* data, const WebCore::KURL& url, const WTF::String& mimeType, const WTF::String& textEncodingName, const WTF::String& frameName);
    ~WebResource();

public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IWebResource
    virtual HRESULT STDMETHODCALLTYPE initWithData( 
        /* [in] */ IStream *data,
        /* [in] */ BSTR url,
        /* [in] */ BSTR mimeType,
        /* [in] */ BSTR textEncodingName,
        /* [in] */ BSTR frameName);
        
    virtual HRESULT STDMETHODCALLTYPE data( 
        /* [retval][out] */ IStream **data);
       
    virtual HRESULT STDMETHODCALLTYPE URL( 
        /* [retval][out] */ BSTR *url);
        
    virtual HRESULT STDMETHODCALLTYPE MIMEType( 
        /* [retval][out] */ BSTR *mime);
        
    virtual HRESULT STDMETHODCALLTYPE textEncodingName( 
        /* [retval][out] */ BSTR *encodingName);
        
    virtual HRESULT STDMETHODCALLTYPE frameName( 
        /* [retval][out] */ BSTR *name);

private:
    ULONG m_refCount;
    COMPtr<IStream> m_data;
    WebCore::KURL m_url;
    WTF::String m_mimeType;
    WTF::String m_textEncodingName;
    WTF::String m_frameName;
};

#endif
