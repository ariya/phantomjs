/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
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

#include "config.h"
#include "WebKitDLL.h"
#include "WebDataSource.h"

#include "WebKit.h"
#include "MemoryStream.h"
#include "WebDocumentLoader.h"
#include "WebError.h"
#include "WebFrame.h"
#include "WebKit.h"
#include "WebHTMLRepresentation.h"
#include "WebKitStatisticsPrivate.h"
#include "WebMutableURLRequest.h"
#include "WebResource.h"
#include "WebURLResponse.h"
#include <WebCore/BString.h>
#include <WebCore/CachedResourceLoader.h>
#include <WebCore/Document.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/KURL.h>
#include <WebCore/ResourceBuffer.h>

using namespace WebCore;

// WebDataSource ----------------------------------------------------------------

// {F230854D-7091-428a-8DB5-37CABA44C105}
const GUID IID_WebDataSource = 
{ 0x5c2f9099, 0xe65e, 0x4a0f, { 0x9c, 0xa0, 0x6a, 0xd6, 0x92, 0x52, 0xa6, 0x2a } };

WebDataSource::WebDataSource(WebDocumentLoader* loader)
    : m_refCount(0)
    , m_loader(loader)
{
    WebDataSourceCount++;
    gClassCount++;
    gClassNameCount.add("WebDataSource");
}

WebDataSource::~WebDataSource()
{
    if (m_loader)
        m_loader->detachDataSource();
    WebDataSourceCount--;
    gClassCount--;
    gClassNameCount.remove("WebDataSource");
}

WebDataSource* WebDataSource::createInstance(WebDocumentLoader* loader)
{
    WebDataSource* instance = new WebDataSource(loader);
    instance->AddRef();
    return instance;
}

WebDocumentLoader* WebDataSource::documentLoader() const
{
    return m_loader.get();
}

// IWebDataSourcePrivate ------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebDataSource::overrideEncoding( 
    /* [retval][out] */ BSTR* /*encoding*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebDataSource::setOverrideEncoding( 
    /* [in] */ BSTR /*encoding*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebDataSource::mainDocumentError(
        /* [retval][out] */ IWebError** error)
{
    if (!error) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *error = 0;

    if (!m_loader)
        return E_FAIL;

    if (m_loader->mainDocumentError().isNull())
        return S_OK;

    *error = WebError::createInstance(m_loader->mainDocumentError());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDataSource::setDeferMainResourceDataLoad(
    /* [in] */ BOOL flag)
{
    if (!m_loader)
        return E_FAIL;

    m_loader->setDeferMainResourceDataLoad(flag);
    return S_OK;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebDataSource::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_WebDataSource))
        *ppvObject = this;
    else if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebDataSource*>(this);
    else if (IsEqualGUID(riid, IID_IWebDataSource))
        *ppvObject = static_cast<IWebDataSource*>(this);
    else if (IsEqualGUID(riid, IID_IWebDataSourcePrivate))
        *ppvObject = static_cast<IWebDataSourcePrivate*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebDataSource::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebDataSource::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebDataSource --------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebDataSource::initWithRequest( 
    /* [in] */ IWebURLRequest* /*request*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebDataSource::data( 
    /* [retval][out] */ IStream** stream)
{
    *stream = 0;
    if (!m_loader)
        return E_FAIL;

    RefPtr<ResourceBuffer> buffer = m_loader->mainResourceData();
    return MemoryStream::createInstance(buffer ? buffer->sharedBuffer() : 0).copyRefTo(stream);
}

HRESULT STDMETHODCALLTYPE WebDataSource::representation( 
    /* [retval][out] */ IWebDocumentRepresentation** rep)
{
    HRESULT hr = S_OK;
    if (!m_representation) {
        WebHTMLRepresentation* htmlRep = WebHTMLRepresentation::createInstance(static_cast<WebFrame*>(m_loader->frameLoader()->client()));
        hr = htmlRep->QueryInterface(IID_IWebDocumentRepresentation, (void**) &m_representation);
        htmlRep->Release();
    }

    return m_representation.copyRefTo(rep);
}

HRESULT STDMETHODCALLTYPE WebDataSource::webFrame( 
    /* [retval][out] */ IWebFrame** frame)
{
    *frame = static_cast<WebFrame*>(m_loader->frameLoader()->client());
    (*frame)->AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDataSource::initialRequest( 
    /* [retval][out] */ IWebURLRequest** request)
{
    *request = WebMutableURLRequest::createInstance(m_loader->originalRequest());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDataSource::request( 
    /* [retval][out] */ IWebMutableURLRequest** request)
{
    *request = WebMutableURLRequest::createInstance(m_loader->request());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDataSource::response( 
    /* [retval][out] */ IWebURLResponse** response)
{
    *response = WebURLResponse::createInstance(m_loader->response());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDataSource::textEncodingName( 
    /* [retval][out] */ BSTR* name)
{
    String encoding = m_loader->overrideEncoding();
    if (encoding.isNull())
        encoding = m_loader->response().textEncodingName();

    *name = BString(encoding).release();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDataSource::isLoading( 
    /* [retval][out] */ BOOL* loading)
{
    *loading = m_loader->isLoadingInAPISense();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDataSource::pageTitle( 
    /* [retval][out] */ BSTR* title)
{
    *title = BString(m_loader->title().string()).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDataSource::unreachableURL( 
    /* [retval][out] */ BSTR* url)
{
    KURL unreachableURL = m_loader->unreachableURL();
    BString urlString((LPOLESTR)unreachableURL.string().characters(), unreachableURL.string().length());

    *url = urlString.release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDataSource::webArchive( 
    /* [retval][out] */ IWebArchive** /*archive*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebDataSource::mainResource( 
    /* [retval][out] */ IWebResource** /*resource*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebDataSource::subresources( 
    /* [retval][out] */ IEnumVARIANT** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebDataSource::subresourceForURL( 
    /* [in] */ BSTR url,
    /* [retval][out] */ IWebResource** resource)
{
    if (!resource) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *resource = 0;

    Document *doc = m_loader->frameLoader()->frame()->document();

    if (!doc)
        return E_FAIL;

    CachedResource *cachedResource = doc->cachedResourceLoader()->cachedResource(String(url));

    if (!cachedResource)
        return E_FAIL;

    ResourceBuffer* buffer = cachedResource->resourceBuffer();
    *resource = WebResource::createInstance(buffer ? buffer->sharedBuffer() : 0, cachedResource->response());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDataSource::addSubresource( 
    /* [in] */ IWebResource* /*subresource*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
