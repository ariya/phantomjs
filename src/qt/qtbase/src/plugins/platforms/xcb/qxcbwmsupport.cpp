/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qxcbwmsupport.h"
#include "qxcbscreen.h"

#include <qdebug.h>

QT_BEGIN_NAMESPACE

QXcbWMSupport::QXcbWMSupport(QXcbConnection *c)
    : QXcbObject(c)
{
    updateNetWMAtoms();
    updateVirtualRoots();
}

bool QXcbWMSupport::isSupportedByWM(xcb_atom_t atom) const
{
    return net_wm_atoms.contains(atom);
}



void QXcbWMSupport::updateNetWMAtoms()
{
    net_wm_atoms.clear();

    xcb_window_t root = connection()->primaryScreen()->root();
    int offset = 0;
    int remaining = 0;
    do {
        xcb_get_property_cookie_t cookie = xcb_get_property(xcb_connection(), false, root, atom(QXcbAtom::_NET_SUPPORTED), XCB_ATOM_ATOM, offset, 1024);
        xcb_get_property_reply_t *reply = xcb_get_property_reply(xcb_connection(), cookie, NULL);
        if (!reply)
            break;

        remaining = 0;

        if (reply->type == XCB_ATOM_ATOM && reply->format == 32) {
            int len = xcb_get_property_value_length(reply)/4;
            xcb_atom_t *atoms = (xcb_atom_t *)xcb_get_property_value(reply);
            int s = net_wm_atoms.size();
            net_wm_atoms.resize(s + len);
            memcpy(net_wm_atoms.data() + s, atoms, len*sizeof(xcb_atom_t));

            remaining = reply->bytes_after;
            offset += len;
        }

        free(reply);
    } while (remaining > 0);
}

// update the virtual roots array
void QXcbWMSupport::updateVirtualRoots()
{
    net_virtual_roots.clear();

    if (!isSupportedByWM(atom(QXcbAtom::_NET_VIRTUAL_ROOTS)))
        return;

    xcb_window_t root = connection()->primaryScreen()->root();
    int offset = 0;
    int remaining = 0;
    do {
        xcb_get_property_cookie_t cookie = xcb_get_property(xcb_connection(), false, root, atom(QXcbAtom::_NET_VIRTUAL_ROOTS), XCB_ATOM_ATOM, offset, 1024);
        xcb_get_property_reply_t *reply = xcb_get_property_reply(xcb_connection(), cookie, NULL);
        if (!reply)
            break;

        remaining = 0;

        if (reply->type == XCB_ATOM_ATOM && reply->format == 32) {
            int len = xcb_get_property_value_length(reply)/4;
            xcb_atom_t *atoms = (xcb_atom_t *)xcb_get_property_value(reply);
            int s = net_wm_atoms.size();
            net_wm_atoms.resize(s + len);
            memcpy(net_wm_atoms.data() + s, atoms, len*sizeof(xcb_atom_t));

            remaining = reply->bytes_after;
            offset += len;
        }

        free(reply);
    } while (remaining > 0);

#ifdef Q_XCB_DEBUG
    qDebug() << "======== updateVirtualRoots";
    for (int i = 0; i < net_virtual_roots.size(); ++i)
        qDebug() << connection()->atomName(net_virtual_roots.at(i));
    qDebug() << "======== updateVirtualRoots";
#endif
}

QT_END_NAMESPACE
