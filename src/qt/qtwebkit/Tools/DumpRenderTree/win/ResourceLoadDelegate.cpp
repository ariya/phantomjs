/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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

#include "config.h"
#include "ResourceLoadDelegate.h"

#include "DumpRenderTree.h"
#include "TestRunner.h"
#include <WebKit/WebKitCOMAPI.h>
#include <comutil.h>
#include <sstream>
#include <tchar.h>
#include <wtf/Vector.h>

using namespace std;

static inline wstring wstringFromBSTR(BSTR str)
{
    return wstring(str, ::SysStringLen(str));
}

static inline wstring wstringFromInt(int i)
{
    wostringstream ss;
    ss << i;
    return ss.str();
}

static inline BSTR BSTRFromString(const string& str)
{
    int length = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), 0, 0);
    BSTR result = ::SysAllocStringLen(0, length);
    ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), result, length);
    return result;
}

wstring ResourceLoadDelegate::descriptionSuitableForTestResult(unsigned long identifier) const
{
    IdentifierMap::const_iterator it = m_urlMap.find(identifier);
    
    if (it == m_urlMap.end())
        return L"<unknown>";

    return urlSuitableForTestResult(it->value);
}

wstring ResourceLoadDelegate::descriptionSuitableForTestResult(IWebURLRequest* request)
{
    if (!request)
        return L"(null)";

    BSTR urlBSTR;
    if (FAILED(request->URL(&urlBSTR)))
        return wstring();
    
    wstring url = urlSuitableForTestResult(wstringFromBSTR(urlBSTR));
    ::SysFreeString(urlBSTR);
    
    BSTR mainDocumentURLBSTR;
    if (FAILED(request->mainDocumentURL(&mainDocumentURLBSTR)))
        return wstring();
    
    wstring mainDocumentURL = urlSuitableForTestResult(wstringFromBSTR(mainDocumentURLBSTR));
    ::SysFreeString(mainDocumentURLBSTR);
    
    BSTR httpMethodBSTR;
    if (FAILED(request->HTTPMethod(&httpMethodBSTR)))
        return wstring();
    
    wstring httpMethod = wstringFromBSTR(httpMethodBSTR);
    ::SysFreeString(httpMethodBSTR);

    return L"<NSURLRequest URL " + url + L", main document URL " + mainDocumentURL + L", http method " + httpMethod + L">";
}

wstring ResourceLoadDelegate::descriptionSuitableForTestResult(IWebURLResponse* response)
{
    if (!response)
        return L"(null)";

    BSTR urlBSTR;
    if (FAILED(response->URL(&urlBSTR)))
        return wstring();
    
    wstring url = urlSuitableForTestResult(wstringFromBSTR(urlBSTR));
    ::SysFreeString(urlBSTR);

    int statusCode = 0;
    COMPtr<IWebHTTPURLResponse> httpResponse;
    if (response && SUCCEEDED(response->QueryInterface(&httpResponse)))
        httpResponse->statusCode(&statusCode);
    
    return L"<NSURLResponse " + url + L", http status code " + wstringFromInt(statusCode) + L">";
}

wstring ResourceLoadDelegate::descriptionSuitableForTestResult(IWebError* error, unsigned long identifier) const
{
    wstring result = L"<NSError ";

    BSTR domainSTR;
    if (FAILED(error->domain(&domainSTR)))
        return wstring();

    wstring domain = wstringFromBSTR(domainSTR);
    ::SysFreeString(domainSTR);

    int code;
    if (FAILED(error->code(&code)))
        return wstring();

    if (domain == L"CFURLErrorDomain") {
        domain = L"NSURLErrorDomain";

        // Convert kCFURLErrorUnknown to NSURLErrorUnknown
        if (code == -998)
            code = -1;
    } else if (domain == L"kCFErrorDomainWinSock") {
        domain = L"NSURLErrorDomain";

        // Convert the winsock error code to an NSURLError code.
        if (code == WSAEADDRNOTAVAIL)
            code = -1004; // NSURLErrorCannotConnectToHose;
    }

    result += L"domain " + domain;
    result += L", code " + wstringFromInt(code);

    BSTR failingURLSTR;
    if (FAILED(error->failingURL(&failingURLSTR)))
        return wstring();

    if (failingURLSTR) {
        result += L", failing URL \"" + urlSuitableForTestResult(wstringFromBSTR(failingURLSTR)) + L"\"";
        ::SysFreeString(failingURLSTR);
    }

    result += L">";

    return result;
}

ResourceLoadDelegate::ResourceLoadDelegate()
    : m_refCount(1)
{
}

ResourceLoadDelegate::~ResourceLoadDelegate()
{
}

HRESULT STDMETHODCALLTYPE ResourceLoadDelegate::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebResourceLoadDelegate*>(this);
    else if (IsEqualGUID(riid, IID_IWebResourceLoadDelegate))
        *ppvObject = static_cast<IWebResourceLoadDelegate*>(this);
    else if (IsEqualGUID(riid, IID_IWebResourceLoadDelegatePrivate2))
        *ppvObject = static_cast<IWebResourceLoadDelegatePrivate2*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE ResourceLoadDelegate::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE ResourceLoadDelegate::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

HRESULT STDMETHODCALLTYPE ResourceLoadDelegate::identifierForInitialRequest( 
    /* [in] */ IWebView* webView,
    /* [in] */ IWebURLRequest* request,
    /* [in] */ IWebDataSource* dataSource,
    /* [in] */ unsigned long identifier)
{ 
    if (!done && gTestRunner->dumpResourceLoadCallbacks()) {
        BSTR urlStr;
        if (FAILED(request->URL(&urlStr)))
            return E_FAIL;

        ASSERT(!urlMap().contains(identifier));
        urlMap().set(identifier, wstringFromBSTR(urlStr));
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE ResourceLoadDelegate::removeIdentifierForRequest(
    /* [in] */ IWebView* webView,
    /* [in] */ unsigned long identifier)
{
    urlMap().remove(identifier);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE ResourceLoadDelegate::willSendRequest( 
    /* [in] */ IWebView* webView,
    /* [in] */ unsigned long identifier,
    /* [in] */ IWebURLRequest* request,
    /* [in] */ IWebURLResponse* redirectResponse,
    /* [in] */ IWebDataSource* dataSource,
    /* [retval][out] */ IWebURLRequest **newRequest)
{
    if (!done && gTestRunner->dumpResourceLoadCallbacks()) {
        printf("%S - willSendRequest %S redirectResponse %S\n", 
            descriptionSuitableForTestResult(identifier).c_str(),
            descriptionSuitableForTestResult(request).c_str(),
            descriptionSuitableForTestResult(redirectResponse).c_str());
    }

    if (!done && !gTestRunner->deferMainResourceDataLoad()) {
        COMPtr<IWebDataSourcePrivate> dataSourcePrivate(Query, dataSource);
        if (!dataSourcePrivate)
            return E_FAIL;
        dataSourcePrivate->setDeferMainResourceDataLoad(FALSE);
    }

    if (!done && gTestRunner->willSendRequestReturnsNull()) {
        *newRequest = 0;
        return S_OK;
    }

    if (!done && gTestRunner->willSendRequestReturnsNullOnRedirect() && redirectResponse) {
        printf("Returning null for this redirect\n");
        *newRequest = 0;
        return S_OK;
    }

    IWebMutableURLRequest* requestCopy = 0;
    request->mutableCopy(&requestCopy);
    const set<string>& clearHeaders = gTestRunner->willSendRequestClearHeaders();
    for (set<string>::const_iterator header = clearHeaders.begin(); header != clearHeaders.end(); ++header) {
      BSTR bstrHeader = BSTRFromString(*header);
      requestCopy->setValue(0, bstrHeader);
      SysFreeString(bstrHeader);
    }

    *newRequest = requestCopy;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ResourceLoadDelegate::didReceiveAuthenticationChallenge( 
    /* [in] */ IWebView *webView,
    /* [in] */ unsigned long identifier,
    /* [in] */ IWebURLAuthenticationChallenge *challenge,
    /* [in] */ IWebDataSource *dataSource)
{
    COMPtr<IWebURLAuthenticationChallengeSender> sender;
    if (!challenge || FAILED(challenge->sender(&sender)))
        return E_FAIL;

    if (!gTestRunner->handlesAuthenticationChallenges()) {
        printf("%S - didReceiveAuthenticationChallenge - Simulating cancelled authentication sheet\n", descriptionSuitableForTestResult(identifier).c_str());
        sender->continueWithoutCredentialForAuthenticationChallenge(challenge);
        return S_OK;
    }
    
    const char* user = gTestRunner->authenticationUsername().c_str();
    const char* password = gTestRunner->authenticationPassword().c_str();

    printf("%S - didReceiveAuthenticationChallenge - Responding with %s:%s\n", descriptionSuitableForTestResult(identifier).c_str(), user, password);

    COMPtr<IWebURLCredential> credential;
    if (FAILED(WebKitCreateInstance(CLSID_WebURLCredential, 0, IID_IWebURLCredential, (void**)&credential)))
        return E_FAIL;
    credential->initWithUser(_bstr_t(user), _bstr_t(password), WebURLCredentialPersistenceForSession);

    sender->useCredential(credential.get(), challenge);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ResourceLoadDelegate::didReceiveResponse(
    /* [in] */ IWebView* webView, 
    /* [in] */ unsigned long identifier, 
    /* [in] */ IWebURLResponse* response, 
    /* [in] */ IWebDataSource* dataSource)
{
    if (!done && gTestRunner->dumpResourceLoadCallbacks()) {
        printf("%S - didReceiveResponse %S\n",
            descriptionSuitableForTestResult(identifier).c_str(),
            descriptionSuitableForTestResult(response).c_str());
    }
    if (!done && gTestRunner->dumpResourceResponseMIMETypes()) {
        BSTR mimeTypeBSTR;
        if (FAILED(response->MIMEType(&mimeTypeBSTR)))
            E_FAIL;
    
        wstring mimeType = wstringFromBSTR(mimeTypeBSTR);
        ::SysFreeString(mimeTypeBSTR);

        BSTR urlBSTR;
        if (FAILED(response->URL(&urlBSTR)))
            E_FAIL;
    
        wstring url = wstringFromBSTR(urlBSTR);
        ::SysFreeString(urlBSTR);

        printf("%S has MIME type %S\n", lastPathComponent(url).c_str(), mimeType.c_str());
    }

    return S_OK;
}


HRESULT STDMETHODCALLTYPE ResourceLoadDelegate::didFinishLoadingFromDataSource( 
    /* [in] */ IWebView* webView,
    /* [in] */ unsigned long identifier,
    /* [in] */ IWebDataSource* dataSource)
{
    if (!done && gTestRunner->dumpResourceLoadCallbacks()) {
        printf("%S - didFinishLoading\n",
            descriptionSuitableForTestResult(identifier).c_str());
    }

    removeIdentifierForRequest(webView, identifier);

    return S_OK;
}
        
HRESULT STDMETHODCALLTYPE ResourceLoadDelegate::didFailLoadingWithError( 
    /* [in] */ IWebView* webView,
    /* [in] */ unsigned long identifier,
    /* [in] */ IWebError* error,
    /* [in] */ IWebDataSource* dataSource)
{
    if (!done && gTestRunner->dumpResourceLoadCallbacks()) {
        printf("%S - didFailLoadingWithError: %S\n", 
            descriptionSuitableForTestResult(identifier).c_str(),
            descriptionSuitableForTestResult(error, identifier).c_str());
    }

    removeIdentifierForRequest(webView, identifier);

    return S_OK;
}
