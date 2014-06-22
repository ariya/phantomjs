/*
 * Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "EditorClientBlackBerry.h"

#include "AutofillManager.h"
#include "DOMSupport.h"
#include "DumpRenderTreeClient.h"
#include "EditCommand.h"
#include "Frame.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "InputHandler.h"
#include "KeyboardEvent.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PlatformKeyboardEvent.h"
#include "SelectionHandler.h"
#include "Settings.h"
#include "SpellChecker.h"
#include "WebPage_p.h"
#include "WindowsKeyboardCodes.h"

using namespace BlackBerry::WebKit;

namespace WebCore {

// Arbitrary depth limit for the undo stack, to keep it from using
// unbounded memory. This is the maximum number of distinct undoable
// actions -- unbroken stretches of typed characters are coalesced
// into a single action only when not interrupted by string replacements
// triggered by replaceText calls.
static const size_t maximumUndoStackDepth = 1000;

EditorClientBlackBerry::EditorClientBlackBerry(WebPagePrivate* webPagePrivate)
    : m_webPagePrivate(webPagePrivate)
    , m_waitingForCursorFocus(false)
    , m_spellCheckState(SpellCheckDefault)
    , m_inRedo(false)
{
}

void EditorClientBlackBerry::pageDestroyed()
{
    delete this;
}

bool EditorClientBlackBerry::shouldDeleteRange(Range* range)
{
    if (m_webPagePrivate->m_dumpRenderTree)
        return m_webPagePrivate->m_dumpRenderTree->shouldDeleteDOMRange(range);
    return true;
}

bool EditorClientBlackBerry::smartInsertDeleteEnabled()
{
    Page* page = WebPagePrivate::core(m_webPagePrivate->m_webPage);
    if (!page)
        return false;
    return page->settings()->smartInsertDeleteEnabled();
}

bool EditorClientBlackBerry::isSelectTrailingWhitespaceEnabled()
{
    Page* page = WebPagePrivate::core(m_webPagePrivate->m_webPage);
    if (!page)
        return false;
    return page->settings()->selectTrailingWhitespaceEnabled();
}

void EditorClientBlackBerry::enableSpellChecking(bool enable)
{
    m_spellCheckState = enable ? SpellCheckDefault : SpellCheckOff;
}

bool EditorClientBlackBerry::shouldSpellCheckFocusedField()
{
    const Frame* frame = m_webPagePrivate->focusedOrMainFrame();
    if (!frame || !frame->document() || !frame->editor())
        return false;

    const Node* node = frame->document()->focusedElement();
    // NOTE: This logic is taken from EditorClientImpl::shouldSpellcheckByDefault
    // If |node| is null, we default to allowing spellchecking. This is done in
    // order to mitigate the issue when the user clicks outside the textbox, as a
    // result of which |node| becomes null, resulting in all the spell check
    // markers being deleted. Also, the Frame will decide not to do spellchecking
    // if the user can't edit - so returning true here will not cause any problems
    // to the Frame's behavior.
    if (!node)
        return true;

    // If the field does not support autocomplete, do not do spellchecking.
    if (node->isElementNode()) {
        const Element* element = toElement(node);
        if (isHTMLInputElement(element) && !DOMSupport::elementSupportsAutocomplete(element))
            return false;
    }

    // Check if the node disables spell checking directly.
    return frame->editor().isSpellCheckingEnabledInFocusedNode();
}

bool EditorClientBlackBerry::isContinuousSpellCheckingEnabled()
{
    if (m_spellCheckState == SpellCheckOff)
        return false;
    if (m_spellCheckState == SpellCheckOn)
        return true;
    return shouldSpellCheckFocusedField();
}

void EditorClientBlackBerry::toggleContinuousSpellChecking()
{
    // Use the current state to determine how to toggle, if it hasn't
    // been explicitly set, it will toggle based on the field type.
    if (isContinuousSpellCheckingEnabled())
        m_spellCheckState = SpellCheckOff;
    else
        m_spellCheckState = SpellCheckOn;
}

bool EditorClientBlackBerry::isGrammarCheckingEnabled()
{
    notImplemented();
    return false;
}

void EditorClientBlackBerry::toggleGrammarChecking()
{
    notImplemented();
}

int EditorClientBlackBerry::spellCheckerDocumentTag()
{
    notImplemented();
    return 0;
}

bool EditorClientBlackBerry::shouldBeginEditing(Range* range)
{
    if (m_webPagePrivate->m_dumpRenderTree)
        return m_webPagePrivate->m_dumpRenderTree->shouldBeginEditingInDOMRange(range);

    return true;
}

bool EditorClientBlackBerry::shouldEndEditing(Range* range)
{
    if (m_webPagePrivate->m_dumpRenderTree)
        return m_webPagePrivate->m_dumpRenderTree->shouldEndEditingInDOMRange(range);
    return true;
}

bool EditorClientBlackBerry::shouldInsertNode(Node* node, Range* range, EditorInsertAction insertAction)
{
    if (m_webPagePrivate->m_dumpRenderTree)
        return m_webPagePrivate->m_dumpRenderTree->shouldInsertNode(node, range, static_cast<int>(insertAction));
    return true;
}

bool EditorClientBlackBerry::shouldInsertText(const WTF::String& text, Range* range, EditorInsertAction insertAction)
{
    if (m_webPagePrivate->m_dumpRenderTree)
        return m_webPagePrivate->m_dumpRenderTree->shouldInsertText(text, range, static_cast<int>(insertAction));
    return true;
}

bool EditorClientBlackBerry::shouldChangeSelectedRange(Range* fromRange, Range* toRange, EAffinity affinity, bool stillSelecting)
{
    if (m_webPagePrivate->m_dumpRenderTree)
        return m_webPagePrivate->m_dumpRenderTree->shouldChangeSelectedDOMRangeToDOMRangeAffinityStillSelecting(fromRange, toRange, static_cast<int>(affinity), stillSelecting);

    Frame* frame = m_webPagePrivate->focusedOrMainFrame();
    if (frame && frame->document()) {
        if (Element* focusedElement = frame->document()->focusedElement()) {
            if (focusedElement->hasTagName(HTMLNames::selectTag))
                return false;
            if (DOMSupport::isPopupInputField(focusedElement))
                return false;
        }

        // Check if this change does not represent a focus change and input is active and if so ensure the keyboard is visible.
        if (m_webPagePrivate->m_inputHandler->isInputMode() && fromRange && toRange && (fromRange->startContainer() == toRange->startContainer()))
            m_webPagePrivate->m_inputHandler->notifyClientOfKeyboardVisibilityChange(true);
    }

    return true;
}

bool EditorClientBlackBerry::shouldApplyStyle(StylePropertySet*, Range*)
{
    notImplemented();
    return true;
}

bool EditorClientBlackBerry::shouldMoveRangeAfterDelete(Range*, Range*)
{
    notImplemented();
    return true;
}

void EditorClientBlackBerry::didBeginEditing()
{
    if (m_webPagePrivate->m_dumpRenderTree)
        m_webPagePrivate->m_dumpRenderTree->didBeginEditing();
}

void EditorClientBlackBerry::respondToChangedContents()
{
    if (m_webPagePrivate->m_dumpRenderTree)
        m_webPagePrivate->m_dumpRenderTree->didChange();
}

void EditorClientBlackBerry::respondToChangedSelection(Frame* frame)
{
    if (m_waitingForCursorFocus)
        m_waitingForCursorFocus = false;
    else
        m_webPagePrivate->selectionChanged(frame);

    if (m_webPagePrivate->m_dumpRenderTree)
        m_webPagePrivate->m_dumpRenderTree->didChangeSelection();
}

void EditorClientBlackBerry::didEndEditing()
{
    if (m_webPagePrivate->m_dumpRenderTree)
        m_webPagePrivate->m_dumpRenderTree->didEndEditing();
}

void EditorClientBlackBerry::respondToSelectionAppearanceChange()
{
    m_webPagePrivate->m_selectionHandler->selectionPositionChanged();
}

void EditorClientBlackBerry::didWriteSelectionToPasteboard()
{
    notImplemented();
}

void EditorClientBlackBerry::willWriteSelectionToPasteboard(WebCore::Range*)
{
    notImplemented();
}

void EditorClientBlackBerry::getClientPasteboardDataForRange(WebCore::Range*, Vector<String>&, Vector<RefPtr<WebCore::SharedBuffer> >&)
{
    notImplemented();
}

void EditorClientBlackBerry::didSetSelectionTypesForPasteboard()
{
    notImplemented();
}

void EditorClientBlackBerry::registerUndoStep(PassRefPtr<UndoStep> step)
{
    // Remove the oldest item if we've reached the maximum capacity for the stack.
    if (m_undoStack.size() == maximumUndoStackDepth)
        m_undoStack.removeFirst();

    if (!m_inRedo)
        m_redoStack.clear();

    m_undoStack.append(step);
}

void EditorClientBlackBerry::registerRedoStep(PassRefPtr<UndoStep> step)
{
    m_redoStack.append(step);
}

void EditorClientBlackBerry::clearUndoRedoOperations()
{
    m_undoStack.clear();
    m_redoStack.clear();
}

bool EditorClientBlackBerry::canUndo() const
{
    return !m_undoStack.isEmpty();
}

bool EditorClientBlackBerry::canRedo() const
{
    return !m_redoStack.isEmpty();
}

bool EditorClientBlackBerry::canCopyCut(Frame*, bool defaultValue) const
{
    return defaultValue;
}

bool EditorClientBlackBerry::canPaste(Frame*, bool defaultValue) const
{
    return defaultValue;
}

void EditorClientBlackBerry::undo()
{
    if (canUndo()) {
        EditCommandStack::iterator back = --m_undoStack.end();
        RefPtr<UndoStep> command(*back);
        m_undoStack.remove(back);

        // Unapply will call us back to push this command onto the redo stack.
        command->unapply();
    }
}

void EditorClientBlackBerry::redo()
{
    if (canRedo()) {
        EditCommandStack::iterator back = --m_redoStack.end();
        RefPtr<UndoStep> command(*back);
        m_redoStack.remove(back);

        ASSERT(!m_inRedo);
        m_inRedo = true;

        // Reapply will call us back to push this command onto the undo stack.
        command->reapply();
        m_inRedo = false;
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
    { VK_DOWN,   0,                  "MoveDown"                                    },
    { VK_DOWN,   ShiftKey,           "MoveDownAndModifySelection"                  },
    { VK_PRIOR,  0,                  "MovePageUp"                                  },
    { VK_PRIOR,  ShiftKey,           "MovePageUpAndModifySelection"                },
    { VK_NEXT,   0,                  "MovePageDown"                                },
    { VK_NEXT,   ShiftKey,           "MovePageDownAndModifySelection"              },
    { VK_HOME,   0,                  "MoveToBeginningOfLine"                       },
    { VK_HOME,   ShiftKey,           "MoveToBeginningOfLineAndModifySelection"     },
    { VK_HOME,   CtrlKey,            "MoveToBeginningOfDocument"                   },
    { VK_HOME,   CtrlKey | ShiftKey, "MoveToBeginningOfDocumentAndModifySelection" },
    { VK_END,    0,                  "MoveToEndOfLine"                             },
    { VK_END,    ShiftKey,           "MoveToEndOfLineAndModifySelection"           },
    { VK_END,    CtrlKey,            "MoveToEndOfDocument"                         },
    { VK_END,    CtrlKey | ShiftKey, "MoveToEndOfDocumentAndModifySelection"       },

    { 'B',       CtrlKey,            "ToggleBold"                                  },
    { 'I',       CtrlKey,            "ToggleItalic"                                },
    { 'U',       CtrlKey,            "ToggleUnderline"                             },

    { VK_BACK,   0,                  "DeleteBackward"                              },
    { VK_BACK,   ShiftKey,           "DeleteBackward"                              },
    { VK_DELETE, 0,                  "DeleteForward"                               },
    { VK_BACK,   CtrlKey,            "DeleteWordBackward"                          },
    { VK_DELETE, CtrlKey,            "DeleteWordForward"                           },

    { 'C',       CtrlKey,            "Copy"                                        },
    { 'V',       CtrlKey,            "Paste"                                       },
    { 'V',       CtrlKey | ShiftKey, "PasteAndMatchStyle"                          },
    { 'X',       CtrlKey,            "Cut"                                         },
    { VK_INSERT, CtrlKey,            "Copy"                                        },
    { VK_DELETE, ShiftKey,           "Cut"                                         },
    { VK_INSERT, ShiftKey,           "Paste"                                       },

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
    { '\r',   ShiftKey,           "InsertLineBreak"                             },
    { '\r',   AltKey | ShiftKey,  "InsertNewline"                               },
};


const char* EditorClientBlackBerry::interpretKeyEvent(const KeyboardEvent* event)
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

void EditorClientBlackBerry::handleKeyboardEvent(KeyboardEvent* event)
{
    ASSERT(event);

    const PlatformKeyboardEvent* platformEvent = event->keyEvent();
    if (!platformEvent)
        return;

    ASSERT(event->target()->toNode());
    Frame* frame = event->target()->toNode()->document()->frame();
    ASSERT(frame);

    String commandName = interpretKeyEvent(event);

    // Check to see we are not trying to insert text on key down.
    ASSERT(!(event->type() == eventNames().keydownEvent && frame->editor().command(commandName).isTextInsertion()));

    if (!commandName.isEmpty()) {
        // Hot key handling. Cancel processing mode.
        if (commandName != "DeleteBackward")
            m_webPagePrivate->m_inputHandler->setProcessingChange(false);

        if (frame->editor().command(commandName).execute())
            event->setDefaultHandled();
        return;
    }

    if (!frame->editor().canEdit())
        return;

    // Text insertion commands should only be triggered from keypressEvent.
    // There is an assert guaranteeing this in
    // EventHandler::handleTextInputEvent. Note that windowsVirtualKeyCode
    // is not set for keypressEvent: special keys should have been already
    // handled in keydownEvent, which is called first.
    if (event->type() != eventNames().keypressEvent)
        return;

    // Don't insert null or control characters as they can result in unexpected behaviour.
    if (event->charCode() < ' ')
        return;

    // Don't insert anything if a modifier is pressed.
    if (event->ctrlKey() || event->altKey())
        return;

    if (!platformEvent->text().isEmpty()) {
        if (frame->editor().insertText(platformEvent->text(), event))
            event->setDefaultHandled();
    }
}

void EditorClientBlackBerry::handleInputMethodKeydown(KeyboardEvent*)
{
    notImplemented();
}

void EditorClientBlackBerry::textFieldDidBeginEditing(Element*)
{
    notImplemented();
}

void EditorClientBlackBerry::textFieldDidEndEditing(Element* element)
{
    if (m_webPagePrivate->m_webSettings->isFormAutofillEnabled()) {
        if (HTMLInputElement* inputElement = element->toInputElement())
            m_webPagePrivate->m_autofillManager->textFieldDidEndEditing(inputElement);
    }
}

void EditorClientBlackBerry::textDidChangeInTextField(Element* element)
{
    if (m_webPagePrivate->m_webSettings->isFormAutofillEnabled()) {
        if (HTMLInputElement* inputElement = element->toInputElement())
            m_webPagePrivate->m_autofillManager->didChangeInTextField(inputElement);
    }
}

bool EditorClientBlackBerry::doTextFieldCommandFromEvent(Element*, KeyboardEvent*)
{
    notImplemented();
    return false;
}

void EditorClientBlackBerry::textWillBeDeletedInTextField(Element*)
{
    notImplemented();
}

void EditorClientBlackBerry::textDidChangeInTextArea(Element*)
{
    notImplemented();
}

bool EditorClientBlackBerry::shouldEraseMarkersAfterChangeSelection(TextCheckingType) const
{
    const Frame* frame = m_webPagePrivate->focusedOrMainFrame();
    return !frame || !frame->settings() || (!frame->settings()->asynchronousSpellCheckingEnabled() && !frame->settings()->unifiedTextCheckerEnabled());
}

void EditorClientBlackBerry::ignoreWordInSpellDocument(const WTF::String&)
{
    notImplemented();
}

void EditorClientBlackBerry::learnWord(const WTF::String&)
{
    notImplemented();
}

void EditorClientBlackBerry::checkSpellingOfString(const UChar*, int, int*, int*)
{
    notImplemented();
}

WTF::String EditorClientBlackBerry::getAutoCorrectSuggestionForMisspelledWord(const WTF::String&)
{
    notImplemented();
    return WTF::String();
}

void EditorClientBlackBerry::checkGrammarOfString(const UChar*, int, WTF::Vector<GrammarDetail, 0u>&, int*, int*)
{
    notImplemented();
}

void EditorClientBlackBerry::requestCheckingOfString(PassRefPtr<TextCheckingRequest> textCheckingRequest)
{
    RefPtr<SpellCheckRequest> spellCheckRequest = static_cast<SpellCheckRequest*>(textCheckingRequest.get());
    m_webPagePrivate->m_inputHandler->requestCheckingOfString(spellCheckRequest);
}

void EditorClientBlackBerry::checkTextOfParagraph(const UChar*, int, TextCheckingTypeMask, Vector<TextCheckingResult>&)
{
    notImplemented();
}

TextCheckerClient* EditorClientBlackBerry::textChecker()
{
    return this;
}

void EditorClientBlackBerry::updateSpellingUIWithGrammarString(const WTF::String&, const GrammarDetail&)
{
    notImplemented();
}

void EditorClientBlackBerry::updateSpellingUIWithMisspelledWord(const WTF::String&)
{
    notImplemented();
}

void EditorClientBlackBerry::showSpellingUI(bool)
{
    notImplemented();
}

bool EditorClientBlackBerry::spellingUIIsShowing()
{
    notImplemented();
    return false;
}

void EditorClientBlackBerry::getGuessesForWord(const WTF::String&, WTF::Vector<WTF::String, 0u>&)
{
    notImplemented();
}

void EditorClientBlackBerry::getGuessesForWord(const String&, const String&, Vector<String>&)
{
    notImplemented();
}

void EditorClientBlackBerry::willSetInputMethodState()
{
    notImplemented();
}

void EditorClientBlackBerry::setInputMethodState(bool)
{
    m_webPagePrivate->m_inputHandler->focusedNodeChanged();
}

} // namespace WebCore
