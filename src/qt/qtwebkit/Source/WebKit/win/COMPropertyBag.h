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

#ifndef COMPropertyBag_h
#define COMPropertyBag_h

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <ocidl.h>
#include <unknwn.h>

#include <wtf/Noncopyable.h>
#include <wtf/HashMap.h>

#include "COMVariantSetter.h"

template<typename ValueType, typename KeyType = typename WTF::String, typename HashType = typename WTF::StringHash>
class COMPropertyBag : public IPropertyBag, public IPropertyBag2 {
    WTF_MAKE_NONCOPYABLE(COMPropertyBag);
public:
    typedef HashMap<KeyType, ValueType, HashType> HashMapType;

    static COMPropertyBag* createInstance(const HashMapType&);
    static COMPropertyBag* adopt(HashMapType&);

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // IPropertyBag
    virtual HRESULT STDMETHODCALLTYPE Read(LPCOLESTR pszPropName, VARIANT*, IErrorLog*);
    virtual HRESULT STDMETHODCALLTYPE Write(LPCOLESTR pszPropName, VARIANT*);

    // IPropertyBag2
    virtual HRESULT STDMETHODCALLTYPE Read(ULONG cProperties, PROPBAG2*, IErrorLog*, VARIANT* pvarValue, HRESULT* phrError);
    virtual HRESULT STDMETHODCALLTYPE Write(ULONG cProperties, PROPBAG2*, VARIANT*);
    virtual HRESULT STDMETHODCALLTYPE CountProperties(ULONG* pcProperties);
    virtual HRESULT STDMETHODCALLTYPE GetPropertyInfo(ULONG iProperty, ULONG cProperties, PROPBAG2* pPropBag, ULONG* pcProperties);
    virtual HRESULT STDMETHODCALLTYPE LoadObject(LPCOLESTR pstrName, DWORD dwHint, IUnknown*, IErrorLog*);

private:
    COMPropertyBag()
        : m_refCount(0)
    {
    }

    COMPropertyBag(const HashMapType& hashMap)
        : m_refCount(0)
        , m_hashMap(hashMap)
    {
    }

    ~COMPropertyBag() {}

    ULONG m_refCount;
    HashMapType m_hashMap;
};

// COMPropertyBag ------------------------------------------------------------------
template<typename ValueType, typename KeyType, typename HashType>
COMPropertyBag<ValueType, KeyType, HashType>* COMPropertyBag<typename ValueType, typename KeyType, HashType>::createInstance(const HashMapType& hashMap)
{
    COMPropertyBag* instance = new COMPropertyBag(hashMap);
    instance->AddRef();
    return instance;
}

template<typename ValueType, typename KeyType, typename HashType>
COMPropertyBag<ValueType, KeyType, HashType>* COMPropertyBag<typename ValueType, typename KeyType, HashType>::adopt(HashMapType& hashMap)
{
    COMPropertyBag* instance = new COMPropertyBag;
    instance->m_hashMap.swap(hashMap);
    instance->AddRef();
    return instance;
}

// IUnknown ------------------------------------------------------------------------
template<typename ValueType, typename KeyType, typename HashType>
HRESULT STDMETHODCALLTYPE COMPropertyBag<ValueType, KeyType, HashType>::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IPropertyBag*>(this);
    else if (IsEqualGUID(riid, IID_IPropertyBag))
        *ppvObject = static_cast<IPropertyBag*>(this);
    else if (IsEqualGUID(riid, IID_IPropertyBag2))
        *ppvObject = static_cast<IPropertyBag2*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

template<typename ValueType, typename KeyType, typename HashType>
ULONG STDMETHODCALLTYPE COMPropertyBag<ValueType, KeyType, HashType>::AddRef()
{
    return ++m_refCount;
}

template<typename ValueType, typename KeyType, typename HashType>
ULONG STDMETHODCALLTYPE COMPropertyBag<ValueType, KeyType, HashType>::Release()
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete this;

    return newRef;
}

// IPropertyBag --------------------------------------------------------------------

template<typename ValueType, typename KeyType, typename HashType>
HRESULT STDMETHODCALLTYPE COMPropertyBag<ValueType, KeyType, HashType>::Read(LPCOLESTR pszPropName, VARIANT* pVar, IErrorLog* pErrorLog)
{
    if (!pszPropName)
        return E_POINTER;

    HashMapType::const_iterator it = m_hashMap.find(String(pszPropName));
    HashMapType::const_iterator end = m_hashMap.end();
    if (it == end)
        return E_INVALIDARG;

    VARTYPE requestedType = V_VT(pVar);
    V_VT(pVar) = VT_EMPTY;
    COMVariantSetter<ValueType>::setVariant(pVar, it->value);

    if (requestedType != COMVariantSetter<ValueType>::variantType(it->value) && requestedType != VT_EMPTY)
        return ::VariantChangeType(pVar, pVar, VARIANT_NOUSEROVERRIDE | VARIANT_ALPHABOOL, requestedType);

    return S_OK;
}

template<typename ValueType, typename KeyType, typename HashType>
HRESULT STDMETHODCALLTYPE COMPropertyBag<ValueType, KeyType, HashType>::Write(LPCOLESTR pszPropName, VARIANT* pVar)
{
    return E_FAIL;
}

template<typename ValueType, typename KeyType, typename HashType>
HRESULT STDMETHODCALLTYPE COMPropertyBag<ValueType, KeyType, HashType>::Read(ULONG cProperties, PROPBAG2* pPropBag, IErrorLog* pErrorLog, VARIANT* pvarValue, HRESULT* phrError)
{
    if (!pPropBag || !pvarValue || !phrError)
        return E_POINTER;

    HRESULT hr = S_OK;

    for (ULONG i = 0; i < cProperties; ++i) {
        VariantInit(&pvarValue[i]);
        pvarValue[i].vt = pPropBag[i].vt;
        phrError[i] = Read(pPropBag[i].pstrName, &pvarValue[i], pErrorLog);
        if (FAILED(phrError[i]))
            hr = E_FAIL;
    }

    return hr;
}

template<typename ValueType, typename KeyType, typename HashType>
HRESULT STDMETHODCALLTYPE COMPropertyBag<ValueType, KeyType, HashType>::Write(ULONG cProperties, PROPBAG2*, VARIANT*)
{
    return E_NOTIMPL;
}

template<typename ValueType, typename KeyType, typename HashType>
HRESULT STDMETHODCALLTYPE COMPropertyBag<ValueType, KeyType, HashType>::CountProperties(ULONG* pcProperties)
{
    if (!pcProperties)
        return E_POINTER;

    *pcProperties = m_hashMap.size();
    return S_OK;
}

template<typename ValueType, typename KeyType, typename HashType>
HRESULT STDMETHODCALLTYPE COMPropertyBag<ValueType, KeyType, HashType>::GetPropertyInfo(ULONG iProperty, ULONG cProperties, PROPBAG2* pPropBag, ULONG* pcProperties)
{
    if (!pPropBag || !pcProperties)
        return E_POINTER;

    if (m_hashMap.size() <= iProperty)
        return E_INVALIDARG;

    *pcProperties = 0;
    typedef HashMapType::const_iterator Iterator;
    Iterator current = m_hashMap.begin();
    Iterator end = m_hashMap.end();
    for (ULONG i = 0; i < iProperty; ++i, ++current)
        ;
    for (ULONG j = 0; j < cProperties && current != end; ++j, ++current) {
        // FIXME: the following fields aren't filled in
        //pPropBag[j].cfType;   // (CLIPFORMAT) Clipboard format or MIME type of the property.
        //pPropBag[j].clsid;    // (CLSID) CLSID of the object. This member is valid only if dwType is PROPBAG2_TYPE_OBJECT.

        pPropBag[j].dwType = PROPBAG2_TYPE_DATA;
        pPropBag[j].vt = COMVariantSetter<ValueType>::variantType(current->value);
        pPropBag[j].dwHint = iProperty + j;
        pPropBag[j].pstrName = (LPOLESTR)CoTaskMemAlloc(sizeof(wchar_t)*(current->key.length()+1));
        if (!pPropBag[j].pstrName)
            return E_OUTOFMEMORY;
        wcscpy_s(pPropBag[j].pstrName, current->key.length()+1, static_cast<String>(current->key).charactersWithNullTermination().data());
        ++*pcProperties;
    }
    return S_OK;
}

template<typename ValueType, typename KeyType, typename HashType>
HRESULT STDMETHODCALLTYPE COMPropertyBag<ValueType, KeyType, HashType>::LoadObject(LPCOLESTR pstrName, DWORD dwHint, IUnknown*, IErrorLog*)
{
    return E_NOTIMPL;
}

#endif // COMPropertyBag_h
