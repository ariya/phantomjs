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

#ifndef QXCBSYSTEMTRAYTRACKER_H
#define QXCBSYSTEMTRAYTRACKER_H

#include "qxcbconnection.h"

#include <xcb/xcb.h>

QT_BEGIN_NAMESPACE

class QXcbConnection;
class QScreen;

class QXcbSystemTrayTracker : public QObject, public QXcbWindowEventListener
{
    Q_OBJECT
public:
    static QXcbSystemTrayTracker *create(QXcbConnection *connection);

    xcb_window_t trayWindow();
    void requestSystemTrayWindowDock(xcb_window_t window) const;
    QRect systemTrayWindowGlobalGeometry(xcb_window_t window) const;

    void notifyManagerClientMessageEvent(const xcb_client_message_event_t *);

    void handleDestroyNotifyEvent(const xcb_destroy_notify_event_t *);

signals:
    void systemTrayWindowChanged(QScreen *screen);

private:
    explicit QXcbSystemTrayTracker(QXcbConnection *connection,
                                   xcb_atom_t trayAtom,
                                   xcb_atom_t selection,
                                   QObject *parent = 0);
    static xcb_window_t locateTrayWindow(const QXcbConnection *connection, xcb_atom_t selection);
    void emitSystemTrayWindowChanged();

    const xcb_atom_t m_selection;
    const xcb_atom_t m_trayAtom;
    QXcbConnection *m_connection;
    xcb_window_t m_trayWindow;
};

QT_END_NAMESPACE

#endif // QXCBSYSTEMTRAYTRACKER_H
