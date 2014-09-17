/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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
#ifndef QKEYMAPPER_P_H
#define QKEYMAPPER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qobject.h>
#include <private/qobject_p.h>
#include <qkeysequence.h>
#include <qlist.h>
#include <qlocale.h>
#include <qevent.h>
#include <qhash.h>

#if defined (Q_WS_MAC64)
# include <private/qt_mac_p.h>
#endif

QT_BEGIN_NAMESPACE

class QKeyMapperPrivate;
class QKeyMapper : public QObject
{
    Q_OBJECT
public:
    explicit QKeyMapper();
    ~QKeyMapper();

    static QKeyMapper *instance();
    static void changeKeyboard();
    static bool sendKeyEvent(QWidget *widget, bool grab,
                             QEvent::Type type, int code, Qt::KeyboardModifiers modifiers,
                             const QString &text, bool autorepeat, int count,
                             quint32 nativeScanCode, quint32 nativeVirtualKey, quint32 nativeModifiers,
                             bool *unusedExceptForCocoa = 0);
    static QList<int> possibleKeys(QKeyEvent *e);

private:
    friend QKeyMapperPrivate *qt_keymapper_private();
    Q_DECLARE_PRIVATE(QKeyMapper)
    Q_DISABLE_COPY(QKeyMapper)
};



#if defined(Q_OS_WIN)
enum WindowsNativeModifiers {
    ShiftLeft            = 0x00000001,
    ControlLeft          = 0x00000002,
    AltLeft              = 0x00000004,
    MetaLeft             = 0x00000008,
    ShiftRight           = 0x00000010,
    ControlRight         = 0x00000020,
    AltRight             = 0x00000040,
    MetaRight            = 0x00000080,
    CapsLock             = 0x00000100,
    NumLock              = 0x00000200,
    ScrollLock           = 0x00000400,
    ExtendedKey          = 0x01000000,

    // Convenience mappings
    ShiftAny             = 0x00000011,
    ControlAny           = 0x00000022,
    AltAny               = 0x00000044,
    MetaAny              = 0x00000088,
    LockAny              = 0x00000700
};
# if !defined(tagMSG)
    typedef struct tagMSG MSG;
# endif
#elif defined(Q_WS_MAC)
QT_BEGIN_INCLUDE_NAMESPACE
# include <private/qt_mac_p.h>
QT_END_INCLUDE_NAMESPACE
#elif defined(Q_WS_X11)

QT_BEGIN_INCLUDE_NAMESPACE
typedef ulong XID;
typedef XID KeySym;
QT_END_INCLUDE_NAMESPACE

struct QXCoreDesc {
    int min_keycode;
    int max_keycode;
    int keysyms_per_keycode;
    KeySym *keysyms;
    uchar mode_switch;
    uchar num_lock;
    KeySym lock_meaning;
};

#endif

struct KeyboardLayoutItem;
typedef struct __TISInputSource * TISInputSourceRef;
class QKeyEvent;
class QKeyMapperPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QKeyMapper)
public:
    QKeyMapperPrivate();
    ~QKeyMapperPrivate();

    void clearMappings();
    QList<int> possibleKeys(QKeyEvent *e);

    QLocale keyboardInputLocale;
    Qt::LayoutDirection keyboardInputDirection;

#if defined(Q_OS_WIN)
    void clearRecordedKeys();
    void updateKeyMap(const MSG &msg);
    bool translateKeyEvent(QWidget *receiver, const MSG &msg, bool grab);
    void updatePossibleKeyCodes(unsigned char *kbdBuffer, quint32 scancode, quint32 vk_key);
    bool isADeadKey(unsigned int vk_key, unsigned int modifiers);
    void deleteLayouts();

    KeyboardLayoutItem *keyLayout[256];

#elif defined(Q_WS_X11)

    QList<int> possibleKeysXKB(QKeyEvent *event);
    QList<int> possibleKeysCore(QKeyEvent *event);

    bool translateKeyEventInternal(QWidget *keywidget,
                                   const XEvent *,
                                   KeySym &keysym,
                                   int& count,
                                   QString& text,
                                   Qt::KeyboardModifiers& modifiers,
                                   int &code,
                                   QEvent::Type &type,
                                   bool statefulTranslation = true);
    bool translateKeyEvent(QWidget *keywidget,
                           const XEvent *,
                           bool grab);

    int xkb_currentGroup;
    QXCoreDesc coreDesc;

#elif defined(Q_WS_MAC)
    bool updateKeyboard();
    void updateKeyMap(EventHandlerCallRef, EventRef, void *);
    bool translateKeyEvent(QWidget *, EventHandlerCallRef, EventRef, void *, bool);
    void deleteLayouts();

    enum { NullMode, UnicodeMode, OtherMode } keyboard_mode;
    union {
        const UCKeyboardLayout *unicode;
        void *other;
    } keyboard_layout_format;
#ifdef Q_WS_MAC64
    QCFType<TISInputSourceRef> currentInputSource;
#else
    KeyboardLayoutRef currentKeyboardLayout;
#endif
    KeyboardLayoutKind keyboard_kind;
    UInt32 keyboard_dead;
    KeyboardLayoutItem *keyLayout[256];
#elif defined(Q_WS_QWS)
#elif defined(Q_OS_SYMBIAN)
public:
    QString translateKeyEvent(int keySym, Qt::KeyboardModifiers modifiers);
    int mapS60KeyToQt(TUint s60key);
    int mapS60ScanCodesToQt(TUint s60key);
    int mapQtToS60Key(int qtKey);
    int mapQtToS60ScanCodes(int qtKey);
    int mapS60RemConIdToS60Key(int s60RemConId);
    int mapS60RemConIdToS60ScanCodes(int s60RemConId);
    void updateInputLanguage();
#endif
};

QKeyMapperPrivate *qt_keymapper_private(); // from qkeymapper.cpp

QT_END_NAMESPACE

#endif // QKEYMAPPER_P_H
