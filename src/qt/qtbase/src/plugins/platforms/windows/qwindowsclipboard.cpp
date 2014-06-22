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

#include "qwindowsclipboard.h"
#include "qwindowscontext.h"
#include "qwindowsole.h"
#include "qwindowsmime.h"
#include "qwindowsguieventdispatcher.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QClipboard>
#include <QtGui/QColor>
#include <QtGui/QImage>

#include <QtCore/QDebug>
#include <QtCore/QMimeData>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE

static const char formatTextPlainC[] = "text/plain";
static const char formatTextHtmlC[] = "text/html";

/*!
    \class QWindowsClipboard
    \brief Clipboard implementation.

    Registers a non-visible clipboard viewer window that
    receives clipboard events in its own window procedure to be
    able to receive clipboard-changed events, which
    QPlatformClipboard needs to emit. That requires housekeeping
    of the next in the viewer chain.

    \note The OLE-functions used in this class require OleInitialize().

    \internal
    \ingroup qt-lighthouse-win
*/

QDebug operator<<(QDebug d, const QMimeData &m)
{
    QDebug nospace = d.nospace();
    const QStringList formats = m.formats();
    nospace << "QMimeData: " << formats.join(QStringLiteral(", ")) << '\n'
            << "  Text=" << m.hasText() << " HTML=" << m.hasHtml()
            << " Color=" << m.hasColor() << " Image=" << m.hasImage()
            << " URLs=" << m.hasUrls() << '\n';
    if (m.hasText())
        nospace << "  Text: '" << m.text() << "'\n";
    if (m.hasHtml())
        nospace << "  HTML: '" << m.html() << "'\n";
    if (m.hasColor())
        nospace << "  Color: " << qvariant_cast<QColor>(m.colorData()) << '\n';
    if (m.hasImage())
        nospace << "  Image: " << qvariant_cast<QImage>(m.imageData()).size() << '\n';
    if (m.hasUrls())
        nospace << "  URLs: " << m.urls() << '\n';
    return d;
}

/*!
    \class QWindowsClipboardRetrievalMimeData
    \brief Special mime data class managing delayed retrieval of clipboard data.

    Implementation of QWindowsInternalMimeDataBase that obtains the
    IDataObject from the clipboard.

    \sa QWindowsInternalMimeDataBase, QWindowsClipboard
    \internal
    \ingroup qt-lighthouse-win
*/

IDataObject *QWindowsClipboardRetrievalMimeData::retrieveDataObject() const
{
    IDataObject * pDataObj = 0;
    if (OleGetClipboard(&pDataObj) == S_OK)
        return pDataObj;
    return 0;
}

void QWindowsClipboardRetrievalMimeData::releaseDataObject(IDataObject *dataObject) const
{
    dataObject->Release();
}

extern "C" LRESULT QT_WIN_CALLBACK qClipboardViewerWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    if (QWindowsClipboard::instance()
        && QWindowsClipboard::instance()->clipboardViewerWndProc(hwnd, message, wParam, lParam, &result))
        return result;
    return DefWindowProc(hwnd, message, wParam, lParam);
}

// QTBUG-36958, ensure the clipboard is flushed before
// QGuiApplication is destroyed since OleFlushClipboard()
// might query the data again which causes problems
// for QMimeData-derived classes using QPixmap/QImage.
static void cleanClipboardPostRoutine()
{
    if (QWindowsClipboard *cl = QWindowsClipboard::instance())
        cl->cleanup();
}

QWindowsClipboard *QWindowsClipboard::m_instance = 0;

QWindowsClipboard::QWindowsClipboard() :
    m_data(0), m_clipboardViewer(0), m_nextClipboardViewer(0)
{
    QWindowsClipboard::m_instance = this;
    qAddPostRoutine(cleanClipboardPostRoutine);
}

QWindowsClipboard::~QWindowsClipboard()
{
    cleanup();
    QWindowsClipboard::m_instance = 0;
}

void QWindowsClipboard::cleanup()
{
    unregisterViewer(); // Should release data if owner.
    releaseIData();
}

void QWindowsClipboard::releaseIData()
{
    if (m_data) {
        delete m_data->mimeData();
        m_data->releaseQt();
        m_data->Release();
        m_data = 0;
    }
}

void QWindowsClipboard::registerViewer()
{
    m_clipboardViewer = QWindowsContext::instance()->
        createDummyWindow(QStringLiteral("Qt5ClipboardView"), L"Qt5ClipboardView",
                          qClipboardViewerWndProc, WS_OVERLAPPED);
    m_nextClipboardViewer = SetClipboardViewer(m_clipboardViewer);

    qCDebug(lcQpaMime) << __FUNCTION__ << "m_clipboardViewer: " << m_clipboardViewer << "next: " << m_nextClipboardViewer;
}

void QWindowsClipboard::unregisterViewer()
{
    if (m_clipboardViewer) {
        ChangeClipboardChain(m_clipboardViewer, m_nextClipboardViewer);
        DestroyWindow(m_clipboardViewer);
        m_clipboardViewer = m_nextClipboardViewer = 0;
    }
}

static bool isProcessBeingDebugged(HWND hwnd)
{
    DWORD pid = 0;
    if (!GetWindowThreadProcessId(hwnd, &pid) || !pid)
        return false;
    const HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!processHandle)
        return false;
    BOOL debugged = FALSE;
    CheckRemoteDebuggerPresent(processHandle, &debugged);
    CloseHandle(processHandle);
    return debugged != FALSE;
}

void QWindowsClipboard::propagateClipboardMessage(UINT message, WPARAM wParam, LPARAM lParam) const
{
    if (!m_nextClipboardViewer)
        return;
    // In rare cases, a clipboard viewer can hang (application crashed,
    // suspended by a shell prompt 'Select' or debugger).
    if (QWindowsContext::user32dll.isHungAppWindow
        && QWindowsContext::user32dll.isHungAppWindow(m_nextClipboardViewer)) {
        qWarning("%s: Cowardly refusing to send clipboard message to hung application...", Q_FUNC_INFO);
        return;
    }
    // Do not block if the process is being debugged, specifically, if it is
    // displaying a runtime assert, which is not caught by isHungAppWindow().
    if (isProcessBeingDebugged(m_nextClipboardViewer))
        PostMessage(m_nextClipboardViewer, message, wParam, lParam);
    else
        SendMessage(m_nextClipboardViewer, message, wParam, lParam);
}

/*!
    \brief Windows procedure of the clipboard viewer. Emits changed and does
    housekeeping of the viewer chain.
*/

bool QWindowsClipboard::clipboardViewerWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *result)
{
    *result = 0;
    if (QWindowsContext::verbose)
        qCDebug(lcQpaMime) << __FUNCTION__ << hwnd << message << QWindowsGuiEventDispatcher::windowsMessageName(message);

    switch (message) {
    case WM_CHANGECBCHAIN: {
        const HWND toBeRemoved = (HWND)wParam;
        if (toBeRemoved == m_nextClipboardViewer) {
            m_nextClipboardViewer = (HWND)lParam;
        } else {
            propagateClipboardMessage(message, wParam, lParam);
        }
    }
        return true;
    case WM_DRAWCLIPBOARD: {
        const bool owned = ownsClipboard();
        qCDebug(lcQpaMime) << "Clipboard changed owned " << owned;
        emitChanged(QClipboard::Clipboard);
        // clean up the clipboard object if we no longer own the clipboard
        if (!owned && m_data)
            releaseIData();
        propagateClipboardMessage(message, wParam, lParam);
    }
        return true;
    case WM_DESTROY:
        // Recommended shutdown
        if (ownsClipboard()) {
            qCDebug(lcQpaMime) << "Clipboard owner on shutdown, releasing.";
            OleFlushClipboard();
            releaseIData();
        }
        return true;
    } // switch (message)
    return false;
}

QMimeData *QWindowsClipboard::mimeData(QClipboard::Mode mode)
{
    qCDebug(lcQpaMime) << __FUNCTION__ <<  mode;
    if (mode != QClipboard::Clipboard)
        return 0;
    if (ownsClipboard())
        return m_data->mimeData();
    return &m_retrievalData;
}

void QWindowsClipboard::setMimeData(QMimeData *mimeData, QClipboard::Mode mode)
{
    qCDebug(lcQpaMime) << __FUNCTION__ <<  mode << *mimeData;
    if (mode != QClipboard::Clipboard)
        return;

    const bool newData = !m_data || m_data->mimeData() != mimeData;
    if (newData) {
        releaseIData();
        if (mimeData)
            m_data = new QWindowsOleDataObject(mimeData);
    }

    const HRESULT src = OleSetClipboard(m_data);
    if (src != S_OK) {
        QString mimeDataFormats = mimeData ?
            mimeData->formats().join(QStringLiteral(", ")) : QString(QStringLiteral("NULL"));
        qErrnoWarning("OleSetClipboard: Failed to set mime data (%s) on clipboard: %s",
                      qPrintable(mimeDataFormats),
                      QWindowsContext::comErrorString(src).constData());
        releaseIData();
        return;
    }
}

void QWindowsClipboard::clear()
{
    const HRESULT src = OleSetClipboard(0);
    if (src != S_OK)
        qErrnoWarning("OleSetClipboard: Failed to clear the clipboard: 0x%lx", src);
}

bool QWindowsClipboard::supportsMode(QClipboard::Mode mode) const
{
        return mode == QClipboard::Clipboard;
}

// Need a non-virtual in destructor.
bool QWindowsClipboard::ownsClipboard() const
{
    return m_data && OleIsCurrentClipboard(m_data) == S_OK;
}

bool QWindowsClipboard::ownsMode(QClipboard::Mode mode) const
{
    const bool result = mode == QClipboard::Clipboard ?
        ownsClipboard() : false;
    qCDebug(lcQpaMime) << __FUNCTION__ <<  mode << result;
    return result;
}

QT_END_NAMESPACE
