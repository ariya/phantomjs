/****************************************************************************
**
** Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
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

#include "qwindowsguieventdispatcher.h"
#include "qwindowscontext.h"

#include <qpa/qwindowsysteminterface.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QStack>
#include <QtCore/QDebug>

#include <windowsx.h>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsGuiEventDispatcher
    \brief Event dispatcher for Windows

    Maintains a global stack storing the current event dispatcher and
    its processing flags for access from the Windows procedure
    qWindowsWndProc. Handling the Lighthouse gui events should be done
    from within the qWindowsWndProc to ensure correct processing of messages.

    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsGuiEventDispatcher::QWindowsGuiEventDispatcher(QObject *parent) :
    QEventDispatcherWin32(parent), m_flags(0)
{
    setObjectName(QStringLiteral("QWindowsGuiEventDispatcher"));
}

bool QWindowsGuiEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    m_flags = flags;
    if (QWindowsContext::verbose > 2 && lcQpaEvents().isDebugEnabled())
        qCDebug(lcQpaEvents) << '>' << __FUNCTION__ << objectName() << flags;
    const bool rc = QEventDispatcherWin32::processEvents(flags);
    if (QWindowsContext::verbose > 2 && lcQpaEvents().isDebugEnabled())
        qCDebug(lcQpaEvents) << '<' << __FUNCTION__ << "returns" << rc;
    return rc;
}

void QWindowsGuiEventDispatcher::sendPostedEvents()
{
    QCoreApplication::sendPostedEvents();
    QWindowSystemInterface::sendWindowSystemEvents(m_flags);
}

// Helpers for printing debug output for WM_* messages.
struct MessageDebugEntry
{
    UINT message;
    const char *description;
    bool interesting;
};

static const MessageDebugEntry
messageDebugEntries[] = {
    {WM_CREATE, "WM_CREATE", true},
    {WM_PAINT, "WM_PAINT", true},
    {WM_CLOSE, "WM_CLOSE", true},
    {WM_DESTROY, "WM_DESTROY", true},
    {WM_MOVE, "WM_MOVE", true},
    {WM_SIZE, "WM_SIZE", true},
    {WM_MOUSEACTIVATE,"WM_MOUSEACTIVATE", true},
    {WM_CHILDACTIVATE, "WM_CHILDACTIVATE", true},
    {WM_PARENTNOTIFY, "WM_PARENTNOTIFY", true},
    {WM_ENTERIDLE, "WM_ENTERIDLE", false},
    {WM_GETICON, "WM_GETICON", false},
    {WM_KEYDOWN, "WM_KEYDOWN", true},
    {WM_SYSKEYDOWN, "WM_SYSKEYDOWN", true},
    {WM_SYSCOMMAND, "WM_SYSCOMMAND", true},
    {WM_KEYUP, "WM_KEYUP", true},
    {WM_SYSKEYUP, "WM_SYSKEYUP", true},
    {WM_IME_CHAR, "WM_IMECHAR", true},
    {WM_IME_KEYDOWN, "WM_IMECHAR", true},
    {WM_CANCELMODE,  "WM_CANCELMODE", true},
    {WM_CHAR, "WM_CHAR", true},
    {WM_DEADCHAR, "WM_DEADCHAR", true},
    {WM_ACTIVATE, "WM_ACTIVATE", true},
    {WM_GETMINMAXINFO, "WM_GETMINMAXINFO", true},
    {WM_SETFOCUS, "WM_SETFOCUS", true},
    {WM_KILLFOCUS, "WM_KILLFOCUS", true},
    {WM_ENABLE, "WM_ENABLE", true},
    {WM_SHOWWINDOW, "WM_SHOWWINDOW", true},
    {WM_WINDOWPOSCHANGING, "WM_WINDOWPOSCHANGING", true},
    {WM_WINDOWPOSCHANGED, "WM_WINDOWPOSCHANGED", true},
    {WM_SETCURSOR, "WM_SETCURSOR", false},
    {WM_GETFONT, "WM_GETFONT", true},
    {WM_NCMOUSEMOVE, "WM_NCMOUSEMOVE", true},
    {WM_LBUTTONDOWN, "WM_LBUTTONDOWN", true},
    {WM_LBUTTONUP, "WM_LBUTTONUP", true},
    {WM_LBUTTONDBLCLK, "WM_LBUTTONDBLCLK", true},
    {WM_RBUTTONDOWN, "WM_RBUTTONDOWN", true},
    {WM_RBUTTONUP, "WM_RBUTTONUP", true},
    {WM_RBUTTONDBLCLK, "WM_RBUTTONDBLCLK", true},
    {WM_MBUTTONDOWN, "WM_MBUTTONDOWN", true},
    {WM_MBUTTONUP, "WM_MBUTTONUP", true},
    {WM_MBUTTONDBLCLK, "WM_MBUTTONDBLCLK", true},
    {WM_MOUSEWHEEL, "WM_MOUSEWHEEL", true},
    {WM_XBUTTONDOWN, "WM_XBUTTONDOWN", true},
    {WM_XBUTTONUP, "WM_XBUTTONUP", true},
    {WM_XBUTTONDBLCLK, "WM_XBUTTONDBLCLK", true},
    {WM_MOUSEHWHEEL, "WM_MOUSEHWHEEL", true},
    {WM_NCCREATE, "WM_NCCREATE", true},
    {WM_NCCALCSIZE, "WM_NCCALCSIZE", true},
    {WM_NCACTIVATE, "WM_NCACTIVATE", true},
    {WM_NCMOUSELEAVE, "WM_NCMOUSELEAVE", true},
    {WM_NCLBUTTONDOWN, "WM_NCLBUTTONDOWN", true},
    {WM_NCLBUTTONUP, "WM_NCLBUTTONUP", true},
    {WM_ACTIVATEAPP, "WM_ACTIVATEAPP", true},
    {WM_NCPAINT, "WM_NCPAINT", true},
    {WM_ERASEBKGND, "WM_ERASEBKGND", true},
    {WM_MOUSEMOVE, "WM_MOUSEMOVE", true},
    {WM_MOUSELEAVE, "WM_MOUSELEAVE", true},
    {WM_NCHITTEST, "WM_NCHITTEST", false},
    {WM_IME_SETCONTEXT, "WM_IME_SETCONTEXT", true},
    {WM_INPUTLANGCHANGE, "WM_INPUTLANGCHANGE", true},
    {WM_IME_NOTIFY, "WM_IME_NOTIFY", true},
#if defined(WM_DWMNCRENDERINGCHANGED)
    {WM_DWMNCRENDERINGCHANGED, "WM_DWMNCRENDERINGCHANGED", true},
#endif
    {WM_IME_SETCONTEXT, "WM_IME_SETCONTEXT", true},
    {WM_IME_NOTIFY, "WM_IME_NOTIFY", true},
    {WM_TOUCH, "WM_TOUCH", true},
    {WM_CHANGECBCHAIN, "WM_CHANGECBCHAIN", true},
    {WM_DRAWCLIPBOARD, "WM_DRAWCLIPBOARD", true},
    {WM_RENDERFORMAT, "WM_RENDERFORMAT", true},
    {WM_RENDERALLFORMATS, "WM_RENDERALLFORMATS", true},
    {WM_DESTROYCLIPBOARD, "WM_DESTROYCLIPBOARD", true},
    {WM_CAPTURECHANGED, "WM_CAPTURECHANGED", true},
    {WM_IME_STARTCOMPOSITION, "WM_IME_STARTCOMPOSITION", true},
    {WM_IME_COMPOSITION, "WM_IME_COMPOSITION", true},
    {WM_IME_ENDCOMPOSITION, "WM_IME_ENDCOMPOSITION", true},
    {WM_IME_NOTIFY, "WM_IME_NOTIFY", true},
    {WM_IME_REQUEST, "WM_IME_REQUEST", true},
#if !defined(Q_OS_WINCE) && !defined(QT_NO_SESSIONMANAGER)
    {WM_QUERYENDSESSION, "WM_QUERYENDSESSION", true},
    {WM_ENDSESSION, "WM_ENDSESSION", true},
#endif
    {WM_DISPLAYCHANGE, "WM_DISPLAYCHANGE", true},
    {WM_THEMECHANGED, "WM_THEMECHANGED", true}
};

static inline const MessageDebugEntry *messageDebugEntry(UINT msg)
{
    for (size_t i = 0; i < sizeof(messageDebugEntries)/sizeof(MessageDebugEntry); i++)
        if (messageDebugEntries[i].message == msg)
            return messageDebugEntries + i;
    return 0;
}

const char *QWindowsGuiEventDispatcher::windowsMessageName(UINT msg)
{
    if (const MessageDebugEntry *e = messageDebugEntry(msg))
        return e->description;
    return "Unknown";
}

QT_END_NAMESPACE
