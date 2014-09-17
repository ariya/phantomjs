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

#include <private/qt_mac_p.h>
#include <qdebug.h>
#include <qevent.h>
#include <private/qevent_p.h>
#include <qtextcodec.h>
#include <qapplication.h>
#include <qinputcontext.h>
#include <private/qkeymapper_p.h>
#include <private/qapplication_p.h>
#include <private/qmacinputcontext_p.h>

QT_BEGIN_NAMESPACE

QT_USE_NAMESPACE

/*****************************************************************************
  QKeyMapper debug facilities
 *****************************************************************************/
//#define DEBUG_KEY_BINDINGS
//#define DEBUG_KEY_BINDINGS_MODIFIERS
//#define DEBUG_KEY_MAPS

/*****************************************************************************
  Internal variables and functions
 *****************************************************************************/
bool qt_mac_eat_unicode_key = false;
extern bool qt_sendSpontaneousEvent(QObject *obj, QEvent *event); //qapplication_mac.cpp

Q_GUI_EXPORT void qt_mac_secure_keyboard(bool b)
{
    static bool secure = false;
    if (b != secure){
        b ? EnableSecureEventInput() : DisableSecureEventInput();
        secure = b;
    }
}

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

// Possible modifier states.
// NOTE: The order of these states match the order in QKeyMapperPrivate::updatePossibleKeyCodes()!
static const Qt::KeyboardModifiers ModsTbl[] = {
    Qt::NoModifier,                                             // 0
    Qt::ShiftModifier,                                          // 1
    Qt::ControlModifier,                                        // 2
    Qt::ControlModifier | Qt::ShiftModifier,                    // 3
    Qt::AltModifier,                                            // 4
    Qt::AltModifier | Qt::ShiftModifier,                        // 5
    Qt::AltModifier | Qt::ControlModifier,                      // 6
    Qt::AltModifier | Qt::ShiftModifier | Qt::ControlModifier,  // 7
    Qt::MetaModifier,                                           // 8
    Qt::MetaModifier | Qt::ShiftModifier,                       // 9
    Qt::MetaModifier | Qt::ControlModifier,                    // 10
    Qt::MetaModifier | Qt::ControlModifier | Qt::ShiftModifier,// 11
    Qt::MetaModifier | Qt::AltModifier,                        // 12
    Qt::MetaModifier | Qt::AltModifier | Qt::ShiftModifier,    // 13
    Qt::MetaModifier | Qt::AltModifier | Qt::ControlModifier,  // 14
    Qt::MetaModifier | Qt::AltModifier | Qt::ShiftModifier | Qt::ControlModifier,  // 15
};

/* key maps */
struct qt_mac_enum_mapper
{
    int mac_code;
    int qt_code;
#if defined(DEBUG_KEY_BINDINGS)
#   define QT_MAC_MAP_ENUM(x) x, #x
    const char *desc;
#else
#   define QT_MAC_MAP_ENUM(x) x
#endif
};

//modifiers
static qt_mac_enum_mapper qt_mac_modifier_symbols[] = {
    { shiftKey, QT_MAC_MAP_ENUM(Qt::ShiftModifier) },
    { rightShiftKey, QT_MAC_MAP_ENUM(Qt::ShiftModifier) },
    { controlKey, QT_MAC_MAP_ENUM(Qt::MetaModifier) },
    { rightControlKey, QT_MAC_MAP_ENUM(Qt::MetaModifier) },
    { cmdKey, QT_MAC_MAP_ENUM(Qt::ControlModifier) },
    { optionKey, QT_MAC_MAP_ENUM(Qt::AltModifier) },
    { rightOptionKey, QT_MAC_MAP_ENUM(Qt::AltModifier) },
    { kEventKeyModifierNumLockMask, QT_MAC_MAP_ENUM(Qt::KeypadModifier) },
    { 0, QT_MAC_MAP_ENUM(0) }
};
Qt::KeyboardModifiers qt_mac_get_modifiers(int keys)
{
#ifdef DEBUG_KEY_BINDINGS_MODIFIERS
    qDebug("Qt: internal: **Mapping modifiers: %d (0x%04x)", keys, keys);
#endif
    Qt::KeyboardModifiers ret = Qt::NoModifier;
    for (int i = 0; qt_mac_modifier_symbols[i].qt_code; i++) {
        if (keys & qt_mac_modifier_symbols[i].mac_code) {
#ifdef DEBUG_KEY_BINDINGS_MODIFIERS
            qDebug("Qt: internal: got modifier: %s", qt_mac_modifier_symbols[i].desc);
#endif
            ret |= Qt::KeyboardModifier(qt_mac_modifier_symbols[i].qt_code);
        }
    }
    if (qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta)) {
        Qt::KeyboardModifiers oldModifiers = ret;
        ret &= ~(Qt::MetaModifier | Qt::ControlModifier);
        if (oldModifiers & Qt::ControlModifier)
            ret |= Qt::MetaModifier;
        if (oldModifiers & Qt::MetaModifier)
            ret |= Qt::ControlModifier;
    }
    return ret;
}
static int qt_mac_get_mac_modifiers(Qt::KeyboardModifiers keys)
{
#ifdef DEBUG_KEY_BINDINGS_MODIFIERS
    qDebug("Qt: internal: **Mapping modifiers: %d (0x%04x)", (int)keys, (int)keys);
#endif
    int ret = 0;
    for (int i = 0; qt_mac_modifier_symbols[i].qt_code; i++) {
        if (keys & qt_mac_modifier_symbols[i].qt_code) {
#ifdef DEBUG_KEY_BINDINGS_MODIFIERS
            qDebug("Qt: internal: got modifier: %s", qt_mac_modifier_symbols[i].desc);
#endif
            ret |= qt_mac_modifier_symbols[i].mac_code;
        }
    }

    if (qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta)) {
        int oldModifiers = ret;
        ret &= ~(controlKeyBit | cmdKeyBit);
        if (oldModifiers & controlKeyBit)
            ret |= cmdKeyBit;
        if (oldModifiers & cmdKeyBit)
            ret |= controlKeyBit;
    }
    return ret;
}
void qt_mac_send_modifiers_changed(quint32 modifiers, QObject *object)
{
    static quint32 cachedModifiers = 0;
    quint32 lastModifiers = cachedModifiers,
          changedModifiers = lastModifiers ^ modifiers;
    cachedModifiers = modifiers;

    //check the bits
    static qt_mac_enum_mapper modifier_key_symbols[] = {
        { shiftKeyBit, QT_MAC_MAP_ENUM(Qt::Key_Shift) },
        { rightShiftKeyBit, QT_MAC_MAP_ENUM(Qt::Key_Shift) }, //???
        { controlKeyBit, QT_MAC_MAP_ENUM(Qt::Key_Meta) },
        { rightControlKeyBit, QT_MAC_MAP_ENUM(Qt::Key_Meta) }, //???
        { cmdKeyBit, QT_MAC_MAP_ENUM(Qt::Key_Control) },
        { optionKeyBit, QT_MAC_MAP_ENUM(Qt::Key_Alt) },
        { rightOptionKeyBit, QT_MAC_MAP_ENUM(Qt::Key_Alt) }, //???
        { alphaLockBit, QT_MAC_MAP_ENUM(Qt::Key_CapsLock) },
        { kEventKeyModifierNumLockBit, QT_MAC_MAP_ENUM(Qt::Key_NumLock) },
        {   0, QT_MAC_MAP_ENUM(0) } };
    for (int i = 0; i <= 32; i++) { //just check each bit
        if (!(changedModifiers & (1 << i)))
            continue;
        QEvent::Type etype = QEvent::KeyPress;
        if (lastModifiers & (1 << i))
            etype = QEvent::KeyRelease;
        int key = 0;
        for (uint x = 0; modifier_key_symbols[x].mac_code; x++) {
            if (modifier_key_symbols[x].mac_code == i) {
#ifdef DEBUG_KEY_BINDINGS_MODIFIERS
                qDebug("got modifier changed: %s", modifier_key_symbols[x].desc);
#endif
                key = modifier_key_symbols[x].qt_code;
                break;
            }
        }
        if (!key) {
#ifdef DEBUG_KEY_BINDINGS_MODIFIERS
            qDebug("could not get modifier changed: %d", i);
#endif
            continue;
        }
#ifdef DEBUG_KEY_BINDINGS_MODIFIERS
        qDebug("KeyEvent (modif): Sending %s to %s::%s: %d - 0x%08x",
               etype == QEvent::KeyRelease ? "KeyRelease" : "KeyPress",
               object ? object->metaObject()->className() : "none",
               object ? object->objectName().toLatin1().constData() : "",
               key, (int)modifiers);
#endif
        QKeyEvent ke(etype, key, qt_mac_get_modifiers(modifiers ^ (1 << i)), QLatin1String(""));
        qt_sendSpontaneousEvent(object, &ke);
    }
}

//keyboard keys (non-modifiers)
static qt_mac_enum_mapper qt_mac_keyboard_symbols[] = {
    { kHomeCharCode, QT_MAC_MAP_ENUM(Qt::Key_Home) },
    { kEnterCharCode, QT_MAC_MAP_ENUM(Qt::Key_Enter) },
    { kEndCharCode, QT_MAC_MAP_ENUM(Qt::Key_End) },
    { kBackspaceCharCode, QT_MAC_MAP_ENUM(Qt::Key_Backspace) },
    { kTabCharCode, QT_MAC_MAP_ENUM(Qt::Key_Tab) },
    { kPageUpCharCode, QT_MAC_MAP_ENUM(Qt::Key_PageUp) },
    { kPageDownCharCode, QT_MAC_MAP_ENUM(Qt::Key_PageDown) },
    { kReturnCharCode, QT_MAC_MAP_ENUM(Qt::Key_Return) },
    { kEscapeCharCode, QT_MAC_MAP_ENUM(Qt::Key_Escape) },
    { kLeftArrowCharCode, QT_MAC_MAP_ENUM(Qt::Key_Left) },
    { kRightArrowCharCode, QT_MAC_MAP_ENUM(Qt::Key_Right) },
    { kUpArrowCharCode, QT_MAC_MAP_ENUM(Qt::Key_Up) },
    { kDownArrowCharCode, QT_MAC_MAP_ENUM(Qt::Key_Down) },
    { kHelpCharCode, QT_MAC_MAP_ENUM(Qt::Key_Help) },
    { kDeleteCharCode, QT_MAC_MAP_ENUM(Qt::Key_Delete) },
//ascii maps, for debug
    { ':', QT_MAC_MAP_ENUM(Qt::Key_Colon) },
    { ';', QT_MAC_MAP_ENUM(Qt::Key_Semicolon) },
    { '<', QT_MAC_MAP_ENUM(Qt::Key_Less) },
    { '=', QT_MAC_MAP_ENUM(Qt::Key_Equal) },
    { '>', QT_MAC_MAP_ENUM(Qt::Key_Greater) },
    { '?', QT_MAC_MAP_ENUM(Qt::Key_Question) },
    { '@', QT_MAC_MAP_ENUM(Qt::Key_At) },
    { ' ', QT_MAC_MAP_ENUM(Qt::Key_Space) },
    { '!', QT_MAC_MAP_ENUM(Qt::Key_Exclam) },
    { '"', QT_MAC_MAP_ENUM(Qt::Key_QuoteDbl) },
    { '#', QT_MAC_MAP_ENUM(Qt::Key_NumberSign) },
    { '$', QT_MAC_MAP_ENUM(Qt::Key_Dollar) },
    { '%', QT_MAC_MAP_ENUM(Qt::Key_Percent) },
    { '&', QT_MAC_MAP_ENUM(Qt::Key_Ampersand) },
    { '\'', QT_MAC_MAP_ENUM(Qt::Key_Apostrophe) },
    { '(', QT_MAC_MAP_ENUM(Qt::Key_ParenLeft) },
    { ')', QT_MAC_MAP_ENUM(Qt::Key_ParenRight) },
    { '*', QT_MAC_MAP_ENUM(Qt::Key_Asterisk) },
    { '+', QT_MAC_MAP_ENUM(Qt::Key_Plus) },
    { ',', QT_MAC_MAP_ENUM(Qt::Key_Comma) },
    { '-', QT_MAC_MAP_ENUM(Qt::Key_Minus) },
    { '.', QT_MAC_MAP_ENUM(Qt::Key_Period) },
    { '/', QT_MAC_MAP_ENUM(Qt::Key_Slash) },
    { '[', QT_MAC_MAP_ENUM(Qt::Key_BracketLeft) },
    { ']', QT_MAC_MAP_ENUM(Qt::Key_BracketRight) },
    { '\\', QT_MAC_MAP_ENUM(Qt::Key_Backslash) },
    { '_', QT_MAC_MAP_ENUM(Qt::Key_Underscore) },
    { '`', QT_MAC_MAP_ENUM(Qt::Key_QuoteLeft) },
    { '{', QT_MAC_MAP_ENUM(Qt::Key_BraceLeft) },
    { '}', QT_MAC_MAP_ENUM(Qt::Key_BraceRight) },
    { '|', QT_MAC_MAP_ENUM(Qt::Key_Bar) },
    { '~', QT_MAC_MAP_ENUM(Qt::Key_AsciiTilde) },
    { '^', QT_MAC_MAP_ENUM(Qt::Key_AsciiCircum) },
    {   0, QT_MAC_MAP_ENUM(0) }
};

static qt_mac_enum_mapper qt_mac_keyvkey_symbols[] = { //real scan codes
    { 122, QT_MAC_MAP_ENUM(Qt::Key_F1) },
    { 120, QT_MAC_MAP_ENUM(Qt::Key_F2) },
    { 99,  QT_MAC_MAP_ENUM(Qt::Key_F3) },
    { 118, QT_MAC_MAP_ENUM(Qt::Key_F4) },
    { 96,  QT_MAC_MAP_ENUM(Qt::Key_F5) },
    { 97,  QT_MAC_MAP_ENUM(Qt::Key_F6) },
    { 98,  QT_MAC_MAP_ENUM(Qt::Key_F7) },
    { 100, QT_MAC_MAP_ENUM(Qt::Key_F8) },
    { 101, QT_MAC_MAP_ENUM(Qt::Key_F9) },
    { 109, QT_MAC_MAP_ENUM(Qt::Key_F10) },
    { 103, QT_MAC_MAP_ENUM(Qt::Key_F11) },
    { 111, QT_MAC_MAP_ENUM(Qt::Key_F12) },
    { 105, QT_MAC_MAP_ENUM(Qt::Key_F13) },
    { 107, QT_MAC_MAP_ENUM(Qt::Key_F14) },
    { 113, QT_MAC_MAP_ENUM(Qt::Key_F15) },
    { 106, QT_MAC_MAP_ENUM(Qt::Key_F16) },
    {   0, QT_MAC_MAP_ENUM(0) }
};

static qt_mac_enum_mapper qt_mac_private_unicode[] = {
    { 0xF700, QT_MAC_MAP_ENUM(Qt::Key_Up) },            //NSUpArrowFunctionKey
    { 0xF701, QT_MAC_MAP_ENUM(Qt::Key_Down) },          //NSDownArrowFunctionKey
    { 0xF702, QT_MAC_MAP_ENUM(Qt::Key_Left) },          //NSLeftArrowFunctionKey
    { 0xF703, QT_MAC_MAP_ENUM(Qt::Key_Right) },         //NSRightArrowFunctionKey
    { 0xF727, QT_MAC_MAP_ENUM(Qt::Key_Insert) },        //NSInsertFunctionKey
    { 0xF728, QT_MAC_MAP_ENUM(Qt::Key_Delete) },        //NSDeleteFunctionKey
    { 0xF729, QT_MAC_MAP_ENUM(Qt::Key_Home) },          //NSHomeFunctionKey
    { 0xF72B, QT_MAC_MAP_ENUM(Qt::Key_End) },           //NSEndFunctionKey
    { 0xF72C, QT_MAC_MAP_ENUM(Qt::Key_PageUp) },        //NSPageUpFunctionKey
    { 0xF72D, QT_MAC_MAP_ENUM(Qt::Key_PageDown) },      //NSPageDownFunctionKey
    { 0xF72F, QT_MAC_MAP_ENUM(Qt::Key_ScrollLock) },    //NSScrollLockFunctionKey
    { 0xF730, QT_MAC_MAP_ENUM(Qt::Key_Pause) },         //NSPauseFunctionKey
    { 0xF731, QT_MAC_MAP_ENUM(Qt::Key_SysReq) },        //NSSysReqFunctionKey
    { 0xF735, QT_MAC_MAP_ENUM(Qt::Key_Menu) },          //NSMenuFunctionKey
    { 0xF738, QT_MAC_MAP_ENUM(Qt::Key_Print) },         //NSPrintFunctionKey
    { 0xF73A, QT_MAC_MAP_ENUM(Qt::Key_Clear) },         //NSClearDisplayFunctionKey
    { 0xF73D, QT_MAC_MAP_ENUM(Qt::Key_Insert) },        //NSInsertCharFunctionKey
    { 0xF73E, QT_MAC_MAP_ENUM(Qt::Key_Delete) },        //NSDeleteCharFunctionKey
    { 0xF741, QT_MAC_MAP_ENUM(Qt::Key_Select) },        //NSSelectFunctionKey
    { 0xF742, QT_MAC_MAP_ENUM(Qt::Key_Execute) },       //NSExecuteFunctionKey
    { 0xF746, QT_MAC_MAP_ENUM(Qt::Key_Help) },          //NSHelpFunctionKey
    { 0xF747, QT_MAC_MAP_ENUM(Qt::Key_Mode_switch) },   //NSModeSwitchFunctionKey
    {   0,    QT_MAC_MAP_ENUM(0) }
};

static int qt_mac_get_key(int modif, const QChar &key, int virtualKey)
{
#ifdef DEBUG_KEY_BINDINGS
    qDebug("**Mapping key: %d (0x%04x) - %d (0x%04x)", key.unicode(), key.unicode(), virtualKey, virtualKey);
#endif

    if (key == kClearCharCode && virtualKey == 0x47)
        return Qt::Key_Clear;

    if (key.isDigit()) {
#ifdef DEBUG_KEY_BINDINGS
            qDebug("%d: got key: %d", __LINE__, key.digitValue());
#endif
        return key.digitValue() + Qt::Key_0;
    }

    if (key.isLetter()) {
#ifdef DEBUG_KEY_BINDINGS
        qDebug("%d: got key: %d", __LINE__, (key.toUpper().unicode() - 'A'));
#endif
        return (key.toUpper().unicode() - 'A') + Qt::Key_A;
    }
    if (key.isSymbol()) {
#ifdef DEBUG_KEY_BINDINGS
        qDebug("%d: got key: %d", __LINE__, (key.unicode()));
#endif
        return key.unicode();
    }

    for (int i = 0; qt_mac_keyboard_symbols[i].qt_code; i++) {
        if (qt_mac_keyboard_symbols[i].mac_code == key) {
            /* To work like Qt for X11 we issue Backtab when Shift + Tab are pressed */
            if (qt_mac_keyboard_symbols[i].qt_code == Qt::Key_Tab && (modif & Qt::ShiftModifier)) {
#ifdef DEBUG_KEY_BINDINGS
                qDebug("%d: got key: Qt::Key_Backtab", __LINE__);
#endif
                return Qt::Key_Backtab;
            }

#ifdef DEBUG_KEY_BINDINGS
            qDebug("%d: got key: %s", __LINE__, qt_mac_keyboard_symbols[i].desc);
#endif
            return qt_mac_keyboard_symbols[i].qt_code;
        }
    }

    //last ditch try to match the scan code
    for (int i = 0; qt_mac_keyvkey_symbols[i].qt_code; i++) {
        if (qt_mac_keyvkey_symbols[i].mac_code == virtualKey) {
#ifdef DEBUG_KEY_BINDINGS
            qDebug("%d: got key: %s", __LINE__, qt_mac_keyvkey_symbols[i].desc);
#endif
            return qt_mac_keyvkey_symbols[i].qt_code;
        }
    }

    // check if they belong to key codes in private unicode range
    if (key >= 0xf700 && key <= 0xf747) {
        if (key >= 0xf704 && key <= 0xf726) {
            return Qt::Key_F1 + (key.unicode() - 0xf704) ;
        }
        for (int i = 0; qt_mac_private_unicode[i].qt_code; i++) {
            if (qt_mac_private_unicode[i].mac_code == key) {
                return qt_mac_private_unicode[i].qt_code;
            }
        }

    }

    //oh well
#ifdef DEBUG_KEY_BINDINGS
    qDebug("Unknown case.. %s:%d %d[%d] %d", __FILE__, __LINE__, key.unicode(), key.toLatin1(), virtualKey);
#endif
    return Qt::Key_unknown;
}

static Boolean qt_KeyEventComparatorProc(EventRef inEvent, void *data)
{
    UInt32 ekind = GetEventKind(inEvent),
           eclass = GetEventClass(inEvent);
    return (eclass == kEventClassKeyboard && (void *)ekind == data);
}

static bool translateKeyEventInternal(EventHandlerCallRef er, EventRef keyEvent, int *qtKey,
                                      QChar *outChar, Qt::KeyboardModifiers *outModifiers, bool *outHandled)
{
#if !defined(QT_MAC_USE_COCOA) || defined(Q_OS_MAC64)
    Q_UNUSED(er);
    Q_UNUSED(outHandled);
#endif
    const UInt32 ekind = GetEventKind(keyEvent);
    {
        UInt32 mac_modifiers = 0;
        GetEventParameter(keyEvent, kEventParamKeyModifiers, typeUInt32, 0,
                          sizeof(mac_modifiers), 0, &mac_modifiers);
#ifdef DEBUG_KEY_BINDINGS_MODIFIERS
        qDebug("************ Mapping modifiers and key ***********");
#endif
        *outModifiers = qt_mac_get_modifiers(mac_modifiers);
#ifdef DEBUG_KEY_BINDINGS_MODIFIERS
        qDebug("------------ Mapping modifiers and key -----------");
#endif
    }

    //get keycode
    UInt32 keyCode = 0;
    GetEventParameter(keyEvent, kEventParamKeyCode, typeUInt32, 0, sizeof(keyCode), 0, &keyCode);

    //get mac mapping
    static UInt32 tmp_unused_state = 0L;
    const UCKeyboardLayout *uchrData = 0;
#if defined(Q_OS_MAC32)
    KeyboardLayoutRef keyLayoutRef = 0;
    KLGetCurrentKeyboardLayout(&keyLayoutRef);
    OSStatus err;
    if (keyLayoutRef != 0) {
        err = KLGetKeyboardLayoutProperty(keyLayoutRef, kKLuchrData,
                                  (reinterpret_cast<const void **>(&uchrData)));
        if (err != noErr) {
            qWarning("Qt::internal::unable to get keyboardlayout %ld %s:%d",
                     long(err), __FILE__, __LINE__);
        }
    }
#else
    QCFType<TISInputSourceRef> inputSource = TISCopyCurrentKeyboardInputSource();
    Q_ASSERT(inputSource != 0);
    CFDataRef data = static_cast<CFDataRef>(TISGetInputSourceProperty(inputSource,
                                                                 kTISPropertyUnicodeKeyLayoutData));
    uchrData = data ? reinterpret_cast<const UCKeyboardLayout *>(CFDataGetBytePtr(data)) : 0;
#endif
    *qtKey = Qt::Key_unknown;
    if (uchrData) {
        // The easy stuff; use the unicode stuff!
        UniChar string[4];
        UniCharCount actualLength;
        UInt32 currentModifiers = GetCurrentEventKeyModifiers();
        UInt32 currentModifiersWOAltOrControl = currentModifiers & ~(controlKey | optionKey);
        int keyAction;
        switch (ekind) {
        default:
        case kEventRawKeyDown:
            keyAction = kUCKeyActionDown;
            break;
        case kEventRawKeyUp:
            keyAction = kUCKeyActionUp;
            break;
        case kEventRawKeyRepeat:
            keyAction = kUCKeyActionAutoKey;
            break;
        }
        OSStatus err = UCKeyTranslate(uchrData, keyCode, keyAction,
                                  ((currentModifiersWOAltOrControl >> 8) & 0xff), LMGetKbdType(),
                                  kUCKeyTranslateNoDeadKeysMask, &tmp_unused_state, 4, &actualLength,
                                  string);
        if (err == noErr) {
            *outChar = QChar(string[0]);
            *qtKey = qt_mac_get_key(*outModifiers, *outChar, keyCode);
            if (currentModifiersWOAltOrControl != currentModifiers) {
                // Now get the real char.
                err = UCKeyTranslate(uchrData, keyCode, keyAction,
                                     ((currentModifiers >> 8) & 0xff), LMGetKbdType(),
                                      kUCKeyTranslateNoDeadKeysMask, &tmp_unused_state, 4, &actualLength,
                                      string);
                if (err == noErr)
                    *outChar = QChar(string[0]);
            }
        } else {
            qWarning("Qt::internal::UCKeyTranslate is returnining %ld %s:%d",
                     long(err), __FILE__, __LINE__);
        }
    }
#ifdef Q_OS_MAC32
    else {
        // The road less travelled; use KeyTranslate
        const void *keyboard_layout;
        KeyboardLayoutRef keyLayoutRef = 0;
        KLGetCurrentKeyboardLayout(&keyLayoutRef);
        err = KLGetKeyboardLayoutProperty(keyLayoutRef, kKLKCHRData,
                                  reinterpret_cast<const void **>(&keyboard_layout));

        int translatedChar = KeyTranslate(keyboard_layout, (GetCurrentEventKeyModifiers() &
                                                             (kEventKeyModifierNumLockMask|shiftKey|cmdKey|
                                                              rightShiftKey|alphaLock)) | keyCode,
                                           &tmp_unused_state);
        if (!translatedChar) {
#ifdef QT_MAC_USE_COCOA
            if (outHandled) {
                qt_mac_eat_unicode_key = false;
                if (er)
                    CallNextEventHandler(er, keyEvent);
                *outHandled = qt_mac_eat_unicode_key;
            }
#endif
            return false;
        }

        //map it into qt keys
        *qtKey = qt_mac_get_key(*outModifiers, QChar(translatedChar), keyCode);
        if (*outModifiers & (Qt::AltModifier | Qt::ControlModifier)) {
            if (translatedChar & (1 << 7)) //high ascii
                translatedChar = 0;
        } else {          //now get the real ascii value
            UInt32 tmp_mod = 0L;
            static UInt32 tmp_state = 0L;
            if (*outModifiers & Qt::ShiftModifier)
                tmp_mod |= shiftKey;
            if (*outModifiers & Qt::MetaModifier)
                tmp_mod |= controlKey;
            if (*outModifiers & Qt::ControlModifier)
                tmp_mod |= cmdKey;
            if (GetCurrentEventKeyModifiers() & alphaLock) //no Qt mapper
                tmp_mod |= alphaLock;
            if (*outModifiers & Qt::AltModifier)
                tmp_mod |= optionKey;
            if (*outModifiers & Qt::KeypadModifier)
                tmp_mod |= kEventKeyModifierNumLockMask;
            translatedChar = KeyTranslate(keyboard_layout, tmp_mod | keyCode, &tmp_state);
        }
        {
            ByteCount unilen = 0;
            if (GetEventParameter(keyEvent, kEventParamKeyUnicodes, typeUnicodeText, 0, 0, &unilen, 0)
                    == noErr && unilen == 2) {
                GetEventParameter(keyEvent, kEventParamKeyUnicodes, typeUnicodeText, 0, unilen, 0, outChar);
            } else if (translatedChar) {
                static QTextCodec *c = 0;
                if (!c)
                    c = QTextCodec::codecForName("Apple Roman");
		char tmpChar = (char)translatedChar; // **sigh**
                *outChar = c->toUnicode(&tmpChar, 1).at(0);
            } else {
                *qtKey = qt_mac_get_key(*outModifiers, QChar(translatedChar), keyCode);
            }
        }
    }
#endif
    if (*qtKey == Qt::Key_unknown)
        *qtKey = qt_mac_get_key(*outModifiers, *outChar, keyCode);
    return true;
}

QKeyMapperPrivate::QKeyMapperPrivate()
{
    memset(keyLayout, 0, sizeof(keyLayout));
    keyboard_layout_format.unicode = 0;
#ifdef Q_OS_MAC32
    keyboard_mode = NullMode;
#else
    currentInputSource = 0;
#endif
}

QKeyMapperPrivate::~QKeyMapperPrivate()
{
    deleteLayouts();
}

bool
QKeyMapperPrivate::updateKeyboard()
{
    const UCKeyboardLayout *uchrData = 0;
#ifdef Q_OS_MAC32
    KeyboardLayoutRef keyLayoutRef = 0;
    KLGetCurrentKeyboardLayout(&keyLayoutRef);

    if (keyboard_mode != NullMode && currentKeyboardLayout == keyLayoutRef)
        return false;

    OSStatus err;
    if (keyLayoutRef != 0) {
        err = KLGetKeyboardLayoutProperty(keyLayoutRef, kKLuchrData,
                                  const_cast<const void **>(reinterpret_cast<const void **>(&uchrData)));
        if (err != noErr) {
            qWarning("Qt::internal::unable to get unicode keyboardlayout %ld %s:%d",
                     long(err), __FILE__, __LINE__);
        }
    }
#else
    QCFType<TISInputSourceRef> source = TISCopyCurrentKeyboardInputSource();
    if (keyboard_mode != NullMode && source == currentInputSource) {
        return false;
    }
    Q_ASSERT(source != 0);
    CFDataRef data = static_cast<CFDataRef>(TISGetInputSourceProperty(source,
                                                                 kTISPropertyUnicodeKeyLayoutData));
    uchrData = data ? reinterpret_cast<const UCKeyboardLayout *>(CFDataGetBytePtr(data)) : 0;
#endif

    keyboard_kind = LMGetKbdType();
    if (uchrData) {
        keyboard_layout_format.unicode = uchrData;
        keyboard_mode = UnicodeMode;
    }
#ifdef Q_OS_MAC32
    else {
        void *happy;
        err = KLGetKeyboardLayoutProperty(keyLayoutRef, kKLKCHRData,
                                  const_cast<const void **>(reinterpret_cast<void **>(&happy)));
        if (err != noErr) {
            qFatal("Qt::internal::unable to get non-unicode layout, cannot procede %ld %s:%d",
                     long(err), __FILE__, __LINE__);
        }
        keyboard_layout_format.other = happy;
        keyboard_mode = OtherMode;
    }

    currentKeyboardLayout = keyLayoutRef;
#else
    currentInputSource = source;
#endif
    keyboard_dead = 0;
    CFStringRef iso639Code;
#ifdef Q_OS_MAC32
# ifndef kKLLanguageCode
# define kKLLanguageCode 9
# endif
    KLGetKeyboardLayoutProperty(currentKeyboardLayout, kKLLanguageCode,
                                reinterpret_cast<const void **>(&iso639Code));
#else
    CFArrayRef array = static_cast<CFArrayRef>(TISGetInputSourceProperty(currentInputSource, kTISPropertyInputSourceLanguages));
    iso639Code = static_cast<CFStringRef>(CFArrayGetValueAtIndex(array, 0)); // Actually a RFC3066bis, but it's close enough
#endif
    if (iso639Code) {
        keyboardInputLocale = QLocale(QCFString::toQString(iso639Code));
        keyboardInputDirection = keyboardInputLocale.textDirection();
    } else {
        keyboardInputLocale = QLocale::c();
        keyboardInputDirection = Qt::LeftToRight;
    }
    return true;
}

void
QKeyMapperPrivate::deleteLayouts()
{
    keyboard_mode = NullMode;
    for (int i = 0; i < 255; ++i) {
        if (keyLayout[i]) {
            delete keyLayout[i];
            keyLayout[i] = 0;
        }
    }
}

void
QKeyMapperPrivate::clearMappings()
{
    deleteLayouts();
    updateKeyboard();
}

QList<int>
QKeyMapperPrivate::possibleKeys(QKeyEvent *e)
{
    QList<int> ret;

    KeyboardLayoutItem *kbItem = keyLayout[e->nativeVirtualKey()];
    if (!kbItem) // Key is not in any keyboard layout (e.g. eisu-key on Japanese keyboard) 
        return ret;

    int baseKey = kbItem->qtKey[0];
    Qt::KeyboardModifiers keyMods = e->modifiers();
    ret << int(baseKey + keyMods); // The base key is _always_ valid, of course

    for (int i = 1; i < 8; ++i) {
        Qt::KeyboardModifiers neededMods = ModsTbl[i];
        int key = kbItem->qtKey[i];
        if (key && key != baseKey && ((keyMods & neededMods) == neededMods))
            ret << int(key + (keyMods & ~neededMods));
    }

    return ret;
}

bool QKeyMapperPrivate::translateKeyEvent(QWidget *widget, EventHandlerCallRef er, EventRef event,
                                          void *info, bool grab)
{
    Q_ASSERT(GetEventClass(event) == kEventClassKeyboard);
    bool handled_event=true;
    UInt32 ekind = GetEventKind(event);

    // unfortunately modifiers changed event looks quite different, so I have a separate
    // code path
    if (ekind == kEventRawKeyModifiersChanged) {
        //figure out changed modifiers, wish Apple would just send a delta
        UInt32 modifiers = 0;
        GetEventParameter(event, kEventParamKeyModifiers, typeUInt32, 0,
                          sizeof(modifiers), 0, &modifiers);
        qt_mac_send_modifiers_changed(modifiers, widget);
        return true;
    }

    QInputContext *currentContext = qApp->inputContext();
    if (currentContext && currentContext->isComposing()) {
        if (ekind == kEventRawKeyDown) {
            QMacInputContext *context = qobject_cast<QMacInputContext*>(currentContext);
            if (context)
                context->setLastKeydownEvent(event);
        }
        return false;
    }
    // Once we process the key down , we don't need to send the saved event again from
    // kEventTextInputUnicodeForKeyEvent, so clear it.
    if (currentContext && ekind == kEventRawKeyDown) {
        QMacInputContext *context = qobject_cast<QMacInputContext*>(currentContext);
        if (context)
            context->setLastKeydownEvent(0);
    }

    //get modifiers
    Qt::KeyboardModifiers modifiers;
    int qtKey;
    QChar ourChar;
    if (translateKeyEventInternal(er, event, &qtKey, &ourChar, &modifiers,
                                  &handled_event) == false)
        return handled_event;
    QString text(ourChar);
    /* This is actually wrong - but unfortunately it is the best that can be
       done for now because of the Control/Meta mapping problems */
    if (modifiers & (Qt::ControlModifier | Qt::MetaModifier)
        && !qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta)) {
        text = QString();
    }


    if (widget) {
#ifndef QT_MAC_USE_COCOA
        Q_UNUSED(info);
        // Try not to call "other" event handlers if we have a popup,
        // However, if the key has text
        // then we should pass it along because otherwise then people
        // can use input method stuff.
        if (!qApp->activePopupWidget()
                || (qApp->activePopupWidget() && !text.isEmpty())) {
            //Find out if someone else wants the event, namely
            //is it of use to text services? If so we won't bother
            //with a QKeyEvent.
            qt_mac_eat_unicode_key = false;
            if (er)
                CallNextEventHandler(er, event);
            extern bool qt_mac_menubar_is_open();   
            if (qt_mac_eat_unicode_key || qt_mac_menubar_is_open()) {
                return true;
            }
        }
#endif
        // Try to compress key events.
        if (!text.isEmpty() && widget->testAttribute(Qt::WA_KeyCompression)) {
            EventTime lastTime = GetEventTime(event);
            for (;;) {
                EventRef releaseEvent = FindSpecificEventInQueue(GetMainEventQueue(),
                                                                 qt_KeyEventComparatorProc,
                                                                 (void*)kEventRawKeyUp);
                if (!releaseEvent)
                    break;
                const EventTime releaseTime = GetEventTime(releaseEvent);
                if (releaseTime < lastTime)
                    break;
                lastTime = releaseTime;

                EventRef pressEvent = FindSpecificEventInQueue(GetMainEventQueue(),
                                                               qt_KeyEventComparatorProc,
                                                               (void*)kEventRawKeyDown);
                if (!pressEvent)
                    break;
                const EventTime pressTime = GetEventTime(pressEvent);
                if (pressTime < lastTime)
                    break;
                lastTime = pressTime;

                Qt::KeyboardModifiers compressMod;
                int compressQtKey = 0;
                QChar compressChar;
                if (translateKeyEventInternal(er, pressEvent,
                                              &compressQtKey, &compressChar, &compressMod, 0)
                    == false) {
                    break;
                }
                // Copied from qapplication_x11.cpp (change both).

                bool stopCompression =
                    // 1) misc keys
                    (compressQtKey >= Qt::Key_Escape && compressQtKey <= Qt::Key_SysReq)
                    // 2) cursor movement
                    || (compressQtKey >= Qt::Key_Home && compressQtKey <= Qt::Key_PageDown)
                    // 3) extra keys
                    || (compressQtKey >= Qt::Key_Super_L && compressQtKey <= Qt::Key_Direction_R)
                    // 4) something that a) doesn't translate to text or b) translates
                    //    to newline text
                    || (compressQtKey == 0)
                    || (compressChar == QLatin1Char('\n'))
                    || (compressQtKey == Qt::Key_unknown);

                if (compressMod == modifiers && !compressChar.isNull() && !stopCompression) {
#ifdef DEBUG_KEY_BINDINGS
                    qDebug("compressing away %c", compressChar.toLatin1());
#endif
                    text += compressChar;
                    // Clean up
                    RemoveEventFromQueue(GetMainEventQueue(), releaseEvent);
                    RemoveEventFromQueue(GetMainEventQueue(), pressEvent);
                } else {
#ifdef DEBUG_KEY_BINDINGS
                    qDebug("stoping compression..");
#endif
                    break;
                }
            }
        }

        // There is no way to get the scan code from carbon. But we cannot use the value 0, since
        // it indicates that the event originates from somewhere else than the keyboard
        UInt32 macScanCode = 1;
        UInt32 macVirtualKey = 0;
        GetEventParameter(event, kEventParamKeyCode, typeUInt32, 0, sizeof(macVirtualKey), 0, &macVirtualKey);
        UInt32 macModifiers = 0;
        GetEventParameter(event, kEventParamKeyModifiers, typeUInt32, 0,
                          sizeof(macModifiers), 0, &macModifiers);
#ifdef QT_MAC_USE_COCOA
        // The unicode characters in the range 0xF700-0xF747 are reserved
        // by Mac OS X for transient use as keyboard function keys. We
        // wont send 'text' for such key events. This is done to match
        // behavior on other platforms.
        unsigned int *unicodeKey = (unsigned int*)info;
        if (*unicodeKey >= 0xf700 && *unicodeKey <= 0xf747)
            text = QString();
        bool isAccepted;
#endif
        handled_event = QKeyMapper::sendKeyEvent(widget, grab,
                                                 (ekind == kEventRawKeyUp) ? QEvent::KeyRelease : QEvent::KeyPress,
                                                 qtKey, modifiers, text, ekind == kEventRawKeyRepeat, 0,
                                                 macScanCode, macVirtualKey, macModifiers
#ifdef QT_MAC_USE_COCOA
                                                 ,&isAccepted
#endif
                                                 );
#ifdef QT_MAC_USE_COCOA
        *unicodeKey = (unsigned int)isAccepted;
#endif
    }
    return handled_event;
}

void
QKeyMapperPrivate::updateKeyMap(EventHandlerCallRef, EventRef event, void *
#if defined(QT_MAC_USE_COCOA)
                                unicodeKey // unicode character from NSEvent (modifiers applied)
#endif
                                )
{
    UInt32 macVirtualKey = 0;
    GetEventParameter(event, kEventParamKeyCode, typeUInt32, 0, sizeof(macVirtualKey), 0, &macVirtualKey);
    if (updateKeyboard())
       QKeyMapper::changeKeyboard();
    else if (keyLayout[macVirtualKey])
        return;

    UniCharCount buffer_size = 10;
    UniChar buffer[buffer_size];
    keyLayout[macVirtualKey] = new KeyboardLayoutItem;
    for (int i = 0; i < 16; ++i) {
        UniCharCount out_buffer_size = 0;
        keyLayout[macVirtualKey]->qtKey[i] = 0;
#ifdef Q_WS_MAC32
        if (keyboard_mode == UnicodeMode) {
#endif
            const UInt32 keyModifier = ((qt_mac_get_mac_modifiers(ModsTbl[i]) >> 8) & 0xFF);
            OSStatus err = UCKeyTranslate(keyboard_layout_format.unicode, macVirtualKey, kUCKeyActionDown, keyModifier,
                                          keyboard_kind, 0, &keyboard_dead, buffer_size, &out_buffer_size, buffer);
            if (err == noErr && out_buffer_size) {
                const QChar unicode(buffer[0]);
                int qtkey = qt_mac_get_key(keyModifier, unicode, macVirtualKey);
                if (qtkey == Qt::Key_unknown)
                    qtkey = unicode.unicode();
                keyLayout[macVirtualKey]->qtKey[i] = qtkey;
            }
#ifndef Q_WS_MAC32
            else {
                const QChar unicode(*((UniChar *)unicodeKey));
                int qtkey = qt_mac_get_key(keyModifier, unicode, macVirtualKey);
                if (qtkey == Qt::Key_unknown)
                    qtkey = unicode.unicode();
                keyLayout[macVirtualKey]->qtKey[i] = qtkey;
            }
#endif
#ifdef Q_WS_MAC32            
        } else {
            const UInt32 keyModifier = (qt_mac_get_mac_modifiers(ModsTbl[i]));

            uchar translatedChar = KeyTranslate(keyboard_layout_format.other, keyModifier | macVirtualKey, &keyboard_dead);
            if (translatedChar) {
                static QTextCodec *c = 0;
                if (!c)
                    c = QTextCodec::codecForName("Apple Roman");
                const QChar unicode(c->toUnicode((const char *)&translatedChar, 1).at(0));
                int qtkey = qt_mac_get_key(keyModifier, unicode, macVirtualKey);
                if (qtkey == Qt::Key_unknown)
                    qtkey = unicode.unicode();
                keyLayout[macVirtualKey]->qtKey[i] = qtkey;
            }
        }
#endif
    }
#ifdef DEBUG_KEY_MAPS
    qDebug("updateKeyMap for virtual key = 0x%02x!", (uint)macVirtualKey);
    for (int i = 0; i < 16; ++i) {
        qDebug("    [%d] (%d,0x%02x,'%c')", i,
               keyLayout[macVirtualKey]->qtKey[i],
               keyLayout[macVirtualKey]->qtKey[i],
               keyLayout[macVirtualKey]->qtKey[i]);
    }
#endif
}

bool
QKeyMapper::sendKeyEvent(QWidget *widget, bool grab,
                         QEvent::Type type, int code, Qt::KeyboardModifiers modifiers,
                         const QString &text, bool autorepeat, int count,
                         quint32 nativeScanCode, quint32 nativeVirtualKey,
                         quint32 nativeModifiers, bool *isAccepted)
{
    Q_UNUSED(count);
    if (widget && widget->isEnabled()) {
        bool key_event = true;
#if defined(QT3_SUPPORT) && !defined(QT_NO_SHORTCUT)
        if (type == QEvent::KeyPress && !grab
           && QApplicationPrivate::instance()->use_compat()) {
               QKeyEventEx accel_ev(type, code, modifiers,
                                    text, autorepeat, qMax(1, int(text.length())),
                                    nativeScanCode, nativeVirtualKey, nativeModifiers);
            if (QApplicationPrivate::instance()->qt_tryAccelEvent(widget, &accel_ev)) {
#if defined(DEBUG_KEY_BINDINGS) || defined(DEBUG_KEY_BINDINGS_MODIFIERS)
                qDebug("KeyEvent: %s::%s consumed Accel: %s",
                       widget ? widget->metaObject()->className() : "none",
                       widget ? widget->objectName().toLatin1().constData() : "",
                       text.toLatin1().constData());
#endif
                key_event = false;
            } else {
                if (accel_ev.isAccepted()) {
#if defined(DEBUG_KEY_BINDINGS) || defined(DEBUG_KEY_BINDINGS_MODIFIERS)
                    qDebug("KeyEvent: %s::%s overrode Accel: %s",
                           widget ? widget->metaObject()->className() : "none",
                           widget ? widget->objectName().toLatin1().constData() : "",
                           text.toLatin1().constData());
#endif
                }
            }
        }
#else
Q_UNUSED(grab);
#endif // QT3_SUPPORT && !QT_NO_SHORTCUT
        if (key_event) {
#if defined(DEBUG_KEY_BINDINGS) || defined(DEBUG_KEY_BINDINGS_MODIFIERS)
            qDebug("KeyEvent: Sending %s to %s::%s: %s 0x%08x%s",
                   type == QEvent::KeyRelease ? "KeyRelease" : "KeyPress",
                   widget ? widget->metaObject()->className() : "none",
                   widget ? widget->objectName().toLatin1().constData() : "",
                   text.toLatin1().constData(), int(modifiers),
                   autorepeat ? " Repeat" : "");
#endif
            QKeyEventEx ke(type, code, modifiers, text, autorepeat, qMax(1, text.length()),
                           nativeScanCode, nativeVirtualKey, nativeModifiers);
            bool retMe = qt_sendSpontaneousEvent(widget,&ke);
            if (isAccepted)
                *isAccepted = ke.isAccepted();
            return retMe;
        }
    }
    return false;
}

QT_END_NAMESPACE
