/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PlatformKeyboardEvent.h"

#include "NotImplemented.h"
#include "WindowsKeyboardCodes.h"

#include <QKeyEvent>
#include <ctype.h>
#include <wtf/CurrentTime.h>

namespace WebCore {

String keyIdentifierForQtKeyCode(int keyCode)
{
    // Based on http://www.w3.org/TR/DOM-Level-3-Events/#key-values-list
    switch (keyCode) {
    case Qt::Key_unknown:
        return ASCIILiteral("Unidentified");
    case Qt::Key_Alt:
        return ASCIILiteral("Alt");
    case Qt::Key_AltGr:
        return ASCIILiteral("AltGraph");
    case Qt::Key_Control:
        return ASCIILiteral("Control");
    case Qt::Key_Meta:
        return ASCIILiteral("Meta");
    case Qt::Key_Shift:
        return ASCIILiteral("Shift");
    case Qt::Key_Multi_key:
        return ASCIILiteral("Compose");
    case Qt::Key_F1:
        return ASCIILiteral("F1");
    case Qt::Key_F2:
        return ASCIILiteral("F2");
    case Qt::Key_F3:
        return ASCIILiteral("F3");
    case Qt::Key_F4:
        return ASCIILiteral("F4");
    case Qt::Key_F5:
        return ASCIILiteral("F5");
    case Qt::Key_F6:
        return ASCIILiteral("F6");
    case Qt::Key_F7:
        return ASCIILiteral("F7");
    case Qt::Key_F8:
        return ASCIILiteral("F8");
    case Qt::Key_F9:
        return ASCIILiteral("F9");
    case Qt::Key_F10:
        return ASCIILiteral("F10");
    case Qt::Key_F11:
        return ASCIILiteral("F11");
    case Qt::Key_F12:
        return ASCIILiteral("F12");
    case Qt::Key_F13:
        return ASCIILiteral("F13");
    case Qt::Key_F14:
        return ASCIILiteral("F14");
    case Qt::Key_F15:
        return ASCIILiteral("F15");
    case Qt::Key_F16:
        return ASCIILiteral("F16");
    case Qt::Key_F17:
        return ASCIILiteral("F17");
    case Qt::Key_F18:
        return ASCIILiteral("F18");
    case Qt::Key_F19:
        return ASCIILiteral("F19");
    case Qt::Key_F20:
        return ASCIILiteral("F20");
    case Qt::Key_F21:
        return ASCIILiteral("F21");
    case Qt::Key_F22:
        return ASCIILiteral("F22");
    case Qt::Key_F23:
        return ASCIILiteral("F23");
    case Qt::Key_F24:
        return ASCIILiteral("F24");
    // 'LaunchApplication1'
    // 'LaunchApplication2'
    case Qt::Key_LaunchMail:
        return ASCIILiteral("LaunchMail");
    // 'List'
    // 'Props'
    // 'Soft1'
    // 'Soft2'
    // 'Soft3'
    // 'Soft4'
    case Qt::Key_Yes:
        return ASCIILiteral("Accept");
    // 'Again'
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return ASCIILiteral("Enter");
    // 'Find'
    case Qt::Key_Help:
        return ASCIILiteral("Help");
    // 'Info'
    case Qt::Key_Menu:
    case Qt::Key_MenuKB:
        return ASCIILiteral("Menu");
    case Qt::Key_Pause:
        return ASCIILiteral("Pause");
    case Qt::Key_Play:
        return ASCIILiteral("Play");
    case Qt::Key_Execute:
        return ASCIILiteral("Execute");
    case Qt::Key_Cancel:
        return ASCIILiteral("Cancel");
    case Qt::Key_Escape:
        return ASCIILiteral("Esc");
    // 'Exit'
    case Qt::Key_Zoom:
        return ASCIILiteral("Zoom");
    case Qt::Key_Comma:
        return ASCIILiteral("Seperator");
    case Qt::Key_Plus:
        return ASCIILiteral("Add");
    case Qt::Key_Minus:
        return ASCIILiteral("Subtract");
    case Qt::Key_Asterisk:
        return ASCIILiteral("Multiply");
    case Qt::Key_Slash:
        return ASCIILiteral("Divide");
    case Qt::Key_Equal:
        return ASCIILiteral("Equals");
    case Qt::Key_Period:
        return ASCIILiteral("Decimal");
    case Qt::Key_MonBrightnessDown:
        return ASCIILiteral("BrightnessDown");
    case Qt::Key_MonBrightnessUp:
        return ASCIILiteral("BrightnessUp");
    case Qt::Key_Camera:
        return ASCIILiteral("Camera");
    case Qt::Key_Eject:
        return ASCIILiteral("Eject");
    case Qt::Key_PowerDown:
    case Qt::Key_PowerOff:
        return ASCIILiteral("Power");
    case Qt::Key_Print:
        return ASCIILiteral("PrintScreen");
    case Qt::Key_Favorites:
        return ASCIILiteral("BrowserFavorites");
    case Qt::Key_HomePage:
        return ASCIILiteral("BrowserHome");
    case Qt::Key_Refresh:
        return ASCIILiteral("BrowserRefresh");
    case Qt::Key_Search:
        return ASCIILiteral("BrowserSearch");
    case Qt::Key_Stop:
        return ASCIILiteral("BrowserStop");
    case Qt::Key_Back:
        return ASCIILiteral("BrowserBack");
    case Qt::Key_Forward:
        return ASCIILiteral("BrowserForward");
    case Qt::Key_Left:
        return ASCIILiteral("Left");
    case Qt::Key_PageDown:
        return ASCIILiteral("PageDown");
    case Qt::Key_PageUp:
        return ASCIILiteral("PageUp");
    case Qt::Key_Right:
        return ASCIILiteral("Right");
    case Qt::Key_Up:
        return ASCIILiteral("Up");
    // 'UpLeft'
    // 'UpRight'
    case Qt::Key_Down:
        return ASCIILiteral("Down");
    // 'DownLeft'
    // 'DownRight'
    case Qt::Key_Home:
        return ASCIILiteral("Home");
    case Qt::Key_End:
        return ASCIILiteral("End");
    case Qt::Key_Select:
        return ASCIILiteral("Select");
    case Qt::Key_Clear:
        return ASCIILiteral("Clear");
    case Qt::Key_Copy:
        return ASCIILiteral("Copy");
    case Qt::Key_Cut:
        return ASCIILiteral("Cut");
    // 'EraseEof'
    case Qt::Key_Insert:
        return ASCIILiteral("Insert");
    case Qt::Key_Paste:
        return ASCIILiteral("Paste");
    // 'Undo'
    case Qt::Key_Dead_Grave:
        return ASCIILiteral("DeadGrave");
    case Qt::Key_Dead_Acute:
        return ASCIILiteral("DeadAcute");
    case Qt::Key_Dead_Circumflex:
        return ASCIILiteral("DeadCircumflex");
    case Qt::Key_Dead_Tilde:
        return ASCIILiteral("DeadTilde");
    case Qt::Key_Dead_Macron:
        return ASCIILiteral("DeadMacron");
    case Qt::Key_Dead_Breve:
        return ASCIILiteral("DeadBreve");
    case Qt::Key_Dead_Abovedot:
        return ASCIILiteral("DeadAboveDot");
    case Qt::Key_Dead_Diaeresis:
        return ASCIILiteral("DeadUmlaut");
    case Qt::Key_Dead_Abovering:
        return ASCIILiteral("DeadAboveRing");
    case Qt::Key_Dead_Doubleacute:
        return ASCIILiteral("DeadDoubleAcute");
    case Qt::Key_Dead_Caron:
        return ASCIILiteral("DeadCaron");
    case Qt::Key_Dead_Cedilla:
        return ASCIILiteral("DeadCedilla");
    case Qt::Key_Dead_Ogonek:
        return ASCIILiteral("DeadOgonek");
    case Qt::Key_Dead_Iota:
        return ASCIILiteral("DeadIota");
    case Qt::Key_Dead_Voiced_Sound:
        return ASCIILiteral("DeadVoicedSound");
    case Qt::Key_Dead_Semivoiced_Sound:
        return ASCIILiteral("DeadSemivoicedSound");

    case Qt::Key_MultipleCandidate:
        return ASCIILiteral("AllCandidate");
    case Qt::Key_SingleCandidate:
        return ASCIILiteral("NextCandidate");
    case Qt::Key_PreviousCandidate:
        return ASCIILiteral("PreviousCandidate");
    case Qt::Key_Codeinput:
        return ASCIILiteral("CodeInput");

    case Qt::Key_Mode_switch:
        return ASCIILiteral("ModeChange");
    case Qt::Key_Hangul:
        return ASCIILiteral("HangulMode");
    case Qt::Key_Hangul_Hanja:
        return ASCIILiteral("HanjaMode");
    case Qt::Key_Hiragana:
        return ASCIILiteral("Hiragana");
    case Qt::Key_Kana_Lock:
    case Qt::Key_Kana_Shift:
        return ASCIILiteral("KanaMode");
    case Qt::Key_Kanji:
        return ASCIILiteral("KanjiMode");
    case Qt::Key_Katakana:
        return ASCIILiteral("Katakana");

    // 'AudioFaderFront'
    // 'AudioFaderRear'
    // 'AudioBalanceLeft'
    // 'AudioBalanceRight'
    case Qt::Key_BassDown:
        return ASCIILiteral("AudioBassBoostDown");
    case Qt::Key_BassUp:
        return ASCIILiteral("AudioBassBoostUp");
    case Qt::Key_VolumeMute:
        return ASCIILiteral("VolumeMute");
    case Qt::Key_VolumeDown:
        return ASCIILiteral("VolumeDown");
    case Qt::Key_VolumeUp:
        return ASCIILiteral("VolumeUp");
    case Qt::Key_MediaPause:
        return ASCIILiteral("MediaPause");
    case Qt::Key_MediaPlay:
        return ASCIILiteral("MediaPlay");
    // 'MediaTrackEnd'
    case Qt::Key_MediaNext:
        return ASCIILiteral("MediaNextTrack");
    case Qt::Key_MediaTogglePlayPause:
        return ASCIILiteral("MediaPlayPause");
    case Qt::Key_MediaPrevious:
        return ASCIILiteral("MediaPreviousTrack");
    // 'MediaTrackSkip'
    // 'MediaTrackStart'
    case Qt::Key_MediaStop:
        return ASCIILiteral("MediaStop");
    case Qt::Key_LaunchMedia:
        return ASCIILiteral("SelectMedia");

    case Qt::Key_AudioForward:
        return ASCIILiteral("FastFwd");
    case Qt::Key_MediaRecord:
        return ASCIILiteral("MediaRecord");
    case Qt::Key_AudioRewind:
        return ASCIILiteral("MediaRewind");
    case Qt::Key_Subtitle:
        return ASCIILiteral("Subtitle");
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    case Qt::Key_Blue:
        return ASCIILiteral("Blue");
    case Qt::Key_ChannelDown:
        return ASCIILiteral("ChannelDown");
    case Qt::Key_ChannelUp:
        return ASCIILiteral("ChannelUp");
    case Qt::Key_Green:
        return ASCIILiteral("Green");
    case Qt::Key_Red:
        return ASCIILiteral("Red");
    case Qt::Key_Yellow:
        return ASCIILiteral("Yellow");
#endif
    // Keys we have returned U+charcode for in the past.
    // FIXME: Change them to correct standard values if others do.
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
        return ASCIILiteral("U+0009"); // return ASCIILiteral("Tab");
    case Qt::Key_Backspace:
        return ASCIILiteral("U+0008"); // return ASCIILiteral("Backspace");
    case Qt::Key_Delete:
        return ASCIILiteral("U+007F"); // return ASCIILiteral("Del");
    default:
        if (keyCode < 128)
            return String::format("U+%04X", toASCIIUpper(keyCode));
        return String();
    }
}

int windowsKeyCodeForKeyEvent(unsigned int keycode, bool isKeypad)
{
    // Determine wheter the event comes from the keypad
    if (isKeypad) {
        switch (keycode) {
        case Qt::Key_0:
            return VK_NUMPAD0; // (60) Numeric keypad 0 key
        case Qt::Key_1:
            return VK_NUMPAD1; // (61) Numeric keypad 1 key
        case Qt::Key_2:
            return  VK_NUMPAD2; // (62) Numeric keypad 2 key
        case Qt::Key_3:
            return VK_NUMPAD3; // (63) Numeric keypad 3 key
        case Qt::Key_4:
            return VK_NUMPAD4; // (64) Numeric keypad 4 key
        case Qt::Key_5:
            return VK_NUMPAD5; // (65) Numeric keypad 5 key
        case Qt::Key_6:
            return VK_NUMPAD6; // (66) Numeric keypad 6 key
        case Qt::Key_7:
            return VK_NUMPAD7; // (67) Numeric keypad 7 key
        case Qt::Key_8:
            return VK_NUMPAD8; // (68) Numeric keypad 8 key
        case Qt::Key_9:
            return VK_NUMPAD9; // (69) Numeric keypad 9 key
        case Qt::Key_Asterisk:
            return VK_MULTIPLY; // (6A) Multiply key
        case Qt::Key_Plus:
            return VK_ADD; // (6B) Add key
        case Qt::Key_Minus:
            return VK_SUBTRACT; // (6D) Subtract key
        case Qt::Key_Period:
            return VK_DECIMAL; // (6E) Decimal key
        case Qt::Key_Slash:
            return VK_DIVIDE; // (6F) Divide key
        case Qt::Key_PageUp:
            return VK_PRIOR; // (21) PAGE UP key
        case Qt::Key_PageDown:
            return VK_NEXT; // (22) PAGE DOWN key
        case Qt::Key_End:
            return VK_END; // (23) END key
        case Qt::Key_Home:
            return VK_HOME; // (24) HOME key
        case Qt::Key_Left:
            return VK_LEFT; // (25) LEFT ARROW key
        case Qt::Key_Up:
            return VK_UP; // (26) UP ARROW key
        case Qt::Key_Right:
            return VK_RIGHT; // (27) RIGHT ARROW key
        case Qt::Key_Down:
            return VK_DOWN; // (28) DOWN ARROW key
        case Qt::Key_Enter:
        case Qt::Key_Return:
            return VK_RETURN; // (0D) Return key
        case Qt::Key_Insert:
            return VK_INSERT; // (2D) INS key
        case Qt::Key_Delete:
            return VK_DELETE; // (2E) DEL key
        default:
            return 0;
        }

    } else

    switch (keycode) {
    case Qt::Key_Backspace:
        return VK_BACK; // (08) BACKSPACE key
    case Qt::Key_Backtab:
    case Qt::Key_Tab:
        return VK_TAB; // (09) TAB key
    case Qt::Key_Clear:
        return VK_CLEAR; // (0C) CLEAR key
    case Qt::Key_Enter:
    case Qt::Key_Return:
        return VK_RETURN; // (0D) Return key
    case Qt::Key_Shift:
        return VK_SHIFT; // (10) SHIFT key
    case Qt::Key_Control:
        return VK_CONTROL; // (11) CTRL key
    case Qt::Key_Alt:
        return VK_MENU; // (12) ALT key

    case Qt::Key_F1:
        return VK_F1;
    case Qt::Key_F2:
        return VK_F2;
    case Qt::Key_F3:
        return VK_F3;
    case Qt::Key_F4:
        return VK_F4;
    case Qt::Key_F5:
        return VK_F5;
    case Qt::Key_F6:
        return VK_F6;
    case Qt::Key_F7:
        return VK_F7;
    case Qt::Key_F8:
        return VK_F8;
    case Qt::Key_F9:
        return VK_F9;
    case Qt::Key_F10:
        return VK_F10;
    case Qt::Key_F11:
        return VK_F11;
    case Qt::Key_F12:
        return VK_F12;
    case Qt::Key_F13:
        return VK_F13;
    case Qt::Key_F14:
        return VK_F14;
    case Qt::Key_F15:
        return VK_F15;
    case Qt::Key_F16:
        return VK_F16;
    case Qt::Key_F17:
        return VK_F17;
    case Qt::Key_F18:
        return VK_F18;
    case Qt::Key_F19:
        return VK_F19;
    case Qt::Key_F20:
        return VK_F20;
    case Qt::Key_F21:
        return VK_F21;
    case Qt::Key_F22:
        return VK_F22;
    case Qt::Key_F23:
        return VK_F23;
    case Qt::Key_F24:
        return VK_F24;

    case Qt::Key_Pause:
        return VK_PAUSE; // (13) PAUSE key
    case Qt::Key_CapsLock:
        return VK_CAPITAL; // (14) CAPS LOCK key
    case Qt::Key_Kana_Lock:
    case Qt::Key_Kana_Shift:
        return VK_KANA; // (15) Input Method Editor (IME) Kana mode
    case Qt::Key_Hangul:
        return VK_HANGUL; // VK_HANGUL (15) IME Hangul mode
        // VK_JUNJA (17) IME Junja mode
        // VK_FINAL (18) IME final mode
    case Qt::Key_Hangul_Hanja:
        return VK_HANJA; // (19) IME Hanja mode
    case Qt::Key_Kanji:
        return VK_KANJI; // (19) IME Kanji mode
    case Qt::Key_Escape:
        return VK_ESCAPE; // (1B) ESC key
        // VK_CONVERT (1C) IME convert
        // VK_NONCONVERT (1D) IME nonconvert
        // VK_ACCEPT (1E) IME accept
        // VK_MODECHANGE (1F) IME mode change request
    case Qt::Key_Space:
        return VK_SPACE; // (20) SPACEBAR
    case Qt::Key_PageUp:
        return VK_PRIOR; // (21) PAGE UP key
    case Qt::Key_PageDown:
        return VK_NEXT; // (22) PAGE DOWN key
    case Qt::Key_End:
        return VK_END; // (23) END key
    case Qt::Key_Home:
        return VK_HOME; // (24) HOME key
    case Qt::Key_Left:
        return VK_LEFT; // (25) LEFT ARROW key
    case Qt::Key_Up:
        return VK_UP; // (26) UP ARROW key
    case Qt::Key_Right:
        return VK_RIGHT; // (27) RIGHT ARROW key
    case Qt::Key_Down:
        return VK_DOWN; // (28) DOWN ARROW key
    case Qt::Key_Select:
        return VK_SELECT; // (29) SELECT key
    case Qt::Key_Print:
        return VK_SNAPSHOT; // (2A) PRINT key
    case Qt::Key_Execute:
        return VK_EXECUTE; // (2B) EXECUTE key
    case Qt::Key_Insert:
        return VK_INSERT; // (2D) INS key
    case Qt::Key_Delete:
        return VK_DELETE; // (2E) DEL key
    case Qt::Key_Help:
        return VK_HELP; // (2F) HELP key
    case Qt::Key_0:
    case Qt::Key_ParenRight:
        return VK_0; // (30) 0) key
    case Qt::Key_1:
    case Qt::Key_Exclam:
        return VK_1; // (31) 1 ! key
    case Qt::Key_2:
    case Qt::Key_At:
        return VK_2; // (32) 2 & key
    case Qt::Key_3:
    case Qt::Key_NumberSign:
        return VK_3; // case '3': case '#';
    case Qt::Key_4:
    case Qt::Key_Dollar: // (34) 4 key '$';
        return VK_4;
    case Qt::Key_5:
    case Qt::Key_Percent:
        return VK_5; // (35) 5 key  '%'
    case Qt::Key_6:
    case Qt::Key_AsciiCircum:
        return VK_6; // (36) 6 key  '^'
    case Qt::Key_7:
    case Qt::Key_Ampersand:
        return VK_7; // (37) 7 key  case '&'
    case Qt::Key_8:
    case Qt::Key_Asterisk:
        return VK_8; // (38) 8 key  '*'
    case Qt::Key_9:
    case Qt::Key_ParenLeft:
        return VK_9; // (39) 9 key '('
    case Qt::Key_A:
        return VK_A; // (41) A key case 'a': case 'A': return 0x41;
    case Qt::Key_B:
        return VK_B; // (42) B key case 'b': case 'B': return 0x42;
    case Qt::Key_C:
        return VK_C; // (43) C key case 'c': case 'C': return 0x43;
    case Qt::Key_D:
        return VK_D; // (44) D key case 'd': case 'D': return 0x44;
    case Qt::Key_E:
        return VK_E; // (45) E key case 'e': case 'E': return 0x45;
    case Qt::Key_F:
        return VK_F; // (46) F key case 'f': case 'F': return 0x46;
    case Qt::Key_G:
        return VK_G; // (47) G key case 'g': case 'G': return 0x47;
    case Qt::Key_H:
        return VK_H; // (48) H key case 'h': case 'H': return 0x48;
    case Qt::Key_I:
        return VK_I; // (49) I key case 'i': case 'I': return 0x49;
    case Qt::Key_J:
        return VK_J; // (4A) J key case 'j': case 'J': return 0x4A;
    case Qt::Key_K:
        return VK_K; // (4B) K key case 'k': case 'K': return 0x4B;
    case Qt::Key_L:
        return VK_L; // (4C) L key case 'l': case 'L': return 0x4C;
    case Qt::Key_M:
        return VK_M; // (4D) M key case 'm': case 'M': return 0x4D;
    case Qt::Key_N:
        return VK_N; // (4E) N key case 'n': case 'N': return 0x4E;
    case Qt::Key_O:
        return VK_O; // (4F) O key case 'o': case 'O': return 0x4F;
    case Qt::Key_P:
        return VK_P; // (50) P key case 'p': case 'P': return 0x50;
    case Qt::Key_Q:
        return VK_Q; // (51) Q key case 'q': case 'Q': return 0x51;
    case Qt::Key_R:
        return VK_R; // (52) R key case 'r': case 'R': return 0x52;
    case Qt::Key_S:
        return VK_S; // (53) S key case 's': case 'S': return 0x53;
    case Qt::Key_T:
        return VK_T; // (54) T key case 't': case 'T': return 0x54;
    case Qt::Key_U:
        return VK_U; // (55) U key case 'u': case 'U': return 0x55;
    case Qt::Key_V:
        return VK_V; // (56) V key case 'v': case 'V': return 0x56;
    case Qt::Key_W:
        return VK_W; // (57) W key case 'w': case 'W': return 0x57;
    case Qt::Key_X:
        return VK_X; // (58) X key case 'x': case 'X': return 0x58;
    case Qt::Key_Y:
        return VK_Y; // (59) Y key case 'y': case 'Y': return 0x59;
    case Qt::Key_Z:
        return VK_Z; // (5A) Z key case 'z': case 'Z': return 0x5A;
    case Qt::Key_Meta:
        return VK_LWIN; // (5B) Left Windows key (Microsoft Natural keyboard)
        // case Qt::Key_Meta_R: FIXME: What to do here?
        //    return VK_RWIN; // (5C) Right Windows key (Natural keyboard)
    case Qt::Key_Menu:
        return VK_APPS; // (5D) Applications key (Natural keyboard)
        // VK_SLEEP (5F) Computer Sleep key
        // VK_SEPARATOR (6C) Separator key
        // VK_SUBTRACT (6D) Subtract key
        // VK_DECIMAL (6E) Decimal key
        // VK_DIVIDE (6F) Divide key
        // handled by key code above

    case Qt::Key_NumLock:
        return VK_NUMLOCK; // (90) NUM LOCK key

    case Qt::Key_ScrollLock:
        return VK_SCROLL; // (91) SCROLL LOCK key

        // VK_LSHIFT (A0) Left SHIFT key
        // VK_RSHIFT (A1) Right SHIFT key
        // VK_LCONTROL (A2) Left CONTROL key
        // VK_RCONTROL (A3) Right CONTROL key
        // VK_LMENU (A4) Left MENU key
        // VK_RMENU (A5) Right MENU key
        // VK_BROWSER_BACK (A6) Windows 2000/XP: Browser Back key
        // VK_BROWSER_FORWARD (A7) Windows 2000/XP: Browser Forward key
        // VK_BROWSER_REFRESH (A8) Windows 2000/XP: Browser Refresh key
        // VK_BROWSER_STOP (A9) Windows 2000/XP: Browser Stop key
        // VK_BROWSER_SEARCH (AA) Windows 2000/XP: Browser Search key
        // VK_BROWSER_FAVORITES (AB) Windows 2000/XP: Browser Favorites key
        // VK_BROWSER_HOME (AC) Windows 2000/XP: Browser Start and Home key

    case Qt::Key_VolumeMute:
        return VK_VOLUME_MUTE; // (AD) Windows 2000/XP: Volume Mute key
    case Qt::Key_VolumeDown:
        return VK_VOLUME_DOWN; // (AE) Windows 2000/XP: Volume Down key
    case Qt::Key_VolumeUp:
        return VK_VOLUME_UP; // (AF) Windows 2000/XP: Volume Up key
    case Qt::Key_MediaNext:
        return VK_MEDIA_NEXT_TRACK; // (B0) Windows 2000/XP: Next Track key
    case Qt::Key_MediaPrevious:
        return VK_MEDIA_PREV_TRACK; // (B1) Windows 2000/XP: Previous Track key
    case Qt::Key_MediaStop:
        return VK_MEDIA_STOP; // (B2) Windows 2000/XP: Stop Media key
    case Qt::Key_MediaTogglePlayPause:
        return VK_MEDIA_PLAY_PAUSE; // (B3) Windows 2000/XP: Play/Pause Media key

        // VK_LAUNCH_MAIL (B4) Windows 2000/XP: Start Mail key
        // VK_LAUNCH_MEDIA_SELECT (B5) Windows 2000/XP: Select Media key
        // VK_LAUNCH_APP1 (B6) Windows 2000/XP: Start Application 1 key
        // VK_LAUNCH_APP2 (B7) Windows 2000/XP: Start Application 2 key

        // VK_OEM_1 (BA) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the ';:' key
    case Qt::Key_Semicolon:
    case Qt::Key_Colon:
        return VK_OEM_1; // case ';': case ':': return 0xBA;
        // VK_OEM_PLUS (BB) Windows 2000/XP: For any country/region, the '+' key
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        return VK_OEM_PLUS; // case '=': case '+': return 0xBB;
        // VK_OEM_COMMA (BC) Windows 2000/XP: For any country/region, the ',' key
    case Qt::Key_Comma:
    case Qt::Key_Less:
        return VK_OEM_COMMA; // case ',': case '<': return 0xBC;
        // VK_OEM_MINUS (BD) Windows 2000/XP: For any country/region, the '-' key
    case Qt::Key_Minus:
    case Qt::Key_Underscore:
        return VK_OEM_MINUS; // case '-': case '_': return 0xBD;
        // VK_OEM_PERIOD (BE) Windows 2000/XP: For any country/region, the '.' key
    case Qt::Key_Period:
    case Qt::Key_Greater:
        return VK_OEM_PERIOD; // case '.': case '>': return 0xBE;
        // VK_OEM_2 (BF) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '/?' key
    case Qt::Key_Slash:
    case Qt::Key_Question:
        return VK_OEM_2; // case '/': case '?': return 0xBF;
        // VK_OEM_3 (C0) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '`~' key
    case Qt::Key_AsciiTilde:
    case Qt::Key_QuoteLeft:
        return VK_OEM_3; // case '`': case '~': return 0xC0;
        // VK_OEM_4 (DB) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '[{' key
    case Qt::Key_BracketLeft:
    case Qt::Key_BraceLeft:
        return VK_OEM_4; // case '[': case '{': return 0xDB;
        // VK_OEM_5 (DC) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '\|' key
    case Qt::Key_Backslash:
    case Qt::Key_Bar:
        return VK_OEM_5; // case '\\': case '|': return 0xDC;
        // VK_OEM_6 (DD) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the ']}' key
    case Qt::Key_BracketRight:
    case Qt::Key_BraceRight:
        return VK_OEM_6; // case ']': case '}': return 0xDD;
        // VK_OEM_7 (DE) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the 'single-quote/double-quote' key
    case Qt::Key_Apostrophe:
    case Qt::Key_QuoteDbl:
        return VK_OEM_7; // case '\'': case '"': return 0xDE;
        // VK_OEM_8 (DF) Used for miscellaneous characters; it can vary by keyboard.
        // VK_OEM_102 (E2) Windows 2000/XP: Either the angle bracket key or the backslash key on the RT 102-key keyboard

    case Qt::Key_AudioRewind:
        return 0xE3; // (E3) Android/GoogleTV: Rewind media key (Windows: VK_ICO_HELP Help key on 1984 Olivetti M24 deluxe keyboard)
    case Qt::Key_AudioForward:
        return 0xE4; // (E4) Android/GoogleTV: Fast forward media key  (Windows: VK_ICO_00 '00' key on 1984 Olivetti M24 deluxe keyboard)

        // VK_PROCESSKEY (E5) Windows 95/98/Me, Windows NT 4.0, Windows 2000/XP: IME PROCESS key
        // VK_PACKET (E7) Windows 2000/XP: Used to pass Unicode characters as if they were keystrokes. The VK_PACKET key is the low word of a 32-bit Virtual Key value used for non-keyboard input methods. For more information, see Remark in KEYBDINPUT,SendInput, WM_KEYDOWN, and WM_KEYUP
        // VK_ATTN (F6) Attn key
        // VK_CRSEL (F7) CrSel key
        // VK_EXSEL (F8) ExSel key
        // VK_EREOF (F9) Erase EOF key
        // VK_PLAY (FA) Play key
        // VK_ZOOM (FB) Zoom key
        // VK_NONAME (FC) Reserved for future use
        // VK_PA1 (FD) PA1 key
        // VK_OEM_CLEAR (FE) Clear key
    default:
        return 0;
    }
}

static bool isVirtualKeyCodeRepresentingCharacter(int code)
{
    switch (code) {
    case VK_SPACE:
    case VK_0:
    case VK_1:
    case VK_2:
    case VK_3:
    case VK_4:
    case VK_5:
    case VK_6:
    case VK_7:
    case VK_8:
    case VK_9:
    case VK_A:
    case VK_B:
    case VK_C:
    case VK_D:
    case VK_E:
    case VK_F:
    case VK_G:
    case VK_H:
    case VK_I:
    case VK_J:
    case VK_K:
    case VK_L:
    case VK_M:
    case VK_N:
    case VK_O:
    case VK_P:
    case VK_Q:
    case VK_R:
    case VK_S:
    case VK_T:
    case VK_U:
    case VK_V:
    case VK_W:
    case VK_X:
    case VK_Y:
    case VK_Z:
    case VK_NUMPAD0:
    case VK_NUMPAD1:
    case VK_NUMPAD2:
    case VK_NUMPAD3:
    case VK_NUMPAD4:
    case VK_NUMPAD5:
    case VK_NUMPAD6:
    case VK_NUMPAD7:
    case VK_NUMPAD8:
    case VK_NUMPAD9:
    case VK_MULTIPLY:
    case VK_ADD:
    case VK_SEPARATOR:
    case VK_SUBTRACT:
    case VK_DECIMAL:
    case VK_DIVIDE:
    case VK_OEM_1:
    case VK_OEM_PLUS:
    case VK_OEM_COMMA:
    case VK_OEM_MINUS:
    case VK_OEM_PERIOD:
    case VK_OEM_2:
    case VK_OEM_3:
    case VK_OEM_4:
    case VK_OEM_5:
    case VK_OEM_6:
    case VK_OEM_7:
        return true;
    default:
        return false;
    }
}

static String keyTextForKeyEvent(const QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
        if (event->text().isNull())
            return ASCIILiteral("\t");
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (event->text().isNull())
            return ASCIILiteral("\r");
    }
    return event->text();
}

PlatformKeyboardEvent::PlatformKeyboardEvent(QKeyEvent* event, bool useNativeVirtualKeyAsDOMKey)
{
    const int state = event->modifiers();
    m_type = (event->type() == QEvent::KeyRelease) ? PlatformEvent::KeyUp : PlatformEvent::KeyDown;

    m_modifiers = 0;
    if ((state & Qt::ShiftModifier) || event->key() == Qt::Key_Backtab) // Simulate Shift+Tab with Key_Backtab
        m_modifiers |= ShiftKey;
    if (state & Qt::ControlModifier)
        m_modifiers |= CtrlKey;
    if (state & Qt::AltModifier)
        m_modifiers |= AltKey;
    if (state & Qt::MetaModifier)
        m_modifiers |= MetaKey;

    m_useNativeVirtualKeyAsDOMKey = useNativeVirtualKeyAsDOMKey;
    m_text = keyTextForKeyEvent(event);
    m_unmodifiedText = m_text; // FIXME: not correct
    m_keyIdentifier = keyIdentifierForQtKeyCode(event->key());
    m_autoRepeat = event->isAutoRepeat();
    m_isKeypad = (state & Qt::KeypadModifier);
    m_isSystemKey = false;
    m_nativeVirtualKeyCode = event->nativeVirtualKey();
    // If QKeyEvent::nativeVirtualKey() is valid (!=0) and useNativeVirtualKeyAsDOMKey is set,
    // then it is a special case desired by QtWebKit embedder to send domain specific keys
    // to Web Applications intented for platforms like HbbTV,CE-HTML,OIPF,..etc.
    if (useNativeVirtualKeyAsDOMKey && m_nativeVirtualKeyCode)
        m_windowsVirtualKeyCode = m_nativeVirtualKeyCode;
    else
        m_windowsVirtualKeyCode = windowsKeyCodeForKeyEvent(event->key(), m_isKeypad);

    m_macCharCode = 0;
    m_qtEvent = event;
    m_timestamp = WTF::currentTime();
}

void PlatformKeyboardEvent::disambiguateKeyDownEvent(Type type, bool)
{
    // Can only change type from KeyDown to RawKeyDown or Char, as we lack information for other conversions.
    ASSERT(m_type == PlatformEvent::KeyDown);
    m_type = type;

    if (type == PlatformEvent::RawKeyDown) {
        m_text = String();
        m_unmodifiedText = String();
    } else {
        /*
            When we receive shortcut events like Ctrl+V then the text in the QKeyEvent is
            empty. If we're asked to disambiguate the event into a Char keyboard event,
            we try to detect this situation and still set the text, to ensure that the
            general event handling sends a key press event after this disambiguation.
        */
        if (!m_useNativeVirtualKeyAsDOMKey && m_text.isEmpty() && m_windowsVirtualKeyCode && isVirtualKeyCodeRepresentingCharacter(m_windowsVirtualKeyCode))
            m_text.append(UChar(m_windowsVirtualKeyCode));

        m_keyIdentifier = String();
        m_windowsVirtualKeyCode = 0;
    }
}

bool PlatformKeyboardEvent::currentCapsLockState()
{
    notImplemented();
    return false;
}

void PlatformKeyboardEvent::getCurrentModifierState(bool& shiftKey, bool& ctrlKey, bool& altKey, bool& metaKey)
{
    notImplemented();
    shiftKey = false;
    ctrlKey = false;
    altKey = false;
    metaKey = false;
}

uint32_t PlatformKeyboardEvent::nativeModifiers() const
{
    ASSERT(m_qtEvent);
    return m_qtEvent->nativeModifiers();
}

uint32_t PlatformKeyboardEvent::nativeScanCode() const
{
    ASSERT(m_qtEvent);
    return m_qtEvent->nativeScanCode();
}

}

// vim: ts=4 sw=4 et
