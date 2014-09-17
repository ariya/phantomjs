/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2010 Apple Inc. All rights reserved.
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "HTMLLegendElement.h"

#include "HTMLNames.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

using namespace HTMLNames;

inline HTMLLegendElement::HTMLLegendElement(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
    : HTMLFormControlElement(tagName, document, form)
{
    ASSERT(hasTagName(legendTag));
}

PassRefPtr<HTMLLegendElement> HTMLLegendElement::create(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
{
    return adoptRef(new HTMLLegendElement(tagName, document, form));
}

bool HTMLLegendElement::supportsFocus() const
{
    return HTMLElement::supportsFocus();
}

const AtomicString& HTMLLegendElement::formControlType() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, legend, ("legend"));
    return legend;
}

HTMLFormControlElement* HTMLLegendElement::associatedControl()
{
    // Check if there's a fieldset belonging to this legend.
    ContainerNode* fieldset = parentNode();
    while (fieldset && !fieldset->hasTagName(fieldsetTag))
        fieldset = fieldset->parentNode();
    if (!fieldset)
        return 0;

    // Find first form element inside the fieldset that is not a legend element.
    // FIXME: Should we consider tabindex?
    Node* node = fieldset;
    while ((node = node->traverseNextNode(fieldset))) {
        if (node->isElementNode()) {
            Element* element = static_cast<Element*>(node);
            if (!element->hasLocalName(legendTag) && element->isFormControlElement())
                return static_cast<HTMLFormControlElement*>(element);
        }
    }

    return 0;
}

void HTMLLegendElement::focus(bool)
{
    if (isFocusable())
        Element::focus();
        
    // To match other browsers' behavior, never restore previous selection.
    if (HTMLFormControlElement* control = associatedControl())
        control->focus(false);
}

void HTMLLegendElement::accessKeyAction(bool sendToAnyElement)
{
    if (HTMLFormControlElement* control = associatedControl())
        control->accessKeyAction(sendToAnyElement);
}
    
} // namespace
