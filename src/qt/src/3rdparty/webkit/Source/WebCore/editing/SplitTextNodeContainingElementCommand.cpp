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
#include "SplitTextNodeContainingElementCommand.h"

#include "Element.h"
#include "Text.h"
#include "RenderObject.h"
#include <wtf/Assertions.h>

namespace WebCore {

SplitTextNodeContainingElementCommand::SplitTextNodeContainingElementCommand(PassRefPtr<Text> text, int offset)
    : CompositeEditCommand(text->document()), m_text(text), m_offset(offset)
{
    ASSERT(m_text);
    ASSERT(m_text->length() > 0);
}

void SplitTextNodeContainingElementCommand::doApply()
{
    ASSERT(m_text);
    ASSERT(m_offset > 0);

    splitTextNode(m_text.get(), m_offset);

    Element* parent = m_text->parentElement();
    if (!parent || !parent->parentElement() || !parent->parentElement()->rendererIsEditable())
        return;

    RenderObject* parentRenderer = parent->renderer();
    if (!parentRenderer || !parentRenderer->isInline()) {
        wrapContentsInDummySpan(parent);
        Node* firstChild = parent->firstChild();
        if (!firstChild || !firstChild->isElementNode())
            return;
        parent = static_cast<Element*>(firstChild);
    }

    splitElement(parent, m_text);
}

}
