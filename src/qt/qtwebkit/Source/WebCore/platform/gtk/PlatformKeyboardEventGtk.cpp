/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2008 Collabora, Ltd.  All rights reserved.
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

#include "GtkVersioning.h"
#include "NotImplemented.h"
#include "TextEncoding.h"
#include "WindowsKeyboardCodes.h"

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <wtf/CurrentTime.h>

namespace WebCore {

// FIXME: This is incomplete.  We should change this to mirror
// more like what Firefox does, and generate these switch statements
// at build time.
String PlatformKeyboardEvent::keyIdentifierForGdkKeyCode(unsigned keyCode)
{
    switch (keyCode) {
        case GDK_Menu:
        case GDK_Alt_L:
        case GDK_Alt_R:
            return "Alt";
        case GDK_Clear:
            return "Clear";
        case GDK_Down:
            return "Down";
            // "End"
        case GDK_End:
            return "End";
            // "Enter"
        case GDK_ISO_Enter:
        case GDK_KP_Enter:
        case GDK_Return:
            return "Enter";
        case GDK_Execute:
            return "Execute";
        case GDK_F1:
            return "F1";
        case GDK_F2:
            return "F2";
        case GDK_F3:
            return "F3";
        case GDK_F4:
            return "F4";
        case GDK_F5:
            return "F5";
        case GDK_F6:
            return "F6";
        case GDK_F7:
            return "F7";
        case GDK_F8:
            return "F8";
        case GDK_F9:
            return "F9";
        case GDK_F10:
            return "F10";
        case GDK_F11:
            return "F11";
        case GDK_F12:
            return "F12";
        case GDK_F13:
            return "F13";
        case GDK_F14:
            return "F14";
        case GDK_F15:
            return "F15";
        case GDK_F16:
            return "F16";
        case GDK_F17:
            return "F17";
        case GDK_F18:
            return "F18";
        case GDK_F19:
            return "F19";
        case GDK_F20:
            return "F20";
        case GDK_F21:
            return "F21";
        case GDK_F22:
            return "F22";
        case GDK_F23:
            return "F23";
        case GDK_F24:
            return "F24";
        case GDK_Help:
            return "Help";
        case GDK_Home:
            return "Home";
        case GDK_Insert:
            return "Insert";
        case GDK_Left:
            return "Left";
        case GDK_Page_Down:
            return "PageDown";
        case GDK_Page_Up:
            return "PageUp";
        case GDK_Pause:
            return "Pause";
        case GDK_3270_PrintScreen:
        case GDK_Print:
            return "PrintScreen";
        case GDK_Right:
            return "Right";
        case GDK_Select:
            return "Select";
        case GDK_Up:
            return "Up";
            // Standard says that DEL becomes U+007F.
        case GDK_Delete:
            return "U+007F";
        case GDK_BackSpace:
            return "U+0008";
        case GDK_ISO_Left_Tab:
        case GDK_3270_BackTab:
        case GDK_Tab:
            return "U+0009";
        default:
            return String::format("U+%04X", gdk_keyval_to_unicode(gdk_keyval_to_upper(keyCode)));
    }
}

int PlatformKeyboardEvent::windowsKeyCodeForGdkKeyCode(unsigned keycode)
{
    switch (keycode) {
        case GDK_KP_0:
            return VK_NUMPAD0;// (60) Numeric keypad 0 key
        case GDK_KP_1:
            return VK_NUMPAD1;// (61) Numeric keypad 1 key
        case GDK_KP_2:
            return  VK_NUMPAD2; // (62) Numeric keypad 2 key
        case GDK_KP_3:
            return VK_NUMPAD3; // (63) Numeric keypad 3 key
        case GDK_KP_4:
            return VK_NUMPAD4; // (64) Numeric keypad 4 key
        case GDK_KP_5:
            return VK_NUMPAD5; //(65) Numeric keypad 5 key
        case GDK_KP_6:
            return VK_NUMPAD6; // (66) Numeric keypad 6 key
        case GDK_KP_7:
            return VK_NUMPAD7; // (67) Numeric keypad 7 key
        case GDK_KP_8:
            return VK_NUMPAD8; // (68) Numeric keypad 8 key
        case GDK_KP_9:
            return VK_NUMPAD9; // (69) Numeric keypad 9 key
        case GDK_KP_Multiply:
            return VK_MULTIPLY; // (6A) Multiply key
        case GDK_KP_Add:
            return VK_ADD; // (6B) Add key
        case GDK_KP_Subtract:
            return VK_SUBTRACT; // (6D) Subtract key
        case GDK_KP_Decimal:
            return VK_DECIMAL; // (6E) Decimal key
        case GDK_KP_Divide:
            return VK_DIVIDE; // (6F) Divide key

        case GDK_KP_Page_Up:
            return VK_PRIOR; // (21) PAGE UP key
        case GDK_KP_Page_Down:
            return VK_NEXT; // (22) PAGE DOWN key
        case GDK_KP_End:
            return VK_END; // (23) END key
        case GDK_KP_Home:
            return VK_HOME; // (24) HOME key
        case GDK_KP_Left:
            return VK_LEFT; // (25) LEFT ARROW key
        case GDK_KP_Up:
            return VK_UP; // (26) UP ARROW key
        case GDK_KP_Right:
            return VK_RIGHT; // (27) RIGHT ARROW key
        case GDK_KP_Down:
            return VK_DOWN; // (28) DOWN ARROW key

        case GDK_BackSpace:
            return VK_BACK; // (08) BACKSPACE key
        case GDK_ISO_Left_Tab:
        case GDK_3270_BackTab:
        case GDK_Tab:
            return VK_TAB; // (09) TAB key
        case GDK_Clear:
            return VK_CLEAR; // (0C) CLEAR key
        case GDK_ISO_Enter:
        case GDK_KP_Enter:
        case GDK_Return:
            return VK_RETURN; //(0D) Return key

            // VK_SHIFT (10) SHIFT key
            // VK_CONTROL (11) CTRL key

        case GDK_Menu:
            return VK_APPS;  // (5D) Applications key (Natural keyboard)

            // VK_MENU (12) ALT key

        case GDK_Pause:
            return VK_PAUSE; // (13) PAUSE key
        case GDK_Caps_Lock:
            return VK_CAPITAL; // (14) CAPS LOCK key
        case GDK_Kana_Lock:
        case GDK_Kana_Shift:
            return VK_KANA; // (15) Input Method Editor (IME) Kana mode
        case GDK_Hangul:
            return VK_HANGUL; // VK_HANGUL (15) IME Hangul mode
            // VK_JUNJA (17) IME Junja mode
            // VK_FINAL (18) IME final mode
        case GDK_Hangul_Hanja:
            return VK_HANJA; // (19) IME Hanja mode
        case GDK_Kanji:
            return VK_KANJI; // (19) IME Kanji mode
        case GDK_Escape:
            return VK_ESCAPE; // (1B) ESC key
            // VK_CONVERT (1C) IME convert
            // VK_NONCONVERT (1D) IME nonconvert
            // VK_ACCEPT (1E) IME accept
            // VK_MODECHANGE (1F) IME mode change request
        case GDK_space:
            return VK_SPACE; // (20) SPACEBAR
        case GDK_Page_Up:
            return VK_PRIOR; // (21) PAGE UP key
        case GDK_Page_Down:
            return VK_NEXT; // (22) PAGE DOWN key
        case GDK_End:
            return VK_END; // (23) END key
        case GDK_Home:
            return VK_HOME; // (24) HOME key
        case GDK_Left:
            return VK_LEFT; // (25) LEFT ARROW key
        case GDK_Up:
            return VK_UP; // (26) UP ARROW key
        case GDK_Right:
            return VK_RIGHT; // (27) RIGHT ARROW key
        case GDK_Down:
            return VK_DOWN; // (28) DOWN ARROW key
        case GDK_Select:
            return VK_SELECT; // (29) SELECT key
        case GDK_Print:
            return VK_SNAPSHOT; // (2C) PRINT SCREEN key
        case GDK_Execute:
            return VK_EXECUTE;// (2B) EXECUTE key
        case GDK_Insert:
        case GDK_KP_Insert:
            return VK_INSERT; // (2D) INS key
        case GDK_Delete:
        case GDK_KP_Delete:
            return VK_DELETE; // (2E) DEL key
        case GDK_Help:
            return VK_HELP; // (2F) HELP key
        case GDK_0:
        case GDK_parenright:
            return VK_0;    //  (30) 0) key
        case GDK_1:
        case GDK_exclam:
            return VK_1; //  (31) 1 ! key
        case GDK_2:
        case GDK_at:
            return VK_2; //  (32) 2 & key
        case GDK_3:
        case GDK_numbersign:
            return VK_3; //case '3': case '#';
        case GDK_4:
        case GDK_dollar: //  (34) 4 key '$';
            return VK_4;
        case GDK_5:
        case GDK_percent:
            return VK_5; //  (35) 5 key  '%'
        case GDK_6:
        case GDK_asciicircum:
            return VK_6; //  (36) 6 key  '^'
        case GDK_7:
        case GDK_ampersand:
            return VK_7; //  (37) 7 key  case '&'
        case GDK_8:
        case GDK_asterisk:
            return VK_8; //  (38) 8 key  '*'
        case GDK_9:
        case GDK_parenleft:
            return VK_9; //  (39) 9 key '('
        case GDK_a:
        case GDK_A:
            return VK_A; //  (41) A key case 'a': case 'A': return 0x41;
        case GDK_b:
        case GDK_B:
            return VK_B; //  (42) B key case 'b': case 'B': return 0x42;
        case GDK_c:
        case GDK_C:
            return VK_C; //  (43) C key case 'c': case 'C': return 0x43;
        case GDK_d:
        case GDK_D:
            return VK_D; //  (44) D key case 'd': case 'D': return 0x44;
        case GDK_e:
        case GDK_E:
            return VK_E; //  (45) E key case 'e': case 'E': return 0x45;
        case GDK_f:
        case GDK_F:
            return VK_F; //  (46) F key case 'f': case 'F': return 0x46;
        case GDK_g:
        case GDK_G:
            return VK_G; //  (47) G key case 'g': case 'G': return 0x47;
        case GDK_h:
        case GDK_H:
            return VK_H; //  (48) H key case 'h': case 'H': return 0x48;
        case GDK_i:
        case GDK_I:
            return VK_I; //  (49) I key case 'i': case 'I': return 0x49;
        case GDK_j:
        case GDK_J:
            return VK_J; //  (4A) J key case 'j': case 'J': return 0x4A;
        case GDK_k:
        case GDK_K:
            return VK_K; //  (4B) K key case 'k': case 'K': return 0x4B;
        case GDK_l:
        case GDK_L:
            return VK_L; //  (4C) L key case 'l': case 'L': return 0x4C;
        case GDK_m:
        case GDK_M:
            return VK_M; //  (4D) M key case 'm': case 'M': return 0x4D;
        case GDK_n:
        case GDK_N:
            return VK_N; //  (4E) N key case 'n': case 'N': return 0x4E;
        case GDK_o:
        case GDK_O:
            return VK_O; //  (4F) O key case 'o': case 'O': return 0x4F;
        case GDK_p:
        case GDK_P:
            return VK_P; //  (50) P key case 'p': case 'P': return 0x50;
        case GDK_q:
        case GDK_Q:
            return VK_Q; //  (51) Q key case 'q': case 'Q': return 0x51;
        case GDK_r:
        case GDK_R:
            return VK_R; //  (52) R key case 'r': case 'R': return 0x52;
        case GDK_s:
        case GDK_S:
            return VK_S; //  (53) S key case 's': case 'S': return 0x53;
        case GDK_t:
        case GDK_T:
            return VK_T; //  (54) T key case 't': case 'T': return 0x54;
        case GDK_u:
        case GDK_U:
            return VK_U; //  (55) U key case 'u': case 'U': return 0x55;
        case GDK_v:
        case GDK_V:
            return VK_V; //  (56) V key case 'v': case 'V': return 0x56;
        case GDK_w:
        case GDK_W:
            return VK_W; //  (57) W key case 'w': case 'W': return 0x57;
        case GDK_x:
        case GDK_X:
            return VK_X; //  (58) X key case 'x': case 'X': return 0x58;
        case GDK_y:
        case GDK_Y:
            return VK_Y; //  (59) Y key case 'y': case 'Y': return 0x59;
        case GDK_z:
        case GDK_Z:
            return VK_Z; //  (5A) Z key case 'z': case 'Z': return 0x5A;
        case GDK_Meta_L:
            return VK_LWIN; // (5B) Left Windows key (Microsoft Natural keyboard)
        case GDK_Meta_R:
            return VK_RWIN; // (5C) Right Windows key (Natural keyboard)
            // VK_SLEEP (5F) Computer Sleep key
            // VK_SEPARATOR (6C) Separator key
            // VK_SUBTRACT (6D) Subtract key
            // VK_DECIMAL (6E) Decimal key
            // VK_DIVIDE (6F) Divide key
            // handled by key code above

        case GDK_Num_Lock:
            return VK_NUMLOCK; // (90) NUM LOCK key

        case GDK_Scroll_Lock:
            return VK_SCROLL; // (91) SCROLL LOCK key

        case GDK_Shift_L:
            return VK_LSHIFT; // (A0) Left SHIFT key
        case GDK_Shift_R:
            return VK_RSHIFT; // (A1) Right SHIFT key
        case GDK_Control_L:
            return VK_LCONTROL; // (A2) Left CONTROL key
        case GDK_Control_R:
            return VK_RCONTROL; // (A3) Right CONTROL key
        case GDK_Alt_L:
            return VK_LMENU; // (A4) Left MENU key
        case GDK_Alt_R:
            return VK_RMENU; // (A5) Right MENU key

            // VK_BROWSER_BACK (A6) Windows 2000/XP: Browser Back key
            // VK_BROWSER_FORWARD (A7) Windows 2000/XP: Browser Forward key
            // VK_BROWSER_REFRESH (A8) Windows 2000/XP: Browser Refresh key
            // VK_BROWSER_STOP (A9) Windows 2000/XP: Browser Stop key
            // VK_BROWSER_SEARCH (AA) Windows 2000/XP: Browser Search key
            // VK_BROWSER_FAVORITES (AB) Windows 2000/XP: Browser Favorites key
            // VK_BROWSER_HOME (AC) Windows 2000/XP: Browser Start and Home key
            // VK_VOLUME_MUTE (AD) Windows 2000/XP: Volume Mute key
            // VK_VOLUME_DOWN (AE) Windows 2000/XP: Volume Down key
            // VK_VOLUME_UP (AF) Windows 2000/XP: Volume Up key
            // VK_MEDIA_NEXT_TRACK (B0) Windows 2000/XP: Next Track key
            // VK_MEDIA_PREV_TRACK (B1) Windows 2000/XP: Previous Track key
            // VK_MEDIA_STOP (B2) Windows 2000/XP: Stop Media key
            // VK_MEDIA_PLAY_PAUSE (B3) Windows 2000/XP: Play/Pause Media key
            // VK_LAUNCH_MAIL (B4) Windows 2000/XP: Start Mail key
            // VK_LAUNCH_MEDIA_SELECT (B5) Windows 2000/XP: Select Media key
            // VK_LAUNCH_APP1 (B6) Windows 2000/XP: Start Application 1 key
            // VK_LAUNCH_APP2 (B7) Windows 2000/XP: Start Application 2 key

            // VK_OEM_1 (BA) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the ';:' key
        case GDK_semicolon:
        case GDK_colon:
            return VK_OEM_1; //case ';': case ':': return 0xBA;
            // VK_OEM_PLUS (BB) Windows 2000/XP: For any country/region, the '+' key
        case GDK_plus:
        case GDK_equal:
            return VK_OEM_PLUS; //case '=': case '+': return 0xBB;
            // VK_OEM_COMMA (BC) Windows 2000/XP: For any country/region, the ',' key
        case GDK_comma:
        case GDK_less:
            return VK_OEM_COMMA; //case ',': case '<': return 0xBC;
            // VK_OEM_MINUS (BD) Windows 2000/XP: For any country/region, the '-' key
        case GDK_minus:
        case GDK_underscore:
            return VK_OEM_MINUS; //case '-': case '_': return 0xBD;
            // VK_OEM_PERIOD (BE) Windows 2000/XP: For any country/region, the '.' key
        case GDK_period:
        case GDK_greater:
            return VK_OEM_PERIOD; //case '.': case '>': return 0xBE;
            // VK_OEM_2 (BF) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '/?' key
        case GDK_slash:
        case GDK_question:
            return VK_OEM_2; //case '/': case '?': return 0xBF;
            // VK_OEM_3 (C0) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '`~' key
        case GDK_asciitilde:
        case GDK_quoteleft:
            return VK_OEM_3; //case '`': case '~': return 0xC0;
            // VK_OEM_4 (DB) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '[{' key
        case GDK_bracketleft:
        case GDK_braceleft:
            return VK_OEM_4; //case '[': case '{': return 0xDB;
            // VK_OEM_5 (DC) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '\|' key
        case GDK_backslash:
        case GDK_bar:
            return VK_OEM_5; //case '\\': case '|': return 0xDC;
            // VK_OEM_6 (DD) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the ']}' key
        case GDK_bracketright:
        case GDK_braceright:
            return VK_OEM_6; // case ']': case '}': return 0xDD;
            // VK_OEM_7 (DE) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the 'single-quote/double-quote' key
        case GDK_quoteright:
        case GDK_quotedbl:
            return VK_OEM_7; // case '\'': case '"': return 0xDE;
            // VK_OEM_8 (DF) Used for miscellaneous characters; it can vary by keyboard.
            // VK_OEM_102 (E2) Windows 2000/XP: Either the angle bracket key or the backslash key on the RT 102-key keyboard
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
        case GDK_F1:
        case GDK_F2:
        case GDK_F3:
        case GDK_F4:
        case GDK_F5:
        case GDK_F6:
        case GDK_F7:
        case GDK_F8:
        case GDK_F9:
        case GDK_F10:
        case GDK_F11:
        case GDK_F12:
        case GDK_F13:
        case GDK_F14:
        case GDK_F15:
        case GDK_F16:
        case GDK_F17:
        case GDK_F18:
        case GDK_F19:
        case GDK_F20:
        case GDK_F21:
        case GDK_F22:
        case GDK_F23:
        case GDK_F24:
            return VK_F1 + (keycode - GDK_F1);
        case GDK_KEY_VoidSymbol:
            return VK_PROCESSKEY;
        default:
            return 0;
    }

}

String PlatformKeyboardEvent::singleCharacterString(unsigned val)
{
    switch (val) {
        case GDK_ISO_Enter:
        case GDK_KP_Enter:
        case GDK_Return:
            return String("\r");
        case GDK_BackSpace:
            return String("\x8");
        case GDK_Tab:
            return String("\t");
        default:
            gunichar c = gdk_keyval_to_unicode(val);
            glong nwc;
            gunichar2* uchar16 = g_ucs4_to_utf16(&c, 1, 0, &nwc, 0);

            String retVal;
            if (uchar16)
                retVal = String((UChar*)uchar16, nwc);
            else
                retVal = String();

            g_free(uchar16);

            return retVal;
    }
}

static PlatformEvent::Type eventTypeForGdkKeyEvent(GdkEventKey* event)
{
    return event->type == GDK_KEY_RELEASE ? PlatformEvent::KeyUp : PlatformEvent::KeyDown;
}

static PlatformEvent::Modifiers modifiersForGdkKeyEvent(GdkEventKey* event)
{
    unsigned int modifiers = 0;
    if (event->state & GDK_SHIFT_MASK || event->keyval == GDK_3270_BackTab)
        modifiers |= PlatformEvent::ShiftKey;
    if (event->state & GDK_CONTROL_MASK)
        modifiers |= PlatformEvent::CtrlKey;
    if (event->state & GDK_MOD1_MASK)
        modifiers |= PlatformEvent::AltKey;
    if (event->state & GDK_META_MASK)
        modifiers |= PlatformEvent::MetaKey;
    return static_cast<PlatformEvent::Modifiers>(modifiers);
}

// Keep this in sync with the other platform event constructors
PlatformKeyboardEvent::PlatformKeyboardEvent(GdkEventKey* event, const CompositionResults& compositionResults)
    : PlatformEvent(eventTypeForGdkKeyEvent(event), modifiersForGdkKeyEvent(event), currentTime())
    , m_text(compositionResults.simpleString.length() ? compositionResults.simpleString : singleCharacterString(event->keyval))
    , m_unmodifiedText(m_text)
    , m_keyIdentifier(keyIdentifierForGdkKeyCode(event->keyval))
    , m_windowsVirtualKeyCode(windowsKeyCodeForGdkKeyCode(event->keyval))
    , m_nativeVirtualKeyCode(event->keyval)
    , m_macCharCode(0)
    , m_autoRepeat(false)
    , m_isKeypad(event->keyval >= GDK_KP_Space && event->keyval <= GDK_KP_9)
    , m_isSystemKey(false)
    , m_gdkEventKey(event)
    , m_compositionResults(compositionResults)
{
    // To match the behavior of IE, we return VK_PROCESSKEY for keys that triggered composition results.
    if (compositionResults.compositionUpdated())
        m_windowsVirtualKeyCode = VK_PROCESSKEY;
}

void PlatformKeyboardEvent::disambiguateKeyDownEvent(Type type, bool backwardCompatibilityMode)
{
    // Can only change type from KeyDown to RawKeyDown or Char, as we lack information for other conversions.
    ASSERT(m_type == KeyDown);
    m_type = type;

    if (backwardCompatibilityMode)
        return;

    if (type == PlatformEvent::RawKeyDown) {
        m_text = String();
        m_unmodifiedText = String();
    } else if (type == PlatformEvent::Char && m_compositionResults.compositionUpdated()) {
        // Having empty text, prevents this Char (which is a DOM keypress) event
        // from going to the DOM. Keys that trigger composition events should not
        // fire keypress.
        m_text = String();
        m_unmodifiedText = String();
    } else {
        m_keyIdentifier = String();
        m_windowsVirtualKeyCode = 0;
    }
}

bool PlatformKeyboardEvent::currentCapsLockState()
{
    return gdk_keymap_get_caps_lock_state(gdk_keymap_get_default());
}

void PlatformKeyboardEvent::getCurrentModifierState(bool& shiftKey, bool& ctrlKey, bool& altKey, bool& metaKey)
{
    GdkModifierType state;
    gtk_get_current_event_state(&state);

    shiftKey = state & GDK_SHIFT_MASK;
    ctrlKey = state & GDK_CONTROL_MASK;
    altKey = state & GDK_MOD1_MASK;
    metaKey = state & GDK_META_MASK;
}

}
