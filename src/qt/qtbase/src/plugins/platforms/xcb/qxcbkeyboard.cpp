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
#include "qxcbkeyboard.h"
#include "qxcbwindow.h"
#include "qxcbscreen.h"

#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformcursor.h>

#include <QtCore/QTextCodec>
#include <QtCore/QMetaMethod>
#include <QtCore/QDir>
#include <private/qguiapplication_p.h>

#include <stdio.h>
#include <X11/keysym.h>

#ifndef XK_ISO_Left_Tab
#define XK_ISO_Left_Tab         0xFE20
#endif

#ifndef XK_dead_hook
#define XK_dead_hook            0xFE61
#endif

#ifndef XK_dead_horn
#define XK_dead_horn            0xFE62
#endif

#ifndef XK_Codeinput
#define XK_Codeinput            0xFF37
#endif

#ifndef XK_Kanji_Bangou
#define XK_Kanji_Bangou         0xFF37 /* same as codeinput */
#endif

// Fix old X libraries
#ifndef XK_KP_Home
#define XK_KP_Home              0xFF95
#endif
#ifndef XK_KP_Left
#define XK_KP_Left              0xFF96
#endif
#ifndef XK_KP_Up
#define XK_KP_Up                0xFF97
#endif
#ifndef XK_KP_Right
#define XK_KP_Right             0xFF98
#endif
#ifndef XK_KP_Down
#define XK_KP_Down              0xFF99
#endif
#ifndef XK_KP_Prior
#define XK_KP_Prior             0xFF9A
#endif
#ifndef XK_KP_Next
#define XK_KP_Next              0xFF9B
#endif
#ifndef XK_KP_End
#define XK_KP_End               0xFF9C
#endif
#ifndef XK_KP_Insert
#define XK_KP_Insert            0xFF9E
#endif
#ifndef XK_KP_Delete
#define XK_KP_Delete            0xFF9F
#endif

// the next lines are taken on 10/2009 from X.org (X11/XF86keysym.h), defining some special
// multimedia keys. They are included here as not every system has them.
#define XF86XK_MonBrightnessUp     0x1008FF02
#define XF86XK_MonBrightnessDown   0x1008FF03
#define XF86XK_KbdLightOnOff       0x1008FF04
#define XF86XK_KbdBrightnessUp     0x1008FF05
#define XF86XK_KbdBrightnessDown   0x1008FF06
#define XF86XK_Standby             0x1008FF10
#define XF86XK_AudioLowerVolume    0x1008FF11
#define XF86XK_AudioMute           0x1008FF12
#define XF86XK_AudioRaiseVolume    0x1008FF13
#define XF86XK_AudioPlay           0x1008FF14
#define XF86XK_AudioStop           0x1008FF15
#define XF86XK_AudioPrev           0x1008FF16
#define XF86XK_AudioNext           0x1008FF17
#define XF86XK_HomePage            0x1008FF18
#define XF86XK_Mail                0x1008FF19
#define XF86XK_Start               0x1008FF1A
#define XF86XK_Search              0x1008FF1B
#define XF86XK_AudioRecord         0x1008FF1C
#define XF86XK_Calculator          0x1008FF1D
#define XF86XK_Memo                0x1008FF1E
#define XF86XK_ToDoList            0x1008FF1F
#define XF86XK_Calendar            0x1008FF20
#define XF86XK_PowerDown           0x1008FF21
#define XF86XK_ContrastAdjust      0x1008FF22
#define XF86XK_Back                0x1008FF26
#define XF86XK_Forward             0x1008FF27
#define XF86XK_Stop                0x1008FF28
#define XF86XK_Refresh             0x1008FF29
#define XF86XK_PowerOff            0x1008FF2A
#define XF86XK_WakeUp              0x1008FF2B
#define XF86XK_Eject               0x1008FF2C
#define XF86XK_ScreenSaver         0x1008FF2D
#define XF86XK_WWW                 0x1008FF2E
#define XF86XK_Sleep               0x1008FF2F
#define XF86XK_Favorites           0x1008FF30
#define XF86XK_AudioPause          0x1008FF31
#define XF86XK_AudioMedia          0x1008FF32
#define XF86XK_MyComputer          0x1008FF33
#define XF86XK_LightBulb           0x1008FF35
#define XF86XK_Shop                0x1008FF36
#define XF86XK_History             0x1008FF37
#define XF86XK_OpenURL             0x1008FF38
#define XF86XK_AddFavorite         0x1008FF39
#define XF86XK_HotLinks            0x1008FF3A
#define XF86XK_BrightnessAdjust    0x1008FF3B
#define XF86XK_Finance             0x1008FF3C
#define XF86XK_Community           0x1008FF3D
#define XF86XK_AudioRewind         0x1008FF3E
#define XF86XK_BackForward         0x1008FF3F
#define XF86XK_Launch0             0x1008FF40
#define XF86XK_Launch1             0x1008FF41
#define XF86XK_Launch2             0x1008FF42
#define XF86XK_Launch3             0x1008FF43
#define XF86XK_Launch4             0x1008FF44
#define XF86XK_Launch5             0x1008FF45
#define XF86XK_Launch6             0x1008FF46
#define XF86XK_Launch7             0x1008FF47
#define XF86XK_Launch8             0x1008FF48
#define XF86XK_Launch9             0x1008FF49
#define XF86XK_LaunchA             0x1008FF4A
#define XF86XK_LaunchB             0x1008FF4B
#define XF86XK_LaunchC             0x1008FF4C
#define XF86XK_LaunchD             0x1008FF4D
#define XF86XK_LaunchE             0x1008FF4E
#define XF86XK_LaunchF             0x1008FF4F
#define XF86XK_ApplicationLeft     0x1008FF50
#define XF86XK_ApplicationRight    0x1008FF51
#define XF86XK_Book                0x1008FF52
#define XF86XK_CD                  0x1008FF53
#define XF86XK_Calculater          0x1008FF54
#define XF86XK_Clear               0x1008FF55
#define XF86XK_ClearGrab           0x1008FE21
#define XF86XK_Close               0x1008FF56
#define XF86XK_Copy                0x1008FF57
#define XF86XK_Cut                 0x1008FF58
#define XF86XK_Display             0x1008FF59
#define XF86XK_DOS                 0x1008FF5A
#define XF86XK_Documents           0x1008FF5B
#define XF86XK_Excel               0x1008FF5C
#define XF86XK_Explorer            0x1008FF5D
#define XF86XK_Game                0x1008FF5E
#define XF86XK_Go                  0x1008FF5F
#define XF86XK_iTouch              0x1008FF60
#define XF86XK_LogOff              0x1008FF61
#define XF86XK_Market              0x1008FF62
#define XF86XK_Meeting             0x1008FF63
#define XF86XK_MenuKB              0x1008FF65
#define XF86XK_MenuPB              0x1008FF66
#define XF86XK_MySites             0x1008FF67
#define XF86XK_News                0x1008FF69
#define XF86XK_OfficeHome          0x1008FF6A
#define XF86XK_Option              0x1008FF6C
#define XF86XK_Paste               0x1008FF6D
#define XF86XK_Phone               0x1008FF6E
#define XF86XK_Reply               0x1008FF72
#define XF86XK_Reload              0x1008FF73
#define XF86XK_RotateWindows       0x1008FF74
#define XF86XK_RotationPB          0x1008FF75
#define XF86XK_RotationKB          0x1008FF76
#define XF86XK_Save                0x1008FF77
#define XF86XK_Send                0x1008FF7B
#define XF86XK_Spell               0x1008FF7C
#define XF86XK_SplitScreen         0x1008FF7D
#define XF86XK_Support             0x1008FF7E
#define XF86XK_TaskPane            0x1008FF7F
#define XF86XK_Terminal            0x1008FF80
#define XF86XK_Tools               0x1008FF81
#define XF86XK_Travel              0x1008FF82
#define XF86XK_Video               0x1008FF87
#define XF86XK_Word                0x1008FF89
#define XF86XK_Xfer                0x1008FF8A
#define XF86XK_ZoomIn              0x1008FF8B
#define XF86XK_ZoomOut             0x1008FF8C
#define XF86XK_Away                0x1008FF8D
#define XF86XK_Messenger           0x1008FF8E
#define XF86XK_WebCam              0x1008FF8F
#define XF86XK_MailForward         0x1008FF90
#define XF86XK_Pictures            0x1008FF91
#define XF86XK_Music               0x1008FF92
#define XF86XK_Battery             0x1008FF93
#define XF86XK_Bluetooth           0x1008FF94
#define XF86XK_WLAN                0x1008FF95
#define XF86XK_UWB                 0x1008FF96
#define XF86XK_AudioForward        0x1008FF97
#define XF86XK_AudioRepeat         0x1008FF98
#define XF86XK_AudioRandomPlay     0x1008FF99
#define XF86XK_Subtitle            0x1008FF9A
#define XF86XK_AudioCycleTrack     0x1008FF9B
#define XF86XK_Time                0x1008FF9F
#define XF86XK_Select              0x1008FFA0
#define XF86XK_View                0x1008FFA1
#define XF86XK_TopMenu             0x1008FFA2
#define XF86XK_Red                 0x1008FFA3
#define XF86XK_Green               0x1008FFA4
#define XF86XK_Yellow              0x1008FFA5
#define XF86XK_Blue                0x1008FFA6
#define XF86XK_Suspend             0x1008FFA7
#define XF86XK_Hibernate           0x1008FFA8
#define XF86XK_TouchpadToggle      0x1008FFA9
#define XF86XK_TouchpadOn          0x1008FFB0
#define XF86XK_TouchpadOff         0x1008FFB1
#define XF86XK_AudioMicMute        0x1008FFB2


// end of XF86keysyms.h

QT_BEGIN_NAMESPACE

// keyboard mapping table
static const unsigned int KeyTbl[] = {

    // misc keys

    XK_Escape,                  Qt::Key_Escape,
    XK_Tab,                     Qt::Key_Tab,
    XK_ISO_Left_Tab,            Qt::Key_Backtab,
    XK_BackSpace,               Qt::Key_Backspace,
    XK_Return,                  Qt::Key_Return,
    XK_Insert,                  Qt::Key_Insert,
    XK_Delete,                  Qt::Key_Delete,
    XK_Clear,                   Qt::Key_Delete,
    XK_Pause,                   Qt::Key_Pause,
    XK_Print,                   Qt::Key_Print,
    0x1005FF60,                 Qt::Key_SysReq,         // hardcoded Sun SysReq
    0x1007ff00,                 Qt::Key_SysReq,         // hardcoded X386 SysReq

    // cursor movement

    XK_Home,                    Qt::Key_Home,
    XK_End,                     Qt::Key_End,
    XK_Left,                    Qt::Key_Left,
    XK_Up,                      Qt::Key_Up,
    XK_Right,                   Qt::Key_Right,
    XK_Down,                    Qt::Key_Down,
    XK_Prior,                   Qt::Key_PageUp,
    XK_Next,                    Qt::Key_PageDown,

    // modifiers

    XK_Shift_L,                 Qt::Key_Shift,
    XK_Shift_R,                 Qt::Key_Shift,
    XK_Shift_Lock,              Qt::Key_Shift,
    XK_Control_L,               Qt::Key_Control,
    XK_Control_R,               Qt::Key_Control,
    XK_Meta_L,                  Qt::Key_Meta,
    XK_Meta_R,                  Qt::Key_Meta,
    XK_Alt_L,                   Qt::Key_Alt,
    XK_Alt_R,                   Qt::Key_Alt,
    XK_Caps_Lock,               Qt::Key_CapsLock,
    XK_Num_Lock,                Qt::Key_NumLock,
    XK_Scroll_Lock,             Qt::Key_ScrollLock,
    XK_Super_L,                 Qt::Key_Super_L,
    XK_Super_R,                 Qt::Key_Super_R,
    XK_Menu,                    Qt::Key_Menu,
    XK_Hyper_L,                 Qt::Key_Hyper_L,
    XK_Hyper_R,                 Qt::Key_Hyper_R,
    XK_Help,                    Qt::Key_Help,
    0x1000FF74,                 Qt::Key_Backtab,        // hardcoded HP backtab
    0x1005FF10,                 Qt::Key_F11,            // hardcoded Sun F36 (labeled F11)
    0x1005FF11,                 Qt::Key_F12,            // hardcoded Sun F37 (labeled F12)

    // numeric and function keypad keys

    XK_KP_Space,                Qt::Key_Space,
    XK_KP_Tab,                  Qt::Key_Tab,
    XK_KP_Enter,                Qt::Key_Enter,
    //XK_KP_F1,                 Qt::Key_F1,
    //XK_KP_F2,                 Qt::Key_F2,
    //XK_KP_F3,                 Qt::Key_F3,
    //XK_KP_F4,                 Qt::Key_F4,
    XK_KP_Home,                 Qt::Key_Home,
    XK_KP_Left,                 Qt::Key_Left,
    XK_KP_Up,                   Qt::Key_Up,
    XK_KP_Right,                Qt::Key_Right,
    XK_KP_Down,                 Qt::Key_Down,
    XK_KP_Prior,                Qt::Key_PageUp,
    XK_KP_Next,                 Qt::Key_PageDown,
    XK_KP_End,                  Qt::Key_End,
    XK_KP_Begin,                Qt::Key_Clear,
    XK_KP_Insert,               Qt::Key_Insert,
    XK_KP_Delete,               Qt::Key_Delete,
    XK_KP_Equal,                Qt::Key_Equal,
    XK_KP_Multiply,             Qt::Key_Asterisk,
    XK_KP_Add,                  Qt::Key_Plus,
    XK_KP_Separator,            Qt::Key_Comma,
    XK_KP_Subtract,             Qt::Key_Minus,
    XK_KP_Decimal,              Qt::Key_Period,
    XK_KP_Divide,               Qt::Key_Slash,

    // International input method support keys

    // International & multi-key character composition
    XK_ISO_Level3_Shift,        Qt::Key_AltGr,
    XK_Multi_key,               Qt::Key_Multi_key,
    XK_Codeinput,               Qt::Key_Codeinput,
    XK_SingleCandidate,         Qt::Key_SingleCandidate,
    XK_MultipleCandidate,       Qt::Key_MultipleCandidate,
    XK_PreviousCandidate,       Qt::Key_PreviousCandidate,

    // Misc Functions
    XK_Mode_switch,             Qt::Key_Mode_switch,
    XK_script_switch,           Qt::Key_Mode_switch,

    // Japanese keyboard support
    XK_Kanji,                   Qt::Key_Kanji,
    XK_Muhenkan,                Qt::Key_Muhenkan,
    //XK_Henkan_Mode,           Qt::Key_Henkan_Mode,
    XK_Henkan_Mode,             Qt::Key_Henkan,
    XK_Henkan,                  Qt::Key_Henkan,
    XK_Romaji,                  Qt::Key_Romaji,
    XK_Hiragana,                Qt::Key_Hiragana,
    XK_Katakana,                Qt::Key_Katakana,
    XK_Hiragana_Katakana,       Qt::Key_Hiragana_Katakana,
    XK_Zenkaku,                 Qt::Key_Zenkaku,
    XK_Hankaku,                 Qt::Key_Hankaku,
    XK_Zenkaku_Hankaku,         Qt::Key_Zenkaku_Hankaku,
    XK_Touroku,                 Qt::Key_Touroku,
    XK_Massyo,                  Qt::Key_Massyo,
    XK_Kana_Lock,               Qt::Key_Kana_Lock,
    XK_Kana_Shift,              Qt::Key_Kana_Shift,
    XK_Eisu_Shift,              Qt::Key_Eisu_Shift,
    XK_Eisu_toggle,             Qt::Key_Eisu_toggle,
    //XK_Kanji_Bangou,          Qt::Key_Kanji_Bangou,
    //XK_Zen_Koho,              Qt::Key_Zen_Koho,
    //XK_Mae_Koho,              Qt::Key_Mae_Koho,
    XK_Kanji_Bangou,            Qt::Key_Codeinput,
    XK_Zen_Koho,                Qt::Key_MultipleCandidate,
    XK_Mae_Koho,                Qt::Key_PreviousCandidate,

#ifdef XK_KOREAN
    // Korean keyboard support
    XK_Hangul,                  Qt::Key_Hangul,
    XK_Hangul_Start,            Qt::Key_Hangul_Start,
    XK_Hangul_End,              Qt::Key_Hangul_End,
    XK_Hangul_Hanja,            Qt::Key_Hangul_Hanja,
    XK_Hangul_Jamo,             Qt::Key_Hangul_Jamo,
    XK_Hangul_Romaja,           Qt::Key_Hangul_Romaja,
    //XK_Hangul_Codeinput,      Qt::Key_Hangul_Codeinput,
    XK_Hangul_Codeinput,        Qt::Key_Codeinput,
    XK_Hangul_Jeonja,           Qt::Key_Hangul_Jeonja,
    XK_Hangul_Banja,            Qt::Key_Hangul_Banja,
    XK_Hangul_PreHanja,         Qt::Key_Hangul_PreHanja,
    XK_Hangul_PostHanja,        Qt::Key_Hangul_PostHanja,
    //XK_Hangul_SingleCandidate,Qt::Key_Hangul_SingleCandidate,
    //XK_Hangul_MultipleCandidate,Qt::Key_Hangul_MultipleCandidate,
    //XK_Hangul_PreviousCandidate,Qt::Key_Hangul_PreviousCandidate,
    XK_Hangul_SingleCandidate,  Qt::Key_SingleCandidate,
    XK_Hangul_MultipleCandidate,Qt::Key_MultipleCandidate,
    XK_Hangul_PreviousCandidate,Qt::Key_PreviousCandidate,
    XK_Hangul_Special,          Qt::Key_Hangul_Special,
    //XK_Hangul_switch,         Qt::Key_Hangul_switch,
    XK_Hangul_switch,           Qt::Key_Mode_switch,
#endif  // XK_KOREAN

    // dead keys
    XK_dead_grave,              Qt::Key_Dead_Grave,
    XK_dead_acute,              Qt::Key_Dead_Acute,
    XK_dead_circumflex,         Qt::Key_Dead_Circumflex,
    XK_dead_tilde,              Qt::Key_Dead_Tilde,
    XK_dead_macron,             Qt::Key_Dead_Macron,
    XK_dead_breve,              Qt::Key_Dead_Breve,
    XK_dead_abovedot,           Qt::Key_Dead_Abovedot,
    XK_dead_diaeresis,          Qt::Key_Dead_Diaeresis,
    XK_dead_abovering,          Qt::Key_Dead_Abovering,
    XK_dead_doubleacute,        Qt::Key_Dead_Doubleacute,
    XK_dead_caron,              Qt::Key_Dead_Caron,
    XK_dead_cedilla,            Qt::Key_Dead_Cedilla,
    XK_dead_ogonek,             Qt::Key_Dead_Ogonek,
    XK_dead_iota,               Qt::Key_Dead_Iota,
    XK_dead_voiced_sound,       Qt::Key_Dead_Voiced_Sound,
    XK_dead_semivoiced_sound,   Qt::Key_Dead_Semivoiced_Sound,
    XK_dead_belowdot,           Qt::Key_Dead_Belowdot,
    XK_dead_hook,               Qt::Key_Dead_Hook,
    XK_dead_horn,               Qt::Key_Dead_Horn,

    // Special keys from X.org - This include multimedia keys,
        // wireless/bluetooth/uwb keys, special launcher keys, etc.
    XF86XK_Back,                Qt::Key_Back,
    XF86XK_Forward,             Qt::Key_Forward,
    XF86XK_Stop,                Qt::Key_Stop,
    XF86XK_Refresh,             Qt::Key_Refresh,
    XF86XK_Favorites,           Qt::Key_Favorites,
    XF86XK_AudioMedia,          Qt::Key_LaunchMedia,
    XF86XK_OpenURL,             Qt::Key_OpenUrl,
    XF86XK_HomePage,            Qt::Key_HomePage,
    XF86XK_Search,              Qt::Key_Search,
    XF86XK_AudioLowerVolume,    Qt::Key_VolumeDown,
    XF86XK_AudioMute,           Qt::Key_VolumeMute,
    XF86XK_AudioRaiseVolume,    Qt::Key_VolumeUp,
    XF86XK_AudioPlay,           Qt::Key_MediaPlay,
    XF86XK_AudioStop,           Qt::Key_MediaStop,
    XF86XK_AudioPrev,           Qt::Key_MediaPrevious,
    XF86XK_AudioNext,           Qt::Key_MediaNext,
    XF86XK_AudioRecord,         Qt::Key_MediaRecord,
    XF86XK_Mail,                Qt::Key_LaunchMail,
    XF86XK_MyComputer,          Qt::Key_Launch0,  // ### Qt 6: remap properly
    XF86XK_Calculator,          Qt::Key_Launch1,
    XF86XK_Memo,                Qt::Key_Memo,
    XF86XK_ToDoList,            Qt::Key_ToDoList,
    XF86XK_Calendar,            Qt::Key_Calendar,
    XF86XK_PowerDown,           Qt::Key_PowerDown,
    XF86XK_ContrastAdjust,      Qt::Key_ContrastAdjust,
    XF86XK_Standby,             Qt::Key_Standby,
    XF86XK_MonBrightnessUp,     Qt::Key_MonBrightnessUp,
    XF86XK_MonBrightnessDown,   Qt::Key_MonBrightnessDown,
    XF86XK_KbdLightOnOff,       Qt::Key_KeyboardLightOnOff,
    XF86XK_KbdBrightnessUp,     Qt::Key_KeyboardBrightnessUp,
    XF86XK_KbdBrightnessDown,   Qt::Key_KeyboardBrightnessDown,
    XF86XK_PowerOff,            Qt::Key_PowerOff,
    XF86XK_WakeUp,              Qt::Key_WakeUp,
    XF86XK_Eject,               Qt::Key_Eject,
    XF86XK_ScreenSaver,         Qt::Key_ScreenSaver,
    XF86XK_WWW,                 Qt::Key_WWW,
    XF86XK_Sleep,               Qt::Key_Sleep,
    XF86XK_LightBulb,           Qt::Key_LightBulb,
    XF86XK_Shop,                Qt::Key_Shop,
    XF86XK_History,             Qt::Key_History,
    XF86XK_AddFavorite,         Qt::Key_AddFavorite,
    XF86XK_HotLinks,            Qt::Key_HotLinks,
    XF86XK_BrightnessAdjust,    Qt::Key_BrightnessAdjust,
    XF86XK_Finance,             Qt::Key_Finance,
    XF86XK_Community,           Qt::Key_Community,
    XF86XK_AudioRewind,         Qt::Key_AudioRewind,
    XF86XK_BackForward,         Qt::Key_BackForward,
    XF86XK_ApplicationLeft,     Qt::Key_ApplicationLeft,
    XF86XK_ApplicationRight,    Qt::Key_ApplicationRight,
    XF86XK_Book,                Qt::Key_Book,
    XF86XK_CD,                  Qt::Key_CD,
    XF86XK_Calculater,          Qt::Key_Calculator,
    XF86XK_Clear,               Qt::Key_Clear,
    XF86XK_ClearGrab,           Qt::Key_ClearGrab,
    XF86XK_Close,               Qt::Key_Close,
    XF86XK_Copy,                Qt::Key_Copy,
    XF86XK_Cut,                 Qt::Key_Cut,
    XF86XK_Display,             Qt::Key_Display,
    XF86XK_DOS,                 Qt::Key_DOS,
    XF86XK_Documents,           Qt::Key_Documents,
    XF86XK_Excel,               Qt::Key_Excel,
    XF86XK_Explorer,            Qt::Key_Explorer,
    XF86XK_Game,                Qt::Key_Game,
    XF86XK_Go,                  Qt::Key_Go,
    XF86XK_iTouch,              Qt::Key_iTouch,
    XF86XK_LogOff,              Qt::Key_LogOff,
    XF86XK_Market,              Qt::Key_Market,
    XF86XK_Meeting,             Qt::Key_Meeting,
    XF86XK_MenuKB,              Qt::Key_MenuKB,
    XF86XK_MenuPB,              Qt::Key_MenuPB,
    XF86XK_MySites,             Qt::Key_MySites,
    XF86XK_News,                Qt::Key_News,
    XF86XK_OfficeHome,          Qt::Key_OfficeHome,
    XF86XK_Option,              Qt::Key_Option,
    XF86XK_Paste,               Qt::Key_Paste,
    XF86XK_Phone,               Qt::Key_Phone,
    XF86XK_Reply,               Qt::Key_Reply,
    XF86XK_Reload,              Qt::Key_Reload,
    XF86XK_RotateWindows,       Qt::Key_RotateWindows,
    XF86XK_RotationPB,          Qt::Key_RotationPB,
    XF86XK_RotationKB,          Qt::Key_RotationKB,
    XF86XK_Save,                Qt::Key_Save,
    XF86XK_Send,                Qt::Key_Send,
    XF86XK_Spell,               Qt::Key_Spell,
    XF86XK_SplitScreen,         Qt::Key_SplitScreen,
    XF86XK_Support,             Qt::Key_Support,
    XF86XK_TaskPane,            Qt::Key_TaskPane,
    XF86XK_Terminal,            Qt::Key_Terminal,
    XF86XK_Tools,               Qt::Key_Tools,
    XF86XK_Travel,              Qt::Key_Travel,
    XF86XK_Video,               Qt::Key_Video,
    XF86XK_Word,                Qt::Key_Word,
    XF86XK_Xfer,                Qt::Key_Xfer,
    XF86XK_ZoomIn,              Qt::Key_ZoomIn,
    XF86XK_ZoomOut,             Qt::Key_ZoomOut,
    XF86XK_Away,                Qt::Key_Away,
    XF86XK_Messenger,           Qt::Key_Messenger,
    XF86XK_WebCam,              Qt::Key_WebCam,
    XF86XK_MailForward,         Qt::Key_MailForward,
    XF86XK_Pictures,            Qt::Key_Pictures,
    XF86XK_Music,               Qt::Key_Music,
    XF86XK_Battery,             Qt::Key_Battery,
    XF86XK_Bluetooth,           Qt::Key_Bluetooth,
    XF86XK_WLAN,                Qt::Key_WLAN,
    XF86XK_UWB,                 Qt::Key_UWB,
    XF86XK_AudioForward,        Qt::Key_AudioForward,
    XF86XK_AudioRepeat,         Qt::Key_AudioRepeat,
    XF86XK_AudioRandomPlay,     Qt::Key_AudioRandomPlay,
    XF86XK_Subtitle,            Qt::Key_Subtitle,
    XF86XK_AudioCycleTrack,     Qt::Key_AudioCycleTrack,
    XF86XK_Time,                Qt::Key_Time,
    XF86XK_Select,              Qt::Key_Select,
    XF86XK_View,                Qt::Key_View,
    XF86XK_TopMenu,             Qt::Key_TopMenu,
    XF86XK_Red,                 Qt::Key_Red,
    XF86XK_Green,               Qt::Key_Green,
    XF86XK_Yellow,              Qt::Key_Yellow,
    XF86XK_Blue,                Qt::Key_Blue,
    XF86XK_Bluetooth,           Qt::Key_Bluetooth,
    XF86XK_Suspend,             Qt::Key_Suspend,
    XF86XK_Hibernate,           Qt::Key_Hibernate,
    XF86XK_TouchpadToggle,      Qt::Key_TouchpadToggle,
    XF86XK_TouchpadOn,          Qt::Key_TouchpadOn,
    XF86XK_TouchpadOff,         Qt::Key_TouchpadOff,
    XF86XK_AudioMicMute,        Qt::Key_MicMute,
    XF86XK_Launch0,             Qt::Key_Launch2, // ### Qt 6: remap properly
    XF86XK_Launch1,             Qt::Key_Launch3,
    XF86XK_Launch2,             Qt::Key_Launch4,
    XF86XK_Launch3,             Qt::Key_Launch5,
    XF86XK_Launch4,             Qt::Key_Launch6,
    XF86XK_Launch5,             Qt::Key_Launch7,
    XF86XK_Launch6,             Qt::Key_Launch8,
    XF86XK_Launch7,             Qt::Key_Launch9,
    XF86XK_Launch8,             Qt::Key_LaunchA,
    XF86XK_Launch9,             Qt::Key_LaunchB,
    XF86XK_LaunchA,             Qt::Key_LaunchC,
    XF86XK_LaunchB,             Qt::Key_LaunchD,
    XF86XK_LaunchC,             Qt::Key_LaunchE,
    XF86XK_LaunchD,             Qt::Key_LaunchF,
    XF86XK_LaunchE,             Qt::Key_LaunchG,
    XF86XK_LaunchF,             Qt::Key_LaunchH,

    0,                          0
};

// Possible modifier states.
static const Qt::KeyboardModifiers ModsTbl[] = {
    Qt::NoModifier,                                             // 0
    Qt::ShiftModifier,                                          // 1
    Qt::ControlModifier,                                        // 2
    Qt::ControlModifier | Qt::ShiftModifier,                    // 3
    Qt::AltModifier,                                            // 4
    Qt::AltModifier | Qt::ShiftModifier,                        // 5
    Qt::AltModifier | Qt::ControlModifier,                      // 6
    Qt::AltModifier | Qt::ShiftModifier | Qt::ControlModifier,  // 7
    Qt::NoModifier                                              // Fall-back to raw Key_*, for non-latin1 kb layouts
};

Qt::KeyboardModifiers QXcbKeyboard::translateModifiers(int s) const
{
    Qt::KeyboardModifiers ret = 0;
    if (s & XCB_MOD_MASK_SHIFT)
        ret |= Qt::ShiftModifier;
    if (s & XCB_MOD_MASK_CONTROL)
        ret |= Qt::ControlModifier;
    if (s & rmod_masks.alt)
        ret |= Qt::AltModifier;
    if (s & rmod_masks.meta)
        ret |= Qt::MetaModifier;
    if (s & rmod_masks.altgr)
        ret |= Qt::GroupSwitchModifier;
    return ret;
}

void QXcbKeyboard::readXKBConfig()
{
    clearXKBConfig();
    xcb_generic_error_t *error;
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t *config_reply;

    xcb_connection_t *c = xcb_connection();
    xcb_window_t rootWindow = connection()->rootWindow();

    cookie = xcb_get_property(c, 0, rootWindow,
                  atom(QXcbAtom::_XKB_RULES_NAMES), XCB_ATOM_STRING, 0, 1024);

    config_reply = xcb_get_property_reply(c, cookie, &error);
    if (!config_reply) {
        qWarning("Qt: Couldn't interpret the _XKB_RULES_NAMES property");
        return;
    }
    char *xkb_config = (char *)xcb_get_property_value(config_reply);
    int length = xcb_get_property_value_length(config_reply);

    // on old X servers xkb_config can be 0 even if config_reply indicates a succesfull read
    if (!xkb_config || length == 0)
        return;
    // ### TODO some X servers don't set _XKB_RULES_NAMES at all, in these cases it is filled
    // with gibberish, we would need to do some kind of sanity check

    char *names[5] = { 0, 0, 0, 0, 0 };
    char *p = xkb_config, *end = p + length;
    int i = 0;
    // The result from xcb_get_property_value() is not necessarily \0-terminated,
    // we need to make sure that too many or missing '\0' symbols are handled safely.
    do {
        uint len = qstrnlen(p, length);
        names[i++] = p;
        p += len + 1;
        length -= len + 1;
    } while (p < end || i < 5);

    xkb_names.rules = qstrdup(names[0]);
    xkb_names.model = qstrdup(names[1]);
    xkb_names.layout = qstrdup(names[2]);
    xkb_names.variant = qstrdup(names[3]);
    xkb_names.options = qstrdup(names[4]);

    free(config_reply);
}

void QXcbKeyboard::clearXKBConfig()
{
    if (xkb_names.rules)
        delete[] xkb_names.rules;
    if (xkb_names.model)
        delete[] xkb_names.model;
    if (xkb_names.layout)
        delete[] xkb_names.layout;
    if (xkb_names.variant)
        delete[] xkb_names.variant;
    if (xkb_names.options)
        delete[] xkb_names.options;
    memset(&xkb_names, 0, sizeof(xkb_names));
}

void QXcbKeyboard::printKeymapError(const char *error) const
{
    qWarning() << error;
    if (xkb_context) {
        qWarning() << "Current XKB configuration data search paths are: ";
        for (unsigned int i = 0; i < xkb_context_num_include_paths(xkb_context); ++i)
            qWarning() << xkb_context_include_path_get(xkb_context, i);
    }
    qWarning() << "Use QT_XKB_CONFIG_ROOT environmental variable to provide an additional search path, "
                  "add ':' as separator to provide several search paths and/or make sure that XKB configuration data "
                  "directory contains recent enough contents, to update please see http://cgit.freedesktop.org/xkeyboard-config/ .";
}

void QXcbKeyboard::updateKeymap()
{
    m_config = true;
    // set xkb context object
    if (!xkb_context) {
        if (qEnvironmentVariableIsSet("QT_XKB_CONFIG_ROOT")) {
            xkb_context = xkb_context_new((xkb_context_flags)XKB_CONTEXT_NO_DEFAULT_INCLUDES);
            QList<QByteArray> xkbRootList = QByteArray(qgetenv("QT_XKB_CONFIG_ROOT")).split(':');
            foreach (QByteArray xkbRoot, xkbRootList)
                xkb_context_include_path_append(xkb_context, xkbRoot.constData());
        } else {
            xkb_context = xkb_context_new((xkb_context_flags)0);
        }
        if (!xkb_context) {
            printKeymapError("Qt: Failed to create XKB context!");
            m_config = false;
            return;
        }
        // log only critical errors, we do our own error logging from printKeymapError()
        xkb_context_set_log_level(xkb_context, (xkb_log_level)XKB_LOG_LEVEL_CRITICAL);
    }
    // update xkb keymap object
    xkb_keymap_unref(xkb_keymap);
    xkb_keymap = 0;

    struct xkb_state *new_state = 0;
#ifndef QT_NO_XKB
    if (connection()->hasXKB()) {
        xkb_keymap = xkb_x11_keymap_new_from_device(xkb_context, xcb_connection(), core_device_id, (xkb_keymap_compile_flags)0);
        if (xkb_keymap) {
            // Create a new keyboard state object for a keymap
            new_state = xkb_x11_state_new_from_device(xkb_keymap, xcb_connection(), core_device_id);
        }
    }
#endif
    if (!xkb_keymap) {
        // Compile a keymap from RMLVO (rules, models, layouts, variants and options) names
        readXKBConfig();
        xkb_keymap = xkb_keymap_new_from_names(xkb_context, &xkb_names, (xkb_keymap_compile_flags)0);
        if (!xkb_keymap) {
            // last fallback is to used hard-coded keymap name, see DEFAULT_XKB_* in xkbcommon.pri
            qWarning() << "Qt: Could not determine keyboard configuration data"
                          " from X server, will use hard-coded keymap configuration.";
            clearXKBConfig();
            xkb_keymap = xkb_keymap_new_from_names(xkb_context, &xkb_names, (xkb_keymap_compile_flags)0);
        }
        if (xkb_keymap) {
            new_state = xkb_state_new(xkb_keymap);
        } else {
            printKeymapError("Failed to compile a keymap!");
            m_config = false;
            return;
        }

    }
    if (!new_state) {
        qWarning("Qt: Failed to create xkb state!");
        m_config = false;
        return;
    }
    // update xkb state object
    xkb_state_unref(xkb_state);
    xkb_state = new_state;
    if (!connection()->hasXKB())
        updateXKBMods();
}

#ifndef QT_NO_XKB
void QXcbKeyboard::updateXKBState(xcb_xkb_state_notify_event_t *state)
{
    if (m_config && connection()->hasXKB()) {
        const xkb_state_component newState
                = xkb_state_update_mask(xkb_state,
                                  state->baseMods,
                                  state->latchedMods,
                                  state->lockedMods,
                                  state->baseGroup,
                                  state->latchedGroup,
                                  state->lockedGroup);

        if ((newState & XKB_STATE_LAYOUT_EFFECTIVE) == XKB_STATE_LAYOUT_EFFECTIVE) {
            //qWarning("TODO: Support KeyboardLayoutChange on QPA (QTBUG-27681)");
        }
    }
}
#endif

void QXcbKeyboard::updateXKBStateFromCore(quint16 state)
{
    if (m_config && !connection()->hasXKB()) {
        const quint32 modsDepressed = xkb_state_serialize_mods(xkb_state, XKB_STATE_MODS_DEPRESSED);
        const quint32 modsLatched = xkb_state_serialize_mods(xkb_state, XKB_STATE_MODS_LATCHED);
        const quint32 modsLocked = xkb_state_serialize_mods(xkb_state, XKB_STATE_MODS_LOCKED);
        const quint32 xkbMask = xkbModMask(state);

        const quint32 latched = modsLatched & xkbMask;
        const quint32 locked = modsLocked & xkbMask;
        quint32 depressed = modsDepressed & xkbMask;
        // set modifiers in depressed if they don't appear in any of the final masks
        depressed |= ~(depressed | latched | locked) & xkbMask;

        const xkb_state_component newState
                = xkb_state_update_mask(xkb_state,
                              depressed,
                              latched,
                              locked,
                              0,
                              0,
                              (state >> 13) & 3); // bits 13 and 14 report the state keyboard group

        if ((newState & XKB_STATE_LAYOUT_EFFECTIVE) == XKB_STATE_LAYOUT_EFFECTIVE) {
            //qWarning("TODO: Support KeyboardLayoutChange on QPA (QTBUG-27681)");
        }
    }
}

quint32 QXcbKeyboard::xkbModMask(quint16 state)
{
    quint32 xkb_mask = 0;

    if ((state & XCB_MOD_MASK_SHIFT) && xkb_mods.shift != XKB_MOD_INVALID)
        xkb_mask |= (1 << xkb_mods.shift);
    if ((state & XCB_MOD_MASK_LOCK) && xkb_mods.lock != XKB_MOD_INVALID)
        xkb_mask |= (1 << xkb_mods.lock);
    if ((state & XCB_MOD_MASK_CONTROL) && xkb_mods.control != XKB_MOD_INVALID)
        xkb_mask |= (1 << xkb_mods.control);
    if ((state & XCB_MOD_MASK_1) && xkb_mods.mod1 != XKB_MOD_INVALID)
        xkb_mask |= (1 << xkb_mods.mod1);
    if ((state & XCB_MOD_MASK_2) && xkb_mods.mod2 != XKB_MOD_INVALID)
        xkb_mask |= (1 << xkb_mods.mod2);
    if ((state & XCB_MOD_MASK_3) && xkb_mods.mod3 != XKB_MOD_INVALID)
        xkb_mask |= (1 << xkb_mods.mod3);
    if ((state & XCB_MOD_MASK_4) && xkb_mods.mod4 != XKB_MOD_INVALID)
        xkb_mask |= (1 << xkb_mods.mod4);
    if ((state & XCB_MOD_MASK_5) && xkb_mods.mod5 != XKB_MOD_INVALID)
        xkb_mask |= (1 << xkb_mods.mod5);

    return xkb_mask;
}

void QXcbKeyboard::updateXKBMods()
{
    xkb_mods.shift = xkb_keymap_mod_get_index(xkb_keymap, XKB_MOD_NAME_SHIFT);
    xkb_mods.lock = xkb_keymap_mod_get_index(xkb_keymap, XKB_MOD_NAME_CAPS);
    xkb_mods.control = xkb_keymap_mod_get_index(xkb_keymap, XKB_MOD_NAME_CTRL);
    xkb_mods.mod1 = xkb_keymap_mod_get_index(xkb_keymap, "Mod1");
    xkb_mods.mod2 = xkb_keymap_mod_get_index(xkb_keymap, "Mod2");
    xkb_mods.mod3 = xkb_keymap_mod_get_index(xkb_keymap, "Mod3");
    xkb_mods.mod4 = xkb_keymap_mod_get_index(xkb_keymap, "Mod4");
    xkb_mods.mod5 = xkb_keymap_mod_get_index(xkb_keymap, "Mod5");
}

QList<int> QXcbKeyboard::possibleKeys(const QKeyEvent *event) const
{
    // turn off the modifier bits which doesn't participate in shortcuts
    Qt::KeyboardModifiers notNeeded = Qt::MetaModifier | Qt::KeypadModifier | Qt::GroupSwitchModifier;
    Qt::KeyboardModifiers modifiers = event->modifiers() &= ~notNeeded;
    // create a fresh kb state and test against the relevant modifier combinations
    // NOTE: it should be possible to query the keymap directly, once it gets
    // supported by libxkbcommon
    struct xkb_state * kb_state = xkb_state_new(xkb_keymap);
    if (!kb_state) {
        qWarning("QXcbKeyboard: failed to compile xkb keymap");
        return QList<int>();
    }
    // get kb state from the master xkb_state and update the temporary kb_state
    xkb_layout_index_t baseLayout = xkb_state_serialize_layout(xkb_state, XKB_STATE_LAYOUT_DEPRESSED);
    xkb_layout_index_t latchedLayout = xkb_state_serialize_layout(xkb_state, XKB_STATE_LAYOUT_LATCHED);
    xkb_layout_index_t lockedLayout = xkb_state_serialize_layout(xkb_state, XKB_STATE_LAYOUT_LOCKED);
    xkb_mod_index_t latchedMods = xkb_state_serialize_mods(xkb_state, XKB_STATE_MODS_LATCHED);
    xkb_mod_index_t lockedMods = xkb_state_serialize_mods(xkb_state, XKB_STATE_MODS_LOCKED);

    xkb_state_update_mask(kb_state, 0, latchedMods, lockedMods,
                                    baseLayout, latchedLayout, lockedLayout);

    xkb_keysym_t sym = xkb_state_key_get_one_sym(kb_state, event->nativeScanCode());
    if (sym == XKB_KEY_NoSymbol)
        return QList<int>();

    QList<int> result;
    int baseQtKey = keysymToQtKey(sym, modifiers, lookupString(kb_state, event->nativeScanCode()));
    result += (baseQtKey + modifiers); // The base key is _always_ valid, of course

    xkb_mod_index_t shiftMod = xkb_keymap_mod_get_index(xkb_keymap, "Shift");
    xkb_mod_index_t altMod = xkb_keymap_mod_get_index(xkb_keymap, "Alt");
    xkb_mod_index_t controlMod = xkb_keymap_mod_get_index(xkb_keymap, "Control");

    xkb_mod_mask_t depressed;
    struct xkb_keymap *fallback_keymap = 0;
    int qtKey = 0;
    //obtain a list of possible shortcuts for the given key event
    for (uint i = 1; i < sizeof(ModsTbl) / sizeof(*ModsTbl) ; ++i) {
        Qt::KeyboardModifiers neededMods = ModsTbl[i];
        if ((modifiers & neededMods) == neededMods) {

            depressed = 0;
            if (neededMods & Qt::AltModifier)
                depressed |= (1 << altMod);
            if (neededMods & Qt::ShiftModifier)
                depressed |= (1 << shiftMod);
            if (neededMods & Qt::ControlModifier)
                depressed |= (1 << controlMod);

            // update a keyboard state from a set of explicit masks
            if (i == 8) {
                // Add a fall back key for layouts with non Latin-1 characters
                if (baseQtKey > 255) {
                    struct xkb_rule_names names = { xkb_names.rules, xkb_names.model, "us", 0, 0 };
                    fallback_keymap = xkb_keymap_new_from_names(xkb_context, &names, (xkb_keymap_compile_flags)0);
                    if (!fallback_keymap)
                        continue;
                    xkb_state_unref(kb_state);
                    kb_state = xkb_state_new(fallback_keymap);
                    if (!kb_state)
                        continue;
                } else
                    continue;
            } else {
                xkb_state_update_mask(kb_state, depressed, latchedMods, lockedMods,
                                                baseLayout, latchedLayout, lockedLayout);
            }
            sym = xkb_state_key_get_one_sym(kb_state, event->nativeScanCode());

            if (sym == XKB_KEY_NoSymbol)
                continue;

            Qt::KeyboardModifiers mods = modifiers & ~neededMods;
            qtKey = keysymToQtKey(sym, mods, lookupString(kb_state, event->nativeScanCode()));

            if (qtKey == baseQtKey)
                continue;

            result += (qtKey + mods);
        }
    }
    xkb_state_unref(kb_state);
    xkb_keymap_unref(fallback_keymap);

    return result;
 }

int QXcbKeyboard::keysymToQtKey(xcb_keysym_t key) const
{
    int code = 0;
    int i = 0;
    while (KeyTbl[i]) {
        if (key == KeyTbl[i]) {
            code = (int)KeyTbl[i+1];
            break;
        }
        i += 2;
    }

    return code;
}

int QXcbKeyboard::keysymToQtKey(xcb_keysym_t keysym, Qt::KeyboardModifiers &modifiers, QString text) const
{
    int code = 0;
#ifndef QT_NO_TEXTCODEC
    QTextCodec *systemCodec = QTextCodec::codecForLocale();
#endif
    // Commentary in X11/keysymdef says that X codes match ASCII, so it
    // is safe to use the locale functions to process X codes in ISO8859-1.
    // This is mainly for compatibility - applications should not use the
    // Qt keycodes between 128 and 255 (extended ACSII codes), but should
    // rather use the QKeyEvent::text().
    if (keysym < 128 || (keysym < 256
#ifndef QT_NO_TEXTCODEC
                         && systemCodec->mibEnum() == 4
#endif
                         )) {
        // upper-case key, if known
        code = isprint((int)keysym) ? toupper((int)keysym) : 0;
    } else if (keysym >= XK_F1 && keysym <= XK_F35) {
        // function keys
        code = Qt::Key_F1 + ((int)keysym - XK_F1);
    } else if (keysym >= XK_KP_Space && keysym <= XK_KP_9) {
        if (keysym >= XK_KP_0) {
            // numeric keypad keys
            code = Qt::Key_0 + ((int)keysym - XK_KP_0);
        } else {
            code = keysymToQtKey(keysym);
        }
        modifiers |= Qt::KeypadModifier;
    } else if (text.length() == 1 && text.unicode()->unicode() > 0x1f
                                  && text.unicode()->unicode() != 0x7f
                                  && !(keysym >= XK_dead_grave && keysym <= XK_dead_currency)) {
        code = text.unicode()->toUpper().unicode();
    } else {
        // any other keys
        code = keysymToQtKey(keysym);
    }

    return code;
}

QXcbKeyboard::QXcbKeyboard(QXcbConnection *connection)
    : QXcbObject(connection)
    , m_autorepeat_code(0)
    , xkb_context(0)
    , xkb_keymap(0)
    , xkb_state(0)
{
    memset(&xkb_names, 0, sizeof(xkb_names));
#ifndef QT_NO_XKB
    core_device_id = 0;
    if (connection->hasXKB()) {
        updateVModMapping();
        updateVModToRModMapping();
        core_device_id = xkb_x11_get_core_keyboard_device_id(xcb_connection());
        if (core_device_id == -1) {
            qWarning("Qt: couldn't get core keyboard device info");
            return;
        }
    } else {
#endif
        m_key_symbols = xcb_key_symbols_alloc(xcb_connection());
        updateModifiers();
#ifndef QT_NO_XKB
    }
#endif
    updateKeymap();
}

QXcbKeyboard::~QXcbKeyboard()
{
    xkb_state_unref(xkb_state);
    xkb_keymap_unref(xkb_keymap);
    xkb_context_unref(xkb_context);
    if (!connection()->hasXKB())
        xcb_key_symbols_free(m_key_symbols);
    clearXKBConfig();
}

void QXcbKeyboard::updateVModMapping()
{
#ifndef QT_NO_XKB
    xcb_xkb_get_names_cookie_t names_cookie;
    xcb_xkb_get_names_reply_t *name_reply;
    xcb_xkb_get_names_value_list_t names_list;

    memset(&vmod_masks, 0, sizeof(vmod_masks));

    names_cookie = xcb_xkb_get_names(xcb_connection(),
                               XCB_XKB_ID_USE_CORE_KBD,
                               XCB_XKB_NAME_DETAIL_VIRTUAL_MOD_NAMES);

    name_reply = xcb_xkb_get_names_reply(xcb_connection(), names_cookie, 0);
    if (!name_reply) {
        qWarning("Qt: failed to retrieve the virtual modifier names from XKB");
        return;
    }

    const void *buffer = xcb_xkb_get_names_value_list(name_reply);
    xcb_xkb_get_names_value_list_unpack(buffer,
                                        name_reply->nTypes,
                                        name_reply->indicators,
                                        name_reply->virtualMods,
                                        name_reply->groupNames,
                                        name_reply->nKeys,
                                        name_reply->nKeyAliases,
                                        name_reply->nRadioGroups,
                                        name_reply->which,
                                        &names_list);

    int count = 0;
    uint vmod_mask, bit;
    char *vmod_name;
    vmod_mask = name_reply->virtualMods;
    // find the virtual modifiers for which names are defined.
    for (bit = 1; vmod_mask; bit <<= 1) {
        vmod_name = 0;

        if (!(vmod_mask & bit))
            continue;

        vmod_mask &= ~bit;
        // virtualModNames - the list of virtual modifier atoms beginning with the lowest-numbered
        // virtual modifier for which a name is defined and proceeding to the highest.
        QByteArray atomName = connection()->atomName(names_list.virtualModNames[count]);
        vmod_name = atomName.data();
        count++;

        if (!vmod_name)
            continue;

        // similarly we could retrieve NumLock, Super, Hyper modifiers if needed.
        if (qstrcmp(vmod_name, "Alt") == 0)
            vmod_masks.alt = bit;
        else if (qstrcmp(vmod_name, "Meta") == 0)
            vmod_masks.meta = bit;
        else if (qstrcmp(vmod_name, "AltGr") == 0)
            vmod_masks.altgr = bit;
        else if (qstrcmp(vmod_name, "Super") == 0)
            vmod_masks.super = bit;
        else if (qstrcmp(vmod_name, "Hyper") == 0)
            vmod_masks.hyper = bit;
    }

    free(name_reply);
#endif
}

void QXcbKeyboard::updateVModToRModMapping()
{
#ifndef QT_NO_XKB
    xcb_xkb_get_map_cookie_t map_cookie;
    xcb_xkb_get_map_reply_t *map_reply;
    xcb_xkb_get_map_map_t map;

    memset(&rmod_masks, 0, sizeof(rmod_masks));

    map_cookie = xcb_xkb_get_map(xcb_connection(),
                             XCB_XKB_ID_USE_CORE_KBD,
                             XCB_XKB_MAP_PART_VIRTUAL_MODS,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    map_reply = xcb_xkb_get_map_reply(xcb_connection(), map_cookie, 0);
    if (!map_reply) {
        qWarning("Qt: failed to retrieve the virtual modifier map from XKB");
        return;
    }

    const void *buffer = xcb_xkb_get_map_map(map_reply);
    xcb_xkb_get_map_map_unpack(buffer,
                               map_reply->nTypes,
                               map_reply->nKeySyms,
                               map_reply->nKeyActions,
                               map_reply->totalActions,
                               map_reply->totalKeyBehaviors,
                               map_reply->nVModMapKeys,
                               map_reply->totalKeyExplicit,
                               map_reply->totalModMapKeys,
                               map_reply->totalVModMapKeys,
                               map_reply->present,
                               &map);

    uint vmod_mask, bit;
    // the virtual modifiers mask for which a set of corresponding
    // real modifiers is to be returned
    vmod_mask = map_reply->virtualMods;
    int count = 0;

    for (bit = 1; vmod_mask; bit <<= 1) {
        uint modmap;

        if (!(vmod_mask & bit))
            continue;

        vmod_mask &= ~bit;
        // real modifier bindings for the specified virtual modifiers
        modmap = map.vmods_rtrn[count];
        count++;

        if (vmod_masks.alt == bit)
            rmod_masks.alt = modmap;
        else if (vmod_masks.meta == bit)
            rmod_masks.meta = modmap;
        else if (vmod_masks.altgr == bit)
            rmod_masks.altgr = modmap;
        else if (vmod_masks.super == bit)
            rmod_masks.super = modmap;
        else if (vmod_masks.hyper == bit)
            rmod_masks.hyper = modmap;
    }

    free(map_reply);
    resolveMaskConflicts();
#endif
}

void QXcbKeyboard::updateModifiers()
{
    // The core protocol does not provide a convenient way to determine the mapping
    // of modifier bits. Clients must retrieve and search the modifier map to determine
    // the keycodes bound to each modifier, and then retrieve and search the keyboard
    // mapping to determine the keysyms bound to the keycodes. They must repeat this
    // process for all modifiers whenever any part of the modifier mapping is changed.
    memset(&rmod_masks, 0, sizeof(rmod_masks));

    xcb_generic_error_t *error = 0;
    xcb_connection_t *conn = xcb_connection();
    xcb_get_modifier_mapping_cookie_t modMapCookie = xcb_get_modifier_mapping(conn);
    xcb_get_modifier_mapping_reply_t *modMapReply =
        xcb_get_modifier_mapping_reply(conn, modMapCookie, &error);
    if (error) {
        qWarning("Qt: failed to get modifier mapping");
        free(error);
        return;
    }

    // for Alt and Meta L and R are the same
    static const xcb_keysym_t symbols[] = {
        XK_Alt_L, XK_Meta_L, XK_Mode_switch, XK_Super_L, XK_Super_R,
        XK_Hyper_L, XK_Hyper_R
    };
    static const size_t numSymbols = sizeof symbols / sizeof *symbols;

    // Figure out the modifier mapping, ICCCM 6.6
    xcb_keycode_t* modKeyCodes[numSymbols];
    for (size_t i = 0; i < numSymbols; ++i)
        modKeyCodes[i] = xcb_key_symbols_get_keycode(m_key_symbols, symbols[i]);

    xcb_keycode_t *modMap = xcb_get_modifier_mapping_keycodes(modMapReply);
    const int w = modMapReply->keycodes_per_modifier;
    for (size_t i = 0; i < numSymbols; ++i) {
        for (int bit = 0; bit < 8; ++bit) {
            uint mask = 1 << bit;
            for (int x = 0; x < w; ++x) {
                xcb_keycode_t keyCode = modMap[x + bit * w];
                xcb_keycode_t *itk = modKeyCodes[i];
                while (itk && *itk != XCB_NO_SYMBOL)
                    if (*itk++ == keyCode) {
                        uint sym = symbols[i];
                        if ((sym == XK_Alt_L || sym == XK_Alt_R))
                            rmod_masks.alt = mask;
                        if ((sym == XK_Meta_L || sym == XK_Meta_R))
                            rmod_masks.meta = mask;
                        if (sym == XK_Mode_switch)
                            rmod_masks.altgr = mask;
                        if ((sym == XK_Super_L) || (sym == XK_Super_R))
                            rmod_masks.super = mask;
                        if ((sym == XK_Hyper_L) || (sym == XK_Hyper_R))
                            rmod_masks.hyper = mask;
                    }
            }
        }
    }

    for (size_t i = 0; i < numSymbols; ++i)
        free(modKeyCodes[i]);
    free(modMapReply);
    resolveMaskConflicts();
}

void QXcbKeyboard::resolveMaskConflicts()
{
    // if we don't have a meta key (or it's hidden behind alt), use super or hyper to generate
    // Qt::Key_Meta and Qt::MetaModifier, since most newer XFree86/Xorg installations map the Windows
    // key to Super
    if (rmod_masks.alt == rmod_masks.meta)
        rmod_masks.meta = 0;

    if (rmod_masks.meta == 0) {
        // no meta keys... s/meta/super,
        rmod_masks.meta = rmod_masks.super;
        if (rmod_masks.meta == 0) {
            // no super keys either? guess we'll use hyper then
            rmod_masks.meta = rmod_masks.hyper;
        }
    }
}

class KeyChecker
{
public:
    KeyChecker(xcb_window_t window, xcb_keycode_t code, xcb_timestamp_t time)
        : m_window(window)
        , m_code(code)
        , m_time(time)
        , m_error(false)
        , m_release(true)
    {
    }

    bool checkEvent(xcb_generic_event_t *ev)
    {
        if (m_error || !ev)
            return false;

        int type = ev->response_type & ~0x80;
        if (type != XCB_KEY_PRESS && type != XCB_KEY_RELEASE)
            return false;

        xcb_key_press_event_t *event = (xcb_key_press_event_t *)ev;

        if (event->event != m_window || event->detail != m_code) {
            m_error = true;
            return false;
        }

        if (type == XCB_KEY_PRESS) {
            m_error = !m_release || event->time - m_time > 10;
            return !m_error;
        }

        if (m_release) {
            m_error = true;
            return false;
        }

        m_release = true;
        m_time = event->time;

        return false;
    }

    bool release() const { return m_release; }
    xcb_timestamp_t time() const { return m_time; }

private:
    xcb_window_t m_window;
    xcb_keycode_t m_code;
    xcb_timestamp_t m_time;

    bool m_error;
    bool m_release;
};

void QXcbKeyboard::handleKeyEvent(QWindow *window, QEvent::Type type, xcb_keycode_t code,
                                  quint16 state, xcb_timestamp_t time)
{
    Q_XCB_NOOP(connection());

    if (!m_config)
        return;

    // It is crucial the order of xkb_state_key_get_one_sym & xkb_state_update_key operations is not reversed!
    xcb_keysym_t sym = xkb_state_key_get_one_sym(xkb_state, code);

    QPlatformInputContext *inputContext = QGuiApplicationPrivate::platformIntegration()->inputContext();
    QMetaMethod method;

    if (inputContext) {
        int methodIndex = inputContext->metaObject()->indexOfMethod("x11FilterEvent(uint,uint,uint,bool)");
        if (methodIndex != -1)
            method = inputContext->metaObject()->method(methodIndex);
    }

    if (method.isValid()) {
        bool retval = false;
        method.invoke(inputContext, Qt::DirectConnection,
                      Q_RETURN_ARG(bool, retval),
                      Q_ARG(uint, sym),
                      Q_ARG(uint, code),
                      Q_ARG(uint, state),
                      Q_ARG(bool, type == QEvent::KeyPress));
        if (retval)
            return;
    }

    Qt::KeyboardModifiers modifiers = translateModifiers(state);

    QString string = lookupString(xkb_state, code);
    int count = string.size();
    string.truncate(count);

    int qtcode = keysymToQtKey(sym, modifiers, string);
    bool isAutoRepeat = false;

    if (type == QEvent::KeyPress) {
        if (m_autorepeat_code == code) {
            isAutoRepeat = true;
            m_autorepeat_code = 0;
        }
    } else {
        // look ahead for auto-repeat
        KeyChecker checker(((QXcbWindow *)window->handle())->xcb_window(), code, time);
        xcb_generic_event_t *event = connection()->checkEvent(checker);
        if (event) {
            isAutoRepeat = true;
            free(event);
        }
        m_autorepeat_code = isAutoRepeat ? code : 0;
    }

    bool filtered = false;
    if (inputContext) {
        QKeyEvent event(type, qtcode, modifiers, code, sym, state, string, isAutoRepeat, count);
        event.setTimestamp(time);
        filtered = inputContext->filterEvent(&event);
    }

    if (!filtered) {
        if (type == QEvent::KeyPress && qtcode == Qt::Key_Menu) {
            const QPoint globalPos = window->screen()->handle()->cursor()->pos();
            const QPoint pos = window->mapFromGlobal(globalPos);
            QWindowSystemInterface::handleContextMenuEvent(window, false, pos, globalPos, modifiers);
        }
        QWindowSystemInterface::handleExtendedKeyEvent(window, time, type, qtcode, modifiers,
                                                       code, sym, state, string, isAutoRepeat);
    }

    if (isAutoRepeat && type == QEvent::KeyRelease) {
        // since we removed it from the event queue using checkEvent we need to send the key press here
        filtered = false;
        if (method.isValid()) {
            method.invoke(inputContext, Qt::DirectConnection,
                          Q_RETURN_ARG(bool, filtered),
                          Q_ARG(uint, sym),
                          Q_ARG(uint, code),
                          Q_ARG(uint, state),
                          Q_ARG(bool, true));
        }

        if (!filtered && inputContext) {
            QKeyEvent event(QEvent::KeyPress, qtcode, modifiers, code, sym, state, string, isAutoRepeat, count);
            event.setTimestamp(time);
            filtered = inputContext->filterEvent(&event);
        }
        if (!filtered)
            QWindowSystemInterface::handleExtendedKeyEvent(window, time, QEvent::KeyPress, qtcode, modifiers,
                                                           code, sym, state, string, isAutoRepeat);
    }
}

QString QXcbKeyboard::lookupString(struct xkb_state *state, xcb_keycode_t code) const
{
    QByteArray chars;
    chars.resize(1 + xkb_state_key_get_utf8(state, code, 0, 0));
    // equivalent of XLookupString
    xkb_state_key_get_utf8(state, code, chars.data(), chars.size());
    return QString::fromUtf8(chars);
}

void QXcbKeyboard::handleKeyPressEvent(QXcbWindowEventListener *eventListener, const xcb_key_press_event_t *event)
{
    QXcbWindow *window = eventListener->toWindow();
    if (!window)
        return;
    window->updateNetWmUserTime(event->time);
    handleKeyEvent(window->window(), QEvent::KeyPress, event->detail, event->state, event->time);
}

void QXcbKeyboard::handleKeyReleaseEvent(QXcbWindowEventListener *eventListener, const xcb_key_release_event_t *event)
{
    QXcbWindow *window = eventListener->toWindow();
    if (!window)
        return;
    handleKeyEvent(window->window(), QEvent::KeyRelease, event->detail, event->state, event->time);
}

void QXcbKeyboard::handleMappingNotifyEvent(const void *event)
{
    updateKeymap();
    if (connection()->hasXKB()) {
        updateVModMapping();
        updateVModToRModMapping();
    } else {
        void *ev = const_cast<void *>(event);
        xcb_refresh_keyboard_mapping(m_key_symbols, static_cast<xcb_mapping_notify_event_t *>(ev));
        updateModifiers();
    }
}

QT_END_NAMESPACE
