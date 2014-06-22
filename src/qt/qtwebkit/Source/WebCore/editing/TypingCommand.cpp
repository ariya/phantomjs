/*
 * Copyright (C) 2005, 2006, 2007, 2008 Apple Inc.  All rights reserved.
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
#include "TypingCommand.h"

#include "BreakBlockquoteCommand.h"
#include "DeleteSelectionCommand.h"
#include "Document.h"
#include "Editor.h"
#include "Element.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "HTMLNames.h"
#include "InsertLineBreakCommand.h"
#include "InsertParagraphSeparatorCommand.h"
#include "InsertTextCommand.h"
#include "RenderObject.h"
#include "TextIterator.h"
#include "VisiblePosition.h"
#include "VisibleUnits.h"
#include "htmlediting.h"

namespace WebCore {

using namespace HTMLNames;

class TypingCommandLineOperation
{
public:
    TypingCommandLineOperation(TypingCommand* typingCommand, bool selectInsertedText, const String& text)
    : m_typingCommand(typingCommand)
    , m_selectInsertedText(selectInsertedText)
    , m_text(text)
    { }
    
    void operator()(size_t lineOffset, size_t lineLength, bool isLastLine) const
    {
        if (isLastLine) {
            if (!lineOffset || lineLength > 0)
                m_typingCommand->insertTextRunWithoutNewlines(m_text.substring(lineOffset, lineLength), m_selectInsertedText);
        } else {
            if (lineLength > 0)
                m_typingCommand->insertTextRunWithoutNewlines(m_text.substring(lineOffset, lineLength), false);
            m_typingCommand->insertParagraphSeparator();
        }
    }
    
private:
    TypingCommand* m_typingCommand;
    bool m_selectInsertedText;
    const String& m_text;
};

TypingCommand::TypingCommand(Document *document, ETypingCommand commandType, const String &textToInsert, Options options, TextGranularity granularity, TextCompositionType compositionType)
    : TextInsertionBaseCommand(document)
    , m_commandType(commandType)
    , m_textToInsert(textToInsert)
    , m_openForMoreTyping(true)
    , m_selectInsertedText(options & SelectInsertedText)
    , m_smartDelete(options & SmartDelete)
    , m_granularity(granularity)
    , m_compositionType(compositionType)
    , m_killRing(options & KillRing)
    , m_openedByBackwardDelete(false)
    , m_shouldRetainAutocorrectionIndicator(options & RetainAutocorrectionIndicator)
    , m_shouldPreventSpellChecking(options & PreventSpellChecking)
{
    updatePreservesTypingStyle(m_commandType);
}

void TypingCommand::deleteSelection(Document* document, Options options)
{
    ASSERT(document);
    
    Frame* frame = document->frame();
    ASSERT(frame);

    if (!frame->selection()->isRange())
        return;

    if (RefPtr<TypingCommand> lastTypingCommand = lastTypingCommandIfStillOpenForTyping(frame)) {
        lastTypingCommand->setShouldPreventSpellChecking(options & PreventSpellChecking);
        lastTypingCommand->deleteSelection(options & SmartDelete);
        return;
    }

    TypingCommand::create(document, DeleteSelection, "", options)->apply();
}

void TypingCommand::deleteKeyPressed(Document *document, Options options, TextGranularity granularity)
{
    ASSERT(document);
    if (granularity == CharacterGranularity) {
        if (RefPtr<TypingCommand> lastTypingCommand = lastTypingCommandIfStillOpenForTyping(document->frame())) {
            updateSelectionIfDifferentFromCurrentSelection(lastTypingCommand.get(), document->frame());
            lastTypingCommand->setShouldPreventSpellChecking(options & PreventSpellChecking);
            lastTypingCommand->deleteKeyPressed(granularity, options & KillRing);
            return;
        }
    }

    TypingCommand::create(document, DeleteKey, "", options, granularity)->apply();
}

void TypingCommand::forwardDeleteKeyPressed(Document *document, Options options, TextGranularity granularity)
{
    // FIXME: Forward delete in TextEdit appears to open and close a new typing command.
    ASSERT(document);
    Frame* frame = document->frame();
    if (granularity == CharacterGranularity) {
        if (RefPtr<TypingCommand> lastTypingCommand = lastTypingCommandIfStillOpenForTyping(frame)) {
            updateSelectionIfDifferentFromCurrentSelection(lastTypingCommand.get(), frame);
            lastTypingCommand->setShouldPreventSpellChecking(options & PreventSpellChecking);
            lastTypingCommand->forwardDeleteKeyPressed(granularity, options & KillRing);
            return;
        }
    }

    TypingCommand::create(document, ForwardDeleteKey, "", options, granularity)->apply();
}

void TypingCommand::updateSelectionIfDifferentFromCurrentSelection(TypingCommand* typingCommand, Frame* frame)
{
    ASSERT(frame);
    VisibleSelection currentSelection = frame->selection()->selection();
    if (currentSelection == typingCommand->endingSelection())
        return;

    typingCommand->setStartingSelection(currentSelection);
    typingCommand->setEndingSelection(currentSelection);
}

void TypingCommand::insertText(Document* document, const String& text, Options options, TextCompositionType composition)
{
    ASSERT(document);

    Frame* frame = document->frame();
    ASSERT(frame);

    if (!text.isEmpty())
        document->frame()->editor().updateMarkersForWordsAffectedByEditing(isSpaceOrNewline(text.characters()[0]));
    
    insertText(document, text, frame->selection()->selection(), options, composition);
}

// FIXME: We shouldn't need to take selectionForInsertion. It should be identical to FrameSelection's current selection.
void TypingCommand::insertText(Document* document, const String& text, const VisibleSelection& selectionForInsertion, Options options, TextCompositionType compositionType)
{
    ASSERT(document);

    RefPtr<Frame> frame = document->frame();
    ASSERT(frame);

    VisibleSelection currentSelection = frame->selection()->selection();

    String newText = dispatchBeforeTextInsertedEvent(text, selectionForInsertion, compositionType == TextCompositionUpdate);
    
    // Set the starting and ending selection appropriately if we are using a selection
    // that is different from the current selection.  In the future, we should change EditCommand
    // to deal with custom selections in a general way that can be used by all of the commands.
    if (RefPtr<TypingCommand> lastTypingCommand = lastTypingCommandIfStillOpenForTyping(frame.get())) {
        if (lastTypingCommand->endingSelection() != selectionForInsertion) {
            lastTypingCommand->setStartingSelection(selectionForInsertion);
            lastTypingCommand->setEndingSelection(selectionForInsertion);
        }

        lastTypingCommand->setCompositionType(compositionType);
        lastTypingCommand->setShouldRetainAutocorrectionIndicator(options & RetainAutocorrectionIndicator);
        lastTypingCommand->setShouldPreventSpellChecking(options & PreventSpellChecking);
        lastTypingCommand->insertText(newText, options & SelectInsertedText);
        return;
    }

    RefPtr<TypingCommand> cmd = TypingCommand::create(document, InsertText, newText, options, compositionType);
    applyTextInsertionCommand(frame.get(), cmd, selectionForInsertion, currentSelection);
}

void TypingCommand::insertLineBreak(Document *document, Options options)
{
    ASSERT(document);
    if (RefPtr<TypingCommand> lastTypingCommand = lastTypingCommandIfStillOpenForTyping(document->frame())) {
        lastTypingCommand->setShouldRetainAutocorrectionIndicator(options & RetainAutocorrectionIndicator);
        lastTypingCommand->insertLineBreak();
        return;
    }

    applyCommand(TypingCommand::create(document, InsertLineBreak, "", options));
}

void TypingCommand::insertParagraphSeparatorInQuotedContent(Document *document)
{
    ASSERT(document);
    if (RefPtr<TypingCommand> lastTypingCommand = lastTypingCommandIfStillOpenForTyping(document->frame())) {
        lastTypingCommand->insertParagraphSeparatorInQuotedContent();
        return;
    }

    applyCommand(TypingCommand::create(document, InsertParagraphSeparatorInQuotedContent));
}

void TypingCommand::insertParagraphSeparator(Document *document, Options options)
{
    ASSERT(document);
    if (RefPtr<TypingCommand> lastTypingCommand = lastTypingCommandIfStillOpenForTyping(document->frame())) {
        lastTypingCommand->setShouldRetainAutocorrectionIndicator(options & RetainAutocorrectionIndicator);
        lastTypingCommand->insertParagraphSeparator();
        return;
    }

    applyCommand(TypingCommand::create(document, InsertParagraphSeparator, "", options));
}

PassRefPtr<TypingCommand> TypingCommand::lastTypingCommandIfStillOpenForTyping(Frame* frame)
{
    ASSERT(frame);

    RefPtr<CompositeEditCommand> lastEditCommand = frame->editor().lastEditCommand();
    if (!lastEditCommand || !lastEditCommand->isTypingCommand() || !static_cast<TypingCommand*>(lastEditCommand.get())->isOpenForMoreTyping())
        return 0;

    return static_cast<TypingCommand*>(lastEditCommand.get());
}

void TypingCommand::closeTyping(Frame* frame)
{
    if (RefPtr<TypingCommand> lastTypingCommand = lastTypingCommandIfStillOpenForTyping(frame))
        lastTypingCommand->closeTyping();
}

void TypingCommand::doApply()
{
    if (!endingSelection().isNonOrphanedCaretOrRange())
        return;
        
    if (m_commandType == DeleteKey)
        if (m_commands.isEmpty())
            m_openedByBackwardDelete = true;

    switch (m_commandType) {
    case DeleteSelection:
        deleteSelection(m_smartDelete);
        return;
    case DeleteKey:
        deleteKeyPressed(m_granularity, m_killRing);
        return;
    case ForwardDeleteKey:
        forwardDeleteKeyPressed(m_granularity, m_killRing);
        return;
    case InsertLineBreak:
        insertLineBreak();
        return;
    case InsertParagraphSeparator:
        insertParagraphSeparator();
        return;
    case InsertParagraphSeparatorInQuotedContent:
        insertParagraphSeparatorInQuotedContent();
        return;
    case InsertText:
        insertText(m_textToInsert, m_selectInsertedText);
        return;
    }

    ASSERT_NOT_REACHED();
}

EditAction TypingCommand::editingAction() const
{
    return EditActionTyping;
}

void TypingCommand::markMisspellingsAfterTyping(ETypingCommand commandType)
{
    Frame* frame = document()->frame();
    if (!frame)
        return;

#if PLATFORM(MAC)
    if (!frame->editor().isContinuousSpellCheckingEnabled()
        && !frame->editor().isAutomaticQuoteSubstitutionEnabled()
        && !frame->editor().isAutomaticLinkDetectionEnabled()
        && !frame->editor().isAutomaticDashSubstitutionEnabled()
        && !frame->editor().isAutomaticTextReplacementEnabled())
            return;
#else
    if (!frame->editor().isContinuousSpellCheckingEnabled())
        return;
#endif
    // Take a look at the selection that results after typing and determine whether we need to spellcheck. 
    // Since the word containing the current selection is never marked, this does a check to
    // see if typing made a new word that is not in the current selection. Basically, you
    // get this by being at the end of a word and typing a space.    
    VisiblePosition start(endingSelection().start(), endingSelection().affinity());
    VisiblePosition previous = start.previous();
    if (previous.isNotNull()) {
        VisiblePosition p1 = startOfWord(previous, LeftWordIfOnBoundary);
        VisiblePosition p2 = startOfWord(start, LeftWordIfOnBoundary);
        if (p1 != p2) {
            RefPtr<Range> range = makeRange(p1, p2);
            String strippedPreviousWord;
            if (range && (commandType == TypingCommand::InsertText || commandType == TypingCommand::InsertLineBreak || commandType == TypingCommand::InsertParagraphSeparator || commandType == TypingCommand::InsertParagraphSeparatorInQuotedContent))
                strippedPreviousWord = plainText(range.get()).stripWhiteSpace();
            frame->editor().markMisspellingsAfterTypingToWord(p1, endingSelection(), !strippedPreviousWord.isEmpty());
        } else if (commandType == TypingCommand::InsertText)
            frame->editor().startAlternativeTextUITimer();
    }
}

void TypingCommand::typingAddedToOpenCommand(ETypingCommand commandTypeForAddedTyping)
{
    Frame* frame = document()->frame();
    if (!frame)
        return;

    updatePreservesTypingStyle(commandTypeForAddedTyping);

#if PLATFORM(MAC)
    frame->editor().appliedEditing(this);
    // Since the spellchecking code may also perform corrections and other replacements, it should happen after the typing changes.
    if (!m_shouldPreventSpellChecking)
        markMisspellingsAfterTyping(commandTypeForAddedTyping);
#else
    // The old spellchecking code requires that checking be done first, to prevent issues like that in 6864072, where <doesn't> is marked as misspelled.
    markMisspellingsAfterTyping(commandTypeForAddedTyping);
    frame->editor().appliedEditing(this);
#endif
}

void TypingCommand::insertText(const String &text, bool selectInsertedText)
{
    // FIXME: Need to implement selectInsertedText for cases where more than one insert is involved.
    // This requires support from insertTextRunWithoutNewlines and insertParagraphSeparator for extending
    // an existing selection; at the moment they can either put the caret after what's inserted or
    // select what's inserted, but there's no way to "extend selection" to include both an old selection
    // that ends just before where we want to insert text and the newly inserted text.
    TypingCommandLineOperation operation(this, selectInsertedText, text);
    forEachLineInString(text, operation);
}

void TypingCommand::insertTextRunWithoutNewlines(const String &text, bool selectInsertedText)
{
    RefPtr<InsertTextCommand> command = InsertTextCommand::create(document(), text, selectInsertedText,
        m_compositionType == TextCompositionNone ? InsertTextCommand::RebalanceLeadingAndTrailingWhitespaces : InsertTextCommand::RebalanceAllWhitespaces);

    applyCommandToComposite(command, endingSelection());

    typingAddedToOpenCommand(InsertText);
}

void TypingCommand::insertLineBreak()
{
    if (!canAppendNewLineFeedToSelection(endingSelection()))
        return;

    applyCommandToComposite(InsertLineBreakCommand::create(document()));
    typingAddedToOpenCommand(InsertLineBreak);
}

void TypingCommand::insertParagraphSeparator()
{
    if (!canAppendNewLineFeedToSelection(endingSelection()))
        return;

    applyCommandToComposite(InsertParagraphSeparatorCommand::create(document()));
    typingAddedToOpenCommand(InsertParagraphSeparator);
}

void TypingCommand::insertParagraphSeparatorInQuotedContent()
{
    // If the selection starts inside a table, just insert the paragraph separator normally
    // Breaking the blockquote would also break apart the table, which is unecessary when inserting a newline
    if (enclosingNodeOfType(endingSelection().start(), &isTableStructureNode)) {
        insertParagraphSeparator();
        return;
    }
        
    applyCommandToComposite(BreakBlockquoteCommand::create(document()));
    typingAddedToOpenCommand(InsertParagraphSeparatorInQuotedContent);
}

bool TypingCommand::makeEditableRootEmpty()
{
    Element* root = endingSelection().rootEditableElement();
    if (!root || !root->firstChild())
        return false;

    if (root->firstChild() == root->lastChild() && root->firstElementChild() && root->firstElementChild()->hasTagName(brTag)) {
        // If there is a single child and it could be a placeholder, leave it alone.
        if (root->renderer() && root->renderer()->isBlockFlow())
            return false;
    }

    while (Node* child = root->firstChild())
        removeNode(child);

    addBlockPlaceholderIfNeeded(root);
    setEndingSelection(VisibleSelection(firstPositionInNode(root), DOWNSTREAM, endingSelection().isDirectional()));

    return true;
}

void TypingCommand::deleteKeyPressed(TextGranularity granularity, bool killRing)
{
    Frame* frame = document()->frame();
    if (!frame)
        return;

    frame->editor().updateMarkersForWordsAffectedByEditing(false);

    VisibleSelection selectionToDelete;
    VisibleSelection selectionAfterUndo;

    switch (endingSelection().selectionType()) {
    case VisibleSelection::RangeSelection:
        selectionToDelete = endingSelection();
        selectionAfterUndo = selectionToDelete;
        break;
    case VisibleSelection::CaretSelection: {
        // After breaking out of an empty mail blockquote, we still want continue with the deletion
        // so actual content will get deleted, and not just the quote style.
        if (breakOutOfEmptyMailBlockquotedParagraph())
            typingAddedToOpenCommand(DeleteKey);

        m_smartDelete = false;

        FrameSelection selection;
        selection.setSelection(endingSelection());
        selection.modify(FrameSelection::AlterationExtend, DirectionBackward, granularity);
        if (killRing && selection.isCaret() && granularity != CharacterGranularity)
            selection.modify(FrameSelection::AlterationExtend, DirectionBackward, CharacterGranularity);

        if (endingSelection().visibleStart().previous(CannotCrossEditingBoundary).isNull()) {
            // When the caret is at the start of the editable area in an empty list item, break out of the list item.
            if (breakOutOfEmptyListItem()) {
                typingAddedToOpenCommand(DeleteKey);
                return;
            }
            // When there are no visible positions in the editing root, delete its entire contents.
            if (endingSelection().visibleStart().next(CannotCrossEditingBoundary).isNull() && makeEditableRootEmpty()) {
                typingAddedToOpenCommand(DeleteKey);
                return;
            }
        }

        VisiblePosition visibleStart(endingSelection().visibleStart());
        // If we have a caret selection at the beginning of a cell, we have nothing to do.
        Node* enclosingTableCell = enclosingNodeOfType(visibleStart.deepEquivalent(), &isTableCell);
        if (enclosingTableCell && visibleStart == firstPositionInNode(enclosingTableCell))
            return;

        // If the caret is at the start of a paragraph after a table, move content into the last table cell.
        if (isStartOfParagraph(visibleStart) && isFirstPositionAfterTable(visibleStart.previous(CannotCrossEditingBoundary))) {
            // Unless the caret is just before a table.  We don't want to move a table into the last table cell.
            if (isLastPositionBeforeTable(visibleStart))
                return;
            // Extend the selection backward into the last cell, then deletion will handle the move.
            selection.modify(FrameSelection::AlterationExtend, DirectionBackward, granularity);
        // If the caret is just after a table, select the table and don't delete anything.
        } else if (Node* table = isFirstPositionAfterTable(visibleStart)) {
            setEndingSelection(VisibleSelection(positionBeforeNode(table), endingSelection().start(), DOWNSTREAM, endingSelection().isDirectional()));
            typingAddedToOpenCommand(DeleteKey);
            return;
        }

        selectionToDelete = selection.selection();

        if (granularity == CharacterGranularity && selectionToDelete.end().containerNode() == selectionToDelete.start().containerNode()
            && selectionToDelete.end().computeOffsetInContainerNode() - selectionToDelete.start().computeOffsetInContainerNode() > 1) {
            // If there are multiple Unicode code points to be deleted, adjust the range to match platform conventions.
            selectionToDelete.setWithoutValidation(selectionToDelete.end(), selectionToDelete.end().previous(BackwardDeletion));
        }

        if (!startingSelection().isRange() || selectionToDelete.base() != startingSelection().start())
            selectionAfterUndo = selectionToDelete;
        else
            // It's a little tricky to compute what the starting selection would have been in the original document.
            // We can't let the VisibleSelection class's validation kick in or it'll adjust for us based on
            // the current state of the document and we'll get the wrong result.
            selectionAfterUndo.setWithoutValidation(startingSelection().end(), selectionToDelete.extent());
        break;
    }
    case VisibleSelection::NoSelection:
        ASSERT_NOT_REACHED();
        break;
    }
    
    ASSERT(!selectionToDelete.isNone());
    if (selectionToDelete.isNone())
        return;
    
    if (selectionToDelete.isCaret() || !frame->selection()->shouldDeleteSelection(selectionToDelete))
        return;
    
    if (killRing)
        frame->editor().addToKillRing(selectionToDelete.toNormalizedRange().get(), false);
    // Make undo select everything that has been deleted, unless an undo will undo more than just this deletion.
    // FIXME: This behaves like TextEdit except for the case where you open with text insertion and then delete
    // more text than you insert.  In that case all of the text that was around originally should be selected.
    if (m_openedByBackwardDelete)
        setStartingSelection(selectionAfterUndo);
    CompositeEditCommand::deleteSelection(selectionToDelete, m_smartDelete);
    setSmartDelete(false);
    typingAddedToOpenCommand(DeleteKey);
}

void TypingCommand::forwardDeleteKeyPressed(TextGranularity granularity, bool killRing)
{
    Frame* frame = document()->frame();
    if (!frame)
        return;

    frame->editor().updateMarkersForWordsAffectedByEditing(false);

    VisibleSelection selectionToDelete;
    VisibleSelection selectionAfterUndo;

    switch (endingSelection().selectionType()) {
    case VisibleSelection::RangeSelection:
        selectionToDelete = endingSelection();
        selectionAfterUndo = selectionToDelete;
        break;
    case VisibleSelection::CaretSelection: {
        m_smartDelete = false;

        // Handle delete at beginning-of-block case.
        // Do nothing in the case that the caret is at the start of a
        // root editable element or at the start of a document.
        FrameSelection selection;
        selection.setSelection(endingSelection());
        selection.modify(FrameSelection::AlterationExtend, DirectionForward, granularity);
        if (killRing && selection.isCaret() && granularity != CharacterGranularity)
            selection.modify(FrameSelection::AlterationExtend, DirectionForward, CharacterGranularity);

        Position downstreamEnd = endingSelection().end().downstream();
        VisiblePosition visibleEnd = endingSelection().visibleEnd();
        Node* enclosingTableCell = enclosingNodeOfType(visibleEnd.deepEquivalent(), &isTableCell);
        if (enclosingTableCell && visibleEnd == lastPositionInNode(enclosingTableCell))
            return;
        if (visibleEnd == endOfParagraph(visibleEnd))
            downstreamEnd = visibleEnd.next(CannotCrossEditingBoundary).deepEquivalent().downstream();
        // When deleting tables: Select the table first, then perform the deletion
        if (downstreamEnd.containerNode() && downstreamEnd.containerNode()->renderer() && downstreamEnd.containerNode()->renderer()->isTable()
            && downstreamEnd.computeOffsetInContainerNode() <= caretMinOffset(downstreamEnd.containerNode())) {
            setEndingSelection(VisibleSelection(endingSelection().end(), positionAfterNode(downstreamEnd.containerNode()), DOWNSTREAM, endingSelection().isDirectional()));
            typingAddedToOpenCommand(ForwardDeleteKey);
            return;
        }

        // deleting to end of paragraph when at end of paragraph needs to merge the next paragraph (if any)
        if (granularity == ParagraphBoundary && selection.selection().isCaret() && isEndOfParagraph(selection.selection().visibleEnd()))
            selection.modify(FrameSelection::AlterationExtend, DirectionForward, CharacterGranularity);

        selectionToDelete = selection.selection();
        if (!startingSelection().isRange() || selectionToDelete.base() != startingSelection().start())
            selectionAfterUndo = selectionToDelete;
        else {
            // It's a little tricky to compute what the starting selection would have been in the original document.
            // We can't let the VisibleSelection class's validation kick in or it'll adjust for us based on
            // the current state of the document and we'll get the wrong result.
            Position extent = startingSelection().end();
            if (extent.containerNode() != selectionToDelete.end().containerNode())
                extent = selectionToDelete.extent();
            else {
                int extraCharacters;
                if (selectionToDelete.start().containerNode() == selectionToDelete.end().containerNode())
                    extraCharacters = selectionToDelete.end().computeOffsetInContainerNode() - selectionToDelete.start().computeOffsetInContainerNode();
                else
                    extraCharacters = selectionToDelete.end().computeOffsetInContainerNode();
                extent = Position(extent.containerNode(), extent.computeOffsetInContainerNode() + extraCharacters, Position::PositionIsOffsetInAnchor);
            }
            selectionAfterUndo.setWithoutValidation(startingSelection().start(), extent);
        }
        break;
    }
    case VisibleSelection::NoSelection:
        ASSERT_NOT_REACHED();
        break;
    }
    
    ASSERT(!selectionToDelete.isNone());
    if (selectionToDelete.isNone())
        return;
    
    if (selectionToDelete.isCaret() || !frame->selection()->shouldDeleteSelection(selectionToDelete))
        return;
        
    if (killRing)
        frame->editor().addToKillRing(selectionToDelete.toNormalizedRange().get(), false);
    // make undo select what was deleted
    setStartingSelection(selectionAfterUndo);
    CompositeEditCommand::deleteSelection(selectionToDelete, m_smartDelete);
    setSmartDelete(false);
    typingAddedToOpenCommand(ForwardDeleteKey);
}

void TypingCommand::deleteSelection(bool smartDelete)
{
    CompositeEditCommand::deleteSelection(smartDelete);
    typingAddedToOpenCommand(DeleteSelection);
}

void TypingCommand::updatePreservesTypingStyle(ETypingCommand commandType)
{
    switch (commandType) {
    case DeleteSelection:
    case DeleteKey:
    case ForwardDeleteKey:
    case InsertParagraphSeparator:
    case InsertLineBreak:
        m_preservesTypingStyle = true;
        return;
    case InsertParagraphSeparatorInQuotedContent:
    case InsertText:
        m_preservesTypingStyle = false;
        return;
    }
    ASSERT_NOT_REACHED();
    m_preservesTypingStyle = false;
}

bool TypingCommand::isTypingCommand() const
{
    return true;
}

} // namespace WebCore
