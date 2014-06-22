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

#ifndef QXCBKEYBOARD_H
#define QXCBKEYBOARD_H

#include "qxcbobject.h"

#include <xcb/xcb_keysyms.h>

#include <xkbcommon/xkbcommon.h>
#ifndef QT_NO_XKB
#include <xkbcommon/xkbcommon-x11.h>
#endif

#include <QEvent>

QT_BEGIN_NAMESPACE

class QWindow;

class QXcbKeyboard : public QXcbObject
{
public:
    QXcbKeyboard(QXcbConnection *connection);

    ~QXcbKeyboard();

    void handleKeyPressEvent(QXcbWindowEventListener *eventListener, const xcb_key_press_event_t *event);
    void handleKeyReleaseEvent(QXcbWindowEventListener *eventListener, const xcb_key_release_event_t *event);
    void handleMappingNotifyEvent(const void *event);

    Qt::KeyboardModifiers translateModifiers(int s) const;
    void updateKeymap();
    QList<int> possibleKeys(const QKeyEvent *e) const;

    // when XKEYBOARD not present on the X server
    void updateXKBMods();
    quint32 xkbModMask(quint16 state);
    void updateXKBStateFromCore(quint16 state);
#ifndef QT_NO_XKB
    // when XKEYBOARD is present on the X server
    int coreDeviceId() const { return core_device_id; }
    void updateXKBState(xcb_xkb_state_notify_event_t *state);
#endif

protected:
    void handleKeyEvent(QWindow *window, QEvent::Type type, xcb_keycode_t code, quint16 state, xcb_timestamp_t time);

    void resolveMaskConflicts();
    QString lookupString(struct xkb_state *state, xcb_keycode_t code) const;
    int keysymToQtKey(xcb_keysym_t keysym) const;
    int keysymToQtKey(xcb_keysym_t keysym, Qt::KeyboardModifiers &modifiers, QString text) const;
    void printKeymapError(const char *error) const;

    void readXKBConfig();
    void clearXKBConfig();
    // when XKEYBOARD not present on the X server
    void updateModifiers();
    // when XKEYBOARD is present on the X server
    void updateVModMapping();
    void updateVModToRModMapping();

private:
    bool m_config;
    xcb_keycode_t m_autorepeat_code;

    struct xkb_context *xkb_context;
    struct xkb_keymap *xkb_keymap;
    struct xkb_state *xkb_state;
    struct xkb_rule_names xkb_names;

    struct _mod_masks {
        uint alt;
        uint altgr;
        uint meta;
        uint super;
        uint hyper;
    };

    _mod_masks rmod_masks;

    // when XKEYBOARD not present on the X server
    xcb_key_symbols_t *m_key_symbols;
    struct _xkb_mods {
        xkb_mod_index_t shift;
        xkb_mod_index_t lock;
        xkb_mod_index_t control;
        xkb_mod_index_t mod1;
        xkb_mod_index_t mod2;
        xkb_mod_index_t mod3;
        xkb_mod_index_t mod4;
        xkb_mod_index_t mod5;
    };
    _xkb_mods xkb_mods;
#ifndef QT_NO_XKB
    // when XKEYBOARD is present on the X server
    _mod_masks vmod_masks;
    int core_device_id;
#endif
};

QT_END_NAMESPACE

#endif
