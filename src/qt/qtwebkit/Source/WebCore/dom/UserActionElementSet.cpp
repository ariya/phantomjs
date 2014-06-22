/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "UserActionElementSet.h"

#include "Document.h"
#include "Element.h"

namespace WebCore {

UserActionElementSet::UserActionElementSet()
{
}

UserActionElementSet::~UserActionElementSet()
{
}

void UserActionElementSet::didDetach(Element* element)
{
    ASSERT(element->isUserActionElement());
    clearFlags(element, IsActiveFlag | InActiveChainFlag | IsHoveredFlag);
}

void UserActionElementSet::documentDidRemoveLastRef()
{
    m_elements.clear();
}

bool UserActionElementSet::hasFlags(const Element* element, unsigned flags) const
{
    ASSERT(element->isUserActionElement());
    ElementFlagMap::const_iterator found = m_elements.find(const_cast<Element*>(element));
    if (found == m_elements.end())
        return false;
    return found->value & flags;
}

void UserActionElementSet::clearFlags(Element* element, unsigned flags)
{
    if (!element->isUserActionElement()) {
        ASSERT(m_elements.end() == m_elements.find(element));
        return;
    }

    ElementFlagMap::iterator found = m_elements.find(element);
    if (found == m_elements.end()) {
        element->setUserActionElement(false);
        return;
    }

    unsigned updated = found->value & ~flags;
    if (!updated) {
        element->setUserActionElement(false);
        m_elements.remove(found);
        return;
    }

    found->value = updated;
}

void UserActionElementSet::setFlags(Element* element, unsigned flags)
{
    ElementFlagMap::iterator result = m_elements.find(element);
    if (result != m_elements.end()) {
        ASSERT(element->isUserActionElement());
        result->value |= flags;
        return;
    }

    element->setUserActionElement(true);
    m_elements.add(element, flags);
}

}
