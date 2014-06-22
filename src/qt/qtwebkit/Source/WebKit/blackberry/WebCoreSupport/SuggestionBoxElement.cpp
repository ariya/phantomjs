/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "config.h"
#include "SuggestionBoxElement.h"

#include "Frame.h"
#include "HTMLNames.h"
#include "MouseEvent.h"
#include "SuggestionBoxHandler.h"
#include "TouchEvent.h"

namespace WebCore {

SuggestionBoxElement::SuggestionBoxElement(SuggestionBoxHandler* element, String& value, const QualifiedName& name, Document* document)
    : HTMLDivElement(name, document)
    , m_suggestionBox(element)
    , m_value(value)
{
}

SuggestionBoxElement::~SuggestionBoxElement()
{
}

PassRefPtr<SuggestionBoxElement> SuggestionBoxElement::create(SuggestionBoxHandler* element, String& value, Document* document)
{
    return adoptRef(new SuggestionBoxElement(element, value, HTMLNames::divTag, document));
}

void SuggestionBoxElement::defaultEventHandler(Event* event)
{
    if (event->isMouseEvent()) {
        // Capture a mouse down event with left click and set the parent input element to the node's text
        if (event->type() == eventNames().mousedownEvent && static_cast<MouseEvent*>(event)->button() != RightButton && document()->frame()) {
            m_suggestionBox->changeInputElementInnerTextValue(m_value);
            m_suggestionBox->hideDropdownBox();
        }
    }
}

} /* namespace WebCore */
