/*
 * Copyright (C) 2007 Apple Inc. All Rights Reserved.
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

#ifndef COMEnumVariant_h
#define COMEnumVariant_h

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <unknwn.h>


#include "COMVariantSetter.h"

template<typename ContainerType>
class COMEnumVariant : public IEnumVARIANT {
    WTF_MAKE_NONCOPYABLE(COMEnumVariant);
public:
    static COMEnumVariant* adopt(ContainerType&);
    static COMEnumVariant* createInstance(const ContainerType&);

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // IEnumVARIANT
    virtual HRESULT STDMETHODCALLTYPE Next(ULONG celt, VARIANT* rgVar, ULONG* pCeltFetched);
    virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt);
    virtual HRESULT STDMETHODCALLTYPE Reset();
    virtual HRESULT STDMETHODCALLTYPE Clone(IEnumVARIANT** ppEnum);

private:
    COMEnumVariant()
        : m_refCount(0)
    {
    }

    COMEnumVariant(const ContainerType& container)
        : m_refCount(0)
        , m_container(container)       
        , m_currentPos(m_container.begin())
    {
    }

    ~COMEnumVariant() {}

    ULONG m_refCount;

    ContainerType m_container;
    typename ContainerType::const_iterator m_currentPos;
};

// COMEnumVariant ------------------------------------------------------------------
template<typename ContainerType>
COMEnumVariant<typename ContainerType>* COMEnumVariant<ContainerType>::adopt(ContainerType& container) 
{
    COMEnumVariant* instance = new COMEnumVariant;
    instance->m_container.swap(container);
    instance->m_currentPos = instance->m_container.begin();
    instance->AddRef();
    return instance;
}

template<typename ContainerType>
COMEnumVariant<typename ContainerType>* COMEnumVariant<ContainerType>::createInstance(const ContainerType& container)
{
    COMEnumVariant* instance = new COMEnumVariant(container);
    instance->AddRef();
    return instance;
}

// IUnknown ------------------------------------------------------------------------
template<typename ContainerType>
HRESULT STDMETHODCALLTYPE COMEnumVariant<ContainerType>::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<COMEnumVariant*>(this);
    else if (IsEqualGUID(riid, IID_IEnumVARIANT))
        *ppvObject = static_cast<COMEnumVariant*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

template<typename ContainerType>
ULONG STDMETHODCALLTYPE COMEnumVariant<ContainerType>::AddRef()
{
    return ++m_refCount;
}

template<typename ContainerType>
ULONG STDMETHODCALLTYPE COMEnumVariant<ContainerType>::Release()
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete this;

    return newRef;
}

// IEnumVARIANT --------------------------------------------------------------------
template<typename ContainerType>
HRESULT STDMETHODCALLTYPE COMEnumVariant<ContainerType>::Next(ULONG celt, VARIANT* rgVar, ULONG* pCeltFetched)
{
    if (pCeltFetched)
        *pCeltFetched = 0;
    if (!rgVar)
        return E_POINTER;
    for (unsigned i = 0 ; i < celt; i++)
        VariantInit(&rgVar[i]);

    for (unsigned i = 0; i < celt; i++) {
        if (m_currentPos == m_container.end())
            return S_FALSE;

        COMVariantSetter<ContainerType::ValueType>::setVariant(&rgVar[i], *m_currentPos);
        ++m_currentPos;
        if (pCeltFetched)
            (*pCeltFetched)++;
    }

    return S_OK;
}

template<typename ContainerType>
HRESULT STDMETHODCALLTYPE COMEnumVariant<ContainerType>::Skip(ULONG celt) 
{
    for (unsigned i = 0; i < celt; i++) {
        if (m_currentPos == m_container.end())
            return S_FALSE;

        ++m_currentPos;
    }
    return S_OK;
}
    
template<typename ContainerType>
HRESULT STDMETHODCALLTYPE COMEnumVariant<ContainerType>::Reset() 
{
    m_currentPos = m_container.begin();
    return S_OK;
}
    
template<typename ContainerType>
HRESULT STDMETHODCALLTYPE COMEnumVariant<ContainerType>::Clone(IEnumVARIANT** ppEnum)
{
    if (!ppEnum)
        return E_POINTER;

    *ppEnum = 0;
    return E_NOTIMPL;
}

#endif
