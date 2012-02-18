/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
#include "Editor.h"

#include "AXObjectCache.h"
#include "ApplyStyleCommand.h"
#include "CSSComputedStyleDeclaration.h"
#include "CSSMutableStyleDeclaration.h"
#include "CSSProperty.h"
#include "CSSPropertyNames.h"
#include "CSSStyleSelector.h"
#include "CSSValueKeywords.h"
#include "CachedResourceLoader.h"
#include "ClipboardEvent.h"
#include "CompositionEvent.h"
#include "SpellingCorrectionController.h"
#include "CreateLinkCommand.h"
#include "DeleteButtonController.h"
#include "DeleteSelectionCommand.h"
#include "DocumentFragment.h"
#include "DocumentMarkerController.h"
#include "EditingText.h"
#include "EditorClient.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLInputElement.h"
#include "HTMLTextAreaElement.h"
#include "HitTestResult.h"
#include "IndentOutdentCommand.h"
#include "InsertListCommand.h"
#include "KeyboardEvent.h"
#include "KillRing.h"
#include "ModifySelectionListLevel.h"
#include "NodeList.h"
#include "Page.h"
#include "Pasteboard.h"
#include "TextCheckingHelper.h"
#include "RemoveFormatCommand.h"
#include "RenderBlock.h"
#include "RenderPart.h"
#include "RenderTextControl.h"
#include "ReplaceSelectionCommand.h"
#include "Settings.h"
#include "Sound.h"
#include "SpellChecker.h"
#include "SpellingCorrectionCommand.h"
#include "Text.h"
#include "TextEvent.h"
#include "TextIterator.h"
#include "TypingCommand.h"
#include "UserTypingGestureIndicator.h"
#include "htmlediting.h"
#include "markup.h"
#include "visible_units.h"
#include <wtf/UnusedParam.h>
#include <wtf/unicode/CharacterNames.h>
#include <wtf/unicode/Unicode.h>

namespace WebCore {

using namespace std;
using namespace HTMLNames;
using namespace WTF;
using namespace Unicode;

// When an event handler has moved the selection outside of a text control
// we should use the target control's selection for this editing operation.
VisibleSelection Editor::selectionForCommand(Event* event)
{
    VisibleSelection selection = m_frame->selection()->selection();
    if (!event)
        return selection;
    // If the target is a text control, and the current selection is outside of its shadow tree,
    // then use the saved selection for that text control.
    Node* target = event->target()->toNode();
    Node* selectionStart = selection.start().deprecatedNode();
    if (target && (!selectionStart || target->shadowAncestorNode() != selectionStart->shadowAncestorNode())) {
        RefPtr<Range> range;
        if (target->hasTagName(inputTag) && static_cast<HTMLInputElement*>(target)->isTextField())
            range = static_cast<HTMLInputElement*>(target)->selection();
        else if (target->hasTagName(textareaTag))
            range = static_cast<HTMLTextAreaElement*>(target)->selection();

        if (range)
            return VisibleSelection(range.get());
    }
    return selection;
}

// Function considers Mac editing behavior a fallback when Page or Settings is not available.
EditingBehavior Editor::behavior() const
{
    if (!m_frame || !m_frame->settings())
        return EditingBehavior(EditingMacBehavior);

    return EditingBehavior(m_frame->settings()->editingBehaviorType());
}

EditorClient* Editor::client() const
{
    if (Page* page = m_frame->page())
        return page->editorClient();
    return 0;
}


TextCheckerClient* Editor::textChecker() const
{
    if (EditorClient* owner = client())
        return owner->textChecker();
    return 0;
}

void Editor::handleKeyboardEvent(KeyboardEvent* event)
{
    if (EditorClient* c = client())
        c->handleKeyboardEvent(event);
}

void Editor::handleInputMethodKeydown(KeyboardEvent* event)
{
    if (EditorClient* c = client())
        c->handleInputMethodKeydown(event);
}

bool Editor::handleTextEvent(TextEvent* event)
{
    // Default event handling for Drag and Drop will be handled by DragController
    // so we leave the event for it.
    if (event->isDrop())
        return false;

    if (event->isPaste()) {
        if (event->pastingFragment())
            replaceSelectionWithFragment(event->pastingFragment(), false, event->shouldSmartReplace(), event->shouldMatchStyle());
        else 
            replaceSelectionWithText(event->data(), false, event->shouldSmartReplace());
        return true;
    }

    String data = event->data();
    if (data == "\n") {
        if (event->isLineBreak())
            return insertLineBreak();
        return insertParagraphSeparator();
    }

    return insertTextWithoutSendingTextEvent(data, false, event);
}

bool Editor::canEdit() const
{
    return m_frame->selection()->rootEditableElement();
}

bool Editor::canEditRichly() const
{
    return m_frame->selection()->isContentRichlyEditable();
}

// WinIE uses onbeforecut and onbeforepaste to enables the cut and paste menu items.  They
// also send onbeforecopy, apparently for symmetry, but it doesn't affect the menu items.
// We need to use onbeforecopy as a real menu enabler because we allow elements that are not
// normally selectable to implement copy/paste (like divs, or a document body).

bool Editor::canDHTMLCut()
{
    return !m_frame->selection()->isInPasswordField() && !dispatchCPPEvent(eventNames().beforecutEvent, ClipboardNumb);
}

bool Editor::canDHTMLCopy()
{
    return !m_frame->selection()->isInPasswordField() && !dispatchCPPEvent(eventNames().beforecopyEvent, ClipboardNumb);
}

bool Editor::canDHTMLPaste()
{
    return !dispatchCPPEvent(eventNames().beforepasteEvent, ClipboardNumb);
}

bool Editor::canCut() const
{
    return canCopy() && canDelete();
}

static HTMLImageElement* imageElementFromImageDocument(Document* document)
{
    if (!document)
        return 0;
    if (!document->isImageDocument())
        return 0;
    
    HTMLElement* body = document->body();
    if (!body)
        return 0;
    
    Node* node = body->firstChild();
    if (!node)
        return 0;    
    if (!node->hasTagName(imgTag))
        return 0;
    return static_cast<HTMLImageElement*>(node);
}

bool Editor::canCopy() const
{
    if (imageElementFromImageDocument(m_frame->document()))
        return true;
    SelectionController* selection = m_frame->selection();
    return selection->isRange() && !selection->isInPasswordField();
}

bool Editor::canPaste() const
{
    return canEdit();
}

bool Editor::canDelete() const
{
    SelectionController* selection = m_frame->selection();
    return selection->isRange() && selection->rootEditableElement();
}

bool Editor::canDeleteRange(Range* range) const
{
    ExceptionCode ec = 0;
    Node* startContainer = range->startContainer(ec);
    Node* endContainer = range->endContainer(ec);
    if (!startContainer || !endContainer)
        return false;
    
    if (!startContainer->rendererIsEditable() || !endContainer->rendererIsEditable())
        return false;
    
    if (range->collapsed(ec)) {
        VisiblePosition start(Position(startContainer, range->startOffset(ec), Position::PositionIsOffsetInAnchor), DOWNSTREAM);
        VisiblePosition previous = start.previous();
        // FIXME: We sometimes allow deletions at the start of editable roots, like when the caret is in an empty list item.
        if (previous.isNull() || previous.deepEquivalent().deprecatedNode()->rootEditableElement() != startContainer->rootEditableElement())
            return false;
    }
    return true;
}

bool Editor::smartInsertDeleteEnabled()
{   
    return client() && client()->smartInsertDeleteEnabled();
}
    
bool Editor::canSmartCopyOrDelete()
{
    return client() && client()->smartInsertDeleteEnabled() && m_frame->selection()->granularity() == WordGranularity;
}

bool Editor::isSelectTrailingWhitespaceEnabled()
{
    return client() && client()->isSelectTrailingWhitespaceEnabled();
}

bool Editor::deleteWithDirection(SelectionDirection direction, TextGranularity granularity, bool killRing, bool isTypingAction)
{
    if (!canEdit())
        return false;

    if (m_frame->selection()->isRange()) {
        if (isTypingAction) {
            TypingCommand::deleteKeyPressed(m_frame->document(), canSmartCopyOrDelete() ? TypingCommand::SmartDelete : 0, granularity);
            revealSelectionAfterEditingOperation();
        } else {
            if (killRing)
                addToKillRing(selectedRange().get(), false);
            deleteSelectionWithSmartDelete(canSmartCopyOrDelete());
            // Implicitly calls revealSelectionAfterEditingOperation().
        }
    } else {
        TypingCommand::Options options = 0;
        if (canSmartCopyOrDelete())
            options |= TypingCommand::SmartDelete;
        if (killRing)
            options |= TypingCommand::KillRing;
        switch (direction) {
        case DirectionForward:
        case DirectionRight:
            TypingCommand::forwardDeleteKeyPressed(m_frame->document(), options, granularity);
            break;
        case DirectionBackward:
        case DirectionLeft:
            TypingCommand::deleteKeyPressed(m_frame->document(), options, granularity);
            break;
        }
        revealSelectionAfterEditingOperation();
    }

    // FIXME: We should to move this down into deleteKeyPressed.
    // clear the "start new kill ring sequence" setting, because it was set to true
    // when the selection was updated by deleting the range
    if (killRing)
        setStartNewKillRingSequence(false);

    return true;
}

void Editor::deleteSelectionWithSmartDelete(bool smartDelete)
{
    if (m_frame->selection()->isNone())
        return;
    
    applyCommand(DeleteSelectionCommand::create(m_frame->document(), smartDelete));
}

void Editor::pasteAsPlainText(const String& pastingText, bool smartReplace)
{
    Node* target = findEventTargetFromSelection();
    if (!target)
        return;
    ExceptionCode ec = 0;
    target->dispatchEvent(TextEvent::createForPlainTextPaste(m_frame->domWindow(), pastingText, smartReplace), ec);
}

void Editor::pasteAsFragment(PassRefPtr<DocumentFragment> pastingFragment, bool smartReplace, bool matchStyle)
{
    Node* target = findEventTargetFromSelection();
    if (!target)
        return;
    ExceptionCode ec = 0;
    target->dispatchEvent(TextEvent::createForFragmentPaste(m_frame->domWindow(), pastingFragment, smartReplace, matchStyle), ec);
}

void Editor::pasteAsPlainTextBypassingDHTML()
{
    pasteAsPlainTextWithPasteboard(Pasteboard::generalPasteboard());
}

void Editor::pasteAsPlainTextWithPasteboard(Pasteboard* pasteboard)
{
    String text = pasteboard->plainText(m_frame);
    if (client() && client()->shouldInsertText(text, selectedRange().get(), EditorInsertActionPasted))
        pasteAsPlainText(text, canSmartReplaceWithPasteboard(pasteboard));
}

#if !PLATFORM(MAC)
void Editor::pasteWithPasteboard(Pasteboard* pasteboard, bool allowPlainText)
{
    RefPtr<Range> range = selectedRange();
    bool chosePlainText;
    RefPtr<DocumentFragment> fragment = pasteboard->documentFragment(m_frame, range, allowPlainText, chosePlainText);
    if (fragment && shouldInsertFragment(fragment, range, EditorInsertActionPasted))
        pasteAsFragment(fragment, canSmartReplaceWithPasteboard(pasteboard), chosePlainText);
}
#endif

bool Editor::canSmartReplaceWithPasteboard(Pasteboard* pasteboard)
{
    return client() && client()->smartInsertDeleteEnabled() && pasteboard->canSmartReplace();
}

bool Editor::shouldInsertFragment(PassRefPtr<DocumentFragment> fragment, PassRefPtr<Range> replacingDOMRange, EditorInsertAction givenAction)
{
    if (!client())
        return false;
    
    if (fragment) {
        Node* child = fragment->firstChild();
        if (child && fragment->lastChild() == child && child->isCharacterDataNode())
            return client()->shouldInsertText(static_cast<CharacterData*>(child)->data(), replacingDOMRange.get(), givenAction);        
    }

    return client()->shouldInsertNode(fragment.get(), replacingDOMRange.get(), givenAction);
}

void Editor::replaceSelectionWithFragment(PassRefPtr<DocumentFragment> fragment, bool selectReplacement, bool smartReplace, bool matchStyle)
{
    if (m_frame->selection()->isNone() || !fragment)
        return;

    ReplaceSelectionCommand::CommandOptions options = ReplaceSelectionCommand::PreventNesting;
    if (selectReplacement)
        options |= ReplaceSelectionCommand::SelectReplacement;
    if (smartReplace)
        options |= ReplaceSelectionCommand::SmartReplace;
    if (matchStyle)
        options |= ReplaceSelectionCommand::MatchStyle;
    applyCommand(ReplaceSelectionCommand::create(m_frame->document(), fragment, options, EditActionPaste));
    revealSelectionAfterEditingOperation();

    Node* nodeToCheck = m_frame->selection()->rootEditableElement();
    if (m_spellChecker->canCheckAsynchronously(nodeToCheck))
        m_spellChecker->requestCheckingFor(textCheckingTypeMaskFor(MarkSpelling | MarkGrammar), nodeToCheck);
}

void Editor::replaceSelectionWithText(const String& text, bool selectReplacement, bool smartReplace)
{
    replaceSelectionWithFragment(createFragmentFromText(selectedRange().get(), text), selectReplacement, smartReplace, true); 
}

PassRefPtr<Range> Editor::selectedRange()
{
    if (!m_frame)
        return 0;
    return m_frame->selection()->toNormalizedRange();
}

bool Editor::shouldDeleteRange(Range* range) const
{
    ExceptionCode ec;
    if (!range || range->collapsed(ec))
        return false;
    
    if (!canDeleteRange(range))
        return false;

    return client() && client()->shouldDeleteRange(range);
}

bool Editor::tryDHTMLCopy()
{   
    if (m_frame->selection()->isInPasswordField())
        return false;

    if (canCopy())
        // Must be done before oncopy adds types and data to the pboard,
        // also done for security, as it erases data from the last copy/paste.
        Pasteboard::generalPasteboard()->clear();

    return !dispatchCPPEvent(eventNames().copyEvent, ClipboardWritable);
}

bool Editor::tryDHTMLCut()
{
    if (m_frame->selection()->isInPasswordField())
        return false;
    
    if (canCut())
        // Must be done before oncut adds types and data to the pboard,
        // also done for security, as it erases data from the last copy/paste.
        Pasteboard::generalPasteboard()->clear();

    return !dispatchCPPEvent(eventNames().cutEvent, ClipboardWritable);
}

bool Editor::tryDHTMLPaste()
{
    return !dispatchCPPEvent(eventNames().pasteEvent, ClipboardReadable);
}

void Editor::writeSelectionToPasteboard(Pasteboard* pasteboard)
{
    pasteboard->writeSelection(selectedRange().get(), canSmartCopyOrDelete(), m_frame);
}

bool Editor::shouldInsertText(const String& text, Range* range, EditorInsertAction action) const
{
    return client() && client()->shouldInsertText(text, range, action);
}

bool Editor::shouldShowDeleteInterface(HTMLElement* element) const
{
    return client() && client()->shouldShowDeleteInterface(element);
}

void Editor::respondToChangedSelection(const VisibleSelection& oldSelection)
{
    if (client())
        client()->respondToChangedSelection();
    m_deleteButtonController->respondToChangedSelection(oldSelection);
    m_spellingCorrector->respondToChangedSelection(oldSelection);
}

void Editor::respondToChangedContents(const VisibleSelection& endingSelection)
{
    if (AXObjectCache::accessibilityEnabled()) {
        Node* node = endingSelection.start().deprecatedNode();
        if (node)
            m_frame->document()->axObjectCache()->postNotification(node->renderer(), AXObjectCache::AXValueChanged, false);
    }

    updateMarkersForWordsAffectedByEditing(true);

    if (client())
        client()->respondToChangedContents();
}

const SimpleFontData* Editor::fontForSelection(bool& hasMultipleFonts) const
{
#if !PLATFORM(QT)
    hasMultipleFonts = false;

    if (!m_frame->selection()->isRange()) {
        Node* nodeToRemove;
        RenderStyle* style = styleForSelectionStart(nodeToRemove); // sets nodeToRemove

        const SimpleFontData* result = 0;
        if (style)
            result = style->font().primaryFont();
        
        if (nodeToRemove) {
            ExceptionCode ec;
            nodeToRemove->remove(ec);
            ASSERT(!ec);
        }

        return result;
    }

    const SimpleFontData* font = 0;

    RefPtr<Range> range = m_frame->selection()->toNormalizedRange();
    Node* startNode = range->editingStartPosition().deprecatedNode();
    if (startNode) {
        Node* pastEnd = range->pastLastNode();
        // In the loop below, n should eventually match pastEnd and not become nil, but we've seen at least one
        // unreproducible case where this didn't happen, so check for nil also.
        for (Node* n = startNode; n && n != pastEnd; n = n->traverseNextNode()) {
            RenderObject* renderer = n->renderer();
            if (!renderer)
                continue;
            // FIXME: Are there any node types that have renderers, but that we should be skipping?
            const SimpleFontData* f = renderer->style()->font().primaryFont();
            if (!font)
                font = f;
            else if (font != f) {
                hasMultipleFonts = true;
                break;
            }
        }
    }

    return font;
#else
    return 0;
#endif
}

WritingDirection Editor::textDirectionForSelection(bool& hasNestedOrMultipleEmbeddings) const
{
    hasNestedOrMultipleEmbeddings = true;

    if (m_frame->selection()->isNone())
        return NaturalWritingDirection;

    Position position = m_frame->selection()->selection().start().downstream();

    Node* node = position.deprecatedNode();
    if (!node)
        return NaturalWritingDirection;

    Position end;
    if (m_frame->selection()->isRange()) {
        end = m_frame->selection()->selection().end().upstream();

        Node* pastLast = Range::create(m_frame->document(), position.parentAnchoredEquivalent(), end.parentAnchoredEquivalent())->pastLastNode();
        for (Node* n = node; n && n != pastLast; n = n->traverseNextNode()) {
            if (!n->isStyledElement())
                continue;

            RefPtr<CSSComputedStyleDeclaration> style = computedStyle(n);
            RefPtr<CSSValue> unicodeBidi = style->getPropertyCSSValue(CSSPropertyUnicodeBidi);
            if (!unicodeBidi || !unicodeBidi->isPrimitiveValue())
                continue;

            int unicodeBidiValue = static_cast<CSSPrimitiveValue*>(unicodeBidi.get())->getIdent();
            if (unicodeBidiValue == CSSValueEmbed || unicodeBidiValue == CSSValueBidiOverride)
                return NaturalWritingDirection;
        }
    }

    if (m_frame->selection()->isCaret()) {
        RefPtr<EditingStyle> typingStyle = m_frame->selection()->typingStyle();
        WritingDirection direction;
        if (typingStyle && typingStyle->textDirection(direction)) {
            hasNestedOrMultipleEmbeddings = false;
            return direction;
        }
        node = m_frame->selection()->selection().visibleStart().deepEquivalent().deprecatedNode();
    }

    // The selection is either a caret with no typing attributes or a range in which no embedding is added, so just use the start position
    // to decide.
    Node* block = enclosingBlock(node);
    WritingDirection foundDirection = NaturalWritingDirection;

    for (; node != block; node = node->parentNode()) {
        if (!node->isStyledElement())
            continue;

        RefPtr<CSSComputedStyleDeclaration> style = computedStyle(node);
        RefPtr<CSSValue> unicodeBidi = style->getPropertyCSSValue(CSSPropertyUnicodeBidi);
        if (!unicodeBidi || !unicodeBidi->isPrimitiveValue())
            continue;

        int unicodeBidiValue = static_cast<CSSPrimitiveValue*>(unicodeBidi.get())->getIdent();
        if (unicodeBidiValue == CSSValueNormal)
            continue;

        if (unicodeBidiValue == CSSValueBidiOverride)
            return NaturalWritingDirection;

        ASSERT(unicodeBidiValue == CSSValueEmbed);
        RefPtr<CSSValue> direction = style->getPropertyCSSValue(CSSPropertyDirection);
        if (!direction || !direction->isPrimitiveValue())
            continue;

        int directionValue = static_cast<CSSPrimitiveValue*>(direction.get())->getIdent();
        if (directionValue != CSSValueLtr && directionValue != CSSValueRtl)
            continue;

        if (foundDirection != NaturalWritingDirection)
            return NaturalWritingDirection;

        // In the range case, make sure that the embedding element persists until the end of the range.
        if (m_frame->selection()->isRange() && !end.deprecatedNode()->isDescendantOf(node))
            return NaturalWritingDirection;

        foundDirection = directionValue == CSSValueLtr ? LeftToRightWritingDirection : RightToLeftWritingDirection;
    }
    hasNestedOrMultipleEmbeddings = false;
    return foundDirection;
}

bool Editor::hasBidiSelection() const
{
    if (m_frame->selection()->isNone())
        return false;

    Node* startNode;
    if (m_frame->selection()->isRange()) {
        startNode = m_frame->selection()->selection().start().downstream().deprecatedNode();
        Node* endNode = m_frame->selection()->selection().end().upstream().deprecatedNode();
        if (enclosingBlock(startNode) != enclosingBlock(endNode))
            return false;
    } else
        startNode = m_frame->selection()->selection().visibleStart().deepEquivalent().deprecatedNode();

    RenderObject* renderer = startNode->renderer();
    while (renderer && !renderer->isRenderBlock())
        renderer = renderer->parent();

    if (!renderer)
        return false;

    RenderStyle* style = renderer->style();
    if (!style->isLeftToRightDirection())
        return true;

    return toRenderBlock(renderer)->containsNonZeroBidiLevel();
}

TriState Editor::selectionUnorderedListState() const
{
    if (m_frame->selection()->isCaret()) {
        if (enclosingNodeWithTag(m_frame->selection()->selection().start(), ulTag))
            return TrueTriState;
    } else if (m_frame->selection()->isRange()) {
        Node* startNode = enclosingNodeWithTag(m_frame->selection()->selection().start(), ulTag);
        Node* endNode = enclosingNodeWithTag(m_frame->selection()->selection().end(), ulTag);
        if (startNode && endNode && startNode == endNode)
            return TrueTriState;
    }

    return FalseTriState;
}

TriState Editor::selectionOrderedListState() const
{
    if (m_frame->selection()->isCaret()) {
        if (enclosingNodeWithTag(m_frame->selection()->selection().start(), olTag))
            return TrueTriState;
    } else if (m_frame->selection()->isRange()) {
        Node* startNode = enclosingNodeWithTag(m_frame->selection()->selection().start(), olTag);
        Node* endNode = enclosingNodeWithTag(m_frame->selection()->selection().end(), olTag);
        if (startNode && endNode && startNode == endNode)
            return TrueTriState;
    }

    return FalseTriState;
}

PassRefPtr<Node> Editor::insertOrderedList()
{
    if (!canEditRichly())
        return 0;
        
    RefPtr<Node> newList = InsertListCommand::insertList(m_frame->document(), InsertListCommand::OrderedList);
    revealSelectionAfterEditingOperation();
    return newList;
}

PassRefPtr<Node> Editor::insertUnorderedList()
{
    if (!canEditRichly())
        return 0;
        
    RefPtr<Node> newList = InsertListCommand::insertList(m_frame->document(), InsertListCommand::UnorderedList);
    revealSelectionAfterEditingOperation();
    return newList;
}

bool Editor::canIncreaseSelectionListLevel()
{
    return canEditRichly() && IncreaseSelectionListLevelCommand::canIncreaseSelectionListLevel(m_frame->document());
}

bool Editor::canDecreaseSelectionListLevel()
{
    return canEditRichly() && DecreaseSelectionListLevelCommand::canDecreaseSelectionListLevel(m_frame->document());
}

PassRefPtr<Node> Editor::increaseSelectionListLevel()
{
    if (!canEditRichly() || m_frame->selection()->isNone())
        return 0;
    
    RefPtr<Node> newList = IncreaseSelectionListLevelCommand::increaseSelectionListLevel(m_frame->document());
    revealSelectionAfterEditingOperation();
    return newList;
}

PassRefPtr<Node> Editor::increaseSelectionListLevelOrdered()
{
    if (!canEditRichly() || m_frame->selection()->isNone())
        return 0;
    
    RefPtr<Node> newList = IncreaseSelectionListLevelCommand::increaseSelectionListLevelOrdered(m_frame->document());
    revealSelectionAfterEditingOperation();
    return newList.release();
}

PassRefPtr<Node> Editor::increaseSelectionListLevelUnordered()
{
    if (!canEditRichly() || m_frame->selection()->isNone())
        return 0;
    
    RefPtr<Node> newList = IncreaseSelectionListLevelCommand::increaseSelectionListLevelUnordered(m_frame->document());
    revealSelectionAfterEditingOperation();
    return newList.release();
}

void Editor::decreaseSelectionListLevel()
{
    if (!canEditRichly() || m_frame->selection()->isNone())
        return;
    
    DecreaseSelectionListLevelCommand::decreaseSelectionListLevel(m_frame->document());
    revealSelectionAfterEditingOperation();
}

void Editor::removeFormattingAndStyle()
{
    applyCommand(RemoveFormatCommand::create(m_frame->document()));
}

void Editor::clearLastEditCommand() 
{
    m_lastEditCommand.clear();
}

// Returns whether caller should continue with "the default processing", which is the same as 
// the event handler NOT setting the return value to false
bool Editor::dispatchCPPEvent(const AtomicString &eventType, ClipboardAccessPolicy policy)
{
    Node* target = findEventTargetFromSelection();
    if (!target)
        return true;
    
    RefPtr<Clipboard> clipboard = newGeneralClipboard(policy, m_frame);

    ExceptionCode ec = 0;
    RefPtr<Event> evt = ClipboardEvent::create(eventType, true, true, clipboard);
    target->dispatchEvent(evt, ec);
    bool noDefaultProcessing = evt->defaultPrevented();

    // invalidate clipboard here for security
    clipboard->setAccessPolicy(ClipboardNumb);
    
    return !noDefaultProcessing;
}

Node* Editor::findEventTargetFrom(const VisibleSelection& selection) const
{
    Node* target = selection.start().element();
    if (!target)
        target = m_frame->document()->body();
    if (!target)
        return 0;
    return target->shadowAncestorNode();

}

Node* Editor::findEventTargetFromSelection() const
{
    return findEventTargetFrom(m_frame->selection()->selection());
}

void Editor::applyStyle(CSSStyleDeclaration* style, EditAction editingAction)
{
    switch (m_frame->selection()->selectionType()) {
    case VisibleSelection::NoSelection:
        // do nothing
        break;
    case VisibleSelection::CaretSelection:
        computeAndSetTypingStyle(style, editingAction);
        break;
    case VisibleSelection::RangeSelection:
        if (style)
            applyCommand(ApplyStyleCommand::create(m_frame->document(), EditingStyle::create(style).get(), editingAction));
        break;
    }
}
    
bool Editor::shouldApplyStyle(CSSStyleDeclaration* style, Range* range)
{   
    return client()->shouldApplyStyle(style, range);
}
    
void Editor::applyParagraphStyle(CSSStyleDeclaration* style, EditAction editingAction)
{
    switch (m_frame->selection()->selectionType()) {
    case VisibleSelection::NoSelection:
        // do nothing
        break;
    case VisibleSelection::CaretSelection:
    case VisibleSelection::RangeSelection:
        if (style)
            applyCommand(ApplyStyleCommand::create(m_frame->document(), EditingStyle::create(style).get(), editingAction, ApplyStyleCommand::ForceBlockProperties));
        break;
    }
}

void Editor::applyStyleToSelection(CSSStyleDeclaration* style, EditAction editingAction)
{
    if (!style || !style->length() || !canEditRichly())
        return;

    if (client() && client()->shouldApplyStyle(style, m_frame->selection()->toNormalizedRange().get()))
        applyStyle(style, editingAction);
}

void Editor::applyParagraphStyleToSelection(CSSStyleDeclaration* style, EditAction editingAction)
{
    if (!style || !style->length() || !canEditRichly())
        return;
    
    if (client() && client()->shouldApplyStyle(style, m_frame->selection()->toNormalizedRange().get()))
        applyParagraphStyle(style, editingAction);
}

bool Editor::selectionStartHasStyle(int propertyID, const String& value) const
{
    RefPtr<EditingStyle> style = EditingStyle::create(propertyID, value);
    RefPtr<EditingStyle> selectionStyle = selectionStartStyle();
    if (!selectionStyle || !selectionStyle->style())
        return false;
    return style->triStateOfStyle(selectionStyle->style()) == TrueTriState;
}

TriState Editor::selectionHasStyle(int propertyID, const String& value) const
{
    RefPtr<EditingStyle> style = EditingStyle::create(propertyID, value);
    if (!m_frame->selection()->isCaretOrRange())
        return FalseTriState;

    if (m_frame->selection()->isCaret()) {
        RefPtr<EditingStyle> selectionStyle = selectionStartStyle();
        if (!selectionStyle || !selectionStyle->style())
            return FalseTriState;
        return style->triStateOfStyle(selectionStyle->style());
    }

    TriState state = FalseTriState;
    for (Node* node = m_frame->selection()->start().deprecatedNode(); node; node = node->traverseNextNode()) {
        RefPtr<CSSComputedStyleDeclaration> nodeStyle = computedStyle(node);
        if (nodeStyle) {
            TriState nodeState = style->triStateOfStyle(nodeStyle.get(), node->isTextNode() ? EditingStyle::DoNotIgnoreTextOnlyProperties : EditingStyle::IgnoreTextOnlyProperties);
            if (node == m_frame->selection()->start().deprecatedNode())
                state = nodeState;
            else if (state != nodeState && node->isTextNode()) {
                state = MixedTriState;
                break;
            }
        }
        if (node == m_frame->selection()->end().deprecatedNode())
            break;
    }

    return state;
}

String Editor::selectionStartCSSPropertyValue(int propertyID)
{
    RefPtr<EditingStyle> selectionStyle = selectionStartStyle();
    if (!selectionStyle || !selectionStyle->style())
        return String();

    String value = selectionStyle->style()->getPropertyValue(propertyID);

    // If background color is transparent, traverse parent nodes until we hit a different value or document root
    // Also, if the selection is a range, ignore the background color at the start of selection,
    // and find the background color of the common ancestor.
    if (propertyID == CSSPropertyBackgroundColor && (m_frame->selection()->isRange() || hasTransparentBackgroundColor(selectionStyle->style()))) {
        RefPtr<Range> range(m_frame->selection()->toNormalizedRange());
        ExceptionCode ec = 0;
        if (PassRefPtr<CSSValue> value = backgroundColorInEffect(range->commonAncestorContainer(ec)))
            return value->cssText();
    }

    if (propertyID == CSSPropertyFontSize) {
        RefPtr<CSSValue> cssValue = selectionStyle->style()->getPropertyCSSValue(CSSPropertyFontSize);
        if (cssValue->isPrimitiveValue()) {
            value = String::number(legacyFontSizeFromCSSValue(m_frame->document(), static_cast<CSSPrimitiveValue*>(cssValue.get()),
                selectionStyle->shouldUseFixedDefaultFontSize(), AlwaysUseLegacyFontSize));
        }
    }

    return value;
}

void Editor::indent()
{
    applyCommand(IndentOutdentCommand::create(m_frame->document(), IndentOutdentCommand::Indent));
}

void Editor::outdent()
{
    applyCommand(IndentOutdentCommand::create(m_frame->document(), IndentOutdentCommand::Outdent));
}

static void dispatchEditableContentChangedEvents(const EditCommand& command)
{
    Element* startRoot = command.startingRootEditableElement();
    Element* endRoot = command.endingRootEditableElement();
    ExceptionCode ec;
    if (startRoot)
        startRoot->dispatchEvent(Event::create(eventNames().webkitEditableContentChangedEvent, false, false), ec);
    if (endRoot && endRoot != startRoot)
        endRoot->dispatchEvent(Event::create(eventNames().webkitEditableContentChangedEvent, false, false), ec);
}

void Editor::appliedEditing(PassRefPtr<EditCommand> cmd)
{
    m_frame->document()->updateLayout();

    dispatchEditableContentChangedEvents(*cmd);
    VisibleSelection newSelection(cmd->endingSelection());

    m_spellingCorrector->respondToAppliedEditing(cmd.get());

    // Don't clear the typing style with this selection change.  We do those things elsewhere if necessary.
    changeSelectionAfterCommand(newSelection, false, false);

    if (!cmd->preservesTypingStyle())
        m_frame->selection()->clearTypingStyle();

    // Command will be equal to last edit command only in the case of typing
    if (m_lastEditCommand.get() == cmd)
        ASSERT(cmd->isTypingCommand());
    else {
        // Only register a new undo command if the command passed in is
        // different from the last command
        m_lastEditCommand = cmd;
        if (client())
            client()->registerCommandForUndo(m_lastEditCommand);
    }
    respondToChangedContents(newSelection);
}

void Editor::unappliedEditing(PassRefPtr<EditCommand> cmd)
{
    m_frame->document()->updateLayout();
    
    dispatchEditableContentChangedEvents(*cmd);
    
    VisibleSelection newSelection(cmd->startingSelection());
    changeSelectionAfterCommand(newSelection, true, true);
    m_spellingCorrector->respondToUnappliedEditing(cmd.get());
    
    m_lastEditCommand = 0;
    if (client())
        client()->registerCommandForRedo(cmd);
    respondToChangedContents(newSelection);
}

void Editor::reappliedEditing(PassRefPtr<EditCommand> cmd)
{
    m_frame->document()->updateLayout();
    
    dispatchEditableContentChangedEvents(*cmd);
    
    VisibleSelection newSelection(cmd->endingSelection());
    changeSelectionAfterCommand(newSelection, true, true);
    
    m_lastEditCommand = 0;
    if (client())
        client()->registerCommandForUndo(cmd);
    respondToChangedContents(newSelection);
}

Editor::Editor(Frame* frame)
    : m_frame(frame)
    , m_deleteButtonController(adoptPtr(new DeleteButtonController(frame)))
    , m_ignoreCompositionSelectionChange(false)
    , m_shouldStartNewKillRingSequence(false)
    // This is off by default, since most editors want this behavior (this matches IE but not FF).
    , m_shouldStyleWithCSS(false)
    , m_killRing(adoptPtr(new KillRing))
    , m_spellChecker(adoptPtr(new SpellChecker(frame)))
    , m_spellingCorrector(adoptPtr(new SpellingCorrectionController(frame)))
    , m_areMarkedTextMatchesHighlighted(false)
{
}

Editor::~Editor()
{
}

void Editor::clear()
{
    m_compositionNode = 0;
    m_customCompositionUnderlines.clear();
    m_shouldStyleWithCSS = false;
}

bool Editor::insertText(const String& text, Event* triggeringEvent)
{
    return m_frame->eventHandler()->handleTextInputEvent(text, triggeringEvent);
}

bool Editor::insertTextForConfirmedComposition(const String& text)
{
    return m_frame->eventHandler()->handleTextInputEvent(text, 0, TextEventInputComposition);
}

bool Editor::insertTextWithoutSendingTextEvent(const String& text, bool selectInsertedText, TextEvent* triggeringEvent)
{
    if (text.isEmpty())
        return false;

    VisibleSelection selection = selectionForCommand(triggeringEvent);
    if (!selection.isContentEditable())
        return false;
    RefPtr<Range> range = selection.toNormalizedRange();

    if (!shouldInsertText(text, range.get(), EditorInsertActionTyped))
        return true;

    if (!text.isEmpty())
        updateMarkersForWordsAffectedByEditing(isSpaceOrNewline(text[0]));

    bool shouldConsiderApplyingAutocorrection = false;
    if (text == " " || text == "\t")
        shouldConsiderApplyingAutocorrection = true;

    if (text.length() == 1 && isPunct(text[0]) && !isAmbiguousBoundaryCharacter(text[0]))
        shouldConsiderApplyingAutocorrection = true;

    bool autocorrectionWasApplied = shouldConsiderApplyingAutocorrection && m_spellingCorrector->applyAutocorrectionBeforeTypingIfAppropriate();

    // Get the selection to use for the event that triggered this insertText.
    // If the event handler changed the selection, we may want to use a different selection
    // that is contained in the event target.
    selection = selectionForCommand(triggeringEvent);
    if (selection.isContentEditable()) {
        if (Node* selectionStart = selection.start().deprecatedNode()) {
            RefPtr<Document> document = selectionStart->document();

            // Insert the text
            TypingCommand::Options options = 0;
            if (selectInsertedText)
                options |= TypingCommand::SelectInsertedText;
            if (autocorrectionWasApplied)
                options |= TypingCommand::RetainAutocorrectionIndicator;
            TypingCommand::insertText(document.get(), text, selection, options, triggeringEvent && triggeringEvent->isComposition() ? TypingCommand::TextCompositionConfirm : TypingCommand::TextCompositionNone);

            // Reveal the current selection
            if (Frame* editedFrame = document->frame())
                if (Page* page = editedFrame->page())
                    page->focusController()->focusedOrMainFrame()->selection()->revealSelection(ScrollAlignment::alignToEdgeIfNeeded);
        }
    }

    return true;
}

bool Editor::insertLineBreak()
{
    if (!canEdit())
        return false;

    if (!shouldInsertText("\n", m_frame->selection()->toNormalizedRange().get(), EditorInsertActionTyped))
        return true;

    bool autocorrectionIsApplied = m_spellingCorrector->applyAutocorrectionBeforeTypingIfAppropriate();
    TypingCommand::insertLineBreak(m_frame->document(), autocorrectionIsApplied ? TypingCommand::RetainAutocorrectionIndicator : 0);
    revealSelectionAfterEditingOperation();

    return true;
}

bool Editor::insertParagraphSeparator()
{
    if (!canEdit())
        return false;

    if (!canEditRichly())
        return insertLineBreak();

    if (!shouldInsertText("\n", m_frame->selection()->toNormalizedRange().get(), EditorInsertActionTyped))
        return true;

    bool autocorrectionIsApplied = m_spellingCorrector->applyAutocorrectionBeforeTypingIfAppropriate();
    TypingCommand::insertParagraphSeparator(m_frame->document(), autocorrectionIsApplied ? TypingCommand::RetainAutocorrectionIndicator : 0);
    revealSelectionAfterEditingOperation();

    return true;
}

void Editor::cut()
{
    if (tryDHTMLCut())
        return; // DHTML did the whole operation
    if (!canCut()) {
        systemBeep();
        return;
    }
    RefPtr<Range> selection = selectedRange();
    if (shouldDeleteRange(selection.get())) {
        updateMarkersForWordsAffectedByEditing(true);
        if (isNodeInTextFormControl(m_frame->selection()->start().deprecatedNode()))
            Pasteboard::generalPasteboard()->writePlainText(selectedText());
        else
            Pasteboard::generalPasteboard()->writeSelection(selection.get(), canSmartCopyOrDelete(), m_frame);
        didWriteSelectionToPasteboard();
        deleteSelectionWithSmartDelete(canSmartCopyOrDelete());
    }
}

void Editor::copy()
{
    if (tryDHTMLCopy())
        return; // DHTML did the whole operation
    if (!canCopy()) {
        systemBeep();
        return;
    }

    if (isNodeInTextFormControl(m_frame->selection()->start().deprecatedNode()))
        Pasteboard::generalPasteboard()->writePlainText(selectedText());
    else {
        Document* document = m_frame->document();
        if (HTMLImageElement* imageElement = imageElementFromImageDocument(document))
            Pasteboard::generalPasteboard()->writeImage(imageElement, document->url(), document->title());
        else
            Pasteboard::generalPasteboard()->writeSelection(selectedRange().get(), canSmartCopyOrDelete(), m_frame);
    }

    didWriteSelectionToPasteboard();
}

void Editor::paste()
{
    ASSERT(m_frame->document());
    if (tryDHTMLPaste())
        return; // DHTML did the whole operation
    if (!canPaste())
        return;
    updateMarkersForWordsAffectedByEditing(false);
    CachedResourceLoader* loader = m_frame->document()->cachedResourceLoader();
    loader->setAllowStaleResources(true);
    if (m_frame->selection()->isContentRichlyEditable())
        pasteWithPasteboard(Pasteboard::generalPasteboard(), true);
    else
        pasteAsPlainTextWithPasteboard(Pasteboard::generalPasteboard());
    loader->setAllowStaleResources(false);
}

void Editor::pasteAsPlainText()
{
    if (tryDHTMLPaste())
        return;
    if (!canPaste())
        return;
    updateMarkersForWordsAffectedByEditing(false);
    pasteAsPlainTextWithPasteboard(Pasteboard::generalPasteboard());
}

void Editor::performDelete()
{
    if (!canDelete()) {
        systemBeep();
        return;
    }

    addToKillRing(selectedRange().get(), false);
    deleteSelectionWithSmartDelete(canSmartCopyOrDelete());

    // clear the "start new kill ring sequence" setting, because it was set to true
    // when the selection was updated by deleting the range
    setStartNewKillRingSequence(false);
}

void Editor::copyURL(const KURL& url, const String& title)
{
    Pasteboard::generalPasteboard()->writeURL(url, title, m_frame);
}

void Editor::copyImage(const HitTestResult& result)
{
    KURL url = result.absoluteLinkURL();
    if (url.isEmpty())
        url = result.absoluteImageURL();

    Pasteboard::generalPasteboard()->writeImage(result.innerNonSharedNode(), url, result.altDisplayString());
}

bool Editor::isContinuousSpellCheckingEnabled()
{
    return client() && client()->isContinuousSpellCheckingEnabled();
}

void Editor::toggleContinuousSpellChecking()
{
    if (client())
        client()->toggleContinuousSpellChecking();
}

bool Editor::isGrammarCheckingEnabled()
{
    return client() && client()->isGrammarCheckingEnabled();
}

void Editor::toggleGrammarChecking()
{
    if (client())
        client()->toggleGrammarChecking();
}

int Editor::spellCheckerDocumentTag()
{
    return client() ? client()->spellCheckerDocumentTag() : 0;
}

#if USE(AUTOMATIC_TEXT_REPLACEMENT)

void Editor::uppercaseWord()
{
    if (client())
        client()->uppercaseWord();
}

void Editor::lowercaseWord()
{
    if (client())
        client()->lowercaseWord();
}

void Editor::capitalizeWord()
{
    if (client())
        client()->capitalizeWord();
}

void Editor::showSubstitutionsPanel()
{
    if (!client()) {
        LOG_ERROR("No NSSpellChecker");
        return;
    }

    if (client()->substitutionsPanelIsShowing()) {
        client()->showSubstitutionsPanel(false);
        return;
    }
    client()->showSubstitutionsPanel(true);
}

bool Editor::substitutionsPanelIsShowing()
{
    if (!client())
        return false;
    return client()->substitutionsPanelIsShowing();
}

void Editor::toggleSmartInsertDelete()
{
    if (client())
        client()->toggleSmartInsertDelete();
}

bool Editor::isAutomaticQuoteSubstitutionEnabled()
{
    return client() && client()->isAutomaticQuoteSubstitutionEnabled();
}

void Editor::toggleAutomaticQuoteSubstitution()
{
    if (client())
        client()->toggleAutomaticQuoteSubstitution();
}

bool Editor::isAutomaticLinkDetectionEnabled()
{
    return client() && client()->isAutomaticLinkDetectionEnabled();
}

void Editor::toggleAutomaticLinkDetection()
{
    if (client())
        client()->toggleAutomaticLinkDetection();
}

bool Editor::isAutomaticDashSubstitutionEnabled()
{
    return client() && client()->isAutomaticDashSubstitutionEnabled();
}

void Editor::toggleAutomaticDashSubstitution()
{
    if (client())
        client()->toggleAutomaticDashSubstitution();
}

bool Editor::isAutomaticTextReplacementEnabled()
{
    return client() && client()->isAutomaticTextReplacementEnabled();
}

void Editor::toggleAutomaticTextReplacement()
{
    if (client())
        client()->toggleAutomaticTextReplacement();
}

bool Editor::isAutomaticSpellingCorrectionEnabled()
{
    return m_spellingCorrector->isAutomaticSpellingCorrectionEnabled();
}

void Editor::toggleAutomaticSpellingCorrection()
{
    if (client())
        client()->toggleAutomaticSpellingCorrection();
}

#endif

bool Editor::shouldEndEditing(Range* range)
{
    return client() && client()->shouldEndEditing(range);
}

bool Editor::shouldBeginEditing(Range* range)
{
    return client() && client()->shouldBeginEditing(range);
}

void Editor::clearUndoRedoOperations()
{
    if (client())
        client()->clearUndoRedoOperations();
}

bool Editor::canUndo()
{
    return client() && client()->canUndo();
}

void Editor::undo()
{
    if (client())
        client()->undo();
}

bool Editor::canRedo()
{
    return client() && client()->canRedo();
}

void Editor::redo()
{
    if (client())
        client()->redo();
}

void Editor::didBeginEditing()
{
    if (client())
        client()->didBeginEditing();
}

void Editor::didEndEditing()
{
    if (client())
        client()->didEndEditing();
}

void Editor::didWriteSelectionToPasteboard()
{
    if (client())
        client()->didWriteSelectionToPasteboard();
}

void Editor::toggleBold()
{
    command("ToggleBold").execute();
}

void Editor::toggleUnderline()
{
    command("ToggleUnderline").execute();
}

void Editor::setBaseWritingDirection(WritingDirection direction)
{
    Node* focusedNode = frame()->document()->focusedNode();
    if (focusedNode && (focusedNode->hasTagName(textareaTag) || (focusedNode->hasTagName(inputTag) && static_cast<HTMLInputElement*>(focusedNode)->isTextField()))) {
        if (direction == NaturalWritingDirection)
            return;
        toHTMLElement(focusedNode)->setAttribute(dirAttr, direction == LeftToRightWritingDirection ? "ltr" : "rtl");
        frame()->document()->updateStyleIfNeeded();
        return;
    }

    RefPtr<CSSMutableStyleDeclaration> style = CSSMutableStyleDeclaration::create();
    style->setProperty(CSSPropertyDirection, direction == LeftToRightWritingDirection ? "ltr" : direction == RightToLeftWritingDirection ? "rtl" : "inherit", false);
    applyParagraphStyleToSelection(style.get(), EditActionSetWritingDirection);
}

void Editor::selectComposition()
{
    RefPtr<Range> range = compositionRange();
    if (!range)
        return;
    
    // The composition can start inside a composed character sequence, so we have to override checks.
    // See <http://bugs.webkit.org/show_bug.cgi?id=15781>
    VisibleSelection selection;
    selection.setWithoutValidation(range->startPosition(), range->endPosition());
    m_frame->selection()->setSelection(selection, 0);
}

void Editor::confirmComposition()
{
    if (!m_compositionNode)
        return;
    confirmComposition(m_compositionNode->data().substring(m_compositionStart, m_compositionEnd - m_compositionStart), false);
}

void Editor::confirmCompositionWithoutDisturbingSelection()
{
    if (!m_compositionNode)
        return;
    confirmComposition(m_compositionNode->data().substring(m_compositionStart, m_compositionEnd - m_compositionStart), true);
}

void Editor::confirmComposition(const String& text)
{
    confirmComposition(text, false);
}

void Editor::confirmComposition(const String& text, bool preserveSelection)
{
    UserTypingGestureIndicator typingGestureIndicator(m_frame);

    setIgnoreCompositionSelectionChange(true);

    VisibleSelection oldSelection = m_frame->selection()->selection();

    selectComposition();

    if (m_frame->selection()->isNone()) {
        setIgnoreCompositionSelectionChange(false);
        return;
    }
    
    // Dispatch a compositionend event to the focused node.
    // We should send this event before sending a TextEvent as written in Section 6.2.2 and 6.2.3 of
    // the DOM Event specification.
    Node* target = m_frame->document()->focusedNode();
    if (target) {
        RefPtr<CompositionEvent> event = CompositionEvent::create(eventNames().compositionendEvent, m_frame->domWindow(), text);
        ExceptionCode ec = 0;
        target->dispatchEvent(event, ec);
    }

    // If text is empty, then delete the old composition here.  If text is non-empty, InsertTextCommand::input
    // will delete the old composition with an optimized replace operation.
    if (text.isEmpty())
        TypingCommand::deleteSelection(m_frame->document(), 0);

    m_compositionNode = 0;
    m_customCompositionUnderlines.clear();

    insertTextForConfirmedComposition(text);

    if (preserveSelection) {
        m_frame->selection()->setSelection(oldSelection, 0);
        // An open typing command that disagrees about current selection would cause issues with typing later on.
        TypingCommand::closeTyping(m_lastEditCommand.get());
    }

    setIgnoreCompositionSelectionChange(false);
}

void Editor::setComposition(const String& text, const Vector<CompositionUnderline>& underlines, unsigned selectionStart, unsigned selectionEnd)
{
    UserTypingGestureIndicator typingGestureIndicator(m_frame);

    setIgnoreCompositionSelectionChange(true);

    // Updates styles before setting selection for composition to prevent
    // inserting the previous composition text into text nodes oddly.
    // See https://bugs.webkit.org/show_bug.cgi?id=46868
    m_frame->document()->updateStyleIfNeeded();

    selectComposition();

    if (m_frame->selection()->isNone()) {
        setIgnoreCompositionSelectionChange(false);
        return;
    }

    Node* target = m_frame->document()->focusedNode();
    if (target) {
        // Dispatch an appropriate composition event to the focused node.
        // We check the composition status and choose an appropriate composition event since this
        // function is used for three purposes:
        // 1. Starting a new composition.
        //    Send a compositionstart event when this function creates a new composition node, i.e.
        //    m_compositionNode == 0 && !text.isEmpty().
        // 2. Updating the existing composition node.
        //    Send a compositionupdate event when this function updates the existing composition
        //    node, i.e. m_compositionNode != 0 && !text.isEmpty().
        // 3. Canceling the ongoing composition.
        //    Send a compositionend event when function deletes the existing composition node, i.e.
        //    m_compositionNode != 0 && test.isEmpty().
        RefPtr<CompositionEvent> event;
        if (!m_compositionNode) {
            // We should send a compositionstart event only when the given text is not empty because this
            // function doesn't create a composition node when the text is empty.
            if (!text.isEmpty())
                event = CompositionEvent::create(eventNames().compositionstartEvent, m_frame->domWindow(), text);
        } else {
            if (!text.isEmpty())
                event = CompositionEvent::create(eventNames().compositionupdateEvent, m_frame->domWindow(), text);
            else
              event = CompositionEvent::create(eventNames().compositionendEvent, m_frame->domWindow(), text);
        }
        ExceptionCode ec = 0;
        if (event.get())
            target->dispatchEvent(event, ec);
    }

    // If text is empty, then delete the old composition here.  If text is non-empty, InsertTextCommand::input
    // will delete the old composition with an optimized replace operation.
    if (text.isEmpty())
        TypingCommand::deleteSelection(m_frame->document(), TypingCommand::PreventSpellChecking);

    m_compositionNode = 0;
    m_customCompositionUnderlines.clear();

    if (!text.isEmpty()) {
        TypingCommand::insertText(m_frame->document(), text, TypingCommand::SelectInsertedText | TypingCommand::PreventSpellChecking, TypingCommand::TextCompositionUpdate);

        // Find out what node has the composition now.
        Position base = m_frame->selection()->base().downstream();
        Position extent = m_frame->selection()->extent();
        Node* baseNode = base.deprecatedNode();
        unsigned baseOffset = base.deprecatedEditingOffset();
        Node* extentNode = extent.deprecatedNode();
        unsigned extentOffset = extent.deprecatedEditingOffset();

        if (baseNode && baseNode == extentNode && baseNode->isTextNode() && baseOffset + text.length() == extentOffset) {
            m_compositionNode = static_cast<Text*>(baseNode);
            m_compositionStart = baseOffset;
            m_compositionEnd = extentOffset;
            m_customCompositionUnderlines = underlines;
            size_t numUnderlines = m_customCompositionUnderlines.size();
            for (size_t i = 0; i < numUnderlines; ++i) {
                m_customCompositionUnderlines[i].startOffset += baseOffset;
                m_customCompositionUnderlines[i].endOffset += baseOffset;
            }
            if (baseNode->renderer())
                baseNode->renderer()->repaint();

            unsigned start = min(baseOffset + selectionStart, extentOffset);
            unsigned end = min(max(start, baseOffset + selectionEnd), extentOffset);
            RefPtr<Range> selectedRange = Range::create(baseNode->document(), baseNode, start, baseNode, end);                
            m_frame->selection()->setSelectedRange(selectedRange.get(), DOWNSTREAM, false);
        }
    }

    setIgnoreCompositionSelectionChange(false);
}

void Editor::ignoreSpelling()
{
    if (!client())
        return;
        
    RefPtr<Range> selectedRange = frame()->selection()->toNormalizedRange();
    if (selectedRange)
        frame()->document()->markers()->removeMarkers(selectedRange.get(), DocumentMarker::Spelling);

    String text = selectedText();
    ASSERT(text.length());
    textChecker()->ignoreWordInSpellDocument(text);
}

void Editor::learnSpelling()
{
    if (!client())
        return;
        
    // FIXME: On Mac OS X, when use "learn" button on "Spelling and Grammar" panel, we don't call this function. It should remove misspelling markers around the learned word, see <rdar://problem/5396072>.

    RefPtr<Range> selectedRange = frame()->selection()->toNormalizedRange();
    if (selectedRange)
        frame()->document()->markers()->removeMarkers(selectedRange.get(), DocumentMarker::Spelling);

    String text = selectedText();
    ASSERT(text.length());
    textChecker()->learnWord(text);
}

void Editor::advanceToNextMisspelling(bool startBeforeSelection)
{
    ExceptionCode ec = 0;

    // The basic approach is to search in two phases - from the selection end to the end of the doc, and
    // then we wrap and search from the doc start to (approximately) where we started.
    
    // Start at the end of the selection, search to edge of document.  Starting at the selection end makes
    // repeated "check spelling" commands work.
    VisibleSelection selection(frame()->selection()->selection());
    RefPtr<Range> spellingSearchRange(rangeOfContents(frame()->document()));

    bool startedWithSelection = false;
    if (selection.start().deprecatedNode()) {
        startedWithSelection = true;
        if (startBeforeSelection) {
            VisiblePosition start(selection.visibleStart());
            // We match AppKit's rule: Start 1 character before the selection.
            VisiblePosition oneBeforeStart = start.previous();
            setStart(spellingSearchRange.get(), oneBeforeStart.isNotNull() ? oneBeforeStart : start);
        } else
            setStart(spellingSearchRange.get(), selection.visibleEnd());
    }

    Position position = spellingSearchRange->startPosition();
    if (!isEditablePosition(position)) {
        // This shouldn't happen in very often because the Spelling menu items aren't enabled unless the
        // selection is editable.
        // This can happen in Mail for a mix of non-editable and editable content (like Stationary), 
        // when spell checking the whole document before sending the message.
        // In that case the document might not be editable, but there are editable pockets that need to be spell checked.

        position = firstEditablePositionAfterPositionInRoot(position, frame()->document()->documentElement()).deepEquivalent();
        if (position.isNull())
            return;
        
        Position rangeCompliantPosition = position.parentAnchoredEquivalent();
        spellingSearchRange->setStart(rangeCompliantPosition.deprecatedNode(), rangeCompliantPosition.deprecatedEditingOffset(), ec);
        startedWithSelection = false; // won't need to wrap
    }
    
    // topNode defines the whole range we want to operate on 
    Node* topNode = highestEditableRoot(position);
    // FIXME: lastOffsetForEditing() is wrong here if editingIgnoresContent(highestEditableRoot()) returns true (e.g. a <table>)
    spellingSearchRange->setEnd(topNode, lastOffsetForEditing(topNode), ec);

    // If spellingSearchRange starts in the middle of a word, advance to the next word so we start checking
    // at a word boundary. Going back by one char and then forward by a word does the trick.
    if (startedWithSelection) {
        VisiblePosition oneBeforeStart = startVisiblePosition(spellingSearchRange.get(), DOWNSTREAM).previous();
        if (oneBeforeStart.isNotNull())
            setStart(spellingSearchRange.get(), endOfWord(oneBeforeStart));
        // else we were already at the start of the editable node
    }
    
    if (spellingSearchRange->collapsed(ec))
        return; // nothing to search in
    
    // Get the spell checker if it is available
    if (!client())
        return;
        
    // We go to the end of our first range instead of the start of it, just to be sure
    // we don't get foiled by any word boundary problems at the start.  It means we might
    // do a tiny bit more searching.
    Node* searchEndNodeAfterWrap = spellingSearchRange->endContainer(ec);
    int searchEndOffsetAfterWrap = spellingSearchRange->endOffset(ec);
    
    int misspellingOffset = 0;
    GrammarDetail grammarDetail;
    int grammarPhraseOffset = 0;
    RefPtr<Range> grammarSearchRange;
    String badGrammarPhrase;
    String misspelledWord;

#if USE(UNIFIED_TEXT_CHECKING)
    grammarSearchRange = spellingSearchRange->cloneRange(ec);
    bool isSpelling = true;
    int foundOffset = 0;
    String foundItem = TextCheckingHelper(client(), spellingSearchRange).findFirstMisspellingOrBadGrammar(isGrammarCheckingEnabled(), isSpelling, foundOffset, grammarDetail);
    if (isSpelling) {
        misspelledWord = foundItem;
        misspellingOffset = foundOffset;
    } else {
        badGrammarPhrase = foundItem;
        grammarPhraseOffset = foundOffset;
    }
#else
    RefPtr<Range> firstMisspellingRange;
    misspelledWord = TextCheckingHelper(client(), spellingSearchRange).findFirstMisspelling(misspellingOffset, false, firstMisspellingRange);

#if USE(GRAMMAR_CHECKING)
    // Search for bad grammar that occurs prior to the next misspelled word (if any)
    grammarSearchRange = spellingSearchRange->cloneRange(ec);
    if (!misspelledWord.isEmpty()) {
        // Stop looking at start of next misspelled word
        CharacterIterator chars(grammarSearchRange.get());
        chars.advance(misspellingOffset);
        grammarSearchRange->setEnd(chars.range()->startContainer(ec), chars.range()->startOffset(ec), ec);
    }
    
    if (isGrammarCheckingEnabled())
        badGrammarPhrase = TextCheckingHelper(client(), grammarSearchRange).findFirstBadGrammar(grammarDetail, grammarPhraseOffset, false);
#endif
#endif
    
    // If we found neither bad grammar nor a misspelled word, wrap and try again (but don't bother if we started at the beginning of the
    // block rather than at a selection).
    if (startedWithSelection && !misspelledWord && !badGrammarPhrase) {
        spellingSearchRange->setStart(topNode, 0, ec);
        // going until the end of the very first chunk we tested is far enough
        spellingSearchRange->setEnd(searchEndNodeAfterWrap, searchEndOffsetAfterWrap, ec);
        
#if USE(UNIFIED_TEXT_CHECKING)
        grammarSearchRange = spellingSearchRange->cloneRange(ec);
        foundItem = TextCheckingHelper(client(), spellingSearchRange).findFirstMisspellingOrBadGrammar(isGrammarCheckingEnabled(), isSpelling, foundOffset, grammarDetail);
        if (isSpelling) {
            misspelledWord = foundItem;
            misspellingOffset = foundOffset;
        } else {
            badGrammarPhrase = foundItem;
            grammarPhraseOffset = foundOffset;
        }
#else
        misspelledWord = TextCheckingHelper(client(), spellingSearchRange).findFirstMisspelling(misspellingOffset, false, firstMisspellingRange);

#if USE(GRAMMAR_CHECKING)
        grammarSearchRange = spellingSearchRange->cloneRange(ec);
        if (!misspelledWord.isEmpty()) {
            // Stop looking at start of next misspelled word
            CharacterIterator chars(grammarSearchRange.get());
            chars.advance(misspellingOffset);
            grammarSearchRange->setEnd(chars.range()->startContainer(ec), chars.range()->startOffset(ec), ec);
        }

        if (isGrammarCheckingEnabled())
            badGrammarPhrase = TextCheckingHelper(client(), grammarSearchRange).findFirstBadGrammar(grammarDetail, grammarPhraseOffset, false);
#endif
#endif
    }
    
    if (!badGrammarPhrase.isEmpty()) {
        ASSERT(WTF_USE_GRAMMAR_CHECKING);
        // We found bad grammar. Since we only searched for bad grammar up to the first misspelled word, the bad grammar
        // takes precedence and we ignore any potential misspelled word. Select the grammar detail, update the spelling
        // panel, and store a marker so we draw the green squiggle later.
        
        ASSERT(badGrammarPhrase.length() > 0);
        ASSERT(grammarDetail.location != -1 && grammarDetail.length > 0);
        
        // FIXME 4859190: This gets confused with doubled punctuation at the end of a paragraph
        RefPtr<Range> badGrammarRange = TextIterator::subrange(grammarSearchRange.get(), grammarPhraseOffset + grammarDetail.location, grammarDetail.length);
        frame()->selection()->setSelection(VisibleSelection(badGrammarRange.get(), SEL_DEFAULT_AFFINITY));
        frame()->selection()->revealSelection();
        
        client()->updateSpellingUIWithGrammarString(badGrammarPhrase, grammarDetail);
        frame()->document()->markers()->addMarker(badGrammarRange.get(), DocumentMarker::Grammar, grammarDetail.userDescription);
    } else if (!misspelledWord.isEmpty()) {
        // We found a misspelling, but not any earlier bad grammar. Select the misspelling, update the spelling panel, and store
        // a marker so we draw the red squiggle later.
        
        RefPtr<Range> misspellingRange = TextIterator::subrange(spellingSearchRange.get(), misspellingOffset, misspelledWord.length());
        frame()->selection()->setSelection(VisibleSelection(misspellingRange.get(), DOWNSTREAM));
        frame()->selection()->revealSelection();
        
        client()->updateSpellingUIWithMisspelledWord(misspelledWord);
        frame()->document()->markers()->addMarker(misspellingRange.get(), DocumentMarker::Spelling);
    }
}

bool Editor::isSelectionMisspelled()
{
    String selectedString = selectedText();
    int length = selectedString.length();
    if (!length)
        return false;

    if (!client())
        return false;
    
    int misspellingLocation = -1;
    int misspellingLength = 0;
    textChecker()->checkSpellingOfString(selectedString.characters(), length, &misspellingLocation, &misspellingLength);
    
    // The selection only counts as misspelled if the selected text is exactly one misspelled word
    if (misspellingLength != length)
        return false;
    
    // Update the spelling panel to be displaying this error (whether or not the spelling panel is on screen).
    // This is necessary to make a subsequent call to [NSSpellChecker ignoreWord:inSpellDocumentWithTag:] work
    // correctly; that call behaves differently based on whether the spelling panel is displaying a misspelling
    // or a grammar error.
    client()->updateSpellingUIWithMisspelledWord(selectedString);
    
    return true;
}

bool Editor::isSelectionUngrammatical()
{
#if USE(GRAMMAR_CHECKING)
    Vector<String> ignoredGuesses;
    return TextCheckingHelper(client(), frame()->selection()->toNormalizedRange()).isUngrammatical(ignoredGuesses);
#else
    return false;
#endif
}

Vector<String> Editor::guessesForUngrammaticalSelection()
{
#if USE(GRAMMAR_CHECKING)
    Vector<String> guesses;
    // Ignore the result of isUngrammatical; we just want the guesses, whether or not there are any
    TextCheckingHelper(client(), frame()->selection()->toNormalizedRange()).isUngrammatical(guesses);
    return guesses;
#else
    return Vector<String>();
#endif
}

Vector<String> Editor::guessesForMisspelledSelection()
{
    String selectedString = selectedText();
    ASSERT(selectedString.length());

    Vector<String> guesses;
    if (client())
        textChecker()->getGuessesForWord(selectedString, String(), guesses);
    return guesses;
}

Vector<String> Editor::guessesForMisspelledOrUngrammaticalSelection(bool& misspelled, bool& ungrammatical)
{
#if USE(UNIFIED_TEXT_CHECKING)
    return TextCheckingHelper(client(), frame()->selection()->toNormalizedRange()).guessesForMisspelledOrUngrammaticalRange(isGrammarCheckingEnabled(), misspelled, ungrammatical);
#else
    misspelled = isSelectionMisspelled();
    if (misspelled) {
        ungrammatical = false;
        return guessesForMisspelledSelection();
    }
    if (isGrammarCheckingEnabled() && isSelectionUngrammatical()) {
        ungrammatical = true;
        return guessesForUngrammaticalSelection();
    }
    ungrammatical = false;
    return Vector<String>();
#endif
}

void Editor::showSpellingGuessPanel()
{
    if (!client()) {
        LOG_ERROR("No NSSpellChecker");
        return;
    }

    if (client()->spellingUIIsShowing()) {
        client()->showSpellingUI(false);
        return;
    }
    
    advanceToNextMisspelling(true);
    client()->showSpellingUI(true);
}

bool Editor::spellingPanelIsShowing()
{
    if (!client())
        return false;
    return client()->spellingUIIsShowing();
}

void Editor::clearMisspellingsAndBadGrammar(const VisibleSelection &movingSelection)
{
    RefPtr<Range> selectedRange = movingSelection.toNormalizedRange();
    if (selectedRange) {
        frame()->document()->markers()->removeMarkers(selectedRange.get(), DocumentMarker::Spelling);
        frame()->document()->markers()->removeMarkers(selectedRange.get(), DocumentMarker::Grammar);
    }
}

void Editor::markMisspellingsAndBadGrammar(const VisibleSelection &movingSelection)
{
    bool markSpelling = isContinuousSpellCheckingEnabled();
    bool markGrammar = markSpelling && isGrammarCheckingEnabled();

    if (markSpelling) {
        RefPtr<Range> unusedFirstMisspellingRange;
        markMisspellings(movingSelection, unusedFirstMisspellingRange);
    }

    if (markGrammar)
        markBadGrammar(movingSelection);
}

void Editor::markMisspellingsAfterTypingToWord(const VisiblePosition &wordStart, const VisibleSelection& selectionAfterTyping, bool doReplacement)
{
#if USE(UNIFIED_TEXT_CHECKING)
    m_spellingCorrector->applyPendingCorrection(selectionAfterTyping);

    TextCheckingOptions textCheckingOptions = 0;
    if (isContinuousSpellCheckingEnabled())
        textCheckingOptions |= MarkSpelling;

#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    if (doReplacement 
        && (isAutomaticQuoteSubstitutionEnabled()
            || isAutomaticLinkDetectionEnabled()
            || isAutomaticDashSubstitutionEnabled()
            || isAutomaticTextReplacementEnabled()
            || ((textCheckingOptions & MarkSpelling) && isAutomaticSpellingCorrectionEnabled())))
        textCheckingOptions |= PerformReplacement;
#endif
    if (!textCheckingOptions & (MarkSpelling | PerformReplacement))
        return;

    if (isGrammarCheckingEnabled())
        textCheckingOptions |= MarkGrammar;

    VisibleSelection adjacentWords = VisibleSelection(startOfWord(wordStart, LeftWordIfOnBoundary), endOfWord(wordStart, RightWordIfOnBoundary));
    if (textCheckingOptions & MarkGrammar) {
        VisibleSelection selectedSentence = VisibleSelection(startOfSentence(wordStart), endOfSentence(wordStart));
        markAllMisspellingsAndBadGrammarInRanges(textCheckingOptions, adjacentWords.toNormalizedRange().get(), selectedSentence.toNormalizedRange().get());
    } else {
        markAllMisspellingsAndBadGrammarInRanges(textCheckingOptions, adjacentWords.toNormalizedRange().get(), adjacentWords.toNormalizedRange().get());
    }

#else
    UNUSED_PARAM(selectionAfterTyping);
    UNUSED_PARAM(doReplacement);

    if (!isContinuousSpellCheckingEnabled())
        return;

    // Check spelling of one word
    RefPtr<Range> misspellingRange;
    markMisspellings(VisibleSelection(startOfWord(wordStart, LeftWordIfOnBoundary), endOfWord(wordStart, RightWordIfOnBoundary)), misspellingRange);

    // Autocorrect the misspelled word.
    if (!misspellingRange)
        return;
    
    // Get the misspelled word.
    const String misspelledWord = plainText(misspellingRange.get());
    String autocorrectedString = textChecker()->getAutoCorrectSuggestionForMisspelledWord(misspelledWord);

    // If autocorrected word is non empty, replace the misspelled word by this word.
    if (!autocorrectedString.isEmpty()) {
        VisibleSelection newSelection(misspellingRange.get(), DOWNSTREAM);
        if (newSelection != frame()->selection()->selection()) {
            if (!frame()->selection()->shouldChangeSelection(newSelection))
                return;
            frame()->selection()->setSelection(newSelection);
        }

        if (!frame()->editor()->shouldInsertText(autocorrectedString, misspellingRange.get(), EditorInsertActionTyped))
            return;
        frame()->editor()->replaceSelectionWithText(autocorrectedString, false, false);

        // Reset the charet one character further.
        frame()->selection()->moveTo(frame()->selection()->end());
        frame()->selection()->modify(SelectionController::AlterationMove, DirectionForward, CharacterGranularity);
    }

    if (!isGrammarCheckingEnabled())
        return;
    
    // Check grammar of entire sentence
    markBadGrammar(VisibleSelection(startOfSentence(wordStart), endOfSentence(wordStart)));
#endif
}
    
void Editor::markMisspellingsOrBadGrammar(const VisibleSelection& selection, bool checkSpelling, RefPtr<Range>& firstMisspellingRange)
{
    // This function is called with a selection already expanded to word boundaries.
    // Might be nice to assert that here.
    
    // This function is used only for as-you-type checking, so if that's off we do nothing. Note that
    // grammar checking can only be on if spell checking is also on.
    if (!isContinuousSpellCheckingEnabled())
        return;
    
    RefPtr<Range> searchRange(selection.toNormalizedRange());
    if (!searchRange)
        return;
    
    // If we're not in an editable node, bail.
    Node* editableNode = searchRange->startContainer();
    if (!editableNode || !editableNode->rendererIsEditable())
        return;

    if (!isSpellCheckingEnabledFor(editableNode))
        return;

    // Get the spell checker if it is available
    if (!client())
        return;
    
    TextCheckingHelper checker(client(), searchRange);
    if (checkSpelling)
        checker.markAllMisspellings(firstMisspellingRange);
    else {
        ASSERT(WTF_USE_GRAMMAR_CHECKING);
        if (isGrammarCheckingEnabled())
            checker.markAllBadGrammar();
    }    
}

bool Editor::isSpellCheckingEnabledFor(Node* node) const
{
    if (!node)
        return false;
    const Element* focusedElement = node->isElementNode() ? toElement(node) : node->parentElement();
    if (!focusedElement)
        return false;
    return focusedElement->isSpellCheckingEnabled();
}

bool Editor::isSpellCheckingEnabledInFocusedNode() const
{
    return isSpellCheckingEnabledFor(m_frame->selection()->start().deprecatedNode());
}

void Editor::markMisspellings(const VisibleSelection& selection, RefPtr<Range>& firstMisspellingRange)
{
    markMisspellingsOrBadGrammar(selection, true, firstMisspellingRange);
}
    
void Editor::markBadGrammar(const VisibleSelection& selection)
{
    ASSERT(WTF_USE_GRAMMAR_CHECKING);
    RefPtr<Range> firstMisspellingRange;
    markMisspellingsOrBadGrammar(selection, false, firstMisspellingRange);
}

void Editor::markAllMisspellingsAndBadGrammarInRanges(TextCheckingOptions textCheckingOptions, Range* spellingRange, Range* grammarRange)
{
#if USE(UNIFIED_TEXT_CHECKING)
    // There shouldn't be pending autocorrection at this moment.
    ASSERT(!m_spellingCorrector->hasPendingCorrection());

    bool shouldMarkSpelling = textCheckingOptions & MarkSpelling;
    bool shouldMarkGrammar = textCheckingOptions & MarkGrammar;
    bool shouldPerformReplacement = textCheckingOptions & PerformReplacement;
    bool shouldShowCorrectionPanel = textCheckingOptions & ShowCorrectionPanel;
    bool shouldCheckForCorrection = shouldShowCorrectionPanel || (textCheckingOptions & CheckForCorrection);

    // This function is called with selections already expanded to word boundaries.
    ExceptionCode ec = 0;
    if (!client() || !spellingRange || (shouldMarkGrammar && !grammarRange))
        return;

    // If we're not in an editable node, bail.
    Node* editableNode = spellingRange->startContainer();
    if (!editableNode || !editableNode->rendererIsEditable())
        return;

    if (!isSpellCheckingEnabledFor(editableNode))
        return;

    // Expand the range to encompass entire paragraphs, since text checking needs that much context.
    int selectionOffset = 0;
    int ambiguousBoundaryOffset = -1;
    bool selectionChanged = false;
    bool restoreSelectionAfterChange = false;
    bool adjustSelectionForParagraphBoundaries = false;

    TextCheckingParagraph spellingParagraph(spellingRange);
    TextCheckingParagraph grammarParagraph(shouldMarkGrammar ? grammarRange : 0);

    if (shouldMarkGrammar ? (spellingParagraph.isRangeEmpty() && grammarParagraph.isEmpty()) : spellingParagraph.isEmpty())
        return;

    if (shouldPerformReplacement || shouldMarkSpelling || shouldCheckForCorrection) {
        if (m_frame->selection()->selectionType() == VisibleSelection::CaretSelection) {
            // Attempt to save the caret position so we can restore it later if needed
            Position caretPosition = m_frame->selection()->end();
            int offset = spellingParagraph.offsetTo(caretPosition, ec);
            if (!ec) {
                selectionOffset = offset;
                restoreSelectionAfterChange = true;
                if (selectionOffset > 0 && (selectionOffset > spellingParagraph.textLength() || spellingParagraph.textCharAt(selectionOffset - 1) == newlineCharacter))
                    adjustSelectionForParagraphBoundaries = true;
                if (selectionOffset > 0 && selectionOffset <= spellingParagraph.textLength() && isAmbiguousBoundaryCharacter(spellingParagraph.textCharAt(selectionOffset - 1)))
                    ambiguousBoundaryOffset = selectionOffset - 1;
            }
        }
    }

    Vector<TextCheckingResult> results;
    if (shouldMarkGrammar)
        textChecker()->checkTextOfParagraph(grammarParagraph.textCharacters(), grammarParagraph.textLength(), 
                                            textCheckingTypeMaskFor(textCheckingOptions), results);
    else
        textChecker()->checkTextOfParagraph(spellingParagraph.textCharacters(), spellingParagraph.textLength(), 
                                            textCheckingTypeMaskFor(textCheckingOptions), results);
        

    // If this checking is only for showing correction panel, we shouldn't bother to mark misspellings.
    if (shouldShowCorrectionPanel)
        shouldMarkSpelling = false;

    int offsetDueToReplacement = 0;

    for (unsigned i = 0; i < results.size(); i++) {
        int spellingRangeEndOffset = spellingParagraph.checkingEnd() + offsetDueToReplacement;
        const TextCheckingResult* result = &results[i];
        int resultLocation = result->location + offsetDueToReplacement;
        int resultLength = result->length;
        bool resultEndsAtAmbiguousBoundary = ambiguousBoundaryOffset >= 0 && resultLocation + resultLength == ambiguousBoundaryOffset;

        // Only mark misspelling if:
        // 1. Current text checking isn't done for autocorrection, in which case shouldMarkSpelling is false.
        // 2. Result falls within spellingRange.
        // 3. The word in question doesn't end at an ambiguous boundary. For instance, we would not mark
        //    "wouldn'" as misspelled right after apostrophe is typed.
        if (shouldMarkSpelling && result->type == TextCheckingTypeSpelling && resultLocation >= spellingParagraph.checkingStart() && resultLocation + resultLength <= spellingRangeEndOffset && !resultEndsAtAmbiguousBoundary) {
            ASSERT(resultLength > 0 && resultLocation >= 0);
            RefPtr<Range> misspellingRange = spellingParagraph.subrange(resultLocation, resultLength);
            if (!m_spellingCorrector->isSpellingMarkerAllowed(misspellingRange))
                continue;
            misspellingRange->startContainer(ec)->document()->markers()->addMarker(misspellingRange.get(), DocumentMarker::Spelling);
        } else if (shouldMarkGrammar && result->type == TextCheckingTypeGrammar && grammarParagraph.checkingRangeCovers(resultLocation, resultLength)) {
            ASSERT(resultLength > 0 && resultLocation >= 0);
            for (unsigned j = 0; j < result->details.size(); j++) {
                const GrammarDetail* detail = &result->details[j];
                ASSERT(detail->length > 0 && detail->location >= 0);
                if (grammarParagraph.checkingRangeCovers(resultLocation + detail->location, detail->length)) {
                    RefPtr<Range> badGrammarRange = grammarParagraph.subrange(resultLocation + detail->location, detail->length);
                    grammarRange->startContainer(ec)->document()->markers()->addMarker(badGrammarRange.get(), DocumentMarker::Grammar, detail->userDescription);
                }
            }
        } else if (resultLocation + resultLength <= spellingRangeEndOffset && resultLocation + resultLength >= spellingParagraph.checkingStart()
                    && (result->type == TextCheckingTypeLink
                    || result->type == TextCheckingTypeQuote
                    || result->type == TextCheckingTypeDash
                    || result->type == TextCheckingTypeReplacement
                    || result->type == TextCheckingTypeCorrection)) {
            // In this case the result range just has to touch the spelling range, so we can handle replacing non-word text such as punctuation.
            ASSERT(resultLength > 0 && resultLocation >= 0);

            if (shouldShowCorrectionPanel && (resultLocation + resultLength < spellingRangeEndOffset || result->type != TextCheckingTypeCorrection))
                continue;

            int replacementLength = result->replacement.length();

            // Apply replacement if:
            // 1. The replacement length is non-zero.
            // 2. The result doesn't end at an ambiguous boundary.
            //    (FIXME: this is required until 6853027 is fixed and text checking can do this for us
            bool doReplacement = replacementLength > 0 && !resultEndsAtAmbiguousBoundary;
            RefPtr<Range> rangeToReplace = spellingParagraph.subrange(resultLocation, resultLength);
            VisibleSelection selectionToReplace(rangeToReplace.get(), DOWNSTREAM);

            // adding links should be done only immediately after they are typed
            if (result->type == TextCheckingTypeLink && selectionOffset > resultLocation + resultLength + 1)
                continue;

            String replacedString = plainText(rangeToReplace.get());

            // Don't correct spelling in an already-corrected word.
            DocumentMarkerController* markers = m_frame->document()->markers();
            if (markers->hasMarkers(rangeToReplace.get(), DocumentMarker::Replacement)) {
                doReplacement = false;
                if (result->type == TextCheckingTypeCorrection)
                    m_spellingCorrector->recordSpellcheckerResponseForModifiedCorrection(rangeToReplace.get(), replacedString, result->replacement);
            } else if (markers->hasMarkers(rangeToReplace.get(), DocumentMarker::RejectedCorrection))
                doReplacement = false;

            if (!(shouldPerformReplacement || shouldShowCorrectionPanel) || !doReplacement)
                continue;

            if (shouldShowCorrectionPanel) {
                ASSERT(SUPPORT_AUTOCORRECTION_PANEL);
                // shouldShowCorrectionPanel can be true only when the panel is available.
                if (resultLocation + resultLength == spellingRangeEndOffset) {
                    // We only show the correction panel on the last word.
                    m_spellingCorrector->show(rangeToReplace, result->replacement);
                    break;
                }
                // If this function is called for showing correction panel, we ignore other correction or replacement.
                continue;
            }

            if (selectionToReplace != m_frame->selection()->selection()) {
                if (!m_frame->selection()->shouldChangeSelection(selectionToReplace))
                    continue;
            }

            if (result->type == TextCheckingTypeLink) {
                m_frame->selection()->setSelection(selectionToReplace);
                selectionChanged = true;
                restoreSelectionAfterChange = false;
                if (canEditRichly())
                    applyCommand(CreateLinkCommand::create(m_frame->document(), result->replacement));
            } else if (canEdit() && shouldInsertText(result->replacement, rangeToReplace.get(), EditorInsertActionTyped)) {
                applyCommand(SpellingCorrectionCommand::create(rangeToReplace, result->replacement));

                if (AXObjectCache::accessibilityEnabled()) {
                    if (Element* root = m_frame->selection()->selection().rootEditableElement())
                        m_frame->document()->axObjectCache()->postNotification(root->renderer(), AXObjectCache::AXAutocorrectionOccured, true);
                }

                selectionChanged = true;
                offsetDueToReplacement += replacementLength - resultLength;
                if (resultLocation < selectionOffset) {
                    selectionOffset += replacementLength - resultLength;
                    if (ambiguousBoundaryOffset >= 0)
                        ambiguousBoundaryOffset = selectionOffset - 1;
                }

                // Add a marker so that corrections can easily be undone and won't be re-corrected.
                if (result->type == TextCheckingTypeCorrection)
                    m_spellingCorrector->markCorrection(spellingParagraph.subrange(resultLocation, replacementLength), replacedString);
            }
        }
    }

    if (selectionChanged) {
        // Restore the caret position if we have made any replacements
        spellingParagraph.expandRangeToNextEnd();
        if (restoreSelectionAfterChange && selectionOffset >= 0 && selectionOffset <= spellingParagraph.rangeLength()) {
            RefPtr<Range> selectionRange = spellingParagraph.subrange(0, selectionOffset);
            m_frame->selection()->moveTo(selectionRange->endPosition(), DOWNSTREAM);
            if (adjustSelectionForParagraphBoundaries)
                m_frame->selection()->modify(SelectionController::AlterationMove, DirectionForward, CharacterGranularity);
        } else {
            // If this fails for any reason, the fallback is to go one position beyond the last replacement
            m_frame->selection()->moveTo(m_frame->selection()->end());
            m_frame->selection()->modify(SelectionController::AlterationMove, DirectionForward, CharacterGranularity);
        }
    }
#else
    ASSERT_NOT_REACHED();
    UNUSED_PARAM(textCheckingOptions);
    UNUSED_PARAM(spellingRange);
    UNUSED_PARAM(grammarRange);
#endif // USE(UNIFIED_TEXT_CHECKING)
}

void Editor::changeBackToReplacedString(const String& replacedString)
{
#if USE(UNIFIED_TEXT_CHECKING)
    if (replacedString.isEmpty())
        return;

    RefPtr<Range> selection = selectedRange();
    if (!shouldInsertText(replacedString, selection.get(), EditorInsertActionPasted))
        return;
    
    m_spellingCorrector->recordAutocorrectionResponseReversed(replacedString, selection);
    TextCheckingParagraph paragraph(selection);
    replaceSelectionWithText(replacedString, false, false);
    RefPtr<Range> changedRange = paragraph.subrange(paragraph.checkingStart(), replacedString.length());
    changedRange->startContainer()->document()->markers()->addMarker(changedRange.get(), DocumentMarker::Replacement, String());
    m_spellingCorrector->markReversed(changedRange.get());
#else
    ASSERT_NOT_REACHED();
    UNUSED_PARAM(replacedString);
#endif // USE(UNIFIED_TEXT_CHECKING)
}


void Editor::markMisspellingsAndBadGrammar(const VisibleSelection& spellingSelection, bool markGrammar, const VisibleSelection& grammarSelection)
{
#if USE(UNIFIED_TEXT_CHECKING)
    if (!isContinuousSpellCheckingEnabled())
        return;
    TextCheckingOptions textCheckingOptions = MarkSpelling | CheckForCorrection;
    if (markGrammar && isGrammarCheckingEnabled())
        textCheckingOptions |= MarkGrammar;
    markAllMisspellingsAndBadGrammarInRanges(textCheckingOptions, spellingSelection.toNormalizedRange().get(), grammarSelection.toNormalizedRange().get());
#else
    RefPtr<Range> firstMisspellingRange;
    markMisspellings(spellingSelection, firstMisspellingRange);
    if (markGrammar)
        markBadGrammar(grammarSelection);
#endif
}

void Editor::unappliedSpellCorrection(const VisibleSelection& selectionOfCorrected, const String& corrected, const String& correction)
{
    m_spellingCorrector->respondToUnappliedSpellCorrection(selectionOfCorrected, corrected, correction);
}

void Editor::updateMarkersForWordsAffectedByEditing(bool doNotRemoveIfSelectionAtWordBoundary)
{
    if (!m_spellingCorrector->shouldRemoveMarkersUponEditing())
        return;

    // We want to remove the markers from a word if an editing command will change the word. This can happen in one of
    // several scenarios:
    // 1. Insert in the middle of a word.
    // 2. Appending non whitespace at the beginning of word.
    // 3. Appending non whitespace at the end of word.
    // Note that, appending only whitespaces at the beginning or end of word won't change the word, so we don't need to
    // remove the markers on that word.
    // Of course, if current selection is a range, we potentially will edit two words that fall on the boundaries of
    // selection, and remove words between the selection boundaries.
    //
    VisiblePosition startOfSelection = frame()->selection()->selection().start();
    VisiblePosition endOfSelection = frame()->selection()->selection().end();
    if (startOfSelection.isNull())
        return;
    // First word is the word that ends after or on the start of selection.
    VisiblePosition startOfFirstWord = startOfWord(startOfSelection, LeftWordIfOnBoundary);
    VisiblePosition endOfFirstWord = endOfWord(startOfSelection, LeftWordIfOnBoundary);
    // Last word is the word that begins before or on the end of selection
    VisiblePosition startOfLastWord = startOfWord(endOfSelection, RightWordIfOnBoundary);
    VisiblePosition endOfLastWord = endOfWord(endOfSelection, RightWordIfOnBoundary);

    if (startOfFirstWord.isNull()) {
        startOfFirstWord = startOfWord(startOfSelection, RightWordIfOnBoundary);
        endOfFirstWord = endOfWord(startOfSelection, RightWordIfOnBoundary);
    }
    
    if (endOfLastWord.isNull()) {
        startOfLastWord = startOfWord(endOfSelection, LeftWordIfOnBoundary);
        endOfLastWord = endOfWord(endOfSelection, LeftWordIfOnBoundary);
    }

    // If doNotRemoveIfSelectionAtWordBoundary is true, and first word ends at the start of selection,
    // we choose next word as the first word.
    if (doNotRemoveIfSelectionAtWordBoundary && endOfFirstWord == startOfSelection) {
        startOfFirstWord = nextWordPosition(startOfFirstWord);
        endOfFirstWord = endOfWord(startOfFirstWord, RightWordIfOnBoundary);
        if (startOfFirstWord == endOfSelection)
            return;
    }

    // If doNotRemoveIfSelectionAtWordBoundary is true, and last word begins at the end of selection,
    // we choose previous word as the last word.
    if (doNotRemoveIfSelectionAtWordBoundary && startOfLastWord == endOfSelection) {
        startOfLastWord = previousWordPosition(startOfLastWord);
        endOfLastWord = endOfWord(startOfLastWord, RightWordIfOnBoundary);
        if (endOfLastWord == startOfSelection)
            return;
    }

    if (startOfFirstWord.isNull() || endOfFirstWord.isNull() || startOfLastWord.isNull() || endOfLastWord.isNull())
        return;

    // Now we remove markers on everything between startOfFirstWord and endOfLastWord.
    // However, if an autocorrection change a single word to multiple words, we want to remove correction mark from all the
    // resulted words even we only edit one of them. For example, assuming autocorrection changes "avantgarde" to "avant
    // garde", we will have CorrectionIndicator marker on both words and on the whitespace between them. If we then edit garde,
    // we would like to remove the marker from word "avant" and whitespace as well. So we need to get the continous range of
    // of marker that contains the word in question, and remove marker on that whole range.
    Document* document = m_frame->document();
    RefPtr<Range> wordRange = Range::create(document, startOfFirstWord.deepEquivalent(), endOfLastWord.deepEquivalent());

    document->markers()->removeMarkers(wordRange.get(), DocumentMarker::Spelling | DocumentMarker::CorrectionIndicator | DocumentMarker::SpellCheckingExemption, DocumentMarkerController::RemovePartiallyOverlappingMarker);
    document->markers()->clearDescriptionOnMarkersIntersectingRange(wordRange.get(), DocumentMarker::Replacement);
}

PassRefPtr<Range> Editor::rangeForPoint(const IntPoint& windowPoint)
{
    Document* document = m_frame->documentAtPoint(windowPoint);
    if (!document)
        return 0;
    
    Frame* frame = document->frame();
    ASSERT(frame);
    FrameView* frameView = frame->view();
    if (!frameView)
        return 0;
    IntPoint framePoint = frameView->windowToContents(windowPoint);
    VisibleSelection selection(frame->visiblePositionForPoint(framePoint));
    return avoidIntersectionWithNode(selection.toNormalizedRange().get(), m_deleteButtonController->containerElement());
}

void Editor::revealSelectionAfterEditingOperation()
{
    if (m_ignoreCompositionSelectionChange)
        return;

    m_frame->selection()->revealSelection(ScrollAlignment::alignToEdgeIfNeeded);
}

void Editor::setIgnoreCompositionSelectionChange(bool ignore)
{
    if (m_ignoreCompositionSelectionChange == ignore)
        return;

    m_ignoreCompositionSelectionChange = ignore;
    if (!ignore)
        revealSelectionAfterEditingOperation();
}

PassRefPtr<Range> Editor::compositionRange() const
{
    if (!m_compositionNode)
        return 0;
    unsigned length = m_compositionNode->length();
    unsigned start = min(m_compositionStart, length);
    unsigned end = min(max(start, m_compositionEnd), length);
    if (start >= end)
        return 0;
    return Range::create(m_compositionNode->document(), m_compositionNode.get(), start, m_compositionNode.get(), end);
}

bool Editor::getCompositionSelection(unsigned& selectionStart, unsigned& selectionEnd) const
{
    if (!m_compositionNode)
        return false;
    Position start = m_frame->selection()->start();
    if (start.deprecatedNode() != m_compositionNode)
        return false;
    Position end = m_frame->selection()->end();
    if (end.deprecatedNode() != m_compositionNode)
        return false;

    if (static_cast<unsigned>(start.deprecatedEditingOffset()) < m_compositionStart)
        return false;
    if (static_cast<unsigned>(end.deprecatedEditingOffset()) > m_compositionEnd)
        return false;

    selectionStart = start.deprecatedEditingOffset() - m_compositionStart;
    selectionEnd = start.deprecatedEditingOffset() - m_compositionEnd;
    return true;
}

void Editor::transpose()
{
    if (!canEdit())
        return;

     VisibleSelection selection = m_frame->selection()->selection();
     if (!selection.isCaret())
         return;

    // Make a selection that goes back one character and forward two characters.
    VisiblePosition caret = selection.visibleStart();
    VisiblePosition next = isEndOfParagraph(caret) ? caret : caret.next();
    VisiblePosition previous = next.previous();
    if (next == previous)
        return;
    previous = previous.previous();
    if (!inSameParagraph(next, previous))
        return;
    RefPtr<Range> range = makeRange(previous, next);
    if (!range)
        return;
    VisibleSelection newSelection(range.get(), DOWNSTREAM);

    // Transpose the two characters.
    String text = plainText(range.get());
    if (text.length() != 2)
        return;
    String transposed = text.right(1) + text.left(1);

    // Select the two characters.
    if (newSelection != m_frame->selection()->selection()) {
        if (!m_frame->selection()->shouldChangeSelection(newSelection))
            return;
        m_frame->selection()->setSelection(newSelection);
    }

    // Insert the transposed characters.
    if (!shouldInsertText(transposed, range.get(), EditorInsertActionTyped))
        return;
    replaceSelectionWithText(transposed, false, false);
}

void Editor::addToKillRing(Range* range, bool prepend)
{
    if (m_shouldStartNewKillRingSequence)
        killRing()->startNewSequence();

    String text = plainText(range);
    if (prepend)
        killRing()->prepend(text);
    else
        killRing()->append(text);
    m_shouldStartNewKillRingSequence = false;
}

void Editor::startCorrectionPanelTimer()
{
    m_spellingCorrector->startCorrectionPanelTimer(CorrectionPanelInfo::PanelTypeCorrection);
}

void Editor::handleCorrectionPanelResult(const String& correction)
{
    m_spellingCorrector->handleCorrectionPanelResult(correction);
}


void Editor::dismissCorrectionPanelAsIgnored()
{
    m_spellingCorrector->dismiss(ReasonForDismissingCorrectionPanelIgnored);
}

bool Editor::insideVisibleArea(const IntPoint& point) const
{
    if (m_frame->excludeFromTextSearch())
        return false;
    
    // Right now, we only check the visibility of a point for disconnected frames. For all other
    // frames, we assume visibility.
    Frame* frame = m_frame->isDisconnected() ? m_frame : m_frame->tree()->top(true);
    if (!frame->isDisconnected())
        return true;
    
    RenderPart* renderer = frame->ownerRenderer();
    if (!renderer)
        return false;

    RenderBlock* container = renderer->containingBlock();
    if (!(container->style()->overflowX() == OHIDDEN || container->style()->overflowY() == OHIDDEN))
        return true;

    IntRect rectInPageCoords = container->overflowClipRect(0, 0);
    IntRect rectInFrameCoords = IntRect(renderer->x() * -1, renderer->y() * -1,
                                    rectInPageCoords.width(), rectInPageCoords.height());

    return rectInFrameCoords.contains(point);
}

bool Editor::insideVisibleArea(Range* range) const
{
    if (!range)
        return true;

    if (m_frame->excludeFromTextSearch())
        return false;
    
    // Right now, we only check the visibility of a range for disconnected frames. For all other
    // frames, we assume visibility.
    Frame* frame = m_frame->isDisconnected() ? m_frame : m_frame->tree()->top(true);
    if (!frame->isDisconnected())
        return true;
    
    RenderPart* renderer = frame->ownerRenderer();
    if (!renderer)
        return false;

    RenderBlock* container = renderer->containingBlock();
    if (!(container->style()->overflowX() == OHIDDEN || container->style()->overflowY() == OHIDDEN))
        return true;

    IntRect rectInPageCoords = container->overflowClipRect(0, 0);
    IntRect rectInFrameCoords = IntRect(renderer->x() * -1, renderer->y() * -1,
                                    rectInPageCoords.width(), rectInPageCoords.height());
    IntRect resultRect = range->boundingBox();
    
    return rectInFrameCoords.contains(resultRect);
}

PassRefPtr<Range> Editor::firstVisibleRange(const String& target, FindOptions options)
{
    RefPtr<Range> searchRange(rangeOfContents(m_frame->document()));
    RefPtr<Range> resultRange = findPlainText(searchRange.get(), target, options & ~Backwards);
    ExceptionCode ec = 0;

    while (!insideVisibleArea(resultRange.get())) {
        searchRange->setStartAfter(resultRange->endContainer(), ec);
        if (searchRange->startContainer() == searchRange->endContainer())
            return Range::create(m_frame->document());
        resultRange = findPlainText(searchRange.get(), target, options & ~Backwards);
    }
    
    return resultRange;
}

PassRefPtr<Range> Editor::lastVisibleRange(const String& target, FindOptions options)
{
    RefPtr<Range> searchRange(rangeOfContents(m_frame->document()));
    RefPtr<Range> resultRange = findPlainText(searchRange.get(), target, options | Backwards);
    ExceptionCode ec = 0;

    while (!insideVisibleArea(resultRange.get())) {
        searchRange->setEndBefore(resultRange->startContainer(), ec);
        if (searchRange->startContainer() == searchRange->endContainer())
            return Range::create(m_frame->document());
        resultRange = findPlainText(searchRange.get(), target, options | Backwards);
    }
    
    return resultRange;
}

PassRefPtr<Range> Editor::nextVisibleRange(Range* currentRange, const String& target, FindOptions options)
{
    if (m_frame->excludeFromTextSearch())
        return Range::create(m_frame->document());

    RefPtr<Range> resultRange = currentRange;
    RefPtr<Range> searchRange(rangeOfContents(m_frame->document()));
    ExceptionCode ec = 0;
    bool forward = !(options & Backwards);
    for ( ; !insideVisibleArea(resultRange.get()); resultRange = findPlainText(searchRange.get(), target, options)) {
        if (resultRange->collapsed(ec)) {
            if (!resultRange->startContainer()->isInShadowTree())
                break;
            searchRange = rangeOfContents(m_frame->document());
            if (forward)
                searchRange->setStartAfter(resultRange->startContainer()->shadowAncestorNode(), ec);
            else
                searchRange->setEndBefore(resultRange->startContainer()->shadowAncestorNode(), ec);
            continue;
        }

        if (forward)
            searchRange->setStartAfter(resultRange->endContainer(), ec);
        else
            searchRange->setEndBefore(resultRange->startContainer(), ec);

        Node* shadowTreeRoot = searchRange->shadowTreeRootNode();
        if (searchRange->collapsed(ec) && shadowTreeRoot) {
            if (forward)
                searchRange->setEnd(shadowTreeRoot, shadowTreeRoot->childNodeCount(), ec);
            else
                searchRange->setStartBefore(shadowTreeRoot, ec);
        }
        
        if (searchRange->startContainer()->isDocumentNode() && searchRange->endContainer()->isDocumentNode())
            break;
    }
    
    if (insideVisibleArea(resultRange.get()))
        return resultRange;
    
    if (!(options & WrapAround))
        return Range::create(m_frame->document());

    if (options & Backwards)
        return lastVisibleRange(target, options);

    return firstVisibleRange(target, options);
}

void Editor::changeSelectionAfterCommand(const VisibleSelection& newSelection, bool closeTyping, bool clearTypingStyle)
{
    // If the new selection is orphaned, then don't update the selection.
    if (newSelection.start().isOrphan() || newSelection.end().isOrphan())
        return;

    // If there is no selection change, don't bother sending shouldChangeSelection, but still call setSelection,
    // because there is work that it must do in this situation.
    // The old selection can be invalid here and calling shouldChangeSelection can produce some strange calls.
    // See <rdar://problem/5729315> Some shouldChangeSelectedDOMRange contain Ranges for selections that are no longer valid
    bool selectionDidNotChangeDOMPosition = newSelection == m_frame->selection()->selection();
    if (selectionDidNotChangeDOMPosition || m_frame->selection()->shouldChangeSelection(newSelection)) {
        SelectionController::SetSelectionOptions options = 0;
        if (closeTyping)
            options |= SelectionController::CloseTyping;
        if (clearTypingStyle)
            options |= SelectionController::ClearTypingStyle;
        m_frame->selection()->setSelection(newSelection, options);
    }

    // Some editing operations change the selection visually without affecting its position within the DOM.
    // For example when you press return in the following (the caret is marked by ^):
    // <div contentEditable="true"><div>^Hello</div></div>
    // WebCore inserts <div><br></div> *before* the current block, which correctly moves the paragraph down but which doesn't
    // change the caret's DOM position (["hello", 0]).  In these situations the above SelectionController::setSelection call
    // does not call EditorClient::respondToChangedSelection(), which, on the Mac, sends selection change notifications and
    // starts a new kill ring sequence, but we want to do these things (matches AppKit).
    if (selectionDidNotChangeDOMPosition)
        client()->respondToChangedSelection();
}

String Editor::selectedText() const
{
    // We remove '\0' characters because they are not visibly rendered to the user.
    return plainText(m_frame->selection()->toNormalizedRange().get()).replace(0, "");
}

IntRect Editor::firstRectForRange(Range* range) const
{
    int extraWidthToEndOfLine = 0;
    ASSERT(range->startContainer());
    ASSERT(range->endContainer());

    InlineBox* startInlineBox;
    int startCaretOffset;
    Position startPosition = VisiblePosition(range->startPosition()).deepEquivalent();
    if (startPosition.isNull())
        return IntRect();
    startPosition.getInlineBoxAndOffset(DOWNSTREAM, startInlineBox, startCaretOffset);

    RenderObject* startRenderer = startPosition.deprecatedNode()->renderer();
    ASSERT(startRenderer);
    IntRect startCaretRect = startRenderer->localCaretRect(startInlineBox, startCaretOffset, &extraWidthToEndOfLine);
    if (startCaretRect != IntRect())
        startCaretRect = startRenderer->localToAbsoluteQuad(FloatRect(startCaretRect)).enclosingBoundingBox();

    InlineBox* endInlineBox;
    int endCaretOffset;
    Position endPosition = VisiblePosition(range->endPosition()).deepEquivalent();
    if (endPosition.isNull())
        return IntRect();
    endPosition.getInlineBoxAndOffset(UPSTREAM, endInlineBox, endCaretOffset);

    RenderObject* endRenderer = endPosition.deprecatedNode()->renderer();
    ASSERT(endRenderer);
    IntRect endCaretRect = endRenderer->localCaretRect(endInlineBox, endCaretOffset);
    if (endCaretRect != IntRect())
        endCaretRect = endRenderer->localToAbsoluteQuad(FloatRect(endCaretRect)).enclosingBoundingBox();

    if (startCaretRect.y() == endCaretRect.y()) {
        // start and end are on the same line
        return IntRect(min(startCaretRect.x(), endCaretRect.x()),
            startCaretRect.y(),
            abs(endCaretRect.x() - startCaretRect.x()),
            max(startCaretRect.height(), endCaretRect.height()));
    }

    // start and end aren't on the same line, so go from start to the end of its line
    return IntRect(startCaretRect.x(),
        startCaretRect.y(),
        startCaretRect.width() + extraWidthToEndOfLine,
        startCaretRect.height());
}

bool Editor::shouldChangeSelection(const VisibleSelection& oldSelection, const VisibleSelection& newSelection, EAffinity affinity, bool stillSelecting) const
{
    return client() && client()->shouldChangeSelectedRange(oldSelection.toNormalizedRange().get(), newSelection.toNormalizedRange().get(), affinity, stillSelecting);
}

void Editor::computeAndSetTypingStyle(CSSStyleDeclaration* style, EditAction editingAction)
{
    if (!style || !style->length()) {
        m_frame->selection()->clearTypingStyle();
        return;
    }

    // Calculate the current typing style.
    RefPtr<EditingStyle> typingStyle;
    if (m_frame->selection()->typingStyle()) {
        typingStyle = m_frame->selection()->typingStyle()->copy();
        typingStyle->overrideWithStyle(style->makeMutable().get());
    } else
        typingStyle = EditingStyle::create(style);

    typingStyle->prepareToApplyAt(m_frame->selection()->selection().visibleStart().deepEquivalent(), EditingStyle::PreserveWritingDirection);

    // Handle block styles, substracting these from the typing style.
    RefPtr<EditingStyle> blockStyle = typingStyle->extractAndRemoveBlockProperties();
    if (!blockStyle->isEmpty())
        applyCommand(ApplyStyleCommand::create(m_frame->document(), blockStyle.get(), editingAction));

    // Set the remaining style as the typing style.
    m_frame->selection()->setTypingStyle(typingStyle);
}

PassRefPtr<EditingStyle> Editor::selectionStartStyle() const
{
    if (m_frame->selection()->isNone())
        return 0;

    RefPtr<Range> range(m_frame->selection()->toNormalizedRange());
    Position position = range->editingStartPosition();

    // If the pos is at the end of a text node, then this node is not fully selected. 
    // Move it to the next deep equivalent position to avoid removing the style from this node. 
    // e.g. if pos was at Position("hello", 5) in <b>hello<div>world</div></b>, we want Position("world", 0) instead. 
    // We only do this for range because caret at Position("hello", 5) in <b>hello</b>world should give you font-weight: bold. 
    Node* positionNode = position.containerNode(); 
    if (m_frame->selection()->isRange() && positionNode && positionNode->isTextNode() && position.computeOffsetInContainerNode() == positionNode->maxCharacterOffset()) 
        position = nextVisuallyDistinctCandidate(position); 

    Element* element = position.element();
    if (!element)
        return 0;

    RefPtr<EditingStyle> style = EditingStyle::create(element, EditingStyle::AllProperties);
    style->mergeTypingStyle(m_frame->document());
    return style;
}

void Editor::textFieldDidBeginEditing(Element* e)
{
    if (client())
        client()->textFieldDidBeginEditing(e);
}

void Editor::textFieldDidEndEditing(Element* e)
{
    if (client())
        client()->textFieldDidEndEditing(e);
}

void Editor::textDidChangeInTextField(Element* e)
{
    if (client())
        client()->textDidChangeInTextField(e);
}

bool Editor::doTextFieldCommandFromEvent(Element* e, KeyboardEvent* ke)
{
    if (client())
        return client()->doTextFieldCommandFromEvent(e, ke);

    return false;
}

void Editor::textWillBeDeletedInTextField(Element* input)
{
    if (client())
        client()->textWillBeDeletedInTextField(input);
}

void Editor::textDidChangeInTextArea(Element* e)
{
    if (client())
        client()->textDidChangeInTextArea(e);
}

void Editor::applyEditingStyleToBodyElement() const
{
    RefPtr<NodeList> list = m_frame->document()->getElementsByTagName("body");
    unsigned len = list->length();
    for (unsigned i = 0; i < len; i++)
        applyEditingStyleToElement(static_cast<Element*>(list->item(i)));
}

void Editor::applyEditingStyleToElement(Element* element) const
{
    if (!element)
        return;

    CSSStyleDeclaration* style = element->style();
    ASSERT(style);

    ExceptionCode ec = 0;
    style->setProperty(CSSPropertyWordWrap, "break-word", false, ec);
    ASSERT(!ec);
    style->setProperty(CSSPropertyWebkitNbspMode, "space", false, ec);
    ASSERT(!ec);
    style->setProperty(CSSPropertyWebkitLineBreak, "after-white-space", false, ec);
    ASSERT(!ec);
}

RenderStyle* Editor::styleForSelectionStart(Node *&nodeToRemove) const
{
    nodeToRemove = 0;

    if (m_frame->selection()->isNone())
        return 0;

    Position position = m_frame->selection()->selection().visibleStart().deepEquivalent();
    if (!position.isCandidate())
        return 0;
    if (!position.deprecatedNode())
        return 0;

    RefPtr<EditingStyle> typingStyle = m_frame->selection()->typingStyle();
    if (!typingStyle || !typingStyle->style())
        return position.deprecatedNode()->renderer()->style();

    RefPtr<Element> styleElement = m_frame->document()->createElement(spanTag, false);

    ExceptionCode ec = 0;
    String styleText = typingStyle->style()->cssText() + " display: inline";
    styleElement->setAttribute(styleAttr, styleText.impl(), ec);
    ASSERT(!ec);

    styleElement->appendChild(m_frame->document()->createEditingTextNode(""), ec);
    ASSERT(!ec);

    position.deprecatedNode()->parentNode()->appendChild(styleElement, ec);
    ASSERT(!ec);

    nodeToRemove = styleElement.get();
    return styleElement->renderer() ? styleElement->renderer()->style() : 0;
}

// Searches from the beginning of the document if nothing is selected.
bool Editor::findString(const String& target, bool forward, bool caseFlag, bool wrapFlag, bool startInSelection)
{
    FindOptions options = (forward ? 0 : Backwards) | (caseFlag ? 0 : CaseInsensitive) | (wrapFlag ? WrapAround : 0) | (startInSelection ? StartInSelection : 0);
    return findString(target, options);
}

bool Editor::findString(const String& target, FindOptions options)
{
    if (target.isEmpty())
        return false;

    if (m_frame->excludeFromTextSearch())
        return false;

    // Start from an edge of the selection, if there's a selection that's not in shadow content. Which edge
    // is used depends on whether we're searching forward or backward, and whether startInSelection is set.
    RefPtr<Range> searchRange(rangeOfContents(m_frame->document()));
    VisibleSelection selection = m_frame->selection()->selection();

    bool forward = !(options & Backwards);
    bool startInSelection = options & StartInSelection;
    if (forward)
        setStart(searchRange.get(), startInSelection ? selection.visibleStart() : selection.visibleEnd());
    else
        setEnd(searchRange.get(), startInSelection ? selection.visibleEnd() : selection.visibleStart());

    RefPtr<Node> shadowTreeRoot = selection.shadowTreeRootNode();
    if (shadowTreeRoot) {
        ExceptionCode ec = 0;
        if (forward)
            searchRange->setEnd(shadowTreeRoot.get(), shadowTreeRoot->childNodeCount(), ec);
        else
            searchRange->setStart(shadowTreeRoot.get(), 0, ec);
    }

    RefPtr<Range> resultRange(findPlainText(searchRange.get(), target, options));
    // If we started in the selection and the found range exactly matches the existing selection, find again.
    // Build a selection with the found range to remove collapsed whitespace.
    // Compare ranges instead of selection objects to ignore the way that the current selection was made.
    if (startInSelection && areRangesEqual(VisibleSelection(resultRange.get()).toNormalizedRange().get(), selection.toNormalizedRange().get())) {
        searchRange = rangeOfContents(m_frame->document());
        if (forward)
            setStart(searchRange.get(), selection.visibleEnd());
        else
            setEnd(searchRange.get(), selection.visibleStart());

        if (shadowTreeRoot) {
            ExceptionCode ec = 0;
            if (forward)
                searchRange->setEnd(shadowTreeRoot.get(), shadowTreeRoot->childNodeCount(), ec);
            else
                searchRange->setStart(shadowTreeRoot.get(), 0, ec);
        }

        resultRange = findPlainText(searchRange.get(), target, options);
    }

    ExceptionCode exception = 0;

    // If nothing was found in the shadow tree, search in main content following the shadow tree.
    if (resultRange->collapsed(exception) && shadowTreeRoot) {
        searchRange = rangeOfContents(m_frame->document());
        if (forward)
            searchRange->setStartAfter(shadowTreeRoot->shadowHost(), exception);
        else
            searchRange->setEndBefore(shadowTreeRoot->shadowHost(), exception);

        resultRange = findPlainText(searchRange.get(), target, options);
    }

    if (!insideVisibleArea(resultRange.get())) {
        resultRange = nextVisibleRange(resultRange.get(), target, options);
        if (!resultRange)
            return false;
    }

    // If we didn't find anything and we're wrapping, search again in the entire document (this will
    // redundantly re-search the area already searched in some cases).
    if (resultRange->collapsed(exception) && options & WrapAround) {
        searchRange = rangeOfContents(m_frame->document());
        resultRange = findPlainText(searchRange.get(), target, options);
        // We used to return false here if we ended up with the same range that we started with
        // (e.g., the selection was already the only instance of this text). But we decided that
        // this should be a success case instead, so we'll just fall through in that case.
    }

    if (resultRange->collapsed(exception))
        return false;

    m_frame->selection()->setSelection(VisibleSelection(resultRange.get(), DOWNSTREAM));
    m_frame->selection()->revealSelection();
    return true;
}

static bool isFrameInRange(Frame* frame, Range* range)
{
    bool inRange = false;
    for (HTMLFrameOwnerElement* ownerElement = frame->ownerElement(); ownerElement; ownerElement = ownerElement->document()->ownerElement()) {
        if (ownerElement->document() == range->ownerDocument()) {
            ExceptionCode ec = 0;
            inRange = range->intersectsNode(ownerElement, ec);
            break;
        }
    }
    return inRange;
}

unsigned Editor::countMatchesForText(const String& target, FindOptions options, unsigned limit, bool markMatches)
{
    return countMatchesForText(target, 0, options, limit, markMatches);
}

unsigned Editor::countMatchesForText(const String& target, Range* range, FindOptions options, unsigned limit, bool markMatches)
{
    if (target.isEmpty())
        return 0;

    RefPtr<Range> searchRange;
    if (range) {
        if (range->ownerDocument() == m_frame->document())
            searchRange = range;
        else if (!isFrameInRange(m_frame, range))
            return 0;
    }
    if (!searchRange)
        searchRange = rangeOfContents(m_frame->document());

    Node* originalEndContainer = searchRange->endContainer();
    int originalEndOffset = searchRange->endOffset();

    ExceptionCode exception = 0;
    unsigned matchCount = 0;
    do {
        RefPtr<Range> resultRange(findPlainText(searchRange.get(), target, options & ~Backwards));
        if (resultRange->collapsed(exception)) {
            if (!resultRange->startContainer()->isInShadowTree())
                break;

            searchRange->setStartAfter(resultRange->startContainer()->shadowAncestorNode(), exception);
            searchRange->setEnd(originalEndContainer, originalEndOffset, exception);
            continue;
        }

        // Only treat the result as a match if it is visible
        if (insideVisibleArea(resultRange.get())) {
            ++matchCount;
            if (markMatches)
                m_frame->document()->markers()->addMarker(resultRange.get(), DocumentMarker::TextMatch);
        }

        // Stop looking if we hit the specified limit. A limit of 0 means no limit.
        if (limit > 0 && matchCount >= limit)
            break;

        // Set the new start for the search range to be the end of the previous
        // result range. There is no need to use a VisiblePosition here,
        // since findPlainText will use a TextIterator to go over the visible
        // text nodes. 
        searchRange->setStart(resultRange->endContainer(exception), resultRange->endOffset(exception), exception);

        Node* shadowTreeRoot = searchRange->shadowTreeRootNode();
        if (searchRange->collapsed(exception) && shadowTreeRoot)
            searchRange->setEnd(shadowTreeRoot, shadowTreeRoot->childNodeCount(), exception);
    } while (true);

    if (markMatches) {
        // Do a "fake" paint in order to execute the code that computes the rendered rect for each text match.
        if (m_frame->view() && m_frame->contentRenderer()) {
            m_frame->document()->updateLayout(); // Ensure layout is up to date.
            IntRect visibleRect = m_frame->view()->visibleContentRect();
            if (!visibleRect.isEmpty()) {
                GraphicsContext context((PlatformGraphicsContext*)0);
                context.setPaintingDisabled(true);

                PaintBehavior oldBehavior = m_frame->view()->paintBehavior();
                m_frame->view()->setPaintBehavior(oldBehavior | PaintBehaviorFlattenCompositingLayers);
                m_frame->view()->paintContents(&context, visibleRect);
                m_frame->view()->setPaintBehavior(oldBehavior);
            }
        }
    }

    return matchCount;
}

void Editor::setMarkedTextMatchesAreHighlighted(bool flag)
{
    if (flag == m_areMarkedTextMatchesHighlighted)
        return;

    m_areMarkedTextMatchesHighlighted = flag;
    m_frame->document()->markers()->repaintMarkers(DocumentMarker::TextMatch);
}

void Editor::respondToChangedSelection(const VisibleSelection& oldSelection, SelectionController::SetSelectionOptions options)
{
    m_spellingCorrector->stopPendingCorrection(oldSelection);

    bool closeTyping = options & SelectionController::CloseTyping;
    bool isContinuousSpellCheckingEnabled = this->isContinuousSpellCheckingEnabled();
    bool isContinuousGrammarCheckingEnabled = isContinuousSpellCheckingEnabled && isGrammarCheckingEnabled();
    if (isContinuousSpellCheckingEnabled) {
        VisibleSelection newAdjacentWords;
        VisibleSelection newSelectedSentence;
        bool caretBrowsing = m_frame->settings() && m_frame->settings()->caretBrowsingEnabled();
        if (m_frame->selection()->selection().isContentEditable() || caretBrowsing) {
            VisiblePosition newStart(m_frame->selection()->selection().visibleStart());
            newAdjacentWords = VisibleSelection(startOfWord(newStart, LeftWordIfOnBoundary), endOfWord(newStart, RightWordIfOnBoundary));
            if (isContinuousGrammarCheckingEnabled)
                newSelectedSentence = VisibleSelection(startOfSentence(newStart), endOfSentence(newStart));
        }

        // Don't check spelling and grammar if the change of selection is triggered by spelling correction itself.
        bool shouldCheckSpellingAndGrammar = !(options & SelectionController::SpellCorrectionTriggered);

        // When typing we check spelling elsewhere, so don't redo it here.
        // If this is a change in selection resulting from a delete operation,
        // oldSelection may no longer be in the document.
        if (shouldCheckSpellingAndGrammar && closeTyping && oldSelection.isContentEditable() && oldSelection.start().deprecatedNode() && oldSelection.start().anchorNode()->inDocument()) {
            VisiblePosition oldStart(oldSelection.visibleStart());
            VisibleSelection oldAdjacentWords = VisibleSelection(startOfWord(oldStart, LeftWordIfOnBoundary), endOfWord(oldStart, RightWordIfOnBoundary));
            if (oldAdjacentWords != newAdjacentWords) {
                if (isContinuousGrammarCheckingEnabled) {
                    VisibleSelection oldSelectedSentence = VisibleSelection(startOfSentence(oldStart), endOfSentence(oldStart));
                    markMisspellingsAndBadGrammar(oldAdjacentWords, oldSelectedSentence != newSelectedSentence, oldSelectedSentence);
                } else
                    markMisspellingsAndBadGrammar(oldAdjacentWords, false, oldAdjacentWords);
            }
        }

#if !PLATFORM(MAC) || (PLATFORM(MAC) && (defined(BUILDING_ON_LEOPARD) || defined(BUILDING_ON_SNOW_LEOPARD)))
        // This only erases markers that are in the first unit (word or sentence) of the selection.
        // Perhaps peculiar, but it matches AppKit on these Mac OS X versions.
        if (RefPtr<Range> wordRange = newAdjacentWords.toNormalizedRange())
            m_frame->document()->markers()->removeMarkers(wordRange.get(), DocumentMarker::Spelling);
#endif
        if (RefPtr<Range> sentenceRange = newSelectedSentence.toNormalizedRange())
            m_frame->document()->markers()->removeMarkers(sentenceRange.get(), DocumentMarker::Grammar);
    }

    // When continuous spell checking is off, existing markers disappear after the selection changes.
    if (!isContinuousSpellCheckingEnabled)
        m_frame->document()->markers()->removeMarkers(DocumentMarker::Spelling);
    if (!isContinuousGrammarCheckingEnabled)
        m_frame->document()->markers()->removeMarkers(DocumentMarker::Grammar);

    respondToChangedSelection(oldSelection);
}

static Node* findFirstMarkable(Node* node)
{
    while (node) {
        if (!node->renderer())
            return 0;
        if (node->renderer()->isText())
            return node;
        if (node->renderer()->isTextControl())
            node = toRenderTextControl(node->renderer())->visiblePositionForIndex(1).deepEquivalent().deprecatedNode();
        else if (node->firstChild())
            node = node->firstChild();
        else
            node = node->nextSibling();
    }

    return 0;
}

bool Editor::selectionStartHasMarkerFor(DocumentMarker::MarkerType markerType, int from, int length) const
{
    Node* node = findFirstMarkable(m_frame->selection()->start().deprecatedNode());
    if (!node)
        return false;

    unsigned int startOffset = static_cast<unsigned int>(from);
    unsigned int endOffset = static_cast<unsigned int>(from + length);
    Vector<DocumentMarker> markers = m_frame->document()->markers()->markersForNode(node);
    for (size_t i = 0; i < markers.size(); ++i) {
        DocumentMarker marker = markers[i];
        if (marker.startOffset <= startOffset && endOffset <= marker.endOffset && marker.type == markerType)
            return true;
    }

    return false;
}       

TextCheckingTypeMask Editor::textCheckingTypeMaskFor(TextCheckingOptions textCheckingOptions)
{
    bool shouldMarkSpelling = textCheckingOptions & MarkSpelling;
    bool shouldMarkGrammar = textCheckingOptions & MarkGrammar;
    bool shouldShowCorrectionPanel = textCheckingOptions & ShowCorrectionPanel;
    bool shouldCheckForCorrection = shouldShowCorrectionPanel || (textCheckingOptions & CheckForCorrection);

    TextCheckingTypeMask checkingTypes = 0;
    if (shouldMarkSpelling)
        checkingTypes |= TextCheckingTypeSpelling;
    if (shouldMarkGrammar)
        checkingTypes |= TextCheckingTypeGrammar;
    if (shouldCheckForCorrection)
        checkingTypes |= TextCheckingTypeCorrection;

#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    bool shouldPerformReplacement = textCheckingOptions & PerformReplacement;
    if (shouldPerformReplacement) {
        if (isAutomaticLinkDetectionEnabled())
            checkingTypes |= TextCheckingTypeLink;
        if (isAutomaticQuoteSubstitutionEnabled())
            checkingTypes |= TextCheckingTypeQuote;
        if (isAutomaticDashSubstitutionEnabled())
            checkingTypes |= TextCheckingTypeDash;
        if (isAutomaticTextReplacementEnabled())
            checkingTypes |= TextCheckingTypeReplacement;
        if (shouldMarkSpelling && isAutomaticSpellingCorrectionEnabled())
            checkingTypes |= TextCheckingTypeCorrection;
    }
#endif

    return checkingTypes;
}

} // namespace WebCore
