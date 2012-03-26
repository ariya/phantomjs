/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2010 Apple Inc. All rights reserved.
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
#include "HTMLLabelElement.h"

#include "Document.h"
#include "Event.h"
#include "EventNames.h"
#include "HTMLFormControlElement.h"
#include "HTMLFormElement.h"
#include "HTMLNames.h"

namespace WebCore {

using namespace HTMLNames;

static HTMLFormControlElement* nodeAsLabelableFormControl(Node* node)
{
    if (!node || !node->isElementNode() || !static_cast<Element*>(node)->isFormControlElement())
        return 0;
    
    HTMLFormControlElement* formControlElement = static_cast<HTMLFormControlElement*>(node);
    if (!formControlElement->isLabelable())
        return 0;

    return formControlElement;
}

inline HTMLLabelElement::HTMLLabelElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
{
    ASSERT(hasTagName(labelTag));
}

PassRefPtr<HTMLLabelElement> HTMLLabelElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLLabelElement(tagName, document));
}

bool HTMLLabelElement::isFocusable() const
{
    return false;
}

HTMLFormControlElement* HTMLLabelElement::control()
{
    const AtomicString& controlId = getAttribute(forAttr);
    if (controlId.isNull()) {
        // Search the children and descendants of the label element for a form element.
        // per http://dev.w3.org/html5/spec/Overview.html#the-label-element
        // the form element must be "labelable form-associated element".
        Node* node = this;
        while ((node = node->traverseNextNode(this))) {
            if (HTMLFormControlElement* formControlElement = nodeAsLabelableFormControl(node))
                return formControlElement;
        }
        return 0;
    }
    
    // Find the first element whose id is controlId. If it is found and it is a labelable form control,
    // return it, otherwise return 0.
    return nodeAsLabelableFormControl(treeScope()->getElementById(controlId));
}

void HTMLLabelElement::setActive(bool down, bool pause)
{
    if (down == active())
        return;

    // Update our status first.
    HTMLElement::setActive(down, pause);

    // Also update our corresponding control.
    if (HTMLElement* element = control())
        element->setActive(down, pause);
}

void HTMLLabelElement::setHovered(bool over)
{
    if (over == hovered())
        return;
        
    // Update our status first.
    HTMLElement::setHovered(over);

    // Also update our corresponding control.
    if (HTMLElement* element = control())
        element->setHovered(over);
}

void HTMLLabelElement::defaultEventHandler(Event* evt)
{
    static bool processingClick = false;

    if (evt->type() == eventNames().clickEvent && !processingClick) {
        RefPtr<HTMLElement> element = control();

        // If we can't find a control or if the control received the click
        // event, then there's no need for us to do anything.
        if (!element || (evt->target() && element->containsIncludingShadowDOM(evt->target()->toNode())))
            return;

        processingClick = true;

        // Click the corresponding control.
        element->dispatchSimulatedClick(evt);

        // If the control can be focused via the mouse, then do that too.
        if (element->isMouseFocusable())
            element->focus();

        processingClick = false;
        
        evt->setDefaultHandled();
    }
    
    HTMLElement::defaultEventHandler(evt);
}

void HTMLLabelElement::focus(bool)
{
    // to match other browsers, always restore previous selection
    if (HTMLElement* element = control())
        element->focus();
}

void HTMLLabelElement::accessKeyAction(bool sendToAnyElement)
{
    if (HTMLElement* element = control())
        element->accessKeyAction(sendToAnyElement);
    else
        HTMLElement::accessKeyAction(sendToAnyElement);
}

void HTMLLabelElement::parseMappedAttribute(Attribute* attribute)
{
    if (attribute->name() == forAttr) {
        // htmlFor attribute change affects other nodes than this.
        // Clear the caches to ensure that the labels caches are cleared.
        if (document())
            document()->notifyLocalNodeListsLabelChanged();
    } else
        HTMLElement::parseMappedAttribute(attribute);
}
                
} // namespace
