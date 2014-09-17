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

#include "private/qkeymapper_p.h"
#include <private/qcore_symbian_p.h>
#include <e32keys.h>
#include <e32cmn.h>
#include <centralrepository.h>
#include <biditext.h>

QT_BEGIN_NAMESPACE

QKeyMapperPrivate::QKeyMapperPrivate()
{
}

QKeyMapperPrivate::~QKeyMapperPrivate()
{
}

QList<int> QKeyMapperPrivate::possibleKeys(QKeyEvent * /* e */)
{
    QList<int> result;
    return result;
}

void QKeyMapperPrivate::clearMappings()
{
    // stub
}

QString QKeyMapperPrivate::translateKeyEvent(int keySym, Qt::KeyboardModifiers /* modifiers */)
{
    if (keySym >= Qt::Key_Escape) {
        switch (keySym) {
        case Qt::Key_Tab:
            return QString(QChar('\t'));
        case Qt::Key_Return:    // fall through
        case Qt::Key_Enter:
            return QString(QChar('\r'));
        default:
            return QString();
        }
    }

    // Symbian doesn't actually use modifiers, but gives us the character code directly.

    return QString(QChar(keySym));
}

#include <e32keys.h>
#include <remconcoreapi.h>
struct KeyMapping{
    TKeyCode s60KeyCode;
    TStdScanCode s60ScanCode;
    TRemConCoreApiOperationId s60RemConId;
    Qt::Key qtKey;
};

using namespace Qt;

// key mapping table in the format of 
// {S60 key code, S60 scan code, S60 RemCon operation Id, Qt key code}
static const KeyMapping keyMapping[] = {
    {EKeyBackspace, EStdKeyBackspace, ENop, Key_Backspace},
    {EKeyTab, EStdKeyTab, ENop, Key_Tab},
    {EKeyEnter, EStdKeyEnter, ERemConCoreApiEnter, Key_Enter},
    {EKeyEscape, EStdKeyEscape, ENop, Key_Escape},
    {EKeySpace, EStdKeySpace, ENop, Key_Space},
    {EKeyDelete, EStdKeyDelete, ENop, Key_Delete},
    {EKeyPrintScreen, EStdKeyPrintScreen, ENop, Key_SysReq},
    {EKeyPause, EStdKeyPause, ENop, Key_Pause},
    {EKeyHome, EStdKeyHome, ENop, Key_Home},
    {EKeyEnd, EStdKeyEnd, ENop, Key_End},
    {EKeyPageUp, EStdKeyPageUp, ERemConCoreApiPageUp, Key_PageUp},
    {EKeyPageDown, EStdKeyPageDown, ERemConCoreApiPageDown, Key_PageDown},
    {EKeyInsert, EStdKeyInsert, ENop, Key_Insert},
    {EKeyLeftArrow, EStdKeyLeftArrow, ERemConCoreApiLeft, Key_Left},
    {EKeyRightArrow, EStdKeyRightArrow, ERemConCoreApiRight, Key_Right},
    {EKeyUpArrow, EStdKeyUpArrow, ERemConCoreApiUp, Key_Up},
    {EKeyDownArrow, EStdKeyDownArrow, ERemConCoreApiDown, Key_Down},
    {EKeyLeftShift, EStdKeyLeftShift, ENop, Key_Shift},
    {EKeyRightShift, EStdKeyRightShift, ENop, Key_Shift},
    {EKeyLeftAlt, EStdKeyLeftAlt, ENop, Key_Alt},
    {EKeyRightAlt, EStdKeyRightAlt, ENop, Key_AltGr},
    {EKeyLeftCtrl, EStdKeyLeftCtrl, ENop, Key_Control},
    {EKeyRightCtrl, EStdKeyRightCtrl, ENop, Key_Control},
    {EKeyLeftFunc, EStdKeyLeftFunc, ENop, Key_Super_L},
    {EKeyRightFunc, EStdKeyRightFunc, ENop, Key_Super_R},
    {EKeyCapsLock, EStdKeyCapsLock, ENop, Key_CapsLock},
    {EKeyNumLock, EStdKeyNumLock, ENop, Key_NumLock},
    {EKeyScrollLock, EStdKeyScrollLock, ENop, Key_ScrollLock},
    {EKeyF1, EStdKeyF1, ERemConCoreApiF1, Key_F1},
    {EKeyF2, EStdKeyF2, ERemConCoreApiF2, Key_F2},
    {EKeyF3, EStdKeyF3, ERemConCoreApiF3, Key_F3},
    {EKeyF4, EStdKeyF4, ERemConCoreApiF4, Key_F4},
    {EKeyF5, EStdKeyF5, ERemConCoreApiF5, Key_F5},
    {EKeyF6, EStdKeyF6, ENop, Key_F6},
    {EKeyF7, EStdKeyF7, ENop, Key_F7},
    {EKeyF8, EStdKeyF8, ENop, Key_F8},
    {EKeyF9, EStdKeyF9, ENop, Key_F9},
    {EKeyF10, EStdKeyF10, ENop, Key_F10},
    {EKeyF11, EStdKeyF11, ENop, Key_F11},
    {EKeyF12, EStdKeyF12, ENop, Key_F12},
    {EKeyF13, EStdKeyF13, ENop, Key_F13},
    {EKeyF14, EStdKeyF14, ENop, Key_F14},
    {EKeyF15, EStdKeyF15, ENop, Key_F15},
    {EKeyF16, EStdKeyF16, ENop, Key_F16},
    {EKeyF17, EStdKeyF17, ENop, Key_F17},
    {EKeyF18, EStdKeyF18, ENop, Key_F18},
    {EKeyF19, EStdKeyF19, ENop, Key_F19},
    {EKeyF20, EStdKeyF20, ENop, Key_F20},
    {EKeyF21, EStdKeyF21, ENop, Key_F21},
    {EKeyF22, EStdKeyF22, ENop, Key_F22},
    {EKeyF23, EStdKeyF23, ENop, Key_F23},
    {EKeyF24, EStdKeyF24, ENop, Key_F24},
    {EKeyOff, EStdKeyOff, ENop, Key_PowerOff},
//    {EKeyMenu, EStdKeyMenu, ENop, Key_Menu}, // Menu is EKeyApplication0
    {EKeyHelp, EStdKeyHelp, ERemConCoreApiHelp, Key_Help},
    {EKeyDial, EStdKeyDial, ENop, Key_Call},
    {EKeyIncVolume, EStdKeyIncVolume, ERemConCoreApiVolumeUp, Key_VolumeUp},
    {EKeyDecVolume, EStdKeyDecVolume, ERemConCoreApiVolumeDown, Key_VolumeDown},
    {EKeyDevice0, EStdKeyDevice0, ENop, Key_Context1}, // Found by manual testing.
    {EKeyDevice1, EStdKeyDevice1, ENop, Key_Context2}, // Found by manual testing.
    {EKeyDevice3, EStdKeyDevice3, ERemConCoreApiSelect, Key_Select},
    {EKeyDevice7, EStdKeyDevice7, ENop, Key_Camera},  
    {EKeyApplication0, EStdKeyApplication0, ENop, Key_Menu}, // Found by manual testing.
    {EKeyApplication1, EStdKeyApplication1, ENop, Key_Launch1}, // Found by manual testing.
    {EKeyApplication2, EStdKeyApplication2, ERemConCoreApiPlay, Key_MediaPlay}, // Found by manual testing.
    {EKeyApplication3, EStdKeyApplication3, ERemConCoreApiStop, Key_MediaStop}, // Found by manual testing.
    {EKeyApplication4, EStdKeyApplication4, ERemConCoreApiForward, Key_MediaNext}, // Found by manual testing.
    {EKeyApplication5, EStdKeyApplication5, ERemConCoreApiBackward, Key_MediaPrevious}, // Found by manual testing.
    {EKeyApplication6, EStdKeyApplication6, ENop, Key_Launch6},
    {EKeyApplication7, EStdKeyApplication7, ENop, Key_Launch7},
    {EKeyApplication8, EStdKeyApplication8, ENop, Key_Launch8},
    {EKeyApplication9, EStdKeyApplication9, ENop, Key_Launch9},
    {EKeyApplicationA, EStdKeyApplicationA, ENop, Key_LaunchA},
    {EKeyApplicationB, EStdKeyApplicationB, ENop, Key_LaunchB},
    {EKeyApplicationC, EStdKeyApplicationC, ENop, Key_LaunchC},
    {EKeyApplicationD, EStdKeyApplicationD, ENop, Key_LaunchD},
    {EKeyApplicationE, EStdKeyApplicationE, ENop, Key_LaunchE},
    {EKeyApplicationF, EStdKeyApplicationF, ENop, Key_LaunchF},
    {EKeyApplication19, EStdKeyApplication19, ENop, Key_CameraFocus}, 
    {EKeyYes, EStdKeyYes, ENop, Key_Yes},
    {EKeyNo, EStdKeyNo, ENop, Key_No},
    {EKeyDevice20, EStdKeyDevice20, ERemConCoreApiPausePlayFunction, Key_MediaTogglePlayPause},
    {EKeyDevice21, EStdKeyDevice21, ERemConCoreApiRewind, Key_AudioRewind},
    {EKeyDevice22, EStdKeyDevice22, ERemConCoreApiFastForward, Key_AudioForward},
    {TKeyCode(0), TStdScanCode(0), ENop, Qt::Key(0)}
};

int QKeyMapperPrivate::mapS60KeyToQt(TUint s60key)
{
    int res = Qt::Key_unknown;
    for (int i = 0; keyMapping[i].s60KeyCode != 0; i++) {
        if (keyMapping[i].s60KeyCode == s60key) {
            res = keyMapping[i].qtKey;
            break;
        }
    }
    return res;
}

int QKeyMapperPrivate::mapS60ScanCodesToQt(TUint s60scanCode)
{
    int res = Qt::Key_unknown;
    for (int i = 0; keyMapping[i].s60KeyCode != 0; i++) {
        if (keyMapping[i].s60ScanCode == s60scanCode) {
            res = keyMapping[i].qtKey;
            break;
        }
    }
    return res;
}

int QKeyMapperPrivate::mapQtToS60Key(int qtKey)
{
    int res = KErrUnknown;
    for (int i = 0; keyMapping[i].s60KeyCode != 0; i++) {
        if (keyMapping[i].qtKey == qtKey) {
            res = keyMapping[i].s60KeyCode;
            break;
        }
    }
    return res;
}

int QKeyMapperPrivate::mapQtToS60ScanCodes(int qtKey)
{
    int res = KErrUnknown;
    for (int i = 0; keyMapping[i].s60KeyCode != 0; i++) {
        if (keyMapping[i].qtKey == qtKey) {
            res = keyMapping[i].s60ScanCode;
            break;
        }
    }
    return res;
}

int QKeyMapperPrivate::mapS60RemConIdToS60Key(int s60RemConId)
{
    int res = KErrUnknown;
    for (int i = 0; keyMapping[i].s60KeyCode != 0; i++) {
        if (keyMapping[i].s60RemConId == s60RemConId) {
            res = keyMapping[i].s60KeyCode;
            break;
        }
    }
    return res;
}

int QKeyMapperPrivate::mapS60RemConIdToS60ScanCodes(int s60RemConId)
{
    int res = KErrUnknown;
    for (int i = 0; keyMapping[i].s60KeyCode != 0; i++) {
        if (keyMapping[i].s60RemConId == s60RemConId) {
            res = keyMapping[i].s60ScanCode;
            break;
        }
    }
    return res;
}

void QKeyMapperPrivate::updateInputLanguage()
{
#ifdef Q_WS_S60
    TInt err;
    CRepository *repo;
    const TUid KCRUidAknFep = TUid::Uid(0x101F876D);
    const TUint32 KAknFepInputTxtLang = 0x00000005;
    TRAP(err, repo = CRepository::NewL(KCRUidAknFep));
    if (err != KErrNone)
        return;

    TInt symbianLang;
    err = repo->Get(KAknFepInputTxtLang, symbianLang);
    delete repo;
    if (err != KErrNone)
        return;

    QString qtLang = QString::fromAscii(qt_symbianLocaleName(symbianLang));
    keyboardInputLocale = QLocale(qtLang);
    keyboardInputDirection = (TBidiText::ScriptDirectionality(TLanguage(symbianLang)) == TBidiText::ERightToLeft)
            ? Qt::RightToLeft : Qt::LeftToRight;
#else
    keyboardInputLocale = QLocale();
    keyboardInputDirection = Qt::LeftToRight;
#endif
}

QT_END_NAMESPACE
