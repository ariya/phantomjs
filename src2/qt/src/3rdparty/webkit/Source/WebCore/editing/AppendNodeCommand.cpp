/*
 * Copyright (C) 2005, 2006, 2008 Apple Inc. All rights reserved.
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
#include "AppendNodeCommand.h"

#include "AXObjectCache.h"
#include "htmlediting.h"

namespace WebCore {

AppendNodeCommand::AppendNodeCommand(PassRefPtr<ContainerNode> parent, PassRefPtr<Node> node)
    : SimpleEditCommand(parent->document())
    , m_parent(parent)
    , m_node(node)
{
    ASSERT(m_parent);
    ASSERT(m_node);
    ASSERT(!m_node->parentNode());

    ASSERT(m_parent->rendererIsEditable() || !m_parent->attached());
}

static void sendAXTextChangedIgnoringLineBreaks(Node* node, AXObjectCache::AXTextChange textChange)
{
    String nodeValue = node->nodeValue();
    unsigned len = nodeValue.length();
    // Don't consider linebreaks in this command
    if (nodeValue == "\n")
      return;

    node->document()->axObjectCache()->nodeTextChangeNotification(node->renderer(), textChange, 0, len);
}

void AppendNodeCommand::doApply()
{
    if (!m_parent->rendererIsEditable() && m_parent->attached())
        return;
        
    ExceptionCode ec;
    m_parent->appendChild(m_node.get(), ec);

    if (AXObjectCache::accessibilityEnabled())
        sendAXTextChangedIgnoringLineBreaks(m_node.get(), AXObjectCache::AXTextInserted);
}

void AppendNodeCommand::doUnapply()
{
    if (!m_parent->rendererIsEditable())
        return;
        
    // Need to notify this before actually deleting the text
    if (AXObjectCache::accessibilityEnabled())
        sendAXTextChangedIgnoringLineBreaks(m_node.get(), AXObjectCache::AXTextDeleted);

    ExceptionCode ec;
    m_node->remove(ec);
}

} // namespace WebCore
