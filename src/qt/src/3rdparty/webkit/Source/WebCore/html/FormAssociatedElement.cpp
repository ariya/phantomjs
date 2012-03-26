/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
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
#include "FormAssociatedElement.h"

#include "HTMLFormControlElement.h"
#include "HTMLFormElement.h"
#include "HTMLNames.h"
#include "HTMLObjectElement.h"
#include "ValidityState.h"

namespace WebCore {

using namespace HTMLNames;

FormAssociatedElement::FormAssociatedElement(HTMLFormElement* form)
    : m_form(form)
{
}

FormAssociatedElement::~FormAssociatedElement()
{
}

ValidityState* FormAssociatedElement::validity()
{
    if (!m_validityState)
        m_validityState = ValidityState::create(this);

    return m_validityState.get();
}

void FormAssociatedElement::willMoveToNewOwnerDocument()
{
    HTMLElement* element = toHTMLElement(this);
    if (element->fastHasAttribute(formAttr))
        element->document()->unregisterFormElementWithFormAttribute(this);
}

void FormAssociatedElement::insertedIntoDocument()
{
    HTMLElement* element = toHTMLElement(this);
    if (element->fastHasAttribute(formAttr))
        element->document()->registerFormElementWithFormAttribute(this);
}

void FormAssociatedElement::removedFromDocument()
{
    HTMLElement* element = toHTMLElement(this);
    if (element->fastHasAttribute(formAttr))
        element->document()->unregisterFormElementWithFormAttribute(this);
}

void FormAssociatedElement::insertedIntoTree()
{
    HTMLElement* element = toHTMLElement(this);
    if (element->fastHasAttribute(formAttr)) {
        Element* formElement = element->treeScope()->getElementById(element->fastGetAttribute(formAttr));
        if (formElement && formElement->hasTagName(formTag)) {
            if (m_form)
                m_form->removeFormElement(this);
            m_form = static_cast<HTMLFormElement*>(formElement);
            m_form->registerFormElement(this);
        }
    }
    if (!m_form) {
        // This handles the case of a new form element being created by
        // JavaScript and inserted inside a form.  In the case of the parser
        // setting a form, we will already have a non-null value for m_form,
        // and so we don't need to do anything.
        m_form = element->findFormAncestor();
        if (m_form)
            m_form->registerFormElement(this);
    }
}

static inline Node* findRoot(Node* n)
{
    Node* root = n;
    for (; n; n = n->parentNode())
        root = n;
    return root;
}

void FormAssociatedElement::removedFromTree()
{
    HTMLElement* element = toHTMLElement(this);

    // If the form and element are both in the same tree, preserve the connection to the form.
    // Otherwise, null out our form and remove ourselves from the form's list of elements.
    if (m_form && findRoot(element) != findRoot(m_form))
        removeFromForm();
}

void FormAssociatedElement::removeFromForm()
{
    if (!m_form)
        return;
    m_form->removeFormElement(this);
    m_form = 0;
}

void FormAssociatedElement::resetFormOwner(HTMLFormElement* form)
{
    HTMLElement* element = toHTMLElement(this);
    const AtomicString& formId(element->fastGetAttribute(formAttr));
    if (m_form) {
        if (formId.isNull())
            return;
        m_form->removeFormElement(this);
    }
    m_form = 0;
    if (!formId.isNull() && element->inDocument()) {
        // The HTML5 spec says that the element should be associated with
        // the first element in the document to have an ID that equal to
        // the value of form attribute, so we put the result of
        // treeScope()->getElementById() over the given element.
        Element* firstElement = element->treeScope()->getElementById(formId);
        if (firstElement && firstElement->hasTagName(formTag))
            m_form = static_cast<HTMLFormElement*>(firstElement);
        else
            m_form = form;
    } else
        m_form = element->findFormAncestor();
    if (m_form)
        m_form->registerFormElement(this);
}

void FormAssociatedElement::formAttributeChanged()
{
    HTMLElement* element = toHTMLElement(this);
    if (!element->fastHasAttribute(formAttr)) {
        // The form attribute removed. We need to reset form owner here.
        if (m_form)
            m_form->removeFormElement(this);
        m_form = element->findFormAncestor();
        if (m_form)
            form()->registerFormElement(this);
        element->document()->unregisterFormElementWithFormAttribute(this);
    } else
        resetFormOwner(0);
}

const HTMLElement* toHTMLElement(const FormAssociatedElement* associatedElement)
{
    if (associatedElement->isFormControlElement())
        return static_cast<const HTMLFormControlElement*>(associatedElement);
    // Assumes the element is an HTMLObjectElement
    const HTMLElement* element = static_cast<const HTMLObjectElement*>(associatedElement);
    ASSERT(element->hasTagName(objectTag));
    return element;
}

HTMLElement* toHTMLElement(FormAssociatedElement* associatedElement)
{
    return const_cast<HTMLElement*>(toHTMLElement(static_cast<const FormAssociatedElement*>(associatedElement)));
}

} // namespace Webcore
