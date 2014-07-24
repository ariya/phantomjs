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
#include "Document.h"
#include "Editor.h"
#include "Element.h"
#include "EventNames.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "NodeTraversal.h"
#include "VisiblePosition.h"
#include "htmlediting.h"

namespace WebCore {

EditCommand::EditCommand(Document* document)
    : m_document(document)
    , m_parent(0)
{
    ASSERT(m_document);
    ASSERT(m_document->frame());
    setStartingSelection(m_document->frame()->editor().avoidIntersectionWithDeleteButtonController(m_document->frame()->selection()->selection()));
    setEndingSelection(m_startingSelection);
}

EditCommand::EditCommand(Document* document, const VisibleSelection& startingSelection, const VisibleSelection& endingSelection)
    : m_document(document)
    , m_parent(0)
{
    ASSERT(m_document);
    ASSERT(m_document->frame());
    setStartingSelection(startingSelection);
    setEndingSelection(endingSelection);
}

EditCommand::~EditCommand()
{
}

EditAction EditCommand::editingAction() const
{
    return EditActionUnspecified;
}

static inline EditCommandComposition* compositionIfPossible(EditCommand* command)
{
    if (!command->isCompositeEditCommand())
        return 0;
    return toCompositeEditCommand(command)->composition();
}

void EditCommand::setStartingSelection(const VisibleSelection& s)
{
    for (EditCommand* cmd = this; ; cmd = cmd->m_parent) {
        if (EditCommandComposition* composition = compositionIfPossible(cmd)) {
            ASSERT(cmd->isTopLevelCommand());
            composition->setStartingSelection(s);
        }
        cmd->m_startingSelection = s;
        if (!cmd->m_parent || cmd->m_parent->isFirstCommand(cmd))
            break;
    }
}

void EditCommand::setEndingSelection(const VisibleSelection &s)
{
    for (EditCommand* cmd = this; cmd; cmd = cmd->m_parent) {
        if (EditCommandComposition* composition = compositionIfPossible(cmd)) {
            ASSERT(cmd->isTopLevelCommand());
            composition->setEndingSelection(s);
        }
        cmd->m_endingSelection = s;
    }
}

void EditCommand::setParent(CompositeEditCommand* parent)
{
    ASSERT((parent && !m_parent) || (!parent && m_parent));
    ASSERT(!parent || !isCompositeEditCommand() || !toCompositeEditCommand(this)->composition());
    m_parent = parent;
    if (parent) {
        m_startingSelection = parent->m_endingSelection;
        m_endingSelection = parent->m_endingSelection;
    }
}

void SimpleEditCommand::doReapply()
{
    doApply();
}

#ifndef NDEBUG
void SimpleEditCommand::addNodeAndDescendants(Node* startNode, HashSet<Node*>& nodes)
{
    for (Node* node = startNode; node; node = NodeTraversal::next(node, startNode))
        nodes.add(node);
}
#endif

} // namespace WebCore
