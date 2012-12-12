/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdnd_p.h"

#if !(defined(QT_NO_DRAGANDDROP) && defined(QT_NO_CLIPBOARD))

#if defined(Q_OS_WINCE)
#include <shlobj.h>
#include "qguifunctions_wince.h"
#endif

QT_BEGIN_NAMESPACE

QOleEnumFmtEtc::QOleEnumFmtEtc(const QVector<FORMATETC> &fmtetcs)
{
    m_isNull = false;
    m_dwRefs = 1;
    m_nIndex = 0;

    for (int idx = 0; idx < fmtetcs.count(); ++idx) {
        LPFORMATETC destetc = new FORMATETC();
        if (copyFormatEtc(destetc, (LPFORMATETC)&(fmtetcs.at(idx)))) {
            m_lpfmtetcs.append(destetc);
        } else {
            m_isNull = true;
            delete destetc;
            break;
        }
    }
}

QOleEnumFmtEtc::QOleEnumFmtEtc(const QVector<LPFORMATETC> &lpfmtetcs)
{
    m_isNull = false;
    m_dwRefs = 1;
    m_nIndex = 0;

    for (int idx = 0; idx < lpfmtetcs.count(); ++idx) {
        LPFORMATETC srcetc = lpfmtetcs.at(idx);
        LPFORMATETC destetc = new FORMATETC();
        if (copyFormatEtc(destetc, srcetc)) {
            m_lpfmtetcs.append(destetc);
        } else {
            m_isNull = true;
            delete destetc;
            break;
        }
    }
}

QOleEnumFmtEtc::~QOleEnumFmtEtc()
{
    LPMALLOC pmalloc;

#if !defined(Q_OS_WINCE)
    if (CoGetMalloc(MEMCTX_TASK, &pmalloc) == NOERROR) {
#else
    if (SHGetMalloc(&pmalloc) == NOERROR) {
#endif
        for (int idx = 0; idx < m_lpfmtetcs.count(); ++idx) {
            LPFORMATETC tmpetc = m_lpfmtetcs.at(idx);
            if (tmpetc->ptd)
                pmalloc->Free(tmpetc->ptd);
            delete tmpetc;
        }

        pmalloc->Release();
    }
    m_lpfmtetcs.clear();
}

bool QOleEnumFmtEtc::isNull() const
{
    return m_isNull;
}

// IUnknown methods
STDMETHODIMP
QOleEnumFmtEtc::QueryInterface(REFIID riid, void FAR* FAR* ppvObj)
{
    if (riid == IID_IUnknown || riid == IID_IEnumFORMATETC) {
        *ppvObj = this;
        AddRef();
        return NOERROR;
    }
    *ppvObj = NULL;
    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG)
QOleEnumFmtEtc::AddRef(void)
{
    return ++m_dwRefs;
}

STDMETHODIMP_(ULONG)
QOleEnumFmtEtc::Release(void)
{
    if (--m_dwRefs == 0) {
        delete this;
        return 0;
    }
    return m_dwRefs;
}

// IEnumFORMATETC methods
STDMETHODIMP
QOleEnumFmtEtc::Next(ULONG celt, LPFORMATETC rgelt, ULONG FAR* pceltFetched)
{
    ULONG i=0;
    ULONG nOffset;

    if (rgelt == NULL)
        return ResultFromScode(E_INVALIDARG);

    while (i < celt) {
        nOffset = m_nIndex + i;

        if (nOffset < ULONG(m_lpfmtetcs.count())) {
            copyFormatEtc((LPFORMATETC)&(rgelt[i]), m_lpfmtetcs.at(nOffset));
            i++;
        } else {
            break;
        }
    }

    m_nIndex += (WORD)i;

    if (pceltFetched != NULL)
        *pceltFetched = i;

    if (i != celt)
        return ResultFromScode(S_FALSE);

    return NOERROR;
}

STDMETHODIMP
QOleEnumFmtEtc::Skip(ULONG celt)
{
    ULONG i=0;
    ULONG nOffset;

    while (i < celt) {
        nOffset = m_nIndex + i;

        if (nOffset < ULONG(m_lpfmtetcs.count())) {
            i++;
        } else {
            break;
        }
    }

    m_nIndex += (WORD)i;

    if (i != celt)
        return ResultFromScode(S_FALSE);

    return NOERROR;
}

STDMETHODIMP
QOleEnumFmtEtc::Reset()
{
    m_nIndex = 0;
    return NOERROR;
}

STDMETHODIMP
QOleEnumFmtEtc::Clone(LPENUMFORMATETC FAR* newEnum)
{
    if (newEnum == NULL)
        return ResultFromScode(E_INVALIDARG);

    QOleEnumFmtEtc *result = new QOleEnumFmtEtc(m_lpfmtetcs);
    result->m_nIndex = m_nIndex;

    if (result->isNull()) {
        delete result;
        return ResultFromScode(E_OUTOFMEMORY);
    } else {
        *newEnum = result;
    }

    return NOERROR;
}

bool QOleEnumFmtEtc::copyFormatEtc(LPFORMATETC dest, LPFORMATETC src) const
{
    if (dest == NULL || src == NULL)
        return false;

    *dest = *src;

    if (src->ptd) {
        LPMALLOC pmalloc;

#if !defined(Q_OS_WINCE)
        if (CoGetMalloc(MEMCTX_TASK, &pmalloc) != NOERROR)
#else
        if (SHGetMalloc(&pmalloc) != NOERROR)
#endif
            return false;

        pmalloc->Alloc(src->ptd->tdSize);
        memcpy(dest->ptd, src->ptd, size_t(src->ptd->tdSize));

        pmalloc->Release();
    }

    return true;
}

QT_END_NAMESPACE
#endif // QT_NO_DRAGANDDROP && QT_NO_CLIPBOARD
