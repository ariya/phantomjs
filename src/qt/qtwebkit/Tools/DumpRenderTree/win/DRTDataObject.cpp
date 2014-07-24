/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2012 Baidu Inc.  All rights reserved.
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
#include "DRTDataObject.h"

FORMATETC* cfHDropFormat()
{
    static FORMATETC urlFormat = {CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    return &urlFormat;
}

FORMATETC* cfFileNameWFormat()
{
    static UINT cf = RegisterClipboardFormat(L"FileNameW");
    static FORMATETC urlFormat = {cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    return &urlFormat;
}

FORMATETC* cfUrlWFormat()
{
    static UINT cf = RegisterClipboardFormat(L"UniformResourceLocatorW");
    static FORMATETC urlFormat = {cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    return &urlFormat;
}

class WCEnumFormatEtc : public IEnumFORMATETC {
public:
    explicit WCEnumFormatEtc(const Vector<FORMATETC>& formats);
    explicit WCEnumFormatEtc(const Vector<FORMATETC*>& formats);

    // IUnknown members
    STDMETHOD(QueryInterface)(REFIID, void**);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IEnumFORMATETC members
    STDMETHOD(Next)(ULONG, LPFORMATETC, ULONG*);
    STDMETHOD(Skip)(ULONG);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumFORMATETC**);

private:
    long m_ref;
    Vector<FORMATETC> m_formats;
    size_t m_current;
};

WCEnumFormatEtc::WCEnumFormatEtc(const Vector<FORMATETC>& formats)
    : m_ref(1)
    , m_current(0)
    , m_formats(formats)
{
}

WCEnumFormatEtc::WCEnumFormatEtc(const Vector<FORMATETC*>& formats)
    : m_ref(1)
    , m_current(0)
{
    for (size_t i = 0; i < formats.size(); ++i)
        m_formats.append(*formats[i]);
}

STDMETHODIMP WCEnumFormatEtc::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IEnumFORMATETC)) {
        *ppvObject = this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) WCEnumFormatEtc::AddRef()
{
    return InterlockedIncrement(&m_ref);
}

STDMETHODIMP_(ULONG) WCEnumFormatEtc::Release()
{
    long refCount = InterlockedDecrement(&m_ref);
    if (!refCount)
        delete this;
    return refCount;
}

STDMETHODIMP WCEnumFormatEtc::Next(ULONG celt, LPFORMATETC lpFormatEtc, ULONG* pceltFetched)
{
    if (pceltFetched)
        *pceltFetched = 0;

    ULONG cReturn = celt;

    if (celt <= 0 || !lpFormatEtc || m_current >= m_formats.size())
        return S_FALSE;

    if (!pceltFetched && celt != 1) // pceltFetched can be 0 only for 1 item request
        return S_FALSE;

    while (m_current < m_formats.size() && cReturn > 0) {
        *lpFormatEtc++ = m_formats[m_current++];
        --cReturn;
    }
    if (pceltFetched)
        *pceltFetched = celt - cReturn;

    return !cReturn ? S_OK : S_FALSE;
}

STDMETHODIMP WCEnumFormatEtc::Skip(ULONG celt)
{
    if ((m_current + celt) >= m_formats.size())
        return S_FALSE;
    m_current += celt;
    return S_OK;
}

STDMETHODIMP WCEnumFormatEtc::Reset()
{
    m_current = 0;
    return S_OK;
}

STDMETHODIMP WCEnumFormatEtc::Clone(IEnumFORMATETC** ppCloneEnumFormatEtc)
{
    if (!ppCloneEnumFormatEtc)
        return E_POINTER;

    WCEnumFormatEtc* newEnum = new WCEnumFormatEtc(m_formats);
    if (!newEnum)
        return E_OUTOFMEMORY;

    newEnum->AddRef();
    newEnum->m_current = m_current;
    *ppCloneEnumFormatEtc = newEnum;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT DRTDataObject::createInstance(DRTDataObject** result)
{
    if (!result)
        return E_POINTER;
    *result = new DRTDataObject();
    return S_OK;
}

DRTDataObject::DRTDataObject()
    : m_ref(1)
{
}

DRTDataObject::~DRTDataObject()
{
    for (size_t i = 0; i < m_medium.size(); ++i) {
        ReleaseStgMedium(m_medium[i]);
        delete m_medium[i];
    }
    WTF::deleteAllValues(m_formats);
}

STDMETHODIMP DRTDataObject::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDataObject))
        *ppvObject = this;

    if (*ppvObject) {
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) DRTDataObject::AddRef()
{
    return InterlockedIncrement(&m_ref);
}

STDMETHODIMP_(ULONG) DRTDataObject::Release()
{
    long refCount = InterlockedDecrement(&m_ref);
    if (!refCount)
        delete this;
    return refCount;
}

STDMETHODIMP DRTDataObject::GetData(FORMATETC* pformatetcIn, STGMEDIUM* pmedium)
{ 
    if (!pformatetcIn || !pmedium)
        return E_POINTER;
    pmedium->hGlobal = 0;

    for (size_t i = 0; i < m_formats.size(); ++i) {
        if (pformatetcIn->lindex == m_formats[i]->lindex && pformatetcIn->dwAspect == m_formats[i]->dwAspect && pformatetcIn->cfFormat == m_formats[i]->cfFormat) {
            CopyMedium(pmedium, m_medium[i], m_formats[i]);
            return S_OK;
        }
    }
    return DV_E_FORMATETC;
}

STDMETHODIMP DRTDataObject::GetDataHere(FORMATETC*, STGMEDIUM*)
{ 
    return E_NOTIMPL;
}

STDMETHODIMP DRTDataObject::QueryGetData(FORMATETC* pformatetc)
{ 
    if (!pformatetc)
        return E_POINTER;

    if (!(DVASPECT_CONTENT & pformatetc->dwAspect))
        return (DV_E_DVASPECT);

    for (size_t i = 0; i < m_formats.size(); ++i) {
        if (pformatetc->tymed & m_formats[i]->tymed) {
            if (pformatetc->cfFormat == m_formats[i]->cfFormat)
                return S_OK;
        }
    }
    return DV_E_TYMED;
}

STDMETHODIMP DRTDataObject::GetCanonicalFormatEtc(FORMATETC*, FORMATETC*)
{ 
    return DATA_S_SAMEFORMATETC;
}

STDMETHODIMP DRTDataObject::SetData(FORMATETC* pformatetc, STGMEDIUM* pmedium, BOOL fRelease)
{ 
    if (!pformatetc || !pmedium)
        return E_POINTER;

    FORMATETC* formatetc = new FORMATETC;
    if (!formatetc)
        return E_OUTOFMEMORY;

    STGMEDIUM* pStgMed = new STGMEDIUM;

    if (!pStgMed) {
        delete formatetc;
        return E_OUTOFMEMORY;
    }

    ZeroMemory(formatetc, sizeof(FORMATETC));
    ZeroMemory(pStgMed, sizeof(STGMEDIUM));

    *formatetc = *pformatetc;
    m_formats.append(formatetc);

    if (fRelease)
        *pStgMed = *pmedium;
    else
        CopyMedium(pStgMed, pmedium, pformatetc);
    m_medium.append(pStgMed);

    return S_OK;
}

void DRTDataObject::CopyMedium(STGMEDIUM* pMedDest, STGMEDIUM* pMedSrc, FORMATETC* pFmtSrc)
{
    switch (pMedSrc->tymed) {
#if !OS(WINCE)
    case TYMED_HGLOBAL:
        pMedDest->hGlobal = static_cast<HGLOBAL>(OleDuplicateData(pMedSrc->hGlobal, pFmtSrc->cfFormat, 0));
        break;
    case TYMED_GDI:
        pMedDest->hBitmap = static_cast<HBITMAP>(OleDuplicateData(pMedSrc->hBitmap, pFmtSrc->cfFormat, 0));
        break;
    case TYMED_MFPICT:
        pMedDest->hMetaFilePict = static_cast<HMETAFILEPICT>(OleDuplicateData(pMedSrc->hMetaFilePict, pFmtSrc->cfFormat, 0));
        break;
    case TYMED_ENHMF:
        pMedDest->hEnhMetaFile = static_cast<HENHMETAFILE>(OleDuplicateData(pMedSrc->hEnhMetaFile, pFmtSrc->cfFormat, 0));
        break;
    case TYMED_FILE:
        pMedSrc->lpszFileName = static_cast<LPOLESTR>(OleDuplicateData(pMedSrc->lpszFileName, pFmtSrc->cfFormat, 0));
        break;
#endif
    case TYMED_ISTREAM:
        pMedDest->pstm = pMedSrc->pstm;
        pMedSrc->pstm->AddRef();
        break;
    case TYMED_ISTORAGE:
        pMedDest->pstg = pMedSrc->pstg;
        pMedSrc->pstg->AddRef();
        break;
    default:
        break;
    }
    pMedDest->tymed = pMedSrc->tymed;
    pMedDest->pUnkForRelease = 0;
    if (pMedSrc->pUnkForRelease) {
        pMedDest->pUnkForRelease = pMedSrc->pUnkForRelease;
        pMedSrc->pUnkForRelease->AddRef();
    }
}
STDMETHODIMP DRTDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc)
{ 
    if (!ppenumFormatEtc)
        return E_POINTER;

    *ppenumFormatEtc = 0;
    switch (dwDirection) {
    case DATADIR_GET:
        *ppenumFormatEtc = new WCEnumFormatEtc(m_formats);
        if (!(*ppenumFormatEtc))
            return E_OUTOFMEMORY;
        break;

    case DATADIR_SET:
    default:
        return E_NOTIMPL;
        break;
    }

    return S_OK;
}

STDMETHODIMP DRTDataObject::DAdvise(FORMATETC*, DWORD, IAdviseSink*, DWORD*)
{ 
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP DRTDataObject::DUnadvise(DWORD)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DRTDataObject::EnumDAdvise(IEnumSTATDATA**)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

void DRTDataObject::clearData(CLIPFORMAT format)
{
    size_t position = 0;
    while (position < m_formats.size()) {
        if (m_formats[position]->cfFormat == format) {
            FORMATETC* current = m_formats[position];
            m_formats[position] = m_formats[m_formats.size() - 1];
            m_formats[m_formats.size() - 1] = 0;
            m_formats.removeLast();
            delete current;
            STGMEDIUM* medium = m_medium[position];
            m_medium[position] = m_medium[m_medium.size() - 1];
            m_medium[m_medium.size() - 1] = 0;
            m_medium.removeLast();
            ReleaseStgMedium(medium);
            delete medium;
            continue;
        }
        position++;
    }
}
