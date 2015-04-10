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
#include "RemoveCSSPropertyCommand.h"

#include "CSSStyleDeclaration.h"
#include "ExceptionCodePlaceholder.h"
#include "StylePropertySet.h"
#include "StyledElement.h"
#include <wtf/Assertions.h>

namespace WebCore {

RemoveCSSPropertyCommand::RemoveCSSPropertyCommand(Document* document, PassRefPtr<StyledElement> element, CSSPropertyID property)
    : SimpleEditCommand(document)
    , m_element(element)
    , m_property(property)
    , m_important(false)
{
    ASSERT(m_element);
}

RemoveCSSPropertyCommand::~RemoveCSSPropertyCommand()
{
}

void RemoveCSSPropertyCommand::doApply()
{
    const StylePropertySet* style = m_element->inlineStyle();
    m_oldValue = style->getPropertyValue(m_property);
    m_important = style->propertyIsImportant(m_property);

    // Mutate using the CSSOM wrapper so we get the same event behavior as a script.
    // Setting to null string removes the property. We don't have internal version of removeProperty.
    m_element->style()->setPropertyInternal(m_property, String(), false, IGNORE_EXCEPTION);
}

void RemoveCSSPropertyCommand::doUnapply()
{
    m_element->style()->setPropertyInternal(m_property, m_oldValue, m_important, IGNORE_EXCEPTION);
}

#ifndef NDEBUG
void RemoveCSSPropertyCommand::getNodesInCommand(HashSet<Node*>& nodes)
{
    addNodeAndDescendants(m_element.get(), nodes);
}
#endif

} // namespace WebCore
