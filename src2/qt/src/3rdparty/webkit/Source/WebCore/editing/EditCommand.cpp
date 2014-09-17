/*
 * Copyright (C) 2005, 2006, 2007 Apple, Inc.  All rights reserved.
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
#include "EditCommand.h"

#include "CompositeEditCommand.h"
#include "DeleteButtonController.h"
#include "Document.h"
#include "Editor.h"
#include "Element.h"
#include "EventNames.h"
#include "Frame.h"
#include "ScopedEventQueue.h"
#include "SelectionController.h"
#include "VisiblePosition.h"
#include "htmlediting.h"

namespace WebCore {

EditCommand::EditCommand(Document* document) 
    : m_document(document)
    , m_parent(0)
{
    ASSERT(m_document);
    ASSERT(m_document->frame());
    setStartingSelection(avoidIntersectionWithNode(m_document->frame()->selection()->selection(), m_document->frame()->editor()->deleteButtonController()->containerElement()));
    setEndingSelection(m_startingSelection);
}

EditCommand::~EditCommand()
{
}

void EditCommand::apply()
{
    ASSERT(m_document);
    ASSERT(m_document->frame());
 
    Frame* frame = m_document->frame();
    
    if (isTopLevelCommand()) {
        if (!endingSelection().isContentRichlyEditable()) {
            switch (editingAction()) {
                case EditActionTyping:
                case EditActionPaste:
                case EditActionDrag:
                case EditActionSetWritingDirection:
                case EditActionCut:
                case EditActionUnspecified:
                    break;
                default:
                    ASSERT_NOT_REACHED();
                    return;
            }
        }
    }
    
    // Changes to the document may have been made since the last editing operation that 
    // require a layout, as in <rdar://problem/5658603>.  Low level operations, like 
    // RemoveNodeCommand, don't require a layout because the high level operations that 
    // use them perform one if one is necessary (like for the creation of VisiblePositions).
    if (isTopLevelCommand())
        updateLayout();

    {
        EventQueueScope scope;
        DeleteButtonController* deleteButtonController = frame->editor()->deleteButtonController();
        deleteButtonController->disable();
        doApply();
        deleteButtonController->enable();
    }

    if (isTopLevelCommand()) {
        // Only need to call appliedEditing for top-level commands, and TypingCommands do it on their
        // own (see TypingCommand::typingAddedToOpenCommand).
        if (!isTypingCommand())
            frame->editor()->appliedEditing(this);
    }

    setShouldRetainAutocorrectionIndicator(false);
}

void EditCommand::unapply()
{
    ASSERT(m_document);
    ASSERT(m_document->frame());
 
    Frame* frame = m_document->frame();
    
    // Changes to the document may have been made since the last editing operation that 
    // require a layout, as in <rdar://problem/5658603>.  Low level operations, like 
    // RemoveNodeCommand, don't require a layout because the high level operations that 
    // use them perform one if one is necessary (like for the creation of VisiblePositions).
    if (isTopLevelCommand())
        updateLayout();
    
    DeleteButtonController* deleteButtonController = frame->editor()->deleteButtonController();
    deleteButtonController->disable();
    doUnapply();
    deleteButtonController->enable();

    if (isTopLevelCommand())
        frame->editor()->unappliedEditing(this);
}

void EditCommand::reapply()
{
    ASSERT(m_document);
    ASSERT(m_document->frame());
 
    Frame* frame = m_document->frame();
    
    // Changes to the document may have been made since the last editing operation that 
    // require a layout, as in <rdar://problem/5658603>.  Low level operations, like 
    // RemoveNodeCommand, don't require a layout because the high level operations that 
    // use them perform one if one is necessary (like for the creation of VisiblePositions).
    if (isTopLevelCommand())
        updateLayout();

    DeleteButtonController* deleteButtonController = frame->editor()->deleteButtonController();
    deleteButtonController->disable();
    doReapply();
    deleteButtonController->enable();

    if (isTopLevelCommand())
        frame->editor()->reappliedEditing(this);
}

void EditCommand::doReapply()
{
    doApply();
}

EditAction EditCommand::editingAction() const
{
    return EditActionUnspecified;
}

void EditCommand::setStartingSelection(const VisibleSelection& s)
{
    Element* root = s.rootEditableElement();
    for (EditCommand* cmd = this; ; cmd = cmd->m_parent) {
        cmd->m_startingSelection = s;
        cmd->m_startingRootEditableElement = root;
        if (!cmd->m_parent || cmd->m_parent->isFirstCommand(cmd))
            break;
    }
}

void EditCommand::setEndingSelection(const VisibleSelection &s)
{
    Element* root = s.rootEditableElement();
    for (EditCommand* cmd = this; cmd; cmd = cmd->m_parent) {
        cmd->m_endingSelection = s;
        cmd->m_endingRootEditableElement = root;
    }
}

bool EditCommand::preservesTypingStyle() const
{
    return false;
}

bool EditCommand::isInsertTextCommand() const
{
    return false;
}

bool EditCommand::isTypingCommand() const
{
    return false;
}

bool EditCommand::isCreateLinkCommand() const
{
    return false;
}

bool EditCommand::shouldRetainAutocorrectionIndicator() const
{
    return false;
}

void EditCommand::setShouldRetainAutocorrectionIndicator(bool)
{
}

void EditCommand::updateLayout() const
{
    document()->updateLayoutIgnorePendingStylesheets();
}

void EditCommand::setParent(CompositeEditCommand* parent)
{
    ASSERT(parent);
    ASSERT(!m_parent);
    m_parent = parent;
    m_startingSelection = parent->m_endingSelection;
    m_endingSelection = parent->m_endingSelection;
    m_startingRootEditableElement = parent->m_endingRootEditableElement;
    m_endingRootEditableElement = parent->m_endingRootEditableElement;
}

void applyCommand(PassRefPtr<EditCommand> command)
{
    command->apply();
}

} // namespace WebCore
