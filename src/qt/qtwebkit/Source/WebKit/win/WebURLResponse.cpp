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
#include "WebURLResponse.h"

#include "WebKitDLL.h"
#include "WebKit.h"

#include "COMPropertyBag.h"
#include "MarshallingHelpers.h"

#if USE(CFNETWORK)
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

#include <wtf/platform.h>
#include <WebCore/BString.h>
#include <WebCore/KURL.h>
#include <WebCore/LocalizedStrings.h>
#include <WebCore/ResourceHandle.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <wchar.h>

using namespace WebCore;

static String CFHTTPMessageCopyLocalizedShortDescriptionForStatusCode(CFIndex statusCode)
{
    String result;
    if (statusCode < 100 || statusCode >= 600)
        result = WEB_UI_STRING("server error", "HTTP result code string");
    else if (statusCode >= 100 && statusCode <= 199) {
        switch (statusCode) {
            case 100:
                result = WEB_UI_STRING("continue", "HTTP result code string");
                break;
            case 101:
                result = WEB_UI_STRING("switching protocols", "HTTP result code string");
                break;
            default:
                result = WEB_UI_STRING("informational", "HTTP result code string");
                break;
        }
    } else if (statusCode >= 200 && statusCode <= 299) {
        switch (statusCode) {
            case 200:
                result = WEB_UI_STRING("no error", "HTTP result code string");
                break;
            case 201:
                result = WEB_UI_STRING("created", "HTTP result code string");
                break;
            case 202:
                result = WEB_UI_STRING("accepted", "HTTP result code string");
                break;
            case 203:
                result = WEB_UI_STRING("non-authoritative information", "HTTP result code string");
                break;
            case 204:
                result = WEB_UI_STRING("no content", "HTTP result code string");
                break;
            case 205:
                result = WEB_UI_STRING("reset content", "HTTP result code string");
                break;
            case 206:
                result = WEB_UI_STRING("partial content", "HTTP result code string");
                break;
            default:
                result = WEB_UI_STRING("success", "HTTP result code string");
                break;
        } 
    } else if (statusCode >= 300 && statusCode <= 399) {
        switch (statusCode) {
            case 300:
                result = WEB_UI_STRING("multiple choices", "HTTP result code string");
                break;
            case 301:
                result = WEB_UI_STRING("moved permanently", "HTTP result code string");
                break;
            case 302:
                result = WEB_UI_STRING("found", "HTTP result code string");
                break;
            case 303:
                result = WEB_UI_STRING("see other", "HTTP result code string");
                break;
            case 304:
                result = WEB_UI_STRING("not modified", "HTTP result code string");
                break;
            case 305:
                result = WEB_UI_STRING("needs proxy", "HTTP result code string");
                break;
            case 307:
                result = WEB_UI_STRING("temporarily redirected", "HTTP result code string");
                break;
            case 306:   // 306 status code unused in HTTP
            default:
                result = WEB_UI_STRING("redirected", "HTTP result code string");
                break;
        }
    } else if (statusCode >= 400 && statusCode <= 499) {
        switch (statusCode) {
            case 400:
                result = WEB_UI_STRING("bad request", "HTTP result code string");
                break;
            case 401:
                result = WEB_UI_STRING("unauthorized", "HTTP result code string");
                break;
            case 402:
                result = WEB_UI_STRING("payment required", "HTTP result code string");
                break;
            case 403:
                result = WEB_UI_STRING("forbidden", "HTTP result code string");
                break;
            case 404:
                result = WEB_UI_STRING("not found", "HTTP result code string");
                break;
            case 405:
                result = WEB_UI_STRING("method not allowed", "HTTP result code string");
                break;
            case 406:
                result = WEB_UI_STRING("unacceptable", "HTTP result code string");
                break;
            case 407:
                result = WEB_UI_STRING("proxy authentication required", "HTTP result code string");
                break;
            case 408:
                result = WEB_UI_STRING("request timed out", "HTTP result code string");
                break;
            case 409:
                result = WEB_UI_STRING("conflict", "HTTP result code string");
                break;
            case 410:
                result = WEB_UI_STRING("no longer exists", "HTTP result code string");
                break;
            case 411:
                result = WEB_UI_STRING("length required", "HTTP result code string");
                break;
            case 412:
                result = WEB_UI_STRING("precondition failed", "HTTP result code string");
                break;
            case 413:
                result = WEB_UI_STRING("request too large", "HTTP result code string");
                break;
            case 414:
                result = WEB_UI_STRING("requested URL too long", "HTTP result code string");
                break;
            case 415:
                result = WEB_UI_STRING("unsupported media type", "HTTP result code string");
                break;
            case 416:
                result = WEB_UI_STRING("requested range not satisfiable", "HTTP result code string");
                break;
            case 417:
                result = WEB_UI_STRING("expectation failed", "HTTP result code string");
                break;
            default:
                result = WEB_UI_STRING("client error", "HTTP result code string");
                break;
        }
    } else if (statusCode >= 500 && statusCode <= 599) {
        switch (statusCode) {
            case 500:
                result = WEB_UI_STRING("internal server error", "HTTP result code string");
                break;
            case 501:
                result = WEB_UI_STRING("unimplemented", "HTTP result code string");
                break;
            case 502:
                result = WEB_UI_STRING("bad gateway", "HTTP result code string");
                break;
            case 503:
                result = WEB_UI_STRING("service unavailable", "HTTP result code string");
                break;
            case 504:
                result = WEB_UI_STRING("gateway timed out", "HTTP result code string");
                break;
            case 505:
                result = WEB_UI_STRING("unsupported version", "HTTP result code string");
                break;
            default:
                result = WEB_UI_STRING("server error", "HTTP result code string");
                break;
        }
    }
    return result;
}

// IWebURLResponse ----------------------------------------------------------------

WebURLResponse::WebURLResponse()
    :m_refCount(0)
{
    gClassCount++;
    gClassNameCount.add("WebURLResponse");
}

WebURLResponse::~WebURLResponse()
{
    gClassCount--;
    gClassNameCount.remove("WebURLResponse");
}

WebURLResponse* WebURLResponse::createInstance()
{
    WebURLResponse* instance = new WebURLResponse();
    // fake an http response - so it has the IWebHTTPURLResponse interface
    instance->m_response = ResourceResponse(KURL(ParsedURLString, "http://"), String(), 0, String(), String());
    instance->AddRef();
    return instance;
}

WebURLResponse* WebURLResponse::createInstance(const ResourceResponse& response)
{
    if (response.isNull())
        return 0;

    WebURLResponse* instance = new WebURLResponse();
    instance->AddRef();
    instance->m_response = response;

    return instance;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebURLResponse::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebURLResponse*>(this);
    else if (IsEqualGUID(riid, __uuidof(this)))
        *ppvObject = this;
    else if (IsEqualGUID(riid, IID_IWebURLResponse))
        *ppvObject = static_cast<IWebURLResponse*>(this);
    else if (IsEqualGUID(riid, IID_IWebURLResponsePrivate))
        *ppvObject = static_cast<IWebURLResponsePrivate*>(this);
    else if (m_response.isHTTP() && IsEqualGUID(riid, IID_IWebHTTPURLResponse))
        *ppvObject = static_cast<IWebHTTPURLResponse*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebURLResponse::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebURLResponse::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebURLResponse --------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebURLResponse::expectedContentLength( 
    /* [retval][out] */ long long* result)
{
    *result = m_response.expectedContentLength();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLResponse::initWithURL( 
    /* [in] */ BSTR url,
    /* [in] */ BSTR mimeType,
    /* [in] */ int expectedContentLength,
    /* [in] */ BSTR textEncodingName)
{
    m_response = ResourceResponse(MarshallingHelpers::BSTRToKURL(url), String(mimeType), expectedContentLength, String(textEncodingName), String());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLResponse::MIMEType( 
    /* [retval][out] */ BSTR* result)
{
    BString mimeType(m_response.mimeType());
    *result = mimeType.release();
    if (!m_response.mimeType().isNull() && !*result)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLResponse::suggestedFilename( 
    /* [retval][out] */ BSTR* result)
{
    if (!result) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *result = 0;

    if (m_response.url().isEmpty())
        return E_FAIL;

    *result = BString(m_response.suggestedFilename()).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLResponse::textEncodingName( 
    /* [retval][out] */ BSTR* result)
{
    if (!result)
        return E_INVALIDARG;

    BString textEncodingName(m_response.textEncodingName());
    *result = textEncodingName.release();
    if (!m_response.textEncodingName().isNull() && !*result)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLResponse::URL( 
    /* [retval][out] */ BSTR* result)
{
    if (!result)
        return E_INVALIDARG;

    BString url(m_response.url().string());
    *result = url.release();
    if (!m_response.url().isEmpty() && !*result)
        return E_OUTOFMEMORY;

    return S_OK;
}

// IWebHTTPURLResponse --------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebURLResponse::allHeaderFields( 
    /* [retval][out] */ IPropertyBag** headerFields)
{
    ASSERT(m_response.isHTTP());

    *headerFields = COMPropertyBag<String, AtomicString, CaseFoldingHash>::createInstance(m_response.httpHeaderFields());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLResponse::localizedStringForStatusCode( 
    /* [in] */ int statusCode,
    /* [retval][out] */ BSTR* statusString)
{
    ASSERT(m_response.isHTTP());
    if (statusString)
        *statusString = 0;
    String statusText = CFHTTPMessageCopyLocalizedShortDescriptionForStatusCode(statusCode);
    if (!statusText)
        return E_FAIL;
    if (statusString)
        *statusString = BString(statusText).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLResponse::statusCode( 
    /* [retval][out] */ int* statusCode)
{
    ASSERT(m_response.isHTTP());
    if (statusCode)
        *statusCode = m_response.httpStatusCode();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLResponse::isAttachment( 
    /* [retval][out] */ BOOL *attachment)
{
    *attachment = m_response.isAttachment();
    return S_OK;
}


HRESULT STDMETHODCALLTYPE WebURLResponse::sslPeerCertificate( 
    /* [retval][out] */ OLE_HANDLE* result)
{
    if (!result)
        return E_POINTER;
    *result = 0;

#if USE(CFNETWORK)
    CFDictionaryRef dict = certificateDictionary();
    if (!dict)
        return E_FAIL;
    void* data = wkGetSSLPeerCertificateDataBytePtr(dict);
    if (!data)
        return E_FAIL;
    *result = (OLE_HANDLE)(ULONG64)data;
#endif

    return *result ? S_OK : E_FAIL;
}

// WebURLResponse -------------------------------------------------------------

HRESULT WebURLResponse::suggestedFileExtension(BSTR *result)
{
    if (!result)
        return E_POINTER;

    *result = 0;

    if (m_response.mimeType().isEmpty())
        return E_FAIL;

    BString mimeType(m_response.mimeType());
    HKEY key;
    LONG err = RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("MIME\\Database\\Content Type"), 0, KEY_QUERY_VALUE, &key);
    if (!err) {
        HKEY subKey;
        err = RegOpenKeyEx(key, mimeType, 0, KEY_QUERY_VALUE, &subKey);
        if (!err) {
            DWORD keyType = REG_SZ;
            WCHAR extension[MAX_PATH];
            DWORD keySize = sizeof(extension)/sizeof(extension[0]);
            err = RegQueryValueEx(subKey, TEXT("Extension"), 0, &keyType, (LPBYTE)extension, &keySize);
            if (!err && keyType != REG_SZ)
                err = ERROR_INVALID_DATA;
            if (err) {
                // fallback handlers
                if (!wcscmp(mimeType, L"text/html")) {
                    wcscpy(extension, L".html");
                    err = 0;
                } else if (!wcscmp(mimeType, L"application/xhtml+xml")) {
                    wcscpy(extension, L".xhtml");
                    err = 0;
                } else if (!wcscmp(mimeType, L"image/svg+xml")) {
                    wcscpy(extension, L".svg");
                    err = 0;
                }
            }
            if (!err) {
                *result = SysAllocString(extension);
                if (!*result)
                    err = ERROR_OUTOFMEMORY;
            }
            RegCloseKey(subKey);
        }
        RegCloseKey(key);
    }

    return HRESULT_FROM_WIN32(err);
}

const ResourceResponse& WebURLResponse::resourceResponse() const
{
    return m_response;
}

#if USE(CFNETWORK)
CFDictionaryRef WebURLResponse::certificateDictionary() const
{
    if (m_SSLCertificateInfo)
        return m_SSLCertificateInfo.get();

    CFURLResponseRef cfResponse = m_response.cfURLResponse();
    if (!cfResponse)
        return 0;
    m_SSLCertificateInfo = wkGetSSLCertificateInfo(cfResponse);
    return m_SSLCertificateInfo.get();
}
#endif
