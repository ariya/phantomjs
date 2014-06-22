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
#include "CFDictionaryPropertyBag.h"

#include "MarshallingHelpers.h"
#include "WebKit.h"

// CFDictionaryPropertyBag -----------------------------------------------

CFDictionaryPropertyBag::CFDictionaryPropertyBag()
: m_refCount(0)
{
    gClassCount++;
    gClassNameCount.add("CFDictionaryPropertyBag");
}

CFDictionaryPropertyBag::~CFDictionaryPropertyBag()
{
    gClassCount--;
    gClassNameCount.remove("CFDictionaryPropertyBag");
}

COMPtr<CFDictionaryPropertyBag> CFDictionaryPropertyBag::createInstance()
{
    return new CFDictionaryPropertyBag;
}

void CFDictionaryPropertyBag::setDictionary(CFMutableDictionaryRef dictionary)
{
    m_dictionary = dictionary;
}

CFMutableDictionaryRef CFDictionaryPropertyBag::dictionary() const
{
    return m_dictionary.get();
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CFDictionaryPropertyBag::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IPropertyBag*>(this);
    else if (IsEqualGUID(riid, IID_IPropertyBag))
        *ppvObject = static_cast<IPropertyBag*>(this);
    else if (IsEqualGUID(riid, __uuidof(this)))
        *ppvObject = this;
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE CFDictionaryPropertyBag::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE CFDictionaryPropertyBag::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IPropertyBag ------------------------------------------------------------

static bool ConvertCFTypeToVariant(VARIANT* pVar, void* cfObj)
{
    if (!cfObj) {
        V_VT(pVar) = VT_NULL;
        return true;
    }
    else {
        // if caller expects a string, retrieve BSTR from CFStringRef
        if (V_VT(pVar) == VT_BSTR) {
            V_BSTR(pVar) = MarshallingHelpers::CFStringRefToBSTR((CFStringRef) cfObj);
            return true;
        } else if (V_VT(pVar) == VT_I4) {
            V_I4(pVar) = MarshallingHelpers::CFNumberRefToInt((CFNumberRef) cfObj);
            return true;
        } else if (!!(V_VT(pVar)&VT_ARRAY)) {
            if ((V_VT(pVar)&~VT_ARRAY) == VT_BSTR) {
                V_ARRAY(pVar) = MarshallingHelpers::stringArrayToSafeArray((CFArrayRef) cfObj);
                return true;
            } else if ((V_VT(pVar)&~VT_ARRAY) == VT_I4) {
                V_ARRAY(pVar) = MarshallingHelpers::intArrayToSafeArray((CFArrayRef) cfObj);
                return true;
            } else if ((V_VT(pVar)&~VT_ARRAY) == VT_UNKNOWN) {
                V_ARRAY(pVar) = MarshallingHelpers::iunknownArrayToSafeArray((CFArrayRef) cfObj);
                return true;
            }
        }
    }
    return false;
}

static bool ConvertVariantToCFType(VARIANT* pVar, void** cfObj)
{
    if (V_VT(pVar) == VT_NULL) {
        *cfObj = 0;
        return true;
    }
    else {
        // if caller expects a string, retrieve BSTR from CFStringRef
        if (V_VT(pVar) == VT_BSTR) {
            *cfObj = (void*) MarshallingHelpers::BSTRToCFStringRef(V_BSTR(pVar));
            return true;
        } else if (V_VT(pVar) == VT_I4) {
            *cfObj = (void*) MarshallingHelpers::intToCFNumberRef(V_I4(pVar));
            return true;
        } else if (!!(V_VT(pVar)&VT_ARRAY)) {
            if ((V_VT(pVar)&~VT_ARRAY) == VT_BSTR) {
                *cfObj = (void*) MarshallingHelpers::safeArrayToStringArray(V_ARRAY(pVar));
                return true;
            } else if ((V_VT(pVar)&~VT_ARRAY) == VT_I4) {
                *cfObj = (void*) MarshallingHelpers::safeArrayToIntArray(V_ARRAY(pVar));
                return true;
            } else if ((V_VT(pVar)&~VT_ARRAY) == VT_UNKNOWN) {
                *cfObj = (void*) MarshallingHelpers::safeArrayToIUnknownArray(V_ARRAY(pVar));
                return true;
            }
        }
    }
    return false;
}

HRESULT STDMETHODCALLTYPE CFDictionaryPropertyBag::Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog * /*pErrorLog*/)
{
    if (!pszPropName)
        return E_POINTER;
    if (m_dictionary) {
        void* value;
        CFStringRef key = MarshallingHelpers::LPCOLESTRToCFStringRef(pszPropName);
        HRESULT hr = E_FAIL;
        if (CFDictionaryGetValueIfPresent(m_dictionary.get(), key, (const void**) &value)) {
            if (ConvertCFTypeToVariant(pVar, value))
                hr = S_OK;
        } else
            hr = E_INVALIDARG;
        CFRelease(key);
        return hr;
    }
    return E_FAIL;
}
        
HRESULT STDMETHODCALLTYPE CFDictionaryPropertyBag::Write(LPCOLESTR pszPropName, VARIANT* pVar)
{
    if (!pszPropName || !pVar)
        return E_POINTER;
    if (!m_dictionary) {
        m_dictionary = adoptCF(CFDictionaryCreateMutable(0, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
    }
    void* cfObj;
    if (ConvertVariantToCFType(pVar, &cfObj)) {
        CFStringRef key = MarshallingHelpers::LPCOLESTRToCFStringRef(pszPropName);
        CFDictionaryAddValue(m_dictionary.get(), key, cfObj);
        // CFDictionaryAddValue should automatically retain the CF objects passed in, so release them here
        CFRelease(key);
        CFRelease(cfObj);
        return S_OK;
    }
    return E_FAIL;
}