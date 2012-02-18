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
#include "SplitElementCommand.h"

#include "Element.h"
#include "HTMLNames.h"
#include <wtf/Assertions.h>

namespace WebCore {

SplitElementCommand::SplitElementCommand(PassRefPtr<Element> element, PassRefPtr<Node> atChild)
    : SimpleEditCommand(element->document())
    , m_element2(element)
    , m_atChild(atChild)
{
    ASSERT(m_element2);
    ASSERT(m_atChild);
    ASSERT(m_atChild->parentNode() == m_element2);
}

void SplitElementCommand::executeApply()
{
    if (m_atChild->parentNode() != m_element2)
        return;
    
    Vector<RefPtr<Node> > children;
    for (Node* node = m_element2->firstChild(); node != m_atChild; node = node->nextSibling())
        children.append(node);
    
    ExceptionCode ec = 0;
    
    ContainerNode* parent = m_element2->parentNode();
    if (!parent || !parent->rendererIsEditable())
        return;
    parent->insertBefore(m_element1.get(), m_element2.get(), ec);
    if (ec)
        return;

    // Delete id attribute from the second element because the same id cannot be used for more than one element
    m_element2->removeAttribute(HTMLNames::idAttr, ec);
    ASSERT(!ec);

    size_t size = children.size();
    for (size_t i = 0; i < size; ++i)
        m_element1->appendChild(children[i], ec);
}
    
void SplitElementCommand::doApply()
{
    m_element1 = m_element2->cloneElementWithoutChildren();
    
    executeApply();
}

void SplitElementCommand::doUnapply()
{
    if (!m_element1 || !m_element1->rendererIsEditable() || !m_element2->rendererIsEditable())
        return;

    Vector<RefPtr<Node> > children;
    for (Node* node = m_element1->firstChild(); node; node = node->nextSibling())
        children.append(node);

    RefPtr<Node> refChild = m_element2->firstChild();

    ExceptionCode ec = 0;

    size_t size = children.size();
    for (size_t i = 0; i < size; ++i)
        m_element2->insertBefore(children[i].get(), refChild.get(), ec);

    // Recover the id attribute of the original element.
    if (m_element1->hasAttribute(HTMLNames::idAttr))
        m_element2->setAttribute(HTMLNames::idAttr, m_element1->getAttribute(HTMLNames::idAttr));

    m_element1->remove(ec);
}

void SplitElementCommand::doReapply()
{
    if (!m_element1)
        return;
    
    executeApply();
}
    
} // namespace WebCore
