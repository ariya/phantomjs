/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QWINDOWSOLE_H
#define QWINDOWSOLE_H

#include "qtwindows_additional.h"

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QPoint>
#include <QtCore/QPointer>
#include <QtCore/QVector>
#include <QtCore/QRect>

#include <objidl.h>

QT_BEGIN_NAMESPACE

class QMimeData;
class QWindow;

class QWindowsOleDataObject : public IDataObject
{
public:
    explicit QWindowsOleDataObject(QMimeData *mimeData);
    virtual ~QWindowsOleDataObject();

    void releaseQt();
    QMimeData *mimeData() const;
    DWORD reportedPerformedEffect() const;

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IDataObject methods
    STDMETHOD(GetData)(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium);
    STDMETHOD(GetDataHere)(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium);
    STDMETHOD(QueryGetData)(LPFORMATETC pformatetc);
    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut);
    STDMETHOD(SetData)(LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium,
                       BOOL fRelease);
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC FAR* ppenumFormatEtc);
    STDMETHOD(DAdvise)(FORMATETC FAR* pFormatetc, DWORD advf,
                      LPADVISESINK pAdvSink, DWORD FAR* pdwConnection);
    STDMETHOD(DUnadvise)(DWORD dwConnection);
    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA FAR* ppenumAdvise);

private:
    ULONG m_refs;
    QPointer<QMimeData> data;
    int CF_PERFORMEDDROPEFFECT;
    DWORD performedEffect;
};

class QWindowsOleEnumFmtEtc : public IEnumFORMATETC
{
public:
    explicit QWindowsOleEnumFmtEtc(const QVector<FORMATETC> &fmtetcs);
    explicit QWindowsOleEnumFmtEtc(const QVector<LPFORMATETC> &lpfmtetcs);
    virtual ~QWindowsOleEnumFmtEtc();

    bool isNull() const;

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IEnumFORMATETC methods
    STDMETHOD(Next)(ULONG celt, LPFORMATETC rgelt, ULONG FAR* pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)(void);
    STDMETHOD(Clone)(LPENUMFORMATETC FAR* newEnum);

private:
    bool copyFormatEtc(LPFORMATETC dest, LPFORMATETC src) const;

    ULONG m_dwRefs;
    ULONG m_nIndex;
    QVector<LPFORMATETC> m_lpfmtetcs;
    bool m_isNull;
};

QT_END_NAMESPACE

#endif // QWINDOWSOLE_H
