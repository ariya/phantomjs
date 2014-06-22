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

#ifndef QCOCOAKEYMAPPER_H
#define QCOCOAKEYMAPPER_H

#include <qcocoahelpers.h>

#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

#include <QtCore/QList>
#include <QtGui/QKeyEvent>

QT_BEGIN_NAMESPACE

/*
    \internal
    A Mac KeyboardLayoutItem has 8 possible states:
        1. Unmodified
        2. Shift
        3. Control
        4. Control + Shift
        5. Alt
        6. Alt + Shift
        7. Alt + Control
        8. Alt + Control + Shift
        9. Meta
        10. Meta + Shift
        11. Meta + Control
        12. Meta + Control + Shift
        13. Meta + Alt
        14. Meta + Alt + Shift
        15. Meta + Alt + Control
        16. Meta + Alt + Control + Shift
*/
struct KeyboardLayoutItem {
    bool dirty;
    quint32 qtKey[16]; // Can by any Qt::Key_<foo>, or unicode character
};


class QCocoaKeyMapper
{
public:
    QCocoaKeyMapper();
    ~QCocoaKeyMapper();
    static Qt::KeyboardModifiers queryKeyboardModifiers();
    QList<int> possibleKeys(const QKeyEvent *event) const;
    bool updateKeyboard();
    void deleteLayouts();
    void updateKeyMap(unsigned short macVirtualKey, QChar unicodeKey);
    void clearMappings();

private:
    QCFType<TISInputSourceRef> currentInputSource;

    QLocale keyboardInputLocale;
    Qt::LayoutDirection keyboardInputDirection;
    enum { NullMode, UnicodeMode, OtherMode } keyboard_mode;
    union {
        const UCKeyboardLayout *unicode;
        void *other;
    } keyboard_layout_format;
    KeyboardLayoutKind keyboard_kind;
    UInt32 keyboard_dead;
    KeyboardLayoutItem *keyLayout[256];
};

QT_END_NAMESPACE

#endif

