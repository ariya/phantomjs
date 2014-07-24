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

#include "EditorClient.h"
#include "FormController.h"
#include "Frame.h"
#include "HTMLFormControlElement.h"
#include "HTMLFormElement.h"
#include "HTMLNames.h"
#include "HTMLObjectElement.h"
#include "IdTargetObserver.h"
#include "ValidityState.h"

namespace WebCore {

using namespace HTMLNames;

class FormAttributeTargetObserver : IdTargetObserver {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<FormAttributeTargetObserver> create(const AtomicString& id, FormAssociatedElement*);
    virtual void idTargetChanged() OVERRIDE;

private:
    FormAttributeTargetObserver(const AtomicString& id, FormAssociatedElement*);

    FormAssociatedElement* m_element;
};

FormAssociatedElement::FormAssociatedElement()
    : m_form(0)
{
}

FormAssociatedElement::~FormAssociatedElement()
{
    setForm(0);
}

ValidityState* FormAssociatedElement::validity()
{
    if (!m_validityState)
        m_validityState = ValidityState::create(this);

    return m_validityState.get();
}

void FormAssociatedElement::didMoveToNewDocument(Document* oldDocument)
{
    HTMLElement* element = toHTMLElement(this);
    if (oldDocument && element->fastHasAttribute(formAttr))
        resetFormAttributeTargetObserver();
}

void FormAssociatedElement::insertedInto(ContainerNode* insertionPoint)
{
    resetFormOwner();
    if (!insertionPoint->inDocument())
        return;

    HTMLElement* element = toHTMLElement(this);
    if (element->fastHasAttribute(formAttr))
        resetFormAttributeTargetObserver();
}

void FormAssociatedElement::removedFrom(ContainerNode* insertionPoint)
{
    HTMLElement* element = toHTMLElement(this);
    if (insertionPoint->inDocument() && element->fastHasAttribute(formAttr))
        m_formAttributeTargetObserver = nullptr;
    // If the form and element are both in the same tree, preserve the connection to the form.
    // Otherwise, null out our form and remove ourselves from the form's list of elements.
    if (m_form && element->highestAncestor() != m_form->highestAncestor())
        setForm(0);
}

HTMLFormElement* FormAssociatedElement::findAssociatedForm(const HTMLElement* element, HTMLFormElement* currentAssociatedForm)
{
    const AtomicString& formId(element->fastGetAttribute(formAttr));
    if (!formId.isNull() && element->inDocument()) {
        // The HTML5 spec says that the element should be associated with
        // the first element in the document to have an ID that equal to
        // the value of form attribute, so we put the result of
        // treeScope()->getElementById() over the given element.
        HTMLFormElement* newForm = 0;
        Element* newFormCandidate = element->treeScope()->getElementById(formId);
        if (newFormCandidate && isHTMLFormElement(newFormCandidate))
            newForm = toHTMLFormElement(newFormCandidate);
        return newForm;
    }

    if (!currentAssociatedForm)
        return element->findFormAncestor();

    return currentAssociatedForm;
}

void FormAssociatedElement::formRemovedFromTree(const Node* formRoot)
{
    ASSERT(m_form);
    if (toHTMLElement(this)->highestAncestor() != formRoot)
        setForm(0);
}

void FormAssociatedElement::setForm(HTMLFormElement* newForm)
{
    if (m_form == newForm)
        return;
    willChangeForm();
    if (m_form)
        m_form->removeFormElement(this);
    m_form = newForm;
    if (m_form)
        m_form->registerFormElement(this);
    didChangeForm();
}

void FormAssociatedElement::willChangeForm()
{
}

void FormAssociatedElement::didChangeForm()
{
}

void FormAssociatedElement::formWillBeDestroyed()
{
    ASSERT(m_form);
    if (!m_form)
        return;
    willChangeForm();
    m_form = 0;
    didChangeForm();
}

void FormAssociatedElement::resetFormOwner()
{
    HTMLFormElement* originalForm = m_form;
    setForm(findAssociatedForm(toHTMLElement(this), m_form));
    HTMLElement* element = toHTMLElement(this);     
    if (m_form && m_form != originalForm && m_form->inDocument())
        element->document()->didAssociateFormControl(element);
}

void FormAssociatedElement::formAttributeChanged()
{
    HTMLElement* element = toHTMLElement(this);
    if (!element->fastHasAttribute(formAttr)) {
        // The form attribute removed. We need to reset form owner here.
        HTMLFormElement* originalForm = m_form;
        setForm(element->findFormAncestor());
        HTMLElement* element = toHTMLElement(this);
        if (m_form && m_form != originalForm && m_form->inDocument())
            element->document()->didAssociateFormControl(element);
        m_formAttributeTargetObserver = nullptr;
    } else {
        resetFormOwner();
        if (element->inDocument())
            resetFormAttributeTargetObserver();
    }
}

bool FormAssociatedElement::customError() const
{
    const HTMLElement* element = toHTMLElement(this);
    return element->willValidate() && !m_customValidationMessage.isEmpty();
}

bool FormAssociatedElement::hasBadInput() const
{
    return false;
}

bool FormAssociatedElement::patternMismatch() const
{
    return false;
}

bool FormAssociatedElement::rangeOverflow() const
{
    return false;
}

bool FormAssociatedElement::rangeUnderflow() const
{
    return false;
}

bool FormAssociatedElement::stepMismatch() const
{
    return false;
}

bool FormAssociatedElement::tooLong() const
{
    return false;
}

bool FormAssociatedElement::typeMismatch() const
{
    return false;
}

bool FormAssociatedElement::valid() const
{
    bool someError = typeMismatch() || stepMismatch() || rangeUnderflow() || rangeOverflow()
        || tooLong() || patternMismatch() || valueMissing() || hasBadInput() || customError();
    return !someError;
}

bool FormAssociatedElement::valueMissing() const
{
    return false;
}

String FormAssociatedElement::customValidationMessage() const
{
    return m_customValidationMessage;
}

String FormAssociatedElement::validationMessage() const
{
    return customError() ? m_customValidationMessage : String();
}

void FormAssociatedElement::setCustomValidity(const String& error)
{
    m_customValidationMessage = error;
}

void FormAssociatedElement::resetFormAttributeTargetObserver()
{
    m_formAttributeTargetObserver = FormAttributeTargetObserver::create(toHTMLElement(this)->fastGetAttribute(formAttr), this);
}

void FormAssociatedElement::formAttributeTargetChanged()
{
    resetFormOwner();
}

const AtomicString& FormAssociatedElement::name() const
{
    const AtomicString& name = toHTMLElement(this)->getNameAttribute();
    return name.isNull() ? emptyAtom : name;
}

bool FormAssociatedElement::isFormControlElementWithState() const
{
    return false;
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

PassOwnPtr<FormAttributeTargetObserver> FormAttributeTargetObserver::create(const AtomicString& id, FormAssociatedElement* element)
{
    return adoptPtr(new FormAttributeTargetObserver(id, element));
}

FormAttributeTargetObserver::FormAttributeTargetObserver(const AtomicString& id, FormAssociatedElement* element)
    : IdTargetObserver(toHTMLElement(element)->treeScope()->idTargetObserverRegistry(), id)
    , m_element(element)
{
}

void FormAttributeTargetObserver::idTargetChanged()
{
    m_element->formAttributeTargetChanged();
}

} // namespace Webcore
