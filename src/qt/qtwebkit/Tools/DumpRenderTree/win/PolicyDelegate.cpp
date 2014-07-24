/*
 * Copyright (C) 2007, 2009 Apple Inc. All rights reserved.
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
#include "PolicyDelegate.h"

#include "DumpRenderTree.h"
#include "TestRunner.h"
#include <string>

using std::wstring;

static wstring dumpPath(IDOMNode* node)
{
    ASSERT(node);

    wstring result;

    BSTR name;
    if (FAILED(node->nodeName(&name)))
        return result;
    result.assign(name, SysStringLen(name));
    SysFreeString(name);

    COMPtr<IDOMNode> parent;
    if (SUCCEEDED(node->parentNode(&parent)))
        result += TEXT(" > ") + dumpPath(parent.get());

    return result;
}

PolicyDelegate::PolicyDelegate()
    : m_refCount(1)
    , m_permissiveDelegate(false)
    , m_controllerToNotifyDone(0)
{
}

// IUnknown
HRESULT STDMETHODCALLTYPE PolicyDelegate::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebPolicyDelegate*>(this);
    else if (IsEqualGUID(riid, IID_IWebPolicyDelegate))
        *ppvObject = static_cast<IWebPolicyDelegate*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE PolicyDelegate::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE PolicyDelegate::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete this;

    return newRef;
}

HRESULT STDMETHODCALLTYPE PolicyDelegate::decidePolicyForNavigationAction(
    /*[in]*/ IWebView* /*webView*/, 
    /*[in]*/ IPropertyBag* actionInformation, 
    /*[in]*/ IWebURLRequest* request, 
    /*[in]*/ IWebFrame* frame, 
    /*[in]*/ IWebPolicyDecisionListener* listener)
{
    BSTR url;
    request->URL(&url);
    wstring wurl = urlSuitableForTestResult(wstring(url, SysStringLen(url)));

    int navType = 0;
    VARIANT var;
    if (SUCCEEDED(actionInformation->Read(WebActionNavigationTypeKey, &var, 0))) {
        V_VT(&var) = VT_I4;
        navType = V_I4(&var);
    }

    LPCTSTR typeDescription;
    switch (navType) {
        case WebNavigationTypeLinkClicked:
            typeDescription = TEXT("link clicked");
            break;
        case WebNavigationTypeFormSubmitted:
            typeDescription = TEXT("form submitted");
            break;
        case WebNavigationTypeBackForward:
            typeDescription = TEXT("back/forward");
            break;
        case WebNavigationTypeReload:
            typeDescription = TEXT("reload");
            break;
        case WebNavigationTypeFormResubmitted:
            typeDescription = TEXT("form resubmitted");
            break;
        case WebNavigationTypeOther:
            typeDescription = TEXT("other");
            break;
        default:
            typeDescription = TEXT("illegal value");
    }

    wstring message = TEXT("Policy delegate: attempt to load ") + wurl + TEXT(" with navigation type '") + typeDescription + TEXT("'");

    VARIANT actionElementVar;
    if (SUCCEEDED(actionInformation->Read(WebActionElementKey, &actionElementVar, 0))) {
        COMPtr<IPropertyBag> actionElement(Query, V_UNKNOWN(&actionElementVar));
        VARIANT originatingNodeVar;
        if (SUCCEEDED(actionElement->Read(WebElementDOMNodeKey, &originatingNodeVar, 0))) {
            COMPtr<IDOMNode> originatingNode(Query, V_UNKNOWN(&originatingNodeVar));
            message += TEXT(" originating from ") + dumpPath(originatingNode.get());
        }
    }

    printf("%S\n", message.c_str());

    SysFreeString(url);

    if (m_permissiveDelegate)
        listener->use();
    else
        listener->ignore();

    if (m_controllerToNotifyDone) {
        m_controllerToNotifyDone->notifyDone();
        m_controllerToNotifyDone = 0;
    }

    return S_OK;
}


HRESULT STDMETHODCALLTYPE PolicyDelegate::unableToImplementPolicyWithError(
    /*[in]*/ IWebView* /*webView*/, 
    /*[in]*/ IWebError* error, 
    /*[in]*/ IWebFrame* frame)
{
    BSTR domainStr;
    error->domain(&domainStr);
    wstring domainMessage = domainStr;

    int code;
    error->code(&code);
    
    BSTR frameName;
    frame->name(&frameName);
    wstring frameNameMessage = frameName;
    
    printf("Policy delegate: unable to implement policy with error domain '%S', error code %d, in frame '%S'", domainMessage.c_str(), code, frameNameMessage.c_str());
    
    SysFreeString(domainStr);
    SysFreeString(frameName);
    
    return S_OK;
}
