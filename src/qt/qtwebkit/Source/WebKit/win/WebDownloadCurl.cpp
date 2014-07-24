/*
 * Copyright (C) 2008 Brent Fulgham <bfulgham@gmail.com>. All Rights Reserved.
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
#include "WebDownload.h"

#include "DefaultDownloadDelegate.h"
#include "MarshallingHelpers.h"
#include "WebError.h"
#include "WebKit.h"
#include "WebKitLogging.h"
#include "WebMutableURLRequest.h"
#include "WebURLAuthenticationChallenge.h"
#include "WebURLCredential.h"
#include "WebURLResponse.h"

#include <wtf/platform.h>
#include <wtf/text/CString.h>

#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <WebCore/BString.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceHandle.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>

using namespace WebCore;

// WebDownload ----------------------------------------------------------------

void WebDownload::init(ResourceHandle* handle, const ResourceRequest& request, const ResourceResponse& response, IWebDownloadDelegate* delegate)
{
    if (!handle)
        return;

    // Stop previous request
    handle->setDefersLoading(true);

    m_request.adoptRef(WebMutableURLRequest::createInstance(request));

    m_delegate = delegate;

    m_download.init(this, handle, request, response);

    start();
}

void WebDownload::init(const KURL& url, IWebDownloadDelegate* delegate)
{
    m_delegate = delegate;

    m_download.init(this, url);
}

// IWebDownload -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebDownload::initWithRequest(
        /* [in] */ IWebURLRequest* request, 
        /* [in] */ IWebDownloadDelegate* delegate)
{
   notImplemented();
   return E_FAIL;
}

HRESULT STDMETHODCALLTYPE WebDownload::initToResumeWithBundle(
        /* [in] */ BSTR bundlePath, 
        /* [in] */ IWebDownloadDelegate* delegate)
{
   notImplemented();
   return E_FAIL;
}

HRESULT STDMETHODCALLTYPE WebDownload::start()
{
    if (!m_download.start())
        return E_FAIL;

    if (m_delegate)
        m_delegate->didBegin(this);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDownload::cancel()
{
    if (!m_download.cancel())
        return E_FAIL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDownload::cancelForResume()
{
   notImplemented();
   return E_FAIL;
}

HRESULT STDMETHODCALLTYPE WebDownload::deletesFileUponFailure(
        /* [out, retval] */ BOOL* result)
{
    *result = m_download.deletesFileUponFailure() ? TRUE : FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDownload::setDeletesFileUponFailure(
        /* [in] */ BOOL deletesFileUponFailure)
{
    m_download.setDeletesFileUponFailure(deletesFileUponFailure);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebDownload::setDestination(
        /* [in] */ BSTR path, 
        /* [in] */ BOOL allowOverwrite)
{
    size_t len = wcslen(path);
    m_destination = String(path, len);
    m_download.setDestination(m_destination);
    return S_OK;
}

// IWebURLAuthenticationChallengeSender -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebDownload::cancelAuthenticationChallenge(
        /* [in] */ IWebURLAuthenticationChallenge*)
{
   notImplemented();
   return E_FAIL;
}

HRESULT STDMETHODCALLTYPE WebDownload::continueWithoutCredentialForAuthenticationChallenge(
        /* [in] */ IWebURLAuthenticationChallenge* challenge)
{
   notImplemented();
   return E_FAIL;
}

HRESULT STDMETHODCALLTYPE WebDownload::useCredential(
        /* [in] */ IWebURLCredential* credential, 
        /* [in] */ IWebURLAuthenticationChallenge* challenge)
{
   notImplemented();
   return E_FAIL;
}

void WebDownload::didReceiveResponse()
{
    COMPtr<WebDownload> protect = this;

    if (m_delegate) {
        ResourceResponse response = m_download.getResponse();
        COMPtr<WebURLResponse> webResponse(AdoptCOM, WebURLResponse::createInstance(response));
        m_delegate->didReceiveResponse(this, webResponse.get());

        String suggestedFilename = response.suggestedFilename();
        if (suggestedFilename.isEmpty())
            suggestedFilename = pathGetFileName(response.url().string());
        BString suggestedFilenameBSTR(suggestedFilename.characters(), suggestedFilename.length());
        m_delegate->decideDestinationWithSuggestedFilename(this, suggestedFilenameBSTR);
    }
}

void WebDownload::didReceiveDataOfLength(int size)
{
    COMPtr<WebDownload> protect = this;

    if (m_delegate)
        m_delegate->didReceiveDataOfLength(this, size);
}

void WebDownload::didFinish()
{
    COMPtr<WebDownload> protect = this;

    if (m_delegate)
        m_delegate->didFinish(this);
}

void WebDownload::didFail()
{
    COMPtr<WebDownload> protect = this;

    if (m_delegate)
        m_delegate->didFailWithError(this, 0);
}
