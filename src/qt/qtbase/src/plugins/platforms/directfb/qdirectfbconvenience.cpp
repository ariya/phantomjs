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

#include "qdirectfbconvenience.h"
#include "qdirectfbblitter.h"
#include "qdirectfbscreen.h"

#include <private/qpixmap_blitter_p.h>

#include <QtGui/QWindow>
#include <QtGui/QScreen>

QT_BEGIN_NAMESPACE

IDirectFB *QDirectFbConvenience::dfbInterface()
{
    static IDirectFB *dfb = 0;
    if (!dfb) {
        DFBResult result = DirectFBCreate(&dfb);
        if (result != DFB_OK) {
            DirectFBErrorFatal("QDirectFBConvenience: error creating DirectFB interface", result);
            return 0;
        }
    }
    return dfb;
}

IDirectFBDisplayLayer *QDirectFbConvenience::dfbDisplayLayer(int display)
{
    IDirectFBDisplayLayer *layer;
    DFBResult result = QDirectFbConvenience::dfbInterface()->GetDisplayLayer(QDirectFbConvenience::dfbInterface(),display,&layer);
    if (result != DFB_OK)
        DirectFBErrorFatal("QDirectFbConvenience: "
                           "Unable to get primary display layer!", result);
    return layer;
}

QImage::Format QDirectFbConvenience::imageFormatFromSurfaceFormat(const DFBSurfacePixelFormat format, const DFBSurfaceCapabilities caps)
{
    switch (format) {
    case DSPF_LUT8:
        return QImage::Format_Indexed8;
    case DSPF_RGB24:
        return QImage::Format_RGB888;
    case DSPF_ARGB4444:
        return QImage::Format_ARGB4444_Premultiplied;
    case DSPF_RGB444:
        return QImage::Format_RGB444;
    case DSPF_RGB555:
    case DSPF_ARGB1555:
        return QImage::Format_RGB555;
    case DSPF_RGB16:
        return QImage::Format_RGB16;
    case DSPF_ARGB6666:
        return QImage::Format_ARGB6666_Premultiplied;
    case DSPF_RGB18:
        return QImage::Format_RGB666;
    case DSPF_RGB32:
        return QImage::Format_RGB32;
    case DSPF_ARGB: {
            if (caps & DSCAPS_PREMULTIPLIED)
                    return QImage::Format_ARGB32_Premultiplied;
            else return QImage::Format_ARGB32; }
    default:
        break;
    }
    return QImage::Format_Invalid;

}

int QDirectFbConvenience::colorDepthForSurface(const DFBSurfacePixelFormat format)
{
    return ((0x1f << 7) & format) >> 7;
}

/**
 * This is borrowing the reference of the QDirectFbBlitter. You may not store this
 * pointer as a class member but must only use it locally.
 */
IDirectFBSurface *QDirectFbConvenience::dfbSurfaceForPlatformPixmap(QPlatformPixmap *handle)
{
    QBlittablePlatformPixmap *blittablePmData = static_cast<QBlittablePlatformPixmap *>(handle);
    if (blittablePmData) {
        QBlittable *blittable = blittablePmData->blittable();
        QDirectFbBlitter *dfbBlitter = static_cast<QDirectFbBlitter *>(blittable);
        return dfbBlitter->m_surface.data();
    }
    return 0;
}

Qt::MouseButton QDirectFbConvenience::mouseButton(DFBInputDeviceButtonIdentifier identifier)
{
    // The Enum contains values for DIBI_FIRST (= DIBI_LEFT), DIBI_LAST (= 0x1f,) and
    // just 3 enumerated Mouse Buttons. To convert into *ALL* possible Qt::MoueButton values,
    // the parameter is cast as integer.

    switch (int(identifier)) {
    case DIBI_LEFT:                 // value is 0x00
        return Qt::LeftButton;
    case DIBI_RIGHT:                // value is 0x01
        return Qt::RightButton;
    case DIBI_MIDDLE:               // value is 0x02
        return Qt::MidButton;
    case 0x03:
        return Qt::BackButton;
    case 0x04:
        return Qt::ForwardButton;
    case 0x05:
        return Qt::ExtraButton3;
    case 0x06:
        return Qt::ExtraButton4;
    case 0x07:
        return Qt::ExtraButton5;
    case 0x08:
        return Qt::ExtraButton6;
    case 0x09:
        return Qt::ExtraButton7;
    case 0x0a:
        return Qt::ExtraButton8;
    case 0x0b:
        return Qt::ExtraButton9;
    case 0x0c:
        return Qt::ExtraButton10;
    case 0x0d:
        return Qt::ExtraButton11;
    case 0x0e:
        return Qt::ExtraButton12;
    case 0x0f:
        return Qt::ExtraButton13;
    case 0x10:
        return Qt::ExtraButton14;
    case 0x11:
        return Qt::ExtraButton15;
    case 0x12:
        return Qt::ExtraButton16;
    case 0x13:
        return Qt::ExtraButton17;
    case 0x14:
        return Qt::ExtraButton18;
    case 0x15:
        return Qt::ExtraButton19;
    case 0x16:
        return Qt::ExtraButton20;
    case 0x17:
        return Qt::ExtraButton21;
    case 0x18:
        return Qt::ExtraButton22;
    case 0x19:
        return Qt::ExtraButton23;
    case 0x1a:
        return Qt::ExtraButton24;
    default:
        return Qt::NoButton;
    }
}

Qt::MouseButtons QDirectFbConvenience::mouseButtons(DFBInputDeviceButtonMask mask)
{
    Qt::MouseButtons buttons = Qt::NoButton;

    if (mask & DIBM_LEFT) {
        buttons |= Qt::LeftButton;
    }
    if (mask & DIBM_MIDDLE) {
        buttons |= Qt::MidButton;
    }
    if (mask & DIBM_RIGHT) {
        buttons |= Qt::RightButton;
    }
    return buttons;
}

Qt::KeyboardModifiers QDirectFbConvenience::keyboardModifiers(DFBInputDeviceModifierMask mask)
{
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;

    if (mask & DIMM_SHIFT) {
        modifiers |= Qt::ShiftModifier;
    }
    if (mask & DIMM_ALT) {
        modifiers |= Qt::AltModifier;
    }
    if (mask & DIMM_ALTGR) {
        modifiers |= Qt::MetaModifier;
    }
    if (mask & DIMM_CONTROL) {
        modifiers |= Qt::ControlModifier;
    }
    if (mask & DIMM_META) {
        modifiers | Qt::MetaModifier;
    }
    return modifiers;
}

QEvent::Type QDirectFbConvenience::eventType(DFBWindowEventType type)
{
    switch(type) {
    case DWET_BUTTONDOWN:
        return QEvent::MouseButtonPress;
    case DWET_BUTTONUP:
        return QEvent::MouseButtonRelease;
    case DWET_MOTION:
        return QEvent::MouseMove;
    case DWET_WHEEL:
        return QEvent::Wheel;
    case DWET_KEYDOWN:
        return QEvent::KeyPress;
    case DWET_KEYUP:
        return QEvent::KeyRelease;
    default:
        return QEvent::None;
    }
}
QDirectFbKeyMap *QDirectFbConvenience::dfbKeymap = 0;
QDirectFbKeyMap *QDirectFbConvenience::keyMap()
{
    if (!dfbKeymap)
        dfbKeymap = new QDirectFbKeyMap();
    return dfbKeymap;
}

QDirectFbKeyMap::QDirectFbKeyMap()
{
    insert(DIKS_BACKSPACE             , Qt::Key_Backspace);
    insert(DIKS_TAB                   , Qt::Key_Tab);
    insert(DIKS_RETURN                , Qt::Key_Return);
    insert(DIKS_ESCAPE                , Qt::Key_Escape);
    insert(DIKS_DELETE                , Qt::Key_Delete);

    insert(DIKS_CURSOR_LEFT           , Qt::Key_Left);
    insert(DIKS_CURSOR_RIGHT          , Qt::Key_Right);
    insert(DIKS_CURSOR_UP             , Qt::Key_Up);
    insert(DIKS_CURSOR_DOWN           , Qt::Key_Down);
    insert(DIKS_INSERT                , Qt::Key_Insert);
    insert(DIKS_HOME                  , Qt::Key_Home);
    insert(DIKS_END                   , Qt::Key_End);
    insert(DIKS_PAGE_UP               , Qt::Key_PageUp);
    insert(DIKS_PAGE_DOWN             , Qt::Key_PageDown);
    insert(DIKS_PRINT                 , Qt::Key_Print);
    insert(DIKS_PAUSE                 , Qt::Key_Pause);
    insert(DIKS_SELECT                , Qt::Key_Select);
    insert(DIKS_GOTO                  , Qt::Key_OpenUrl);
    insert(DIKS_CLEAR                 , Qt::Key_Clear);
    insert(DIKS_MENU                  , Qt::Key_Menu);
    insert(DIKS_HELP                  , Qt::Key_Help);
    insert(DIKS_INFO                  , Qt::Key_Info);
    insert(DIKS_EXIT                  , Qt::Key_Exit);
    insert(DIKS_SETUP                 , Qt::Key_Settings);

    insert(DIKS_CD                    , Qt::Key_CD);
    insert(DIKS_INTERNET              , Qt::Key_HomePage);
    insert(DIKS_MAIL                  , Qt::Key_LaunchMail);
    insert(DIKS_FAVORITES             , Qt::Key_Favorites);
    insert(DIKS_PHONE                 , Qt::Key_Phone);
    insert(DIKS_PROGRAM               , Qt::Key_Guide);
    insert(DIKS_TIME                  , Qt::Key_Time);

    insert(DIKS_RED                   , Qt::Key_Red);
    insert(DIKS_GREEN                 , Qt::Key_Green);
    insert(DIKS_YELLOW                , Qt::Key_Yellow);
    insert(DIKS_BLUE                  , Qt::Key_Blue);

    insert(DIKS_CHANNEL_UP            , Qt::Key_ChannelUp);
    insert(DIKS_CHANNEL_DOWN          , Qt::Key_ChannelDown);

    insert(DIKS_BACK                  , Qt::Key_Back);
    insert(DIKS_FORWARD               , Qt::Key_Forward);
    insert(DIKS_VOLUME_UP             , Qt::Key_VolumeUp);
    insert(DIKS_VOLUME_DOWN           , Qt::Key_VolumeDown);
    insert(DIKS_MUTE                  , Qt::Key_VolumeMute);
    insert(DIKS_PLAYPAUSE             , Qt::Key_MediaTogglePlayPause);
    insert(DIKS_PLAY                  , Qt::Key_MediaPlay);
    insert(DIKS_STOP                  , Qt::Key_MediaStop);
    insert(DIKS_RECORD                , Qt::Key_MediaRecord);
    insert(DIKS_PREVIOUS              , Qt::Key_MediaPrevious);
    insert(DIKS_NEXT                  , Qt::Key_MediaNext);
    insert(DIKS_REWIND                , Qt::Key_AudioRewind);
    insert(DIKS_FASTFORWARD           , Qt::Key_AudioForward);
    insert(DIKS_SUBTITLE              , Qt::Key_Subtitle);

    insert(DIKS_F1                    , Qt::Key_F1);
    insert(DIKS_F2                    , Qt::Key_F2);
    insert(DIKS_F3                    , Qt::Key_F3);
    insert(DIKS_F4                    , Qt::Key_F4);
    insert(DIKS_F5                    , Qt::Key_F5);
    insert(DIKS_F6                    , Qt::Key_F6);
    insert(DIKS_F7                    , Qt::Key_F7);
    insert(DIKS_F8                    , Qt::Key_F8);
    insert(DIKS_F9                    , Qt::Key_F9);
    insert(DIKS_F10                   , Qt::Key_F10);
    insert(DIKS_F11                   , Qt::Key_F11);
    insert(DIKS_F12                   , Qt::Key_F12);

    insert(DIKS_SHIFT                 , Qt::Key_Shift);
    insert(DIKS_CONTROL               , Qt::Key_Control);
    insert(DIKS_ALT                   , Qt::Key_Alt);
    insert(DIKS_ALTGR                 , Qt::Key_AltGr);

    insert(DIKS_META                  , Qt::Key_Meta);
    insert(DIKS_SUPER                 , Qt::Key_Super_L); // ???
    insert(DIKS_HYPER                 , Qt::Key_Hyper_L); // ???

    insert(DIKS_CAPS_LOCK             , Qt::Key_CapsLock);
    insert(DIKS_NUM_LOCK              , Qt::Key_NumLock);
    insert(DIKS_SCROLL_LOCK           , Qt::Key_ScrollLock);

    insert(DIKS_DEAD_ABOVEDOT         , Qt::Key_Dead_Abovedot);
    insert(DIKS_DEAD_ABOVERING        , Qt::Key_Dead_Abovering);
    insert(DIKS_DEAD_ACUTE            , Qt::Key_Dead_Acute);
    insert(DIKS_DEAD_BREVE            , Qt::Key_Dead_Breve);
    insert(DIKS_DEAD_CARON            , Qt::Key_Dead_Caron);
    insert(DIKS_DEAD_CEDILLA          , Qt::Key_Dead_Cedilla);
    insert(DIKS_DEAD_CIRCUMFLEX       , Qt::Key_Dead_Circumflex);
    insert(DIKS_DEAD_DIAERESIS        , Qt::Key_Dead_Diaeresis);
    insert(DIKS_DEAD_DOUBLEACUTE      , Qt::Key_Dead_Doubleacute);
    insert(DIKS_DEAD_GRAVE            , Qt::Key_Dead_Grave);
    insert(DIKS_DEAD_IOTA             , Qt::Key_Dead_Iota);
    insert(DIKS_DEAD_MACRON           , Qt::Key_Dead_Macron);
    insert(DIKS_DEAD_OGONEK           , Qt::Key_Dead_Ogonek);
    insert(DIKS_DEAD_SEMIVOICED_SOUND , Qt::Key_Dead_Semivoiced_Sound);
    insert(DIKS_DEAD_TILDE            , Qt::Key_Dead_Tilde);
    insert(DIKS_DEAD_VOICED_SOUND     , Qt::Key_Dead_Voiced_Sound);
    insert(DIKS_SPACE                 , Qt::Key_Space);
    insert(DIKS_EXCLAMATION_MARK      , Qt::Key_Exclam);
    insert(DIKS_QUOTATION             , Qt::Key_QuoteDbl);
    insert(DIKS_NUMBER_SIGN           , Qt::Key_NumberSign);
    insert(DIKS_DOLLAR_SIGN           , Qt::Key_Dollar);
    insert(DIKS_PERCENT_SIGN          , Qt::Key_Percent);
    insert(DIKS_AMPERSAND             , Qt::Key_Ampersand);
    insert(DIKS_APOSTROPHE            , Qt::Key_Apostrophe);
    insert(DIKS_PARENTHESIS_LEFT      , Qt::Key_ParenLeft);
    insert(DIKS_PARENTHESIS_RIGHT     , Qt::Key_ParenRight);
    insert(DIKS_ASTERISK              , Qt::Key_Asterisk);
    insert(DIKS_PLUS_SIGN             , Qt::Key_Plus);
    insert(DIKS_COMMA                 , Qt::Key_Comma);
    insert(DIKS_MINUS_SIGN            , Qt::Key_Minus);
    insert(DIKS_PERIOD                , Qt::Key_Period);
    insert(DIKS_SLASH                 , Qt::Key_Slash);
    insert(DIKS_0                     , Qt::Key_0);
    insert(DIKS_1                     , Qt::Key_1);
    insert(DIKS_2                     , Qt::Key_2);
    insert(DIKS_3                     , Qt::Key_3);
    insert(DIKS_4                     , Qt::Key_4);
    insert(DIKS_5                     , Qt::Key_5);
    insert(DIKS_6                     , Qt::Key_6);
    insert(DIKS_7                     , Qt::Key_7);
    insert(DIKS_8                     , Qt::Key_8);
    insert(DIKS_9                     , Qt::Key_9);
    insert(DIKS_COLON                 , Qt::Key_Colon);
    insert(DIKS_SEMICOLON             , Qt::Key_Semicolon);
    insert(DIKS_LESS_THAN_SIGN        , Qt::Key_Less);
    insert(DIKS_EQUALS_SIGN           , Qt::Key_Equal);
    insert(DIKS_GREATER_THAN_SIGN     , Qt::Key_Greater);
    insert(DIKS_QUESTION_MARK         , Qt::Key_Question);
    insert(DIKS_AT                    , Qt::Key_At);
    insert(DIKS_CAPITAL_A             , Qt::Key_A);
    insert(DIKS_CAPITAL_B             , Qt::Key_B);
    insert(DIKS_CAPITAL_C             , Qt::Key_C);
    insert(DIKS_CAPITAL_D             , Qt::Key_D);
    insert(DIKS_CAPITAL_E             , Qt::Key_E);
    insert(DIKS_CAPITAL_F             , Qt::Key_F);
    insert(DIKS_CAPITAL_G             , Qt::Key_G);
    insert(DIKS_CAPITAL_H             , Qt::Key_H);
    insert(DIKS_CAPITAL_I             , Qt::Key_I);
    insert(DIKS_CAPITAL_J             , Qt::Key_J);
    insert(DIKS_CAPITAL_K             , Qt::Key_K);
    insert(DIKS_CAPITAL_L             , Qt::Key_L);
    insert(DIKS_CAPITAL_M             , Qt::Key_M);
    insert(DIKS_CAPITAL_N             , Qt::Key_N);
    insert(DIKS_CAPITAL_O             , Qt::Key_O);
    insert(DIKS_CAPITAL_P             , Qt::Key_P);
    insert(DIKS_CAPITAL_Q             , Qt::Key_Q);
    insert(DIKS_CAPITAL_R             , Qt::Key_R);
    insert(DIKS_CAPITAL_S             , Qt::Key_S);
    insert(DIKS_CAPITAL_T             , Qt::Key_T);
    insert(DIKS_CAPITAL_U             , Qt::Key_U);
    insert(DIKS_CAPITAL_V             , Qt::Key_V);
    insert(DIKS_CAPITAL_W             , Qt::Key_W);
    insert(DIKS_CAPITAL_X             , Qt::Key_X);
    insert(DIKS_CAPITAL_Y             , Qt::Key_Y);
    insert(DIKS_CAPITAL_Z             , Qt::Key_Z);
    insert(DIKS_SQUARE_BRACKET_LEFT   , Qt::Key_BracketLeft);
    insert(DIKS_BACKSLASH             , Qt::Key_Backslash);
    insert(DIKS_SQUARE_BRACKET_RIGHT  , Qt::Key_BracketRight);
    insert(DIKS_CIRCUMFLEX_ACCENT     , Qt::Key_AsciiCircum);
    insert(DIKS_UNDERSCORE            , Qt::Key_Underscore);
    insert(DIKS_SMALL_A               , Qt::Key_A);
    insert(DIKS_SMALL_B               , Qt::Key_B);
    insert(DIKS_SMALL_C               , Qt::Key_C);
    insert(DIKS_SMALL_D               , Qt::Key_D);
    insert(DIKS_SMALL_E               , Qt::Key_E);
    insert(DIKS_SMALL_F               , Qt::Key_F);
    insert(DIKS_SMALL_G               , Qt::Key_G);
    insert(DIKS_SMALL_H               , Qt::Key_H);
    insert(DIKS_SMALL_I               , Qt::Key_I);
    insert(DIKS_SMALL_J               , Qt::Key_J);
    insert(DIKS_SMALL_K               , Qt::Key_K);
    insert(DIKS_SMALL_L               , Qt::Key_L);
    insert(DIKS_SMALL_M               , Qt::Key_M);
    insert(DIKS_SMALL_N               , Qt::Key_N);
    insert(DIKS_SMALL_O               , Qt::Key_O);
    insert(DIKS_SMALL_P               , Qt::Key_P);
    insert(DIKS_SMALL_Q               , Qt::Key_Q);
    insert(DIKS_SMALL_R               , Qt::Key_R);
    insert(DIKS_SMALL_S               , Qt::Key_S);
    insert(DIKS_SMALL_T               , Qt::Key_T);
    insert(DIKS_SMALL_U               , Qt::Key_U);
    insert(DIKS_SMALL_V               , Qt::Key_V);
    insert(DIKS_SMALL_W               , Qt::Key_W);
    insert(DIKS_SMALL_X               , Qt::Key_X);
    insert(DIKS_SMALL_Y               , Qt::Key_Y);
    insert(DIKS_SMALL_Z               , Qt::Key_Z);
    insert(DIKS_CURLY_BRACKET_LEFT    , Qt::Key_BraceLeft);
    insert(DIKS_VERTICAL_BAR          , Qt::Key_Bar);
    insert(DIKS_CURLY_BRACKET_RIGHT   , Qt::Key_BraceRight);
    insert(DIKS_TILDE                 , Qt::Key_AsciiTilde);
}

QDirectFbScreen *toDfbScreen(QWindow *window)
{
    return static_cast<QDirectFbScreen*>(window->screen()->handle());
}

IDirectFBDisplayLayer *toDfbLayer(QPlatformScreen *screen)
{
    return static_cast<QDirectFbScreen*>(screen)->dfbLayer();
}

QT_END_NAMESPACE
