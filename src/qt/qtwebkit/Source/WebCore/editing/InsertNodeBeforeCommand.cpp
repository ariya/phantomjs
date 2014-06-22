/*
 * Copyright (C) 2005, 2008 Apple Inc. All rights reserved.
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
#include "InsertNodeBeforeCommand.h"

#include "AXObjectCache.h"
#include "Document.h"
#include "ExceptionCodePlaceholder.h"
#include "htmlediting.h"

namespace WebCore {

InsertNodeBeforeCommand::InsertNodeBeforeCommand(PassRefPtr<Node> insertChild, PassRefPtr<Node> refChild,
    ShouldAssumeContentIsAlwaysEditable shouldAssumeContentIsAlwaysEditable)
    : SimpleEditCommand(refChild->document())
    , m_insertChild(insertChild)
    , m_refChild(refChild)
    , m_shouldAssumeContentIsAlwaysEditable(shouldAssumeContentIsAlwaysEditable)
{
    ASSERT(m_insertChild);
    ASSERT(!m_insertChild->parentNode());
    ASSERT(m_refChild);
    ASSERT(m_refChild->parentNode());

    ASSERT(m_refChild->parentNode()->rendererIsEditable() || !m_refChild->parentNode()->attached());
}

void InsertNodeBeforeCommand::doApply()
{
    ContainerNode* parent = m_refChild->parentNode();
    if (!parent || (m_shouldAssumeContentIsAlwaysEditable == DoNotAssumeContentIsAlwaysEditable && !parent->isContentEditable(Node::UserSelectAllIsAlwaysNonEditable)))
        return;
    ASSERT(parent->isContentEditable(Node::UserSelectAllIsAlwaysNonEditable));

    parent->insertBefore(m_insertChild.get(), m_refChild.get(), IGNORE_EXCEPTION, AttachLazily);

    if (AXObjectCache* cache = document()->existingAXObjectCache())
        cache->nodeTextChangeNotification(m_insertChild.get(), AXObjectCache::AXTextInserted, 0, m_insertChild->nodeValue());
}

void InsertNodeBeforeCommand::doUnapply()
{
    if (!m_insertChild->isContentEditable(Node::UserSelectAllIsAlwaysNonEditable))
        return;

    // Need to notify this before actually deleting the text
    if (AXObjectCache* cache = document()->existingAXObjectCache())
        cache->nodeTextChangeNotification(m_insertChild.get(), AXObjectCache::AXTextDeleted, 0, m_insertChild->nodeValue());

    m_insertChild->remove(IGNORE_EXCEPTION);
}

#ifndef NDEBUG
void InsertNodeBeforeCommand::getNodesInCommand(HashSet<Node*>& nodes)
{
    addNodeAndDescendants(m_insertChild.get(), nodes);
    addNodeAndDescendants(m_refChild.get(), nodes);
}
#endif

}
