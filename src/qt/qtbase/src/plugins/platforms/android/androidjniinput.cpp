/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
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

#include "androidjniinput.h"
#include "androidjnimain.h"
#include "qandroidplatformintegration.h"

#include <qpa/qwindowsysteminterface.h>
#include <QTouchEvent>
#include <QPointer>

#include <QDebug>

using namespace QtAndroid;

namespace QtAndroidInput
{
    static jmethodID m_showSoftwareKeyboardMethodID = 0;
    static jmethodID m_resetSoftwareKeyboardMethodID = 0;
    static jmethodID m_hideSoftwareKeyboardMethodID = 0;
    static jmethodID m_isSoftwareKeyboardVisibleMethodID = 0;
    static jmethodID m_updateSelectionMethodID = 0;

    static bool m_ignoreMouseEvents = false;

    static QList<QWindowSystemInterface::TouchPoint> m_touchPoints;

    static QPointer<QWindow> m_mouseGrabber;

    void updateSelection(int selStart, int selEnd, int candidatesStart, int candidatesEnd)
    {
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
        qDebug() << ">>> UPDATESELECTION" << selStart << selEnd << candidatesStart << candidatesEnd;
#endif
        env.jniEnv->CallStaticVoidMethod(applicationClass(), m_updateSelectionMethodID,
                                         selStart, selEnd, candidatesStart, candidatesEnd);
    }

    void showSoftwareKeyboard(int left, int top, int width, int height, int inputHints)
    {
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return;

        env.jniEnv->CallStaticVoidMethod(applicationClass(),
                                         m_showSoftwareKeyboardMethodID,
                                         left,
                                         top,
                                         width,
                                         height,
                                         inputHints);
#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
        qDebug() << "@@@ SHOWSOFTWAREKEYBOARD" << left << top << width << height << inputHints;
#endif
    }

    void resetSoftwareKeyboard()
    {
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return;

        env.jniEnv->CallStaticVoidMethod(applicationClass(), m_resetSoftwareKeyboardMethodID);
#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
        qDebug() << "@@@ RESETSOFTWAREKEYBOARD";
#endif
    }

    void hideSoftwareKeyboard()
    {
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return;

        env.jniEnv->CallStaticVoidMethod(applicationClass(), m_hideSoftwareKeyboardMethodID);
#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
        qDebug() << "@@@ HIDESOFTWAREKEYBOARD";
#endif
    }

    bool isSoftwareKeyboardVisible()
    {
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return false;

        bool visibility = env.jniEnv->CallStaticBooleanMethod(applicationClass(), m_isSoftwareKeyboardVisibleMethodID);
#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
        qDebug() << "@@@ ISSOFTWAREKEYBOARDVISIBLE" << visibility;
#endif
        return visibility;
    }


    static void mouseDown(JNIEnv */*env*/, jobject /*thiz*/, jint /*winId*/, jint x, jint y)
    {
        if (m_ignoreMouseEvents)
            return;

        QPoint globalPos(x,y);
        QWindow *tlw = topLevelWindowAt(globalPos);
        m_mouseGrabber = tlw;
        QPoint localPos = tlw ? (globalPos - tlw->position()) : globalPos;
        QWindowSystemInterface::handleMouseEvent(tlw,
                                                 localPos,
                                                 globalPos,
                                                 Qt::MouseButtons(Qt::LeftButton));
    }

    static void mouseUp(JNIEnv */*env*/, jobject /*thiz*/, jint /*winId*/, jint x, jint y)
    {
        QPoint globalPos(x,y);
        QWindow *tlw = m_mouseGrabber.data();
        if (!tlw)
            tlw = topLevelWindowAt(globalPos);
        QPoint localPos = tlw ? (globalPos -tlw->position()) : globalPos;
        QWindowSystemInterface::handleMouseEvent(tlw, localPos, globalPos
                                                , Qt::MouseButtons(Qt::NoButton));
        m_ignoreMouseEvents = false;
        m_mouseGrabber = 0;
    }

    static void mouseMove(JNIEnv */*env*/, jobject /*thiz*/, jint /*winId*/, jint x, jint y)
    {

        if (m_ignoreMouseEvents)
            return;

        QPoint globalPos(x,y);
        QWindow *tlw = m_mouseGrabber.data();
        if (!tlw)
            tlw = topLevelWindowAt(globalPos);
        QPoint localPos = tlw ? (globalPos-tlw->position()) : globalPos;
        QWindowSystemInterface::handleMouseEvent(tlw,
                                                 localPos,
                                                 globalPos,
                                                 Qt::MouseButtons(Qt::LeftButton));
    }

    static void longPress(JNIEnv */*env*/, jobject /*thiz*/, jint /*winId*/, jint x, jint y)
    {
        //### TODO: add proper API for Qt 5.2
        static bool rightMouseFromLongPress = qgetenv("QT_NECESSITAS_COMPATIBILITY_LONG_PRESS").toInt();
        if (!rightMouseFromLongPress)
            return;
        m_ignoreMouseEvents = true;
        QPoint globalPos(x,y);
        QWindow *tlw = topLevelWindowAt(globalPos);
        QPoint localPos = tlw ? (globalPos-tlw->position()) : globalPos;

        // Release left button
        QWindowSystemInterface::handleMouseEvent(tlw,
                                                 localPos,
                                                 globalPos,
                                                 Qt::MouseButtons(Qt::NoButton));

        // Press right button
        QWindowSystemInterface::handleMouseEvent(tlw,
                                                 localPos,
                                                 globalPos,
                                                 Qt::MouseButtons(Qt::RightButton));
    }

    static void touchBegin(JNIEnv */*env*/, jobject /*thiz*/, jint /*winId*/)
    {
        m_touchPoints.clear();
    }

    static void touchAdd(JNIEnv */*env*/, jobject /*thiz*/, jint /*winId*/, jint id, jint action, jboolean /*primary*/, jint x, jint y, jfloat size, jfloat pressure)
    {
        Qt::TouchPointState state = Qt::TouchPointStationary;
        switch (action) {
        case 0:
            state = Qt::TouchPointPressed;
            break;
        case 1:
            state = Qt::TouchPointMoved;
            break;
        case 2:
            state = Qt::TouchPointStationary;
            break;
        case 3:
            state = Qt::TouchPointReleased;
            break;
        }

        const int dw = desktopWidthPixels();
        const int dh = desktopHeightPixels();
        QWindowSystemInterface::TouchPoint touchPoint;
        touchPoint.id = id;
        touchPoint.pressure = pressure;
        touchPoint.normalPosition = QPointF(double(x / dw), double(y / dh));
        touchPoint.state = state;
        touchPoint.area = QRectF(x - double(dw*size) / 2.0,
                                 y - double(dh*size) / 2.0,
                                 double(dw*size),
                                 double(dh*size));
        m_touchPoints.push_back(touchPoint);
    }

    static void touchEnd(JNIEnv */*env*/, jobject /*thiz*/, jint /*winId*/, jint /*action*/)
    {
        if (m_touchPoints.isEmpty())
            return;

        QAndroidPlatformIntegration *platformIntegration = QtAndroid::androidPlatformIntegration();
        if (!platformIntegration)
            return;

        QTouchDevice *touchDevice = platformIntegration->touchDevice();
        if (touchDevice == 0) {
            touchDevice = new QTouchDevice;
            touchDevice->setType(QTouchDevice::TouchScreen);
            touchDevice->setCapabilities(QTouchDevice::Position
                                         | QTouchDevice::Area
                                         | QTouchDevice::Pressure
                                         | QTouchDevice::NormalizedPosition);
            QWindowSystemInterface::registerTouchDevice(touchDevice);
            platformIntegration->setTouchDevice(touchDevice);
        }

        QWindow *window = QtAndroid::topLevelWindowAt(m_touchPoints.at(0).area.center().toPoint());
        QWindowSystemInterface::handleTouchEvent(window, touchDevice, m_touchPoints);
    }

    static int mapAndroidKey(int key)
    {
        // 0--9        0x00000007 -- 0x00000010
        if (key >= 0x00000007 && key <= 0x00000010)
            return Qt::Key_0 + key - 0x00000007;

        // A--Z        0x0000001d -- 0x00000036
        if (key >= 0x0000001d && key <= 0x00000036)
            return Qt::Key_A + key - 0x0000001d;

        // F1--F12     0x00000083 -- 0x0000008e
        if (key >= 0x00000083 && key <= 0x0000008e)
            return Qt::Key_F1 + key - 0x00000083;

        // NUMPAD_0--NUMPAD_9     0x00000090 -- 0x00000099
        if (key >= 0x00000090 && key <= 0x00000099)
            return Qt::KeypadModifier + Qt::Key_0 + key - 0x00000090;

        // BUTTON_1--KEYCODE_BUTTON_16 0x000000bc -- 0x000000cb

        switch (key) {
        case 0x00000000: // KEYCODE_UNKNOWN
            return Qt::Key_unknown;

        case 0x00000001: // KEYCODE_SOFT_LEFT
            return Qt::Key_Left;

        case 0x00000002: // KEYCODE_SOFT_RIGHT
            return Qt::Key_Right;

        // 0x00000003: // KEYCODE_HOME is never delivered to applications.

        case 0x00000004: // KEYCODE_BACK
            return Qt::Key_Back;

        case 0x00000005: // KEYCODE_CALL
            return Qt::Key_Call;

        case 0x00000006: // KEYCODE_ENDCALL
            return Qt::Key_Hangup;

       // 0--9        0x00000007 -- 0x00000010

        case 0x00000011: // KEYCODE_STAR
            return Qt::Key_Asterisk;

        case 0x00000012: // KEYCODE_POUND
            return Qt::Key_NumberSign;

        case 0x00000013: //KEYCODE_DPAD_UP
            return Qt::Key_Up;

        case 0x00000014: // KEYCODE_DPAD_DOWN
            return Qt::Key_Down;

        case 0x00000015: //KEYCODE_DPAD_LEFT
            return Qt::Key_Left;

        case 0x00000016: //KEYCODE_DPAD_RIGHT
            return Qt::Key_Right;

        case 0x00000017: // KEYCODE_DPAD_CENTER
            return Qt::Key_Enter;

        case 0x00000018: // KEYCODE_VOLUME_UP
            return Qt::Key_VolumeUp;

        case 0x00000019: // KEYCODE_VOLUME_DOWN
            return Qt::Key_VolumeDown;

        case 0x0000001a:
            return Qt::Key_PowerOff;

        case 0x0000001b: // KEYCODE_CAMERA
            return Qt::Key_Camera;

        case 0x0000001c: // KEYCODE_CLEAR
            return Qt::Key_Clear;

        // A--Z        0x0000001d -- 0x00000036

        case 0x00000037: // KEYCODE_COMMA
            return Qt::Key_Comma;

        case 0x00000038: // KEYCODE_PERIOD
            return Qt::Key_Period;

        case 0x00000039: // KEYCODE_ALT_LEFT
        case 0x0000003a: // KEYCODE_ALT_RIGHT
            return Qt::Key_Alt;

        case 0x0000003b: // KEYCODE_SHIFT_LEFT
        case 0x0000003c: // KEYCODE_SHIFT_RIGHT
            return Qt::Key_Shift;

        case 0x0000003d: // KEYCODE_TAB
            return Qt::Key_Tab;

        case 0x0000003e: // KEYCODE_SPACE
            return Qt::Key_Space;

        case 0x0000003f: // KEYCODE_SYM
            return Qt::Key_Meta;

        case 0x00000040: // KEYCODE_EXPLORER
            return Qt::Key_Explorer;

        case 0x00000041: //KEYCODE_ENVELOPE
            return Qt::Key_LaunchMail;

        case 0x00000042: // KEYCODE_ENTER
            return Qt::Key_Return;

        case 0x00000043: // KEYCODE_DEL
            return Qt::Key_Backspace;

        case 0x00000044: // KEYCODE_GRAVE
            return Qt::Key_QuoteLeft;

        case 0x00000045: // KEYCODE_MINUS
            return Qt::Key_Minus;

        case 0x00000046: // KEYCODE_EQUALS
            return Qt::Key_Equal;

        case 0x00000047: // KEYCODE_LEFT_BRACKET
            return Qt::Key_BracketLeft;

        case 0x00000048: // KEYCODE_RIGHT_BRACKET
            return Qt::Key_BracketRight;

        case 0x00000049: // KEYCODE_BACKSLASH
            return Qt::Key_Backslash;

        case 0x0000004a: // KEYCODE_SEMICOLON
            return Qt::Key_Semicolon;

        case 0x0000004b: // KEYCODE_APOSTROPHE
            return Qt::Key_Apostrophe;

        case 0x0000004c: // KEYCODE_SLASH
            return Qt::Key_Slash;

        case 0x0000004d: // KEYCODE_AT
            return Qt::Key_At;

        case 0x0000004e: // KEYCODE_NUM
            return Qt::Key_Alt;

        case 0x0000004f: // KEYCODE_HEADSETHOOK
            return 0;

        case 0x00000050: // KEYCODE_FOCUS
            return Qt::Key_CameraFocus;

        case 0x00000051: // KEYCODE_PLUS
            return Qt::Key_Plus;

        case 0x00000052: // KEYCODE_MENU
            return Qt::Key_Menu;

        case 0x00000053: // KEYCODE_NOTIFICATION
            return 0;

        case 0x00000054: // KEYCODE_SEARCH
            return Qt::Key_Search;

        case 0x00000055: // KEYCODE_MEDIA_PLAY_PAUSE
            return Qt::Key_MediaPlay;

        case 0x00000056: // KEYCODE_MEDIA_STOP
            return Qt::Key_MediaStop;

        case 0x00000057: // KEYCODE_MEDIA_NEXT
            return Qt::Key_MediaNext;

        case 0x00000058: // KEYCODE_MEDIA_PREVIOUS
            return Qt::Key_MediaPrevious;

        case 0x00000059: // KEYCODE_MEDIA_REWIND
            return Qt::Key_AudioRewind;

        case 0x0000005a: // KEYCODE_MEDIA_FAST_FORWARD
            return Qt::Key_AudioForward;

        case 0x0000005b: // KEYCODE_MUTE
            return Qt::Key_MicMute;

        case 0x0000005c: // KEYCODE_PAGE_UP
            return Qt::Key_PageUp;

        case 0x0000005d: // KEYCODE_PAGE_DOWN
            return Qt::Key_PageDown;

        case 0x0000005e: // KEYCODE_PICTSYMBOLS
            return 0;

        case 0x00000060: // KEYCODE_BUTTON_A
        case 0x00000061: // KEYCODE_BUTTON_B
        case 0x00000062: // KEYCODE_BUTTON_B
        case 0x00000063: // KEYCODE_BUTTON_X
        case 0x00000064: // KEYCODE_BUTTON_Y
        case 0x00000065: // KEYCODE_BUTTON_Z
        case 0x00000066: // KEYCODE_BUTTON_L1
        case 0x00000067: // KEYCODE_BUTTON_R1
        case 0x00000068: // KEYCODE_BUTTON_L2
        case 0x00000069: // KEYCODE_BUTTON_R2
        case 0x0000006a: // KEYCODE_BUTTON_THUMBL
        case 0x0000006b: // KEYCODE_BUTTON_THUMBR
        case 0x0000006c: // KEYCODE_BUTTON_START
        case 0x0000006d: // KEYCODE_BUTTON_SELECT
        case 0x0000006e: // KEYCODE_BUTTON_MODE
            return 0;

        case 0x0000006f: // KEYCODE_ESCAPE
            return Qt::Key_Escape;

        case 0x00000070: // KEYCODE_FORWARD_DEL
            return Qt::Key_Delete;

        case 0x00000071: // KEYCODE_CTRL_LEFT
        case 0x00000072: // KEYCODE_CTRL_RIGHT
            return Qt::Key_Control;

        case 0x00000073: // KEYCODE_CAPS_LOCK
            return Qt::Key_CapsLock;

        case 0x00000074: // KEYCODE_SCROLL_LOCK
            return Qt::Key_ScrollLock;

        case 0x00000075: // KEYCODE_META_LEFT
        case 0x00000076: // KEYCODE_META_RIGHT
            return Qt::Key_Meta;

        case 0x00000077: // KEYCODE_FUNCTION
            return 0;

        case 0x00000078: // KEYCODE_SYSRQ
            return Qt::Key_Print;

        case 0x00000079: // KEYCODE_BREAK
            return Qt::Key_Pause;

        case 0x0000007a: // KEYCODE_MOVE_HOME
            return Qt::Key_Home;

        case 0x0000007b: // KEYCODE_MOVE_END
            return Qt::Key_End;

        case 0x0000007c: // KEYCODE_MOVE_INSERT
            return Qt::Key_Insert;

        case 0x0000007d: // KEYCODE_FORWARD
            return Qt::Key_Forward;

        case 0x0000007e: // KEYCODE_MEDIA_PLAY
            return Qt::Key_MediaPlay;

        case 0x0000007f: // KEYCODE_MEDIA_PAUSE
            return Qt::Key_MediaPause;

        case 0x00000080: // KEYCODE_MEDIA_CLOSE
        case 0x00000081: // KEYCODE_MEDIA_EJECT
            return Qt::Key_Eject;

        case 0x00000082: // KEYCODE_MEDIA_RECORD
            return Qt::Key_MediaRecord;

        // F1--F12     0x00000083 -- 0x0000008e

        case 0x0000008f: // KEYCODE_NUM_LOCK
            return Qt::Key_NumLock;

        // NUMPAD_0--NUMPAD_9     0x00000090 -- 0x00000099

        case 0x0000009a: // KEYCODE_NUMPAD_DIVIDE
            return Qt::KeypadModifier + Qt::Key_Slash;

        case 0x0000009b: // KEYCODE_NUMPAD_MULTIPLY
            return Qt::KeypadModifier + Qt::Key_Asterisk;

        case 0x0000009c: // KEYCODE_NUMPAD_SUBTRACT
            return Qt::KeypadModifier + Qt::Key_Minus;

        case 0x0000009d: // KEYCODE_NUMPAD_ADD
            return Qt::KeypadModifier + Qt::Key_Plus;

        case 0x0000009e: // KEYCODE_NUMPAD_DOT
            return Qt::KeypadModifier + Qt::Key_Period;

        case 0x0000009f: // KEYCODE_NUMPAD_COMMA
            return Qt::KeypadModifier + Qt::Key_Comma;

        case 0x000000a0: // KEYCODE_NUMPAD_ENTER
            return Qt::Key_Enter;

        case 0x000000a1: // KEYCODE_NUMPAD_EQUALS
            return Qt::KeypadModifier + Qt::Key_Equal;

        case 0x000000a2: // KEYCODE_NUMPAD_LEFT_PAREN
            return Qt::Key_ParenLeft;

        case 0x000000a3: // KEYCODE_NUMPAD_RIGHT_PAREN
            return Qt::Key_ParenRight;

        case 0x000000a4: // KEYCODE_VOLUME_MUTE
            return Qt::Key_VolumeMute;

        case 0x000000a5: // KEYCODE_INFO
            return Qt::Key_Info;

        case 0x000000a6: // KEYCODE_CHANNEL_UP
            return Qt::Key_ChannelUp;

        case 0x000000a7: // KEYCODE_CHANNEL_DOWN
            return Qt::Key_ChannelDown;

        case 0x000000a8: // KEYCODE_ZOOM_IN
            return Qt::Key_ZoomIn;

        case 0x000000a9: // KEYCODE_ZOOM_OUT
            return Qt::Key_ZoomOut;

        case 0x000000aa: // KEYCODE_TV
        case 0x000000ab: // KEYCODE_WINDOW
            return 0;

        case 0x000000ac: // KEYCODE_GUIDE
            return Qt::Key_Guide;

        case 0x000000ad: // KEYCODE_DVR
            return 0;

        case 0x000000ae: // KEYCODE_BOOKMARK
            return Qt::Key_AddFavorite;

        case 0x000000af: // KEYCODE_CAPTIONS
            return Qt::Key_Subtitle;

        case 0x000000b0: // KEYCODE_SETTINGS
            return Qt::Key_Settings;

        case 0x000000b1: // KEYCODE_TV_POWER
        case 0x000000b2: // KEYCODE_TV_INPUT
        case 0x000000b3: // KEYCODE_STB_POWER
        case 0x000000b4: // KEYCODE_STB_INPUT
        case 0x000000b5: // KEYCODE_AVR_POWER
        case 0x000000b6: // KEYCODE_AVR_INPUT
            return 0;

        case 0x000000b7: // KEYCODE_PROG_RED
            return Qt::Key_Red;

        case 0x000000b8: // KEYCODE_PROG_GREEN
            return Qt::Key_Green;

        case 0x000000b9: // KEYCODE_PROG_YELLOW
            return Qt::Key_Yellow;

        case 0x000000ba: // KEYCODE_PROG_BLUE
            return Qt::Key_Blue;

        // 0x000000bb: // KEYCODE_APP_SWITCH is not sent by the Android O.S.

        // BUTTON_1--KEYCODE_BUTTON_16 0x000000bc -- 0x000000cb

        case 0x000000cc: // KEYCODE_LANGUAGE_SWITCH
        case 0x000000cd: // KEYCODE_MANNER_MODE do we need such a thing?
        case 0x000000ce: // KEYCODE_3D_MODE
        case 0x000000cf: // KEYCODE_CONTACTS
            return 0;

        case 0x000000d0: // KEYCODE_CALENDAR
            return Qt::Key_Calendar;

        case 0x000000d1: // KEYCODE_MUSIC
            return Qt::Key_Music;

        case 0x000000d2: // KEYCODE_CALCULATOR
            return Qt::Key_Calculator;

        // 0x000000d3 -- 0x000000da some japanese specific keys, someone who understand what is about should check !

        // 0x000000db: // KEYCODE_ASSIST  not delivered to applications.

        case 0x000000dc: // KEYCODE_BRIGHTNESS_DOWN
            return Qt::Key_KeyboardBrightnessDown;

        case 0x000000dd: // KEYCODE_BRIGHTNESS_UP
            return Qt::Key_KeyboardBrightnessUp;

        case 0x000000de: // KEYCODE_MEDIA_AUDIO_TRACK
            return Qt::Key_AudioCycleTrack;

        default:
            qWarning() << "Unhandled key code " << key << "!";
            return 0;
        }
    }

    // maps 0 to the empty string, and anything else to a single-character string
    static inline QString toString(jint unicode)
    {
        return unicode ? QString(QChar(unicode)) : QString();
    }

    static void keyDown(JNIEnv */*env*/, jobject /*thiz*/, jint key, jint unicode, jint modifier)
    {
        Qt::KeyboardModifiers modifiers;
        if (modifier & 1)
            modifiers |= Qt::ShiftModifier;

        if (modifier & 2)
            modifiers |= Qt::AltModifier;

        if (modifier & 4)
            modifiers |= Qt::MetaModifier;

        QWindowSystemInterface::handleKeyEvent(0,
                                               QEvent::KeyPress,
                                               mapAndroidKey(key),
                                               modifiers,
                                               toString(unicode),
                                               false);
    }

    static void keyUp(JNIEnv */*env*/, jobject /*thiz*/, jint key, jint unicode, jint modifier)
    {
        Qt::KeyboardModifiers modifiers;
        if (modifier & 1)
            modifiers |= Qt::ShiftModifier;

        if (modifier & 2)
            modifiers |= Qt::AltModifier;

        if (modifier & 4)
            modifiers |= Qt::MetaModifier;

        QWindowSystemInterface::handleKeyEvent(0,
                                               QEvent::KeyRelease,
                                               mapAndroidKey(key),
                                               modifiers,
                                               toString(unicode),
                                               false);
    }

    static void keyboardVisibilityChanged(JNIEnv */*env*/, jobject /*thiz*/, jboolean /*visibility*/)
    {
        QAndroidInputContext *inputContext = QAndroidInputContext::androidInputContext();
        if (inputContext)
            inputContext->emitInputPanelVisibleChanged();
#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
        qDebug() << "@@@ KEYBOARDVISIBILITYCHANGED" << inputContext;
#endif
    }

    static JNINativeMethod methods[] = {
        {"touchBegin","(I)V",(void*)touchBegin},
        {"touchAdd","(IIIZIIFF)V",(void*)touchAdd},
        {"touchEnd","(II)V",(void*)touchEnd},
        {"mouseDown", "(III)V", (void *)mouseDown},
        {"mouseUp", "(III)V", (void *)mouseUp},
        {"mouseMove", "(III)V", (void *)mouseMove},
        {"longPress", "(III)V", (void *)longPress},
        {"keyDown", "(III)V", (void *)keyDown},
        {"keyUp", "(III)V", (void *)keyUp},
        {"keyboardVisibilityChanged", "(Z)V", (void *)keyboardVisibilityChanged}
    };

#define GET_AND_CHECK_STATIC_METHOD(VAR, CLASS, METHOD_NAME, METHOD_SIGNATURE) \
    VAR = env->GetStaticMethodID(CLASS, METHOD_NAME, METHOD_SIGNATURE); \
    if (!VAR) { \
        __android_log_print(ANDROID_LOG_FATAL, qtTagText(), methodErrorMsgFmt(), METHOD_NAME, METHOD_SIGNATURE); \
        return false; \
    }

    bool registerNatives(JNIEnv *env)
    {
        jclass appClass = QtAndroid::applicationClass();

        if (env->RegisterNatives(appClass, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
            __android_log_print(ANDROID_LOG_FATAL,"Qt", "RegisterNatives failed");
            return false;
        }

        GET_AND_CHECK_STATIC_METHOD(m_showSoftwareKeyboardMethodID, appClass, "showSoftwareKeyboard", "(IIIII)V");
        GET_AND_CHECK_STATIC_METHOD(m_resetSoftwareKeyboardMethodID, appClass, "resetSoftwareKeyboard", "()V");
        GET_AND_CHECK_STATIC_METHOD(m_hideSoftwareKeyboardMethodID, appClass, "hideSoftwareKeyboard", "()V");
        GET_AND_CHECK_STATIC_METHOD(m_isSoftwareKeyboardVisibleMethodID, appClass, "isSoftwareKeyboardVisible", "()Z");
        GET_AND_CHECK_STATIC_METHOD(m_updateSelectionMethodID, appClass, "updateSelection", "(IIII)V");
        return true;
    }
}
