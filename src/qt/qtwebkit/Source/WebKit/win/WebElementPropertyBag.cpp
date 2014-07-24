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
#include "WebElementPropertyBag.h"

#include "MarshallingHelpers.h"
#include "DOMCoreClasses.h"
#include "WebFrame.h"
#include <WebCore/Document.h>
#include <WebCore/Frame.h>
#include <WebCore/HitTestResult.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/Image.h>
#include <WebCore/KURL.h>
#include <WebCore/RenderObject.h>

using namespace WebCore;

// WebElementPropertyBag -----------------------------------------------
WebElementPropertyBag::WebElementPropertyBag(const HitTestResult& result)
    : m_result(adoptPtr(new HitTestResult(result)))
    , m_refCount(0)
{
    gClassCount++;
    gClassNameCount.add("WebElementPropertyBag");
}

WebElementPropertyBag::~WebElementPropertyBag()
{
    gClassCount--;
    gClassNameCount.remove("WebElementPropertyBag");
}

WebElementPropertyBag* WebElementPropertyBag::createInstance(const HitTestResult& result)
{
    WebElementPropertyBag* instance = new WebElementPropertyBag(result); 
    instance->AddRef();

    return instance;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebElementPropertyBag::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IPropertyBag*>(this);
    else if (IsEqualGUID(riid, IID_IPropertyBag))
        *ppvObject = static_cast<IPropertyBag*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebElementPropertyBag::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebElementPropertyBag::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete this;

    return newRef;
}

static bool isEqual(LPCWSTR s1, LPCWSTR s2)
{
    return !wcscmp(s1, s2);
}

static HRESULT convertStringToVariant(VARIANT* pVar, const String& string)
{
    V_VT(pVar) = VT_BSTR;
    V_BSTR(pVar) = SysAllocStringLen(string.characters(), string.length());
    if (string.length() && !V_BSTR(pVar))
        return E_OUTOFMEMORY;

    return S_OK;
}


HRESULT STDMETHODCALLTYPE WebElementPropertyBag::Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog * /*pErrorLog*/)
{
    if (!pszPropName)
        return E_POINTER;

    if (!m_result)
        return E_FAIL;

    BSTR key = (BSTR)pszPropName;
    VariantClear(pVar);
    if (isEqual(WebElementDOMNodeKey, key)) {
        IDOMNode* node = DOMNode::createInstance(m_result->innerNonSharedNode());
        V_VT(pVar) = VT_UNKNOWN;
        V_UNKNOWN(pVar) = node;
        return S_OK;
    } else if (isEqual(WebElementFrameKey, key)) {
        if (!(m_result->innerNonSharedNode() && m_result->innerNonSharedNode()->document()
           && m_result->innerNonSharedNode()->document()->frame()))
            return E_FAIL;
        Frame* coreFrame = m_result->innerNonSharedNode()->document()->frame();
        WebFrame* webFrame = static_cast<WebFrame*>(coreFrame->loader()->client());
        IWebFrame* iWebFrame;
        if (FAILED(webFrame->QueryInterface(IID_IWebFrame, (void**)&iWebFrame)))
            return E_FAIL;
        V_VT(pVar) = VT_UNKNOWN;
        V_UNKNOWN(pVar) = iWebFrame;
        return S_OK;
    } else if (isEqual(WebElementImageAltStringKey, key))
        return convertStringToVariant(pVar, m_result->altDisplayString());
    else if (isEqual(WebElementImageKey, key)) {
        V_VT(pVar) = VT_BYREF;
        V_BYREF(pVar) = m_result->image();
        return S_OK;
    } else if (isEqual(WebElementImageRectKey, key)) {
        V_VT(pVar) = VT_ARRAY;
        IntRect boundingBox = m_result->innerNonSharedNode() && m_result->innerNonSharedNode()->renderer() ?
                                m_result->innerNonSharedNode()->renderer()->absoluteBoundingBoxRect(true) : IntRect();
        V_ARRAY(pVar) = MarshallingHelpers::intRectToSafeArray(boundingBox);
        return S_OK;
    } else if (isEqual(WebElementImageURLKey, key))
        return convertStringToVariant(pVar, m_result->absoluteImageURL().string());
    else if (isEqual(WebElementIsSelectedKey, key)) {
        V_VT(pVar) = VT_BOOL;
        if (m_result->isSelected())
            V_BOOL(pVar) = VARIANT_TRUE;
        else
            V_BOOL(pVar) = VARIANT_FALSE;
        return S_OK;
    }
    if (isEqual(WebElementMediaURLKey, key))
        return convertStringToVariant(pVar, m_result->absoluteMediaURL().string());
    if (isEqual(WebElementSpellingToolTipKey, key)) {
        TextDirection dir;
        return convertStringToVariant(pVar, m_result->spellingToolTip(dir));
    } else if (isEqual(WebElementTitleKey, key)) {
        TextDirection dir;
        return convertStringToVariant(pVar, m_result->title(dir));
    }
    else if (isEqual(WebElementLinkURLKey, key))
        return convertStringToVariant(pVar, m_result->absoluteLinkURL().string());
    else if (isEqual(WebElementLinkTargetFrameKey, key)) {
        if (!m_result->targetFrame())
            return E_FAIL;
        WebFrame* webFrame = kit(m_result->targetFrame());
        IWebFrame* iWebFrame;
        if (FAILED(webFrame->QueryInterface(IID_IWebFrame, (void**)&iWebFrame)))
            return E_FAIL;
        V_VT(pVar) = VT_UNKNOWN;
        V_UNKNOWN(pVar) = iWebFrame;
        return S_OK;
    } else if (isEqual(WebElementLinkTitleKey, key))
        return convertStringToVariant(pVar, m_result->titleDisplayString());
    else if (isEqual(WebElementLinkLabelKey, key))
        return convertStringToVariant(pVar, m_result->textContent());
    else if (isEqual(WebElementIsContentEditableKey, key)) {
        V_VT(pVar) = VT_BOOL;
        if (m_result->isContentEditable())
            V_BOOL(pVar) = VARIANT_TRUE;
        else
            V_BOOL(pVar) = VARIANT_FALSE;
        return S_OK;
    }

    return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE WebElementPropertyBag::Write(LPCOLESTR pszPropName, VARIANT* pVar)
{
    if (!pszPropName || !pVar)
        return E_POINTER;

    return E_FAIL;
}
