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
#include "MergeIdenticalElementsCommand.h"

#include "Element.h"

namespace WebCore {

MergeIdenticalElementsCommand::MergeIdenticalElementsCommand(PassRefPtr<Element> first, PassRefPtr<Element> second)
    : SimpleEditCommand(first->document())
    , m_element1(first)
    , m_element2(second)
{
    ASSERT(m_element1);
    ASSERT(m_element2);
    ASSERT(m_element1->nextSibling() == m_element2);
}

void MergeIdenticalElementsCommand::doApply()
{
    if (m_element1->nextSibling() != m_element2 || !m_element1->rendererIsEditable() || !m_element2->rendererIsEditable())
        return;

    m_atChild = m_element2->firstChild();

    ExceptionCode ec = 0;

    Vector<RefPtr<Node> > children;
    for (Node* child = m_element1->firstChild(); child; child = child->nextSibling())
        children.append(child);

    size_t size = children.size();
    for (size_t i = 0; i < size; ++i)
        m_element2->insertBefore(children[i].release(), m_atChild.get(), ec);

    m_element1->remove(ec);
}

void MergeIdenticalElementsCommand::doUnapply()
{
    ASSERT(m_element1);
    ASSERT(m_element2);

    RefPtr<Node> atChild = m_atChild.release();

    ContainerNode* parent = m_element2->parentNode();
    if (!parent || !parent->rendererIsEditable())
        return;

    ExceptionCode ec = 0;

    parent->insertBefore(m_element1.get(), m_element2.get(), ec);
    if (ec)
        return;

    Vector<RefPtr<Node> > children;
    for (Node* child = m_element2->firstChild(); child && child != atChild; child = child->nextSibling())
        children.append(child);

    size_t size = children.size();
    for (size_t i = 0; i < size; ++i)
        m_element1->appendChild(children[i].release(), ec);
}

} // namespace WebCore
