/*
 *  Copyright (C) 2007 Alp Toker <alp@atoker.com>
 *  Copyright (C) 2008 Nuanti Ltd.
 *  Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
 *  Copyright (C) 2009-2010 ProFUSION embedded systems
 *  Copyright (C) 2009-2010 Samsung Electronics
 *  Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "EditorClientWinCE.h"

#include "Document.h"
#include "Editor.h"
#include "Frame.h"
#include "KeyboardEvent.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PlatformKeyboardEvent.h"
#include "Settings.h"
#include "UndoStep.h"
#include "WebView.h"

using namespace WebCore;

namespace WebKit {

EditorClientWinCE::EditorClientWinCE(WebView* webView)
    : m_webView(webView)
{
}

EditorClientWinCE::~EditorClientWinCE()
{
}

void EditorClientWinCE::setInputMethodState(bool active)
{
    notImplemented();
}

bool EditorClientWinCE::shouldDeleteRange(Range*)
{
    notImplemented();
    return true;
}

bool EditorClientWinCE::isContinuousSpellCheckingEnabled()
{
    notImplemented();
    return false;
}

bool EditorClientWinCE::isGrammarCheckingEnabled()
{
    notImplemented();
    return false;
}

int EditorClientWinCE::spellCheckerDocumentTag()
{
    notImplemented();
    return 0;
}

bool EditorClientWinCE::shouldBeginEditing(WebCore::Range*)
{
    notImplemented();
    return true;
}

bool EditorClientWinCE::shouldEndEditing(WebCore::Range*)
{
    notImplemented();
    return true;
}

bool EditorClientWinCE::shouldInsertText(const String&, Range*, EditorInsertAction)
{
    notImplemented();
    return true;
}

bool EditorClientWinCE::shouldChangeSelectedRange(Range*, Range*, EAffinity, bool)
{
    notImplemented();
    return true;
}

bool EditorClientWinCE::shouldApplyStyle(WebCore::StylePropertySet*, WebCore::Range*)
{
    notImplemented();
    return true;
}

bool EditorClientWinCE::shouldMoveRangeAfterDelete(WebCore::Range*, WebCore::Range*)
{
    notImplemented();
    return true;
}

void EditorClientWinCE::didBeginEditing()
{
    notImplemented();
}

void EditorClientWinCE::respondToChangedContents()
{
    notImplemented();
}

void EditorClientWinCE::respondToChangedSelection(WebCore::Frame*)
{
    notImplemented();
}

void EditorClientWinCE::didEndEditing()
{
    notImplemented();
}

void EditorClientWinCE::didWriteSelectionToPasteboard()
{
    notImplemented();
}

void EditorClientWinCE::willWriteSelectionToPasteboard(WebCore::Range*)
{
    notImplemented();
}

void EditorClientWinCE::getClientPasteboardDataForRange(WebCore::Range*, Vector<String>&, Vector<RefPtr<WebCore::SharedBuffer> >&)
{
    notImplemented();
}

void EditorClientWinCE::didSetSelectionTypesForPasteboard()
{
    notImplemented();
}

void EditorClientWinCE::registerUndoStep(WTF::PassRefPtr<WebCore::UndoStep>)
{
    notImplemented();
}

void EditorClientWinCE::registerRedoStep(WTF::PassRefPtr<WebCore::UndoStep>)
{
    notImplemented();
}

void EditorClientWinCE::clearUndoRedoOperations()
{
    notImplemented();
}

bool EditorClientWinCE::canCopyCut(WebCore::Frame*, bool defaultValue) const
{
    return defaultValue;
}

bool EditorClientWinCE::canPaste(WebCore::Frame*, bool defaultValue) const
{
    return defaultValue;
}

bool EditorClientWinCE::canUndo() const
{
    notImplemented();
    return false;
}

bool EditorClientWinCE::canRedo() const
{
    notImplemented();
    return false;
}

void EditorClientWinCE::undo()
{
    notImplemented();
}

void EditorClientWinCE::redo()
{
    notImplemented();
}

bool EditorClientWinCE::shouldInsertNode(Node*, Range*, EditorInsertAction)
{
    notImplemented();
    return true;
}

void EditorClientWinCE::pageDestroyed()
{
    delete this;
}

bool EditorClientWinCE::smartInsertDeleteEnabled()
{
    Page* page = m_webView->page();
    if (!page)
        return false;
    return page->settings()->smartInsertDeleteEnabled();
}

bool EditorClientWinCE::isSelectTrailingWhitespaceEnabled()
{
    Page* page = m_webView->page();
    if (!page)
        return false;
    return page->settings()->selectTrailingWhitespaceEnabled();
}

void EditorClientWinCE::toggleContinuousSpellChecking()
{
    notImplemented();
}

void EditorClientWinCE::toggleGrammarChecking()
{
    notImplemented();
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
    { VK_TAB,    0,                  "InsertTab"                                   },
    { VK_TAB,    ShiftKey,           "InsertBacktab"                               },
    { VK_RETURN, 0,                  "InsertNewline"                               },
    { VK_RETURN, CtrlKey,            "InsertNewline"                               },
    { VK_RETURN, AltKey,             "InsertNewline"                               },
    { VK_RETURN, AltKey | ShiftKey,  "InsertNewline"                               },

    // It's not quite clear whether clipboard shortcuts and Undo/Redo should be handled
    // in the application or in WebKit. We chose WebKit for now.
    { 'C',       CtrlKey,            "Copy"                                        },
    { 'V',       CtrlKey,            "Paste"                                       },
    { 'X',       CtrlKey,            "Cut"                                         },
    { 'A',       CtrlKey,            "SelectAll"                                   },
    { VK_INSERT, CtrlKey,            "Copy"                                        },
    { VK_DELETE, ShiftKey,           "Cut"                                         },
    { VK_INSERT, ShiftKey,           "Paste"                                       },
    { 'Z',       CtrlKey,            "Undo"                                        },
    { 'Z',       CtrlKey | ShiftKey, "Redo"                                        }
};

static const KeyPressEntry keyPressEntries[] = {
    { '\t',   0,                  "InsertTab"                                   },
    { '\t',   ShiftKey,           "InsertBacktab"                               },
    { '\r',   0,                  "InsertNewline"                               },
    { '\r',   CtrlKey,            "InsertNewline"                               },
    { '\r',   AltKey,             "InsertNewline"                               },
    { '\r',   AltKey | ShiftKey,  "InsertNewline"                               }
};

const char* EditorClientWinCE::interpretKeyEvent(const KeyboardEvent* event)
{
    ASSERT(event->type() == eventNames().keydownEvent || event->type() == eventNames().keypressEvent);

    static HashMap<int, const char*>* keyDownCommandsMap = 0;
    static HashMap<int, const char*>* keyPressCommandsMap = 0;

    if (!keyDownCommandsMap) {
        keyDownCommandsMap = new HashMap<int, const char*>;
        keyPressCommandsMap = new HashMap<int, const char*>;

        for (size_t i = 0; i < WTF_ARRAY_LENGTH(keyDownEntries); ++i)
            keyDownCommandsMap->set(keyDownEntries[i].modifiers << 16 | keyDownEntries[i].virtualKey, keyDownEntries[i].name);

        for (size_t i = 0; i < WTF_ARRAY_LENGTH(keyPressEntries); ++i)
            keyPressCommandsMap->set(keyPressEntries[i].modifiers << 16 | keyPressEntries[i].charCode, keyPressEntries[i].name);
    }

    unsigned modifiers = 0;
    if (event->shiftKey())
        modifiers |= ShiftKey;
    if (event->altKey())
        modifiers |= AltKey;
    if (event->ctrlKey())
        modifiers |= CtrlKey;

    if (event->type() == eventNames().keydownEvent) {
        int mapKey = modifiers << 16 | event->keyCode();
        return mapKey ? keyDownCommandsMap->get(mapKey) : 0;
    }

    int mapKey = modifiers << 16 | event->charCode();
    return mapKey ? keyPressCommandsMap->get(mapKey) : 0;
}

bool EditorClientWinCE::handleEditingKeyboardEvent(KeyboardEvent* event)
{
    Node* node = event->target()->toNode();
    ASSERT(node);
    Frame* frame = node->document()->frame();
    ASSERT(frame);

    const PlatformKeyboardEvent* keyEvent = event->keyEvent();
    if (!keyEvent)
        return false;

    bool caretBrowsing = frame->settings()->caretBrowsingEnabled();
    if (caretBrowsing) {
        switch (keyEvent->windowsVirtualKeyCode()) {
        case VK_LEFT:
            frame->selection()->modify(keyEvent->shiftKey() ? FrameSelection::AlterationExtend : FrameSelection::AlterationMove,
                    DirectionLeft,
                    keyEvent->ctrlKey() ? WordGranularity : CharacterGranularity,
                    UserTriggered);
            return true;
        case VK_RIGHT:
            frame->selection()->modify(keyEvent->shiftKey() ? FrameSelection::AlterationExtend : FrameSelection::AlterationMove,
                    DirectionRight,
                    keyEvent->ctrlKey() ? WordGranularity : CharacterGranularity,
                    UserTriggered);
            return true;
        case VK_UP:
            frame->selection()->modify(keyEvent->shiftKey() ? FrameSelection::AlterationExtend : FrameSelection::AlterationMove,
                    DirectionBackward,
                    keyEvent->ctrlKey() ? ParagraphGranularity : LineGranularity,
                    UserTriggered);
            return true;
        case VK_DOWN:
            frame->selection()->modify(keyEvent->shiftKey() ? FrameSelection::AlterationExtend : FrameSelection::AlterationMove,
                    DirectionForward,
                    keyEvent->ctrlKey() ? ParagraphGranularity : LineGranularity,
                    UserTriggered);
            return true;
        }
    }

    Editor::Command command = frame->editor().command(interpretKeyEvent(event));

    if (keyEvent->type() == PlatformEvent::RawKeyDown) {
        // WebKit doesn't have enough information about mode to decide how commands that just insert text if executed via Editor should be treated,
        // so we leave it upon WebCore to either handle them immediately (e.g. Tab that changes focus) or let a keypress event be generated
        // (e.g. Tab that inserts a Tab character, or Enter).
        return !command.isTextInsertion() && command.execute(event);
    }

    if (command.execute(event))
        return true;

    // Don't insert null or control characters as they can result in unexpected behaviour
    if (event->charCode() < ' ')
        return false;

    // Don't insert anything if a modifier is pressed
    if (keyEvent->ctrlKey() || keyEvent->altKey())
        return false;

    return frame->editor().insertText(event->keyEvent()->text(), event);
}

void EditorClientWinCE::handleKeyboardEvent(KeyboardEvent* event)
{
    if (handleEditingKeyboardEvent(event))
        event->setDefaultHandled();
}

void EditorClientWinCE::handleInputMethodKeydown(KeyboardEvent* event)
{
    notImplemented();
}

void EditorClientWinCE::textFieldDidBeginEditing(Element*)
{
}

void EditorClientWinCE::textFieldDidEndEditing(Element*)
{
}

void EditorClientWinCE::textDidChangeInTextField(Element*)
{
}

bool EditorClientWinCE::doTextFieldCommandFromEvent(Element*, KeyboardEvent*)
{
    return false;
}

void EditorClientWinCE::textWillBeDeletedInTextField(Element*)
{
    notImplemented();
}

void EditorClientWinCE::textDidChangeInTextArea(Element*)
{
    notImplemented();
}

bool EditorClientWinCE::shouldEraseMarkersAfterChangeSelection(TextCheckingType) const
{
    return true;
}

void EditorClientWinCE::ignoreWordInSpellDocument(const String& text)
{
    notImplemented();
}

void EditorClientWinCE::learnWord(const String& text)
{
    notImplemented();
}

void EditorClientWinCE::checkSpellingOfString(const UChar* text, int length, int* misspellingLocation, int* misspellingLength)
{
    notImplemented();
}

String EditorClientWinCE::getAutoCorrectSuggestionForMisspelledWord(const String& inputWord)
{
    // This method can be implemented using customized algorithms for the particular browser.
    // Currently, it computes an empty string.
    return String();
}

void EditorClientWinCE::checkGrammarOfString(const UChar*, int, Vector<GrammarDetail>&, int*, int*)
{
    notImplemented();
}

void EditorClientWinCE::updateSpellingUIWithGrammarString(const String&, const GrammarDetail&)
{
    notImplemented();
}

void EditorClientWinCE::updateSpellingUIWithMisspelledWord(const String&)
{
    notImplemented();
}

void EditorClientWinCE::showSpellingUI(bool)
{
    notImplemented();
}

bool EditorClientWinCE::spellingUIIsShowing()
{
    notImplemented();
    return false;
}

void EditorClientWinCE::getGuessesForWord(const String& word, const String& context, WTF::Vector<String>& guesses)
{
    notImplemented();
}

void EditorClientWinCE::willSetInputMethodState()
{
    notImplemented();
}

} // namespace WebKit
