/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
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

#ifndef QQNXKEYTRANSLATOR_H
#define QQNXKEYTRANSLATOR_H

#include <sys/keycodes.h>

#if defined(QQNXEVENTTHREAD_DEBUG)
#include <QtCore/QDebug>
#endif

QT_BEGIN_NAMESPACE

Qt::Key keyTranslator( int key )
{
    switch (key) {
    case KEYCODE_PAUSE:
        return Qt::Key_Pause;

    case KEYCODE_SCROLL_LOCK:
        return Qt::Key_ScrollLock;

    case KEYCODE_PRINT:
        return Qt::Key_Print;

    case KEYCODE_SYSREQ:
        return Qt::Key_SysReq;

//    case KEYCODE_BREAK:

    case KEYCODE_ESCAPE:
        return Qt::Key_Escape;

    case KEYCODE_BACKSPACE:
        return Qt::Key_Backspace;

    case KEYCODE_TAB:
        return Qt::Key_Tab;

    case KEYCODE_BACK_TAB:
        return Qt::Key_Backtab;

    case KEYCODE_RETURN:
        return Qt::Key_Return;

    case KEYCODE_CAPS_LOCK:
        return Qt::Key_CapsLock;

    case KEYCODE_LEFT_SHIFT:
    case KEYCODE_RIGHT_SHIFT:
        return Qt::Key_Shift;

    case KEYCODE_LEFT_CTRL:
    case KEYCODE_RIGHT_CTRL:
        return Qt::Key_Control;

    case KEYCODE_LEFT_ALT:
    case KEYCODE_RIGHT_ALT:
        return Qt::Key_Alt;

    case KEYCODE_MENU:
        return Qt::Key_Menu;

    case KEYCODE_LEFT_HYPER:
        return Qt::Key_Hyper_L;

    case KEYCODE_RIGHT_HYPER:
        return Qt::Key_Hyper_R;

    case KEYCODE_INSERT:
        return Qt::Key_Insert;

    case KEYCODE_HOME:
        return Qt::Key_Home;

    case KEYCODE_PG_UP:
        return Qt::Key_PageUp;

    case KEYCODE_DELETE:
        return Qt::Key_Delete;

    case KEYCODE_END:
        return Qt::Key_End;

    case KEYCODE_PG_DOWN:
        return Qt::Key_PageDown;

    case KEYCODE_LEFT:
        return Qt::Key_Left;

    case KEYCODE_RIGHT:
        return Qt::Key_Right;

    case KEYCODE_UP:
        return Qt::Key_Up;

    case KEYCODE_DOWN:
        return Qt::Key_Down;

    case KEYCODE_NUM_LOCK:
        return Qt::Key_NumLock;

    case KEYCODE_KP_PLUS:
        return Qt::Key_Plus;

    case KEYCODE_KP_MINUS:
        return Qt::Key_Minus;

    case KEYCODE_KP_MULTIPLY:
        return Qt::Key_Asterisk;

    case KEYCODE_KP_DIVIDE:
        return Qt::Key_Slash;

    case KEYCODE_KP_ENTER:
        return Qt::Key_Enter;

    case KEYCODE_KP_HOME:
        return Qt::Key_Home;

    case KEYCODE_KP_UP:
        return Qt::Key_Up;

    case KEYCODE_KP_PG_UP:
        return Qt::Key_PageUp;

    case KEYCODE_KP_LEFT:
        return Qt::Key_Left;

    // Is this right?
    case KEYCODE_KP_FIVE:
        return Qt::Key_5;

    case KEYCODE_KP_RIGHT:
        return Qt::Key_Right;

    case KEYCODE_KP_END:
        return Qt::Key_End;

    case KEYCODE_KP_DOWN:
        return Qt::Key_Down;

    case KEYCODE_KP_PG_DOWN:
        return Qt::Key_PageDown;

    case KEYCODE_KP_INSERT:
        return Qt::Key_Insert;

    case KEYCODE_KP_DELETE:
        return Qt::Key_Delete;

    case KEYCODE_F1:
        return Qt::Key_F1;

    case KEYCODE_F2:
        return Qt::Key_F2;

    case KEYCODE_F3:
        return Qt::Key_F3;

    case KEYCODE_F4:
        return Qt::Key_F4;

    case KEYCODE_F5:
        return Qt::Key_F5;

    case KEYCODE_F6:
        return Qt::Key_F6;

    case KEYCODE_F7:
        return Qt::Key_F7;

    case KEYCODE_F8:
        return Qt::Key_F8;

    case KEYCODE_F9:
        return Qt::Key_F9;

    case KEYCODE_F10:
        return Qt::Key_F10;

    case KEYCODE_F11:
        return Qt::Key_F11;

    case KEYCODE_F12:
        return Qt::Key_F12;

    // See keycodes.h for more, but these are all the basics. And printables are already included.

    default:
#if defined(QQNXEVENTTHREAD_DEBUG)
        qDebug() << "QQNX: unknown key for translation:" << key;
#endif
        break;
    }

    return Qt::Key_Escape;
}

bool isKeypadKey( int key )
{
    switch (key)
    {
    case KEYCODE_KP_PLUS:
    case KEYCODE_KP_MINUS:
    case KEYCODE_KP_MULTIPLY:
    case KEYCODE_KP_DIVIDE:
    case KEYCODE_KP_ENTER:
    case KEYCODE_KP_HOME:
    case KEYCODE_KP_UP:
    case KEYCODE_KP_PG_UP:
    case KEYCODE_KP_LEFT:
    case KEYCODE_KP_FIVE:
    case KEYCODE_KP_RIGHT:
    case KEYCODE_KP_END:
    case KEYCODE_KP_DOWN:
    case KEYCODE_KP_PG_DOWN:
    case KEYCODE_KP_INSERT:
    case KEYCODE_KP_DELETE:
        return true;
    default:
        break;
    }

    return false;
}

QT_END_NAMESPACE

#endif // QQNXKEYTRANSLATOR_H
