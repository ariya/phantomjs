/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qclipboard.h"

#ifndef QT_NO_CLIPBOARD

#include "qapplication.h"
#include "qapplication_p.h"
#include "qeventloop.h"
#include "qwidget.h"
#include "qevent.h"
#include "qmime.h"
#include "qt_windows.h"
#include "qdnd_p.h"
#include <private/qwidget_p.h>
#include <private/qsystemlibrary_p.h>

QT_BEGIN_NAMESPACE

#if defined(Q_OS_WINCE)
QT_BEGIN_INCLUDE_NAMESPACE
#include "qguifunctions_wince.h"
QT_END_INCLUDE_NAMESPACE

HRESULT QtCeGetClipboard(IDataObject** obj);
HRESULT QtCeSetClipboard(IDataObject* obj);
void QtCeFlushClipboard();

#define OleGetClipboard QtCeGetClipboard
#define OleSetClipboard QtCeSetClipboard
#define OleFlushClipboard QtCeFlushClipboard

#endif

typedef BOOL (WINAPI *PtrIsHungAppWindow)(HWND);

static PtrIsHungAppWindow ptrIsHungAppWindow = 0;

class QClipboardWatcher : public QInternalMimeData {
public:
    QClipboardWatcher()
        : QInternalMimeData()
    {
    }

    bool hasFormat_sys(const QString &mimetype) const;
    QStringList formats_sys() const;
    QVariant retrieveData_sys(const QString &mimetype, QVariant::Type preferredType) const;
};


bool QClipboardWatcher::hasFormat_sys(const QString &mime) const
{
    IDataObject * pDataObj = 0;

    if (OleGetClipboard(&pDataObj) != S_OK && !pDataObj) // Sanity
        return false;

    bool has = QWindowsMime::converterToMime(mime, pDataObj) != 0;

    pDataObj->Release();

    return has;
}

QStringList QClipboardWatcher::formats_sys() const
{
    QStringList fmts;
    IDataObject * pDataObj = 0;

    if (OleGetClipboard(&pDataObj) != S_OK && !pDataObj) // Sanity
        return QStringList();

    fmts = QWindowsMime::allMimesForFormats(pDataObj);

    pDataObj->Release();

    return fmts;
}

QVariant QClipboardWatcher::retrieveData_sys(const QString &mimeType, QVariant::Type type) const
{
    QVariant result;
    IDataObject * pDataObj = 0;

    if (OleGetClipboard(&pDataObj) != S_OK && !pDataObj) // Sanity
        return result;

    QWindowsMime *converter = QWindowsMime::converterToMime(mimeType, pDataObj);

    if (converter)
        result = converter->convertToMime(mimeType, pDataObj, type);

    pDataObj->Release();

    return result;
}

class QClipboardData
{
public:
    QClipboardData()
        : iData(0)
        , nextClipboardViewer(0)
    {
        clipBoardViewer = new QWidget();
        clipBoardViewer->createWinId();
        clipBoardViewer->setObjectName(QLatin1String("internal clipboard owner"));
        // We don't need this internal widget to appear in QApplication::topLevelWidgets()
        if (QWidgetPrivate::allWidgets)
            QWidgetPrivate::allWidgets->remove(clipBoardViewer);
    }

    ~QClipboardData()
    {
        Q_ASSERT(clipBoardViewer->testAttribute(Qt::WA_WState_Created));
        ChangeClipboardChain(clipBoardViewer->internalWinId(), nextClipboardViewer);
        delete clipBoardViewer;
        releaseIData();
    }

    void releaseIData()
    {
        if (iData) {
            delete iData->mimeData();
            iData->releaseQt();
            iData->Release();
            iData = 0;
        }
    }

    QOleDataObject * iData;
    QWidget *clipBoardViewer;
    HWND nextClipboardViewer;
    QClipboardWatcher watcher;
};

static QClipboardData *ptrClipboardData = 0;

static QClipboardData *clipboardData()
{
    if (ptrClipboardData == 0) {
        ptrClipboardData = new QClipboardData;
        // this needs to be done here to avoid recursion
        Q_ASSERT(ptrClipboardData->clipBoardViewer->testAttribute(Qt::WA_WState_Created));
        ptrClipboardData->nextClipboardViewer = SetClipboardViewer(ptrClipboardData->clipBoardViewer->internalWinId());
    }
    return ptrClipboardData;
}

static void cleanupClipboardData()
{
    delete ptrClipboardData;
    ptrClipboardData = 0;
}

#if defined(Q_OS_WINCE)
HRESULT QtCeGetClipboard(IDataObject** obj)
{
    HWND owner = ptrClipboardData->clipBoardViewer->internalWinId();
    if (!OpenClipboard(owner))
        return !S_OK;

    if (!IsClipboardFormatAvailable(CF_TEXT) && !IsClipboardFormatAvailable(CF_UNICODETEXT))
        return !S_OK;

    HANDLE clipData = GetClipboardData(CF_TEXT);
    QString clipText;
    if (clipData == 0) {
        clipData = GetClipboardData(CF_UNICODETEXT);
        if (clipData != 0)
            clipText = QString::fromWCharArray((wchar_t *)clipData);
    } else {
        clipText = QString::fromLatin1((const char*)clipData);
    }

    QMimeData *mimeData = new QMimeData();
    mimeData->setText(clipText);
    QOleDataObject* data = new QOleDataObject(mimeData);
    *obj = data;
    CloseClipboard();
    return S_OK;
}

HRESULT QtCeSetClipboard(IDataObject* obj)
{
    HWND owner = ptrClipboardData->clipBoardViewer->internalWinId();
    if (!OpenClipboard(owner))
        return !S_OK;

    bool result = false;
    if (obj == 0) {
        result = true;
        EmptyClipboard();
        CloseClipboard();
    } else {
        QOleDataObject* qobj = static_cast<QOleDataObject*>(obj);

        const QMimeData* data = qobj->mimeData();
        if (data->hasText()) {
            EmptyClipboard();
            result = SetClipboardData(CF_UNICODETEXT, wcsdup(reinterpret_cast<const wchar_t *> (data->text().utf16()))) != NULL;
            CloseClipboard();
            result = true;
        }
    }
    return result ? S_OK : !S_OK;
}

void QtCeFlushClipboard() { }
#endif



QClipboard::~QClipboard()
{
    cleanupClipboardData();
}

void QClipboard::setMimeData(QMimeData *src, Mode mode)
{
    if (mode != Clipboard)
        return;
    QClipboardData *d = clipboardData();

    if (!(d->iData && d->iData->mimeData() == src)) {
        d->releaseIData();
        d->iData = new QOleDataObject(src);
    }

    if (OleSetClipboard(d->iData) != S_OK) {
        d->releaseIData();
        qErrnoWarning("QClipboard::setMimeData: Failed to set data on clipboard");
        return;
    }
#if defined(Q_OS_WINCE)
    // As WinCE does not support notifications we send the signal here
    // We will get no event when the clipboard changes outside...
    emit dataChanged();
    emit changed(Clipboard);
#endif
}

void QClipboard::clear(Mode mode)
{
    if (mode != Clipboard) return;

    QClipboardData *d = clipboardData();

    d->releaseIData();

    if (OleSetClipboard(0) != S_OK) {
        qErrnoWarning("QClipboard::clear: Failed to clear data on clipboard");
        return;
    }
#if defined(Q_OS_WINCE)
    // As WinCE does not support notifications we send the signal here
    // We will get no event when the clipboard changes outside...
    emit dataChanged();
    emit changed(Clipboard);
#endif
}

bool QClipboard::event(QEvent *e)
{
    if (e->type() != QEvent::Clipboard)
        return QObject::event(e);

    QClipboardData *d = clipboardData();

    MSG *m = (MSG *)((QClipboardEvent*)e)->data();
    if (!m) {
        // this is sent to render all formats at app shut down
        if (ownsClipboard()) {
            OleFlushClipboard();
            d->releaseIData();
        }
        return true;
    }

    bool propagate = false;

    if (m->message == WM_CHANGECBCHAIN) {
        if ((HWND)m->wParam == d->nextClipboardViewer)
            d->nextClipboardViewer = (HWND)m->lParam;
        else
            propagate = true;
    } else if (m->message == WM_DRAWCLIPBOARD) {
        emitChanged(QClipboard::Clipboard);
        if (!ownsClipboard() && d->iData)
            // clean up the clipboard object if we no longer own the clipboard
            d->releaseIData();
        propagate = true;
    }
    if (propagate && d->nextClipboardViewer) {
        if (ptrIsHungAppWindow == 0) {
            QSystemLibrary library(QLatin1String("User32"));
            ptrIsHungAppWindow = (PtrIsHungAppWindow)library.resolve("IsHungAppWindow");
        }
        if (ptrIsHungAppWindow && ptrIsHungAppWindow(d->nextClipboardViewer)) {
            qWarning("%s: Cowardly refusing to send clipboard message to hung application...", Q_FUNC_INFO);
        } else {
            SendMessage(d->nextClipboardViewer, m->message, m->wParam, m->lParam);
        }
    }

    return true;
}

void QClipboard::connectNotify(const char *signal)
{
    if (qstrcmp(signal,SIGNAL(dataChanged())) == 0) {
        // ensure we are up and running but block signals so the dataChange signal
        // is not emitted while being connected to.
        bool blocked = blockSignals(true);
        QClipboardData *d = clipboardData();
        blockSignals(blocked);
        Q_UNUSED(d);
    }
}

const QMimeData *QClipboard::mimeData(Mode mode) const
{
    if (mode != Clipboard)
        return 0;

    QClipboardData *data = clipboardData();
    // sort cut for local copy / paste
    if (ownsClipboard() && data->iData->mimeData())
        return data->iData->mimeData();
    return &data->watcher;
}

bool QClipboard::supportsMode(Mode mode) const
{
    return (mode == Clipboard);
}

bool QClipboard::ownsMode(Mode mode) const
{
    if (mode == Clipboard) {
        QClipboardData *d = clipboardData();
#if !defined(Q_OS_WINCE)
        return d->iData && OleIsCurrentClipboard(d->iData) == S_OK;
#else
        return d->iData && GetClipboardOwner() == d->clipBoardViewer->internalWinId();
#endif
    } else {
        return false;
    }
}

void QClipboard::ownerDestroyed()
{
}

QT_END_NAMESPACE

#endif // QT_NO_CLIPBOARD
