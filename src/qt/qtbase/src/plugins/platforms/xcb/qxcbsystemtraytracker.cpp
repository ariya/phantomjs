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

#include "qxcbsystemtraytracker.h"
#include "qxcbconnection.h"
#include "qxcbscreen.h"

#include <QtCore/QDebug>
#include <QtCore/QRect>
#include <QtGui/QScreen>

#include <qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

enum {
    SystemTrayRequestDock = 0,
    SystemTrayBeginMessage = 1,
    SystemTrayCancelMessage = 2
};

// QXcbSystemTrayTracker provides API for accessing the tray window and tracks
// its lifecyle by listening for its destruction and recreation.
// See http://standards.freedesktop.org/systemtray-spec/systemtray-spec-latest.html

QXcbSystemTrayTracker *QXcbSystemTrayTracker::create(QXcbConnection *connection)
{
    // Selection, tray atoms for GNOME, NET WM Specification
    const xcb_atom_t trayAtom = connection->atom(QXcbAtom::_NET_SYSTEM_TRAY_OPCODE);
    if (!trayAtom)
        return 0;
    const QByteArray netSysTray = QByteArrayLiteral("_NET_SYSTEM_TRAY_S") + QByteArray::number(connection->primaryScreen());
    const xcb_atom_t selection = connection->internAtom(netSysTray.constData());
    if (!selection)
        return 0;
    return new QXcbSystemTrayTracker(connection, trayAtom, selection, connection);
}

QXcbSystemTrayTracker::QXcbSystemTrayTracker(QXcbConnection *connection,
                                             xcb_atom_t trayAtom,
                                             xcb_atom_t selection,
                                             QObject *parent)
    : QObject(parent)
    , m_selection(selection)
    , m_trayAtom(trayAtom)
    , m_connection(connection)
    , m_trayWindow(0)
{
}

xcb_window_t QXcbSystemTrayTracker::locateTrayWindow(const QXcbConnection *connection, xcb_atom_t selection)
{
    xcb_get_selection_owner_cookie_t cookie = xcb_get_selection_owner(connection->xcb_connection(), selection);
    xcb_get_selection_owner_reply_t *reply = xcb_get_selection_owner_reply(connection->xcb_connection(), cookie, 0);
    if (!reply)
        return 0;
    const xcb_window_t result = reply->owner;
    free(reply);
    return result;
}

// API for QPlatformNativeInterface/QPlatformSystemTrayIcon: Request a window
// to be docked on the tray.
void QXcbSystemTrayTracker::requestSystemTrayWindowDock(xcb_window_t window) const
{
    xcb_client_message_event_t trayRequest;
    memset(&trayRequest, 0, sizeof(trayRequest));
    trayRequest.response_type = XCB_CLIENT_MESSAGE;
    trayRequest.format = 32;
    trayRequest.window = m_trayWindow;
    trayRequest.type = m_trayAtom;
    trayRequest.data.data32[0] = XCB_CURRENT_TIME;
    trayRequest.data.data32[1] = SystemTrayRequestDock;
    trayRequest.data.data32[2] = window;
    xcb_send_event(m_connection->xcb_connection(), 0, m_trayWindow, XCB_EVENT_MASK_NO_EVENT, (const char *)&trayRequest);
}

// API for QPlatformNativeInterface/QPlatformSystemTrayIcon: Return tray window.
xcb_window_t QXcbSystemTrayTracker::trayWindow()
{
    if (!m_trayWindow) {
        m_trayWindow = QXcbSystemTrayTracker::locateTrayWindow(m_connection, m_selection);
        if (m_trayWindow) { // Listen for DestroyNotify on tray.
            m_connection->addWindowEventListener(m_trayWindow, this);
            const quint32 mask = XCB_CW_EVENT_MASK;
            const quint32 value = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
            Q_XCB_CALL2(xcb_change_window_attributes(m_connection->xcb_connection(), m_trayWindow, mask, &value), m_connection);
        }
    }
    return m_trayWindow;
}

// API for QPlatformNativeInterface/QPlatformSystemTrayIcon: Return the geometry of a
// a window parented on the tray. Determines the global geometry via XCB since mapToGlobal
// does not work for the QWindow parented on the tray.
QRect QXcbSystemTrayTracker::systemTrayWindowGlobalGeometry(xcb_window_t window) const
{
    xcb_connection_t *conn = m_connection->xcb_connection();
    xcb_get_geometry_reply_t *geomReply =
        xcb_get_geometry_reply(conn, xcb_get_geometry(conn, window), 0);
    if (!geomReply)
        return QRect();

    xcb_translate_coordinates_reply_t *translateReply =
        xcb_translate_coordinates_reply(conn, xcb_translate_coordinates(conn, window, m_connection->rootWindow(), 0, 0), 0);
    if (!translateReply) {
        free(geomReply);
        return QRect();
    }

    const QRect result(QPoint(translateReply->dst_x, translateReply->dst_y), QSize(geomReply->width, geomReply->height));
    free(translateReply);
    return result;
}

inline void QXcbSystemTrayTracker::emitSystemTrayWindowChanged()
{
    const int screen = m_connection->primaryScreen();
    if (screen >= 0 && screen < m_connection->screens().size()) {
        const QPlatformScreen *ps = m_connection->screens().at(screen);
        emit systemTrayWindowChanged(ps->screen());
    }
}

// Client messages with the "MANAGER" atom on the root window indicate creation of a new tray.
void QXcbSystemTrayTracker::notifyManagerClientMessageEvent(const xcb_client_message_event_t *t)
{
    if (t->data.data32[1] == m_selection)
        emitSystemTrayWindowChanged();
}

// Listen for destruction of the tray.
void QXcbSystemTrayTracker::handleDestroyNotifyEvent(const xcb_destroy_notify_event_t *event)
{
    if (event->window == m_trayWindow) {
        m_connection->removeWindowEventListener(m_trayWindow);
        m_trayWindow = 0;
        emitSystemTrayWindowChanged();
    }
}

QT_END_NAMESPACE
