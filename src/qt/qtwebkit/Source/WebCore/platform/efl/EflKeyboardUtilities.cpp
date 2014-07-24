/*
 * Copyright (C) 2011 Samsung Electronics
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
#include "EflKeyboardUtilities.h"

#include "KeyboardEvent.h"
#include "WindowsKeyboardCodes.h"
#include <wtf/HashMap.h>
#include <wtf/HexNumber.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

typedef HashMap<String, String> KeyMap;
typedef HashMap<String, int> WindowsKeyMap;
typedef HashMap<int, const char*> KeyCommandMap;

static KeyMap& keyMap()
{
    DEFINE_STATIC_LOCAL(KeyMap, keyMap, ());
    return keyMap;
}

static WindowsKeyMap& windowsKeyMap()
{
    DEFINE_STATIC_LOCAL(WindowsKeyMap, windowsKeyMap, ());
    return windowsKeyMap;
}

static KeyCommandMap& keyDownCommandsMap()
{
    DEFINE_STATIC_LOCAL(KeyCommandMap, keyDownCommandsMap, ());
    return keyDownCommandsMap;
}

static KeyCommandMap& keyPressCommandsMap()
{
    DEFINE_STATIC_LOCAL(KeyCommandMap, keyPressCommandsMap, ());
    return keyPressCommandsMap;
}

static inline void addCharactersToKeyMap(const char from, const char to)
{
    for (char c = from; c <= to; c++) {
        StringBuilder builder;
        builder.appendLiteral("U+");
        appendUnsignedAsHexFixedSize(c, builder, 4, WTF::Uppercase);
        keyMap().set(String(&c, 1), builder.toString());
    }
}

static void createKeyMap()
{
    for (unsigned int i = 1; i < 25; i++) {
        StringBuilder builder;
        builder.append('F');
        builder.appendNumber(i);
        String key = builder.toString();
        keyMap().set(key, key);
    }
    keyMap().set(ASCIILiteral("Alt_L"), ASCIILiteral("Alt"));
    keyMap().set(ASCIILiteral("ISO_Level3_Shift"), ASCIILiteral("Alt"));
    keyMap().set(ASCIILiteral("Menu"), ASCIILiteral("Alt"));
    keyMap().set(ASCIILiteral("Shift_L"), ASCIILiteral("Shift"));
    keyMap().set(ASCIILiteral("Shift_R"), ASCIILiteral("Shift"));
    keyMap().set(ASCIILiteral("Down"), ASCIILiteral("Down"));
    keyMap().set(ASCIILiteral("End"), ASCIILiteral("End"));
    keyMap().set(ASCIILiteral("Return"), ASCIILiteral("Enter"));
    keyMap().set(ASCIILiteral("KP_Enter"), ASCIILiteral("Enter"));
    keyMap().set(ASCIILiteral("Home"), ASCIILiteral("Home"));
    keyMap().set(ASCIILiteral("Insert"), ASCIILiteral("Insert"));
    keyMap().set(ASCIILiteral("Left"), ASCIILiteral("Left"));
    keyMap().set(ASCIILiteral("Down"), ASCIILiteral("Down"));
    keyMap().set(ASCIILiteral("Next"), ASCIILiteral("PageDown"));
    keyMap().set(ASCIILiteral("Prior"), ASCIILiteral("PageUp"));
    keyMap().set(ASCIILiteral("Right"), ASCIILiteral("Right"));
    keyMap().set(ASCIILiteral("Up"), ASCIILiteral("Up"));
    keyMap().set(ASCIILiteral("Delete"), ASCIILiteral("U+007F"));
    keyMap().set(ASCIILiteral("Tab"), ASCIILiteral("U+0009"));
    keyMap().set(ASCIILiteral("ISO_Left_Tab"), ASCIILiteral("U+0009"));
    keyMap().set(ASCIILiteral("BackSpace"), ASCIILiteral("U+0008"));
    keyMap().set(ASCIILiteral("space"), ASCIILiteral("U+0020"));
    keyMap().set(ASCIILiteral("Escape"), ASCIILiteral("U+001B"));
    keyMap().set(ASCIILiteral("Print"), ASCIILiteral("PrintScreen"));
    // Keypad location
    keyMap().set(ASCIILiteral("KP_Left"), ASCIILiteral("Left"));
    keyMap().set(ASCIILiteral("KP_Right"), ASCIILiteral("Right"));
    keyMap().set(ASCIILiteral("KP_Up"), ASCIILiteral("Up"));
    keyMap().set(ASCIILiteral("KP_Down"), ASCIILiteral("Down"));
    keyMap().set(ASCIILiteral("KP_Prior"), ASCIILiteral("PageUp"));
    keyMap().set(ASCIILiteral("KP_Next"), ASCIILiteral("PageDown"));
    keyMap().set(ASCIILiteral("KP_Home"), ASCIILiteral("Home"));
    keyMap().set(ASCIILiteral("KP_End"), ASCIILiteral("End"));
    keyMap().set(ASCIILiteral("KP_Insert"), ASCIILiteral("Insert"));
    keyMap().set(ASCIILiteral("KP_Delete"), ASCIILiteral("U+007F"));
 
    addCharactersToKeyMap('a', 'z');
    addCharactersToKeyMap('A', 'Z');
    addCharactersToKeyMap('0', '9');
}

static inline void addCharactersToWinKeyMap(const char from, const char to, const int baseCode)
{
    int i = 0;
    for (char c = from; c <= to; c++, i++)
        windowsKeyMap().set(String(&c, 1), baseCode + i);
}

static void createWindowsKeyMap()
{
    windowsKeyMap().set(ASCIILiteral("Return"), VK_RETURN);
    windowsKeyMap().set(ASCIILiteral("KP_Return"), VK_RETURN);
    windowsKeyMap().set(ASCIILiteral("Alt_L"), VK_LMENU);
    windowsKeyMap().set(ASCIILiteral("Alt_R"), VK_RMENU);
    windowsKeyMap().set(ASCIILiteral("ISO_Level3_Shift"), VK_MENU);
    windowsKeyMap().set(ASCIILiteral("Menu"), VK_MENU);
    windowsKeyMap().set(ASCIILiteral("Shift_L"), VK_LSHIFT);
    windowsKeyMap().set(ASCIILiteral("Shift_R"), VK_RSHIFT);
    windowsKeyMap().set(ASCIILiteral("Control_L"), VK_LCONTROL);
    windowsKeyMap().set(ASCIILiteral("Control_R"), VK_RCONTROL);
    windowsKeyMap().set(ASCIILiteral("Pause"), VK_PAUSE);
    windowsKeyMap().set(ASCIILiteral("Break"), VK_PAUSE);
    windowsKeyMap().set(ASCIILiteral("Caps_Lock"), VK_CAPITAL);
    windowsKeyMap().set(ASCIILiteral("Scroll_Lock"), VK_SCROLL);
    windowsKeyMap().set(ASCIILiteral("Num_Lock"), VK_NUMLOCK);
    windowsKeyMap().set(ASCIILiteral("Escape"), VK_ESCAPE);
    windowsKeyMap().set(ASCIILiteral("Tab"), VK_TAB);
    windowsKeyMap().set(ASCIILiteral("ISO_Left_Tab"), VK_TAB);
    windowsKeyMap().set(ASCIILiteral("BackSpace"), VK_BACK);
    windowsKeyMap().set(ASCIILiteral("space"), VK_SPACE);
    windowsKeyMap().set(ASCIILiteral("Next"), VK_NEXT);
    windowsKeyMap().set(ASCIILiteral("Prior"), VK_PRIOR);
    windowsKeyMap().set(ASCIILiteral("Home"), VK_HOME);
    windowsKeyMap().set(ASCIILiteral("End"), VK_END);
    windowsKeyMap().set(ASCIILiteral("Right"), VK_RIGHT);
    windowsKeyMap().set(ASCIILiteral("Left"), VK_LEFT);
    windowsKeyMap().set(ASCIILiteral("Up"), VK_UP);
    windowsKeyMap().set(ASCIILiteral("Down"), VK_DOWN);
    windowsKeyMap().set(ASCIILiteral("Print"), VK_SNAPSHOT);
    windowsKeyMap().set(ASCIILiteral("Insert"), VK_INSERT);
    windowsKeyMap().set(ASCIILiteral("Delete"), VK_DELETE);

    windowsKeyMap().set(ASCIILiteral("comma"), VK_OEM_COMMA);
    windowsKeyMap().set(ASCIILiteral("less"), VK_OEM_COMMA);
    windowsKeyMap().set(ASCIILiteral("period"), VK_OEM_PERIOD);
    windowsKeyMap().set(ASCIILiteral("greater"), VK_OEM_PERIOD);
    windowsKeyMap().set(ASCIILiteral("semicolon"), VK_OEM_1);
    windowsKeyMap().set(ASCIILiteral("colon"), VK_OEM_1);
    windowsKeyMap().set(ASCIILiteral("slash"), VK_OEM_2);
    windowsKeyMap().set(ASCIILiteral("question"), VK_OEM_2);
    windowsKeyMap().set(ASCIILiteral("grave"), VK_OEM_3);
    windowsKeyMap().set(ASCIILiteral("asciitilde"), VK_OEM_3);
    windowsKeyMap().set(ASCIILiteral("bracketleft"), VK_OEM_4);
    windowsKeyMap().set(ASCIILiteral("braceleft"), VK_OEM_4);
    windowsKeyMap().set(ASCIILiteral("backslash"), VK_OEM_5);
    windowsKeyMap().set(ASCIILiteral("bar"), VK_OEM_5);
    windowsKeyMap().set(ASCIILiteral("bracketright"), VK_OEM_6);
    windowsKeyMap().set(ASCIILiteral("braceright"), VK_OEM_6);
    windowsKeyMap().set(ASCIILiteral("apostrophe"), VK_OEM_7);
    windowsKeyMap().set(ASCIILiteral("quotedbl"), VK_OEM_7);
    // Keypad location
    windowsKeyMap().set(ASCIILiteral("KP_Left"), VK_LEFT);
    windowsKeyMap().set(ASCIILiteral("KP_Right"), VK_RIGHT);
    windowsKeyMap().set(ASCIILiteral("KP_Up"), VK_UP);
    windowsKeyMap().set(ASCIILiteral("KP_Down"), VK_DOWN);
    windowsKeyMap().set(ASCIILiteral("KP_Prior"), VK_PRIOR);
    windowsKeyMap().set(ASCIILiteral("KP_Next"), VK_NEXT);
    windowsKeyMap().set(ASCIILiteral("KP_Home"), VK_HOME);
    windowsKeyMap().set(ASCIILiteral("KP_End"), VK_END);
    windowsKeyMap().set(ASCIILiteral("KP_Insert"), VK_INSERT);
    windowsKeyMap().set(ASCIILiteral("KP_Delete"), VK_DELETE);
    windowsKeyMap().set(ASCIILiteral("KP_Multiply"), VK_MULTIPLY);
    windowsKeyMap().set(ASCIILiteral("KP_Subtract"), VK_SUBTRACT);
    windowsKeyMap().set(ASCIILiteral("KP_Decimal"), VK_DECIMAL);
    windowsKeyMap().set(ASCIILiteral("KP_Devide"), VK_DIVIDE);

    // Set alphabet to the windowsKeyMap.
    addCharactersToWinKeyMap('a', 'z', VK_A);
    addCharactersToWinKeyMap('A', 'Z', VK_A);

    // Set digits to the windowsKeyMap.
    addCharactersToWinKeyMap('0', '9', VK_0);

    // Set digits of keypad to the windowsKeyMap.
    for (unsigned i = 0; i < 10; ++i) {
        StringBuilder builder;
        builder.appendLiteral("KP_");
        builder.appendNumber(i);
        windowsKeyMap().set(builder.toString(), VK_NUMPAD0 + i);
    }

    // Set shifted digits to the windowsKeyMap.
    windowsKeyMap().set(ASCIILiteral("exclam"), VK_1);
    windowsKeyMap().set(ASCIILiteral("at"), VK_2);
    windowsKeyMap().set(ASCIILiteral("numbersign"), VK_3);
    windowsKeyMap().set(ASCIILiteral("dollar"), VK_4);
    windowsKeyMap().set(ASCIILiteral("percent"), VK_5);
    windowsKeyMap().set(ASCIILiteral("asciicircum"), VK_6);
    windowsKeyMap().set(ASCIILiteral("ampersand"), VK_7);
    windowsKeyMap().set(ASCIILiteral("asterisk"), VK_8);
    windowsKeyMap().set(ASCIILiteral("parenleft"), VK_9);
    windowsKeyMap().set(ASCIILiteral("parenright"), VK_0);
    windowsKeyMap().set(ASCIILiteral("minus"), VK_OEM_MINUS);
    windowsKeyMap().set(ASCIILiteral("underscore"), VK_OEM_MINUS);
    windowsKeyMap().set(ASCIILiteral("equal"), VK_OEM_PLUS);
    windowsKeyMap().set(ASCIILiteral("plus"), VK_OEM_PLUS);

    // Set F_XX keys to the windowsKeyMap.
    for (unsigned int i = 1; i < 25; i++) {
        StringBuilder builder;
        builder.append('F');
        builder.appendNumber(i);
        windowsKeyMap().set(builder.toString(), VK_F1 + i - 1);
    }
}

static const unsigned CtrlKey = 1 << 0;
static const unsigned AltKey = 1 << 1;
static const unsigned ShiftKey = 1 << 2;

struct KeyDownEntry {
    unsigned virtualKey;
    unsigned modifiers;
    const char* name;
};

struct KeyPressEntry {
    unsigned charCode;
    unsigned modifiers;
    const char* name;
};

static const KeyDownEntry keyDownEntries[] = {
    { VK_LEFT,   0,                  "MoveLeft"                                    },
    { VK_LEFT,   ShiftKey,           "MoveLeftAndModifySelection"                  },
    { VK_LEFT,   CtrlKey,            "MoveWordLeft"                                },
    { VK_LEFT,   CtrlKey | ShiftKey, "MoveWordLeftAndModifySelection"              },
    { VK_RIGHT,  0,                  "MoveRight"                                   },
    { VK_RIGHT,  ShiftKey,           "MoveRightAndModifySelection"                 },
    { VK_RIGHT,  CtrlKey,            "MoveWordRight"                               },
    { VK_RIGHT,  CtrlKey | ShiftKey, "MoveWordRightAndModifySelection"             },
    { VK_UP,     0,                  "MoveUp"                                      },
    { VK_UP,     ShiftKey,           "MoveUpAndModifySelection"                    },
    { VK_PRIOR,  ShiftKey,           "MovePageUpAndModifySelection"                },
    { VK_DOWN,   0,                  "MoveDown"                                    },
    { VK_DOWN,   ShiftKey,           "MoveDownAndModifySelection"                  },
    { VK_NEXT,   ShiftKey,           "MovePageDownAndModifySelection"              },
    { VK_PRIOR,  0,                  "MovePageUp"                                  },
    { VK_NEXT,   0,                  "MovePageDown"                                },
    { VK_HOME,   0,                  "MoveToBeginningOfLine"                       },
    { VK_HOME,   ShiftKey,           "MoveToBeginningOfLineAndModifySelection"     },
    { VK_HOME,   CtrlKey,            "MoveToBeginningOfDocument"                   },
    { VK_HOME,   CtrlKey | ShiftKey, "MoveToBeginningOfDocumentAndModifySelection" },

    { VK_END,    0,                  "MoveToEndOfLine"                             },
    { VK_END,    ShiftKey,           "MoveToEndOfLineAndModifySelection"           },
    { VK_END,    CtrlKey,            "MoveToEndOfDocument"                         },
    { VK_END,    CtrlKey | ShiftKey, "MoveToEndOfDocumentAndModifySelection"       },

    { VK_BACK,   0,                  "DeleteBackward"                              },
    { VK_BACK,   ShiftKey,           "DeleteBackward"                              },
    { VK_DELETE, 0,                  "DeleteForward"                               },
    { VK_BACK,   CtrlKey,            "DeleteWordBackward"                          },
    { VK_DELETE, CtrlKey,            "DeleteWordForward"                           },

    { 'B',       CtrlKey,            "ToggleBold"                                  },
    { 'I',       CtrlKey,            "ToggleItalic"                                },

    { VK_ESCAPE, 0,                  "Cancel"                                      },
    { VK_OEM_PERIOD, CtrlKey,        "Cancel"                                      },
    { VK_TAB,    0,                  "InsertTab"                                   },
    { VK_TAB,    ShiftKey,           "InsertBacktab"                               },
    { VK_RETURN, 0,                  "InsertNewline"                               },
    { VK_RETURN, CtrlKey,            "InsertNewline"                               },
    { VK_RETURN, AltKey,             "InsertNewline"                               },
    { VK_RETURN, AltKey | ShiftKey,  "InsertNewline"                               },

    { 'C',       CtrlKey,            "Copy"                                        },
    { 'V',       CtrlKey,            "Paste"                                       },
    { 'X',       CtrlKey,            "Cut"                                         },

    { 'A',       CtrlKey,            "SelectAll"                                   },
    { 'Z',       CtrlKey,            "Undo"                                        },
    { 'Z',       CtrlKey | ShiftKey, "Redo"                                        },
    { 'Y',       CtrlKey,            "Redo"                                        },
};

static const KeyPressEntry keyPressEntries[] = {
    { '\t',   0,                  "InsertTab"                                   },
    { '\t',   ShiftKey,           "InsertBacktab"                               },
    { '\r',   0,                  "InsertNewline"                               },
    { '\r',   CtrlKey,            "InsertNewline"                               },
    { '\r',   AltKey,             "InsertNewline"                               },
    { '\r',   AltKey | ShiftKey,  "InsertNewline"                               },
};

static void createKeyDownCommandMap()
{
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(keyDownEntries); ++i)
        keyDownCommandsMap().set(keyDownEntries[i].modifiers << 16 | keyDownEntries[i].virtualKey, keyDownEntries[i].name);
}

static void createKeyPressCommandMap()
{
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(keyPressEntries); ++i)
        keyPressCommandsMap().set(keyPressEntries[i].modifiers << 16 | keyPressEntries[i].charCode, keyPressEntries[i].name);
}

String keyIdentifierForEvasKeyName(const String& keyName)
{
    if (keyMap().isEmpty())
        createKeyMap();

    if (keyMap().contains(keyName))
        return keyMap().get(keyName);

    return keyName;
}

int windowsKeyCodeForEvasKeyName(const String& keyName)
{
    if (windowsKeyMap().isEmpty())
        createWindowsKeyMap();

    if (windowsKeyMap().contains(keyName))
        return windowsKeyMap().get(keyName);

    return 0;
}

const char* getKeyDownCommandName(const KeyboardEvent* event)
{
    unsigned modifiers = 0;
    if (event->shiftKey())
        modifiers |= ShiftKey;
    if (event->altKey())
        modifiers |= AltKey;
    if (event->ctrlKey())
        modifiers |= CtrlKey;

    int mapKey = modifiers << 16 | event->keyCode();
    if (!mapKey)
        return 0;

    if (keyDownCommandsMap().isEmpty())
        createKeyDownCommandMap();

    return keyDownCommandsMap().get(mapKey);
}

const char* getKeyPressCommandName(const KeyboardEvent* event)
{
    unsigned modifiers = 0;
    if (event->shiftKey())
        modifiers |= ShiftKey;
    if (event->altKey())
        modifiers |= AltKey;
    if (event->ctrlKey())
        modifiers |= CtrlKey;

    int mapKey = modifiers << 16 | event->charCode();
    if (!mapKey)
        return 0;

    if (keyPressCommandsMap().isEmpty())
        createKeyPressCommandMap();

    return keyPressCommandsMap().get(mapKey);
}

} // namespace WebCore
