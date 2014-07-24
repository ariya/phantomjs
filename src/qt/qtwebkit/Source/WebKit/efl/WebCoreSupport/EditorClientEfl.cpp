/*
 *  Copyright (C) 2007 Alp Toker <alp@atoker.com>
 *  Copyright (C) 2008 Nuanti Ltd.
 *  Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
 *  Copyright (C) 2009-2010 ProFUSION embedded systems
 *  Copyright (C) 2009-2010 Samsung Electronics
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
#include "EditorClientEfl.h"

#include "Document.h"
#include "DumpRenderTreeSupportEfl.h"
#include "Editor.h"
#include "EflKeyboardUtilities.h"
#include "EventNames.h"
#include "FocusController.h"
#include "Frame.h"
#include "KeyboardEvent.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PlatformKeyboardEvent.h"
#include "Settings.h"
#include "UndoStep.h"
#include "WindowsKeyboardCodes.h"
#include "ewk_frame_private.h"
#include "ewk_private.h"
#include "ewk_view_private.h"

using namespace WebCore;

namespace WebCore {

void EditorClientEfl::willSetInputMethodState()
{
    notImplemented();
}

void EditorClientEfl::setInputMethodState(bool active)
{
    ewk_view_input_method_state_set(m_view, active);
}

bool EditorClientEfl::shouldDeleteRange(Range* range)
{
    evas_object_smart_callback_call(m_view, "editorclient,range,delete", range);
    return true;
}

bool EditorClientEfl::isContinuousSpellCheckingEnabled()
{
    notImplemented();
    return false;
}

bool EditorClientEfl::isGrammarCheckingEnabled()
{
    notImplemented();
    return false;
}

int EditorClientEfl::spellCheckerDocumentTag()
{
    notImplemented();
    return 0;
}

bool EditorClientEfl::shouldBeginEditing(Range* range)
{
    evas_object_smart_callback_call(m_view, "editorclient,editing,begin", range);
    return true;
}

bool EditorClientEfl::shouldEndEditing(Range* range)
{
    evas_object_smart_callback_call(m_view, "editorclient,editing,end", range);
    return true;
}

bool EditorClientEfl::shouldInsertText(const String& text, Range* range, EditorInsertAction action)
{
    CString protectedText = text.utf8();
    Ewk_Should_Insert_Text_Event shouldInsertTextEvent = { protectedText.data(), range, action };
    evas_object_smart_callback_call(m_view, "editorclient,text,insert", &shouldInsertTextEvent);
    return true;
}

bool EditorClientEfl::shouldChangeSelectedRange(Range* fromRange, Range* toRange, EAffinity affinity, bool stillSelecting)
{
    Ewk_Should_Change_Selected_Range_Event shouldChangeSelectedRangeEvent = { fromRange, toRange, affinity, stillSelecting };
    evas_object_smart_callback_call(m_view, "editorclient,selected,range,change", &shouldChangeSelectedRangeEvent);
    return true;
}

bool EditorClientEfl::shouldApplyStyle(StylePropertySet* style, Range* range)
{
    Ewk_Should_Apply_Style_Event shouldApplyStyleEvent = { style, range };
    evas_object_smart_callback_call(m_view, "editorclient,style,apply", &shouldApplyStyleEvent);
    return true;
}

bool EditorClientEfl::shouldMoveRangeAfterDelete(Range*, Range*)
{
    notImplemented();
    return true;
}

void EditorClientEfl::didBeginEditing()
{
    evas_object_smart_callback_call(m_view, "editorclient,editing,began", 0);
}

void EditorClientEfl::respondToChangedContents()
{
    Evas_Object* frame = ewk_view_frame_focused_get(m_view);

    if (!frame)
        frame = ewk_view_frame_main_get(m_view);

    if (!frame)
        return;

    ewk_frame_editor_client_contents_changed(frame);
}

void EditorClientEfl::respondToChangedSelection(Frame* coreFrame)
{
    if (!coreFrame)
        return;

    if (coreFrame->editor().ignoreCompositionSelectionChange())
        return;

    Evas_Object* webFrame = EWKPrivate::kitFrame(coreFrame);
    ewk_frame_editor_client_selection_changed(webFrame);

    coreFrame->editor().cancelCompositionIfSelectionIsInvalid();
}

void EditorClientEfl::didEndEditing()
{
    evas_object_smart_callback_call(m_view, "editorclient,editing,ended", 0);
}

void EditorClientEfl::didWriteSelectionToPasteboard()
{
    notImplemented();
}

void EditorClientEfl::willWriteSelectionToPasteboard(WebCore::Range*)
{
}

void EditorClientEfl::getClientPasteboardDataForRange(WebCore::Range*, Vector<String>&, Vector<RefPtr<WebCore::SharedBuffer> >&)
{
}

void EditorClientEfl::didSetSelectionTypesForPasteboard()
{
    notImplemented();
}

void EditorClientEfl::registerUndoStep(WTF::PassRefPtr<UndoStep> step)
{
    if (!m_isInRedo)
        redoStack.clear();
    undoStack.prepend(step);
}

void EditorClientEfl::registerRedoStep(WTF::PassRefPtr<UndoStep> step)
{
    redoStack.prepend(step);
}

void EditorClientEfl::clearUndoRedoOperations()
{
    undoStack.clear();
    redoStack.clear();
}

bool EditorClientEfl::canCopyCut(Frame*, bool defaultValue) const
{
    return defaultValue;
}

bool EditorClientEfl::canPaste(Frame*, bool defaultValue) const
{
    return defaultValue;
}

bool EditorClientEfl::canUndo() const
{
    return !undoStack.isEmpty();
}

bool EditorClientEfl::canRedo() const
{
    return !redoStack.isEmpty();
}

void EditorClientEfl::undo()
{
    if (canUndo()) {
        RefPtr<WebCore::UndoStep> step = undoStack.takeFirst();
        step->unapply();
    }
}

void EditorClientEfl::redo()
{
    if (canRedo()) {
        RefPtr<WebCore::UndoStep> step = redoStack.takeFirst();

        ASSERT(!m_isInRedo);
        m_isInRedo = true;
        step->reapply();
        m_isInRedo = false;
    }
}

bool EditorClientEfl::shouldInsertNode(Node* node, Range* range, EditorInsertAction action)
{
    Ewk_Should_Insert_Node_Event insertNodeEvent = { node, range, action };
    evas_object_smart_callback_call(m_view, "editorclient,node,insert", &insertNodeEvent);
    return true;
}

void EditorClientEfl::pageDestroyed()
{
    delete this;
}

bool EditorClientEfl::smartInsertDeleteEnabled()
{
    WebCore::Page* corePage = EWKPrivate::corePage(m_view);
    if (!corePage)
        return false;
    return corePage->settings()->smartInsertDeleteEnabled();
}

bool EditorClientEfl::isSelectTrailingWhitespaceEnabled()
{
    WebCore::Page* corePage = EWKPrivate::corePage(m_view);
    if (!corePage)
        return false;
    return corePage->settings()->selectTrailingWhitespaceEnabled();
}

void EditorClientEfl::toggleContinuousSpellChecking()
{
    notImplemented();
}

void EditorClientEfl::toggleGrammarChecking()
{
    notImplemented();
}

const char* EditorClientEfl::interpretKeyEvent(const KeyboardEvent* event)
{
    ASSERT(event->type() == eventNames().keydownEvent || event->type() == eventNames().keypressEvent);

    if (event->type() == eventNames().keydownEvent)
        return getKeyDownCommandName(event);

    return getKeyPressCommandName(event);
}

bool EditorClientEfl::handleEditingKeyboardEvent(KeyboardEvent* event)
{
    Node* node = event->target()->toNode();
    ASSERT(node);
    Frame* frame = node->document()->frame();
    ASSERT(frame);

    const PlatformKeyboardEvent* keyEvent = event->keyEvent();
    if (!keyEvent)
        return false;

    if (!frame->settings())
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

    // Don't allow text insertion for nodes that cannot edit.
    if (!frame->editor().canEdit())
        return false;

    // Don't insert null or control characters as they can result in unexpected behaviour
    if (event->charCode() < ' ')
        return false;

    // Don't insert anything if a modifier is pressed
    if (keyEvent->ctrlKey() || keyEvent->altKey())
        return false;

    return frame->editor().insertText(event->keyEvent()->text(), event);
}

void EditorClientEfl::handleKeyboardEvent(KeyboardEvent* event)
{
    if (handleEditingKeyboardEvent(event))
        event->setDefaultHandled();
}

void EditorClientEfl::handleInputMethodKeydown(KeyboardEvent*)
{
}

EditorClientEfl::EditorClientEfl(Evas_Object* view)
    : m_isInRedo(false)
    , m_view(view)
{
    notImplemented();
}

EditorClientEfl::~EditorClientEfl()
{
    notImplemented();
}

void EditorClientEfl::textFieldDidBeginEditing(Element*)
{
}

void EditorClientEfl::textFieldDidEndEditing(Element*)
{
    notImplemented();
}

void EditorClientEfl::textDidChangeInTextField(Element*)
{
    notImplemented();
}

bool EditorClientEfl::doTextFieldCommandFromEvent(Element*, KeyboardEvent*)
{
    return false;
}

void EditorClientEfl::textWillBeDeletedInTextField(Element*)
{
    notImplemented();
}

void EditorClientEfl::textDidChangeInTextArea(Element*)
{
    notImplemented();
}

bool EditorClientEfl::shouldEraseMarkersAfterChangeSelection(TextCheckingType) const
{
    return true;
}

void EditorClientEfl::ignoreWordInSpellDocument(const String&)
{
    notImplemented();
}

void EditorClientEfl::learnWord(const String&)
{
    notImplemented();
}

void EditorClientEfl::checkSpellingOfString(const UChar*, int, int*, int*)
{
    notImplemented();
}

String EditorClientEfl::getAutoCorrectSuggestionForMisspelledWord(const String&)
{
    notImplemented();
    return String();
}

void EditorClientEfl::checkGrammarOfString(const UChar*, int, Vector<GrammarDetail>&, int*, int*)
{
    notImplemented();
}

void EditorClientEfl::updateSpellingUIWithGrammarString(const String&, const GrammarDetail&)
{
    notImplemented();
}

void EditorClientEfl::updateSpellingUIWithMisspelledWord(const String&)
{
    notImplemented();
}

void EditorClientEfl::showSpellingUI(bool)
{
    notImplemented();
}

bool EditorClientEfl::spellingUIIsShowing()
{
    notImplemented();
    return false;
}

void EditorClientEfl::getGuessesForWord(const String& /*word*/, const String& /*context*/, Vector<String>& /*guesses*/)
{
    notImplemented();
}

}
