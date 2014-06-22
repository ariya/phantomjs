/*
 * Copyright (C) 2004, 2006, 2007, 2008, 2009, 2010 Apple Inc.  All rights reserved.
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

#import "config.h"
#import "KeyEventCocoa.h"

#import "Logging.h"
#import "WindowsKeyboardCodes.h"
#import <wtf/ASCIICType.h>
#import <wtf/text/WTFString.h>

#if PLATFORM(IOS)
#import "KeyEventCodesIOS.h"
#endif

using namespace WTF;

namespace WebCore {

String keyIdentifierForCharCode(unichar charCode)
{
    switch (charCode) {
        // Each identifier listed in the DOM spec is listed here.
        // Many are simply commented out since they do not appear on standard Macintosh keyboards
        // or are on a key that doesn't have a corresponding character.

        // "Accept"
        // "AllCandidates"

        // "Alt"
        case NSMenuFunctionKey:
            return "Alt";

        // "Apps"
        // "BrowserBack"
        // "BrowserForward"
        // "BrowserHome"
        // "BrowserRefresh"
        // "BrowserSearch"
        // "BrowserStop"
        // "CapsLock"

        // "Clear"
        case NSClearLineFunctionKey:
            return "Clear";

        // "CodeInput"
        // "Compose"
        // "Control"
        // "Crsel"
        // "Convert"
        // "Copy"
        // "Cut"

        // "Down"
        case NSDownArrowFunctionKey:
            return "Down";
        // "End"
        case NSEndFunctionKey:
            return "End";
        // "Enter"
        case 0x3: case 0xA: case 0xD: // Macintosh calls the one on the main keyboard Return, but Windows calls it Enter, so we'll do the same for the DOM
            return "Enter";

        // "EraseEof"

        // "Execute"
        case NSExecuteFunctionKey:
            return "Execute";

        // "Exsel"

        // "F1"
        case NSF1FunctionKey:
            return "F1";
        // "F2"
        case NSF2FunctionKey:
            return "F2";
        // "F3"
        case NSF3FunctionKey:
            return "F3";
        // "F4"
        case NSF4FunctionKey:
            return "F4";
        // "F5"
        case NSF5FunctionKey:
            return "F5";
        // "F6"
        case NSF6FunctionKey:
            return "F6";
        // "F7"
        case NSF7FunctionKey:
            return "F7";
        // "F8"
        case NSF8FunctionKey:
            return "F8";
        // "F9"
        case NSF9FunctionKey:
            return "F9";
        // "F10"
        case NSF10FunctionKey:
            return "F10";
        // "F11"
        case NSF11FunctionKey:
            return "F11";
        // "F12"
        case NSF12FunctionKey:
            return "F12";
        // "F13"
        case NSF13FunctionKey:
            return "F13";
        // "F14"
        case NSF14FunctionKey:
            return "F14";
        // "F15"
        case NSF15FunctionKey:
            return "F15";
        // "F16"
        case NSF16FunctionKey:
            return "F16";
        // "F17"
        case NSF17FunctionKey:
            return "F17";
        // "F18"
        case NSF18FunctionKey:
            return "F18";
        // "F19"
        case NSF19FunctionKey:
            return "F19";
        // "F20"
        case NSF20FunctionKey:
            return "F20";
        // "F21"
        case NSF21FunctionKey:
            return "F21";
        // "F22"
        case NSF22FunctionKey:
            return "F22";
        // "F23"
        case NSF23FunctionKey:
            return "F23";
        // "F24"
        case NSF24FunctionKey:
            return "F24";

        // "FinalMode"

        // "Find"
        case NSFindFunctionKey:
            return "Find";

        // "FullWidth"
        // "HalfWidth"
        // "HangulMode"
        // "HanjaMode"

        // "Help"
        case NSHelpFunctionKey:
            return "Help";

        // "Hiragana"

        // "Home"
        case NSHomeFunctionKey:
            return "Home";
        // "Insert"
        case NSInsertFunctionKey:
            return "Insert";

        // "JapaneseHiragana"
        // "JapaneseKatakana"
        // "JapaneseRomaji"
        // "JunjaMode"
        // "KanaMode"
        // "KanjiMode"
        // "Katakana"
        // "LaunchApplication1"
        // "LaunchApplication2"
        // "LaunchMail"

        // "Left"
        case NSLeftArrowFunctionKey:
            return "Left";

        // "Meta"
        // "MediaNextTrack"
        // "MediaPlayPause"
        // "MediaPreviousTrack"
        // "MediaStop"

        // "ModeChange"
        case NSModeSwitchFunctionKey:
            return "ModeChange";

        // "Nonconvert"
        // "NumLock"

        // "PageDown"
        case NSPageDownFunctionKey:
            return "PageDown";
        // "PageUp"
        case NSPageUpFunctionKey:
            return "PageUp";

        // "Paste"

        // "Pause"
        case NSPauseFunctionKey:
            return "Pause";

        // "Play"
        // "PreviousCandidate"

        // "PrintScreen"
        case NSPrintScreenFunctionKey:
            return "PrintScreen";

        // "Process"
        // "Props"

        // "Right"
        case NSRightArrowFunctionKey:
            return "Right";

        // "RomanCharacters"

        // "Scroll"
        case NSScrollLockFunctionKey:
            return "Scroll";
        // "Select"
        case NSSelectFunctionKey:
            return "Select";

        // "SelectMedia"
        // "Shift"

        // "Stop"
        case NSStopFunctionKey:
            return "Stop";
        // "Up"
        case NSUpArrowFunctionKey:
            return "Up";
        // "Undo"
        case NSUndoFunctionKey:
            return "Undo";

        // "VolumeDown"
        // "VolumeMute"
        // "VolumeUp"
        // "Win"
        // "Zoom"

        // More function keys, not in the key identifier specification.
        case NSF25FunctionKey:
            return "F25";
        case NSF26FunctionKey:
            return "F26";
        case NSF27FunctionKey:
            return "F27";
        case NSF28FunctionKey:
            return "F28";
        case NSF29FunctionKey:
            return "F29";
        case NSF30FunctionKey:
            return "F30";
        case NSF31FunctionKey:
            return "F31";
        case NSF32FunctionKey:
            return "F32";
        case NSF33FunctionKey:
            return "F33";
        case NSF34FunctionKey:
            return "F34";
        case NSF35FunctionKey:
            return "F35";

        // Turn 0x7F into 0x08, because backspace needs to always be 0x08.
        case 0x7F:
            return "U+0008";
        // Standard says that DEL becomes U+007F.
        case NSDeleteFunctionKey:
            return "U+007F";

        // Always use 0x09 for tab instead of AppKit's backtab character.
        case NSBackTabCharacter:
            return "U+0009";

        case NSBeginFunctionKey:
        case NSBreakFunctionKey:
        case NSClearDisplayFunctionKey:
        case NSDeleteCharFunctionKey:
        case NSDeleteLineFunctionKey:
        case NSInsertCharFunctionKey:
        case NSInsertLineFunctionKey:
        case NSNextFunctionKey:
        case NSPrevFunctionKey:
        case NSPrintFunctionKey:
        case NSRedoFunctionKey:
        case NSResetFunctionKey:
        case NSSysReqFunctionKey:
        case NSSystemFunctionKey:
        case NSUserFunctionKey:
            // FIXME: We should use something other than the vendor-area Unicode values for the above keys.
            // For now, just fall through to the default.
        default:
            return String::format("U+%04X", toASCIIUpper(charCode));
    }
}

int windowsKeyCodeForKeyCode(uint16_t keyCode)
{
    static const int windowsKeyCode[] = {
        /* 0 */ VK_A,
        /* 1 */ VK_S,
        /* 2 */ VK_D,
        /* 3 */ VK_F,
        /* 4 */ VK_H,
        /* 5 */ VK_G,
        /* 6 */ VK_Z,
        /* 7 */ VK_X,
        /* 8 */ VK_C,
        /* 9 */ VK_V,
        /* 0x0A */ VK_OEM_3, // "Section" - key to the left from 1 (ISO Keyboard Only)
        /* 0x0B */ VK_B,
        /* 0x0C */ VK_Q,
        /* 0x0D */ VK_W,
        /* 0x0E */ VK_E,
        /* 0x0F */ VK_R,
        /* 0x10 */ VK_Y,
        /* 0x11 */ VK_T,
        /* 0x12 */ VK_1,
        /* 0x13 */ VK_2,
        /* 0x14 */ VK_3,
        /* 0x15 */ VK_4,
        /* 0x16 */ VK_6,
        /* 0x17 */ VK_5,
        /* 0x18 */ VK_OEM_PLUS, // =+
        /* 0x19 */ VK_9,
        /* 0x1A */ VK_7,
        /* 0x1B */ VK_OEM_MINUS, // -_
        /* 0x1C */ VK_8,
        /* 0x1D */ VK_0,
        /* 0x1E */ VK_OEM_6, // ]}
        /* 0x1F */ VK_O,
        /* 0x20 */ VK_U,
        /* 0x21 */ VK_OEM_4, // {[
        /* 0x22 */ VK_I,
        /* 0x23 */ VK_P,
        /* 0x24 */ VK_RETURN, // Return
        /* 0x25 */ VK_L,
        /* 0x26 */ VK_J,
        /* 0x27 */ VK_OEM_7, // '"
        /* 0x28 */ VK_K,
        /* 0x29 */ VK_OEM_1, // ;:
        /* 0x2A */ VK_OEM_5, // \|
        /* 0x2B */ VK_OEM_COMMA, // ,<
        /* 0x2C */ VK_OEM_2, // /?
        /* 0x2D */ VK_N,
        /* 0x2E */ VK_M,
        /* 0x2F */ VK_OEM_PERIOD, // .>
        /* 0x30 */ VK_TAB,
        /* 0x31 */ VK_SPACE,
        /* 0x32 */ VK_OEM_3, // `~
        /* 0x33 */ VK_BACK, // Backspace
        /* 0x34 */ 0, // n/a
        /* 0x35 */ VK_ESCAPE,
        /* 0x36 */ VK_APPS, // Right Command
        /* 0x37 */ VK_LWIN, // Left Command
        /* 0x38 */ VK_LSHIFT, // Left Shift
        /* 0x39 */ VK_CAPITAL, // Caps Lock
        /* 0x3A */ VK_LMENU, // Left Option
        /* 0x3B */ VK_LCONTROL, // Left Ctrl
        /* 0x3C */ VK_RSHIFT, // Right Shift
        /* 0x3D */ VK_RMENU, // Right Option
        /* 0x3E */ VK_RCONTROL, // Right Ctrl
        /* 0x3F */ 0, // fn
        /* 0x40 */ VK_F17,
        /* 0x41 */ VK_DECIMAL, // Num Pad .
        /* 0x42 */ 0, // n/a
        /* 0x43 */ VK_MULTIPLY, // Num Pad *
        /* 0x44 */ 0, // n/a
        /* 0x45 */ VK_ADD, // Num Pad +
        /* 0x46 */ 0, // n/a
        /* 0x47 */ VK_CLEAR, // Num Pad Clear
        /* 0x48 */ VK_VOLUME_UP,
        /* 0x49 */ VK_VOLUME_DOWN,
        /* 0x4A */ VK_VOLUME_MUTE,
        /* 0x4B */ VK_DIVIDE, // Num Pad /
        /* 0x4C */ VK_RETURN, // Num Pad Enter
        /* 0x4D */ 0, // n/a
        /* 0x4E */ VK_SUBTRACT, // Num Pad -
        /* 0x4F */ VK_F18,
        /* 0x50 */ VK_F19,
        /* 0x51 */ VK_OEM_PLUS, // Num Pad =. There is no such key on common PC keyboards, mapping to normal "+=".
        /* 0x52 */ VK_NUMPAD0,
        /* 0x53 */ VK_NUMPAD1,
        /* 0x54 */ VK_NUMPAD2,
        /* 0x55 */ VK_NUMPAD3,
        /* 0x56 */ VK_NUMPAD4,
        /* 0x57 */ VK_NUMPAD5,
        /* 0x58 */ VK_NUMPAD6,
        /* 0x59 */ VK_NUMPAD7,
        /* 0x5A */ VK_F20,
        /* 0x5B */ VK_NUMPAD8,
        /* 0x5C */ VK_NUMPAD9,
        /* 0x5D */ 0, // Yen (JIS Keyboard Only)
        /* 0x5E */ 0, // Underscore (JIS Keyboard Only)
        /* 0x5F */ 0, // KeypadComma (JIS Keyboard Only)
        /* 0x60 */ VK_F5,
        /* 0x61 */ VK_F6,
        /* 0x62 */ VK_F7,
        /* 0x63 */ VK_F3,
        /* 0x64 */ VK_F8,
        /* 0x65 */ VK_F9,
        /* 0x66 */ 0, // Eisu (JIS Keyboard Only)
        /* 0x67 */ VK_F11,
        /* 0x68 */ 0, // Kana (JIS Keyboard Only)
        /* 0x69 */ VK_F13,
        /* 0x6A */ VK_F16,
        /* 0x6B */ VK_F14,
        /* 0x6C */ 0, // n/a
        /* 0x6D */ VK_F10,
        /* 0x6E */ 0, // n/a (Windows95 key?)
        /* 0x6F */ VK_F12,
        /* 0x70 */ 0, // n/a
        /* 0x71 */ VK_F15,
        /* 0x72 */ VK_INSERT, // Help
        /* 0x73 */ VK_HOME, // Home
        /* 0x74 */ VK_PRIOR, // Page Up
        /* 0x75 */ VK_DELETE, // Forward Delete
        /* 0x76 */ VK_F4,
        /* 0x77 */ VK_END, // End
        /* 0x78 */ VK_F2,
        /* 0x79 */ VK_NEXT, // Page Down
        /* 0x7A */ VK_F1,
        /* 0x7B */ VK_LEFT, // Left Arrow
        /* 0x7C */ VK_RIGHT, // Right Arrow
        /* 0x7D */ VK_DOWN, // Down Arrow
        /* 0x7E */ VK_UP, // Up Arrow
        /* 0x7F */ 0 // n/a
    };

    if (keyCode >= 0x80)
        return 0;

     return windowsKeyCode[keyCode];
}

int windowsKeyCodeForCharCode(unichar charCode)
{
    switch (charCode) {
#if PLATFORM(IOS)
        case 8: case 0x7F: return VK_BACK;
        case 9: return VK_TAB;
        case 0xD: case 3: return VK_RETURN;
        case 0x1B: return VK_ESCAPE;
        case ' ': return VK_SPACE;
        case NSHomeFunctionKey: return VK_HOME;
        case NSEndFunctionKey: return VK_END;
        case NSPageUpFunctionKey: return VK_PRIOR;
        case NSPageDownFunctionKey: return VK_NEXT;
        case NSUpArrowFunctionKey: return VK_UP;
        case NSDownArrowFunctionKey: return VK_DOWN;
        case NSLeftArrowFunctionKey: return VK_LEFT;
        case NSRightArrowFunctionKey: return VK_RIGHT;
        case NSDeleteFunctionKey: return VK_DELETE;

        case '0': case ')': return VK_0;
        case '1': case '!': return VK_1;
        case '2': case '@': return VK_2;
        case '3': case '#': return VK_3;
        case '4': case '$': return VK_4;
        case '5': case '%': return VK_5;
        case '6': case '^': return VK_6;
        case '7': case '&': return VK_7;
        case '8': case '*': return VK_8;
        case '9': case '(': return VK_9;
#endif
        case 'a': case 'A': return VK_A; 
        case 'b': case 'B': return VK_B; 
        case 'c': case 'C': return VK_C; 
        case 'd': case 'D': return VK_D; 
        case 'e': case 'E': return VK_E; 
        case 'f': case 'F': return VK_F; 
        case 'g': case 'G': return VK_G; 
        case 'h': case 'H': return VK_H; 
        case 'i': case 'I': return VK_I; 
        case 'j': case 'J': return VK_J; 
        case 'k': case 'K': return VK_K; 
        case 'l': case 'L': return VK_L; 
        case 'm': case 'M': return VK_M; 
        case 'n': case 'N': return VK_N; 
        case 'o': case 'O': return VK_O; 
        case 'p': case 'P': return VK_P; 
        case 'q': case 'Q': return VK_Q; 
        case 'r': case 'R': return VK_R; 
        case 's': case 'S': return VK_S; 
        case 't': case 'T': return VK_T; 
        case 'u': case 'U': return VK_U; 
        case 'v': case 'V': return VK_V; 
        case 'w': case 'W': return VK_W; 
        case 'x': case 'X': return VK_X; 
        case 'y': case 'Y': return VK_Y; 
        case 'z': case 'Z': return VK_Z; 

        // AppKit generates Unicode PUA character codes for some function keys; using these when key code is not known.
        case NSPauseFunctionKey: return VK_PAUSE;
        case NSSelectFunctionKey: return VK_SELECT;
        case NSPrintFunctionKey: return VK_PRINT;
        case NSExecuteFunctionKey: return VK_EXECUTE;
        case NSPrintScreenFunctionKey: return VK_SNAPSHOT;
        case NSInsertFunctionKey: return VK_INSERT;
#if PLATFORM(IOS)
        case NSHelpFunctionKey: return VK_INSERT;

        case NSF1FunctionKey: return VK_F1;
        case NSF2FunctionKey: return VK_F2;
        case NSF3FunctionKey: return VK_F3;
        case NSF4FunctionKey: return VK_F4;
        case NSF5FunctionKey: return VK_F5;
        case NSF6FunctionKey: return VK_F6;
        case NSF7FunctionKey: return VK_F7;
        case NSF8FunctionKey: return VK_F8;
        case NSF9FunctionKey: return VK_F9;
        case NSF10FunctionKey: return VK_F10;
        case NSF11FunctionKey: return VK_F11;
        case NSF12FunctionKey: return VK_F12;
        case NSF13FunctionKey: return VK_F13;
        case NSF14FunctionKey: return VK_F14;
        case NSF15FunctionKey: return VK_F15;
        case NSF16FunctionKey: return VK_F16;
        case NSF17FunctionKey: return VK_F17;
        case NSF18FunctionKey: return VK_F18;
        case NSF19FunctionKey: return VK_F19;
        case NSF20FunctionKey: return VK_F20;
#endif
        case NSF21FunctionKey: return VK_F21;
        case NSF22FunctionKey: return VK_F22;
        case NSF23FunctionKey: return VK_F23;
        case NSF24FunctionKey: return VK_F24;
        case NSScrollLockFunctionKey: return VK_SCROLL;

        // This is for U.S. keyboard mapping, and doesn't necessarily make sense for different keyboard layouts.
        // For example, '"' on Windows Russian layout is VK_2, not VK_OEM_7.
        case ';': case ':': return VK_OEM_1; 
        case '=': case '+': return VK_OEM_PLUS; 
        case ',': case '<': return VK_OEM_COMMA; 
        case '-': case '_': return VK_OEM_MINUS; 
        case '.': case '>': return VK_OEM_PERIOD; 
        case '/': case '?': return VK_OEM_2; 
        case '`': case '~': return VK_OEM_3; 
        case '[': case '{': return VK_OEM_4; 
        case '\\': case '|': return VK_OEM_5; 
        case ']': case '}': return VK_OEM_6; 
        case '\'': case '"': return VK_OEM_7; 

    }

    return 0;
}

}
