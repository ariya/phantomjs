/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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
#include "HTMLFormElement.h"

#include "Attribute.h"
#include "DOMFormData.h"
#include "DOMWindow.h"
#include "Document.h"
#include "Event.h"
#include "EventNames.h"
#include "FileList.h"
#include "FileSystem.h"
#include "FormData.h"
#include "FormDataList.h"
#include "FormState.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "HTMLDocument.h"
#include "HTMLFormCollection.h"
#include "HTMLImageElement.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "MIMETypeRegistry.h"
#include "Page.h"
#include "RenderTextControl.h"
#include "ScriptEventListener.h"
#include "Settings.h"
#include "ValidityState.h"
#include <limits>

#if PLATFORM(WX)
#include <wx/defs.h>
#include <wx/filename.h>
#endif

using namespace std;

namespace WebCore {

using namespace HTMLNames;

HTMLFormElement::HTMLFormElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
    , m_associatedElementsBeforeIndex(0)
    , m_associatedElementsAfterIndex(0)
    , m_wasUserSubmitted(false)
    , m_isSubmittingOrPreparingForSubmission(false)
    , m_shouldSubmit(false)
    , m_isInResetFunction(false)
    , m_wasMalformed(false)
    , m_wasDemoted(false)
{
    ASSERT(hasTagName(formTag));
}

PassRefPtr<HTMLFormElement> HTMLFormElement::create(Document* document)
{
    return adoptRef(new HTMLFormElement(formTag, document));
}

PassRefPtr<HTMLFormElement> HTMLFormElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLFormElement(tagName, document));
}

HTMLFormElement::~HTMLFormElement()
{
    if (!autoComplete())
        document()->unregisterForDocumentActivationCallbacks(this);

    for (unsigned i = 0; i < m_associatedElements.size(); ++i)
        m_associatedElements[i]->formDestroyed();
    for (unsigned i = 0; i < m_imageElements.size(); ++i)
        m_imageElements[i]->m_form = 0;
}

bool HTMLFormElement::formWouldHaveSecureSubmission(const String& url)
{
    return document()->completeURL(url).protocolIs("https");
}

bool HTMLFormElement::rendererIsNeeded(RenderStyle* style)
{
    if (!m_wasDemoted)
        return HTMLElement::rendererIsNeeded(style);

    ContainerNode* node = parentNode();
    RenderObject* parentRenderer = node->renderer();
    bool parentIsTableElementPart = (parentRenderer->isTable() && node->hasTagName(tableTag))
        || (parentRenderer->isTableRow() && node->hasTagName(trTag))
        || (parentRenderer->isTableSection() && node->hasTagName(tbodyTag))
        || (parentRenderer->isTableCol() && node->hasTagName(colTag))
        || (parentRenderer->isTableCell() && node->hasTagName(trTag));

    if (!parentIsTableElementPart)
        return true;

    EDisplay display = style->display();
    bool formIsTablePart = display == TABLE || display == INLINE_TABLE || display == TABLE_ROW_GROUP
        || display == TABLE_HEADER_GROUP || display == TABLE_FOOTER_GROUP || display == TABLE_ROW
        || display == TABLE_COLUMN_GROUP || display == TABLE_COLUMN || display == TABLE_CELL
        || display == TABLE_CAPTION;

    return formIsTablePart;
}

void HTMLFormElement::insertedIntoDocument()
{
    if (document()->isHTMLDocument())
        static_cast<HTMLDocument*>(document())->addNamedItem(m_name);

    HTMLElement::insertedIntoDocument();

    if (hasID())
        document()->resetFormElementsOwner(this);
}

void HTMLFormElement::removedFromDocument()
{
    if (document()->isHTMLDocument())
        static_cast<HTMLDocument*>(document())->removeNamedItem(m_name);

    HTMLElement::removedFromDocument();

    if (hasID())
        document()->resetFormElementsOwner(0);
}

void HTMLFormElement::handleLocalEvents(Event* event)
{
    Node* targetNode = event->target()->toNode();
    if (event->eventPhase() != Event::CAPTURING_PHASE && targetNode && targetNode != this && (event->type() == eventNames().submitEvent || event->type() == eventNames().resetEvent)) {
        event->stopPropagation();
        return;
    }
    HTMLElement::handleLocalEvents(event);
}

unsigned HTMLFormElement::length() const
{
    unsigned len = 0;
    for (unsigned i = 0; i < m_associatedElements.size(); ++i)
        if (m_associatedElements[i]->isEnumeratable())
            ++len;
    return len;
}

Node* HTMLFormElement::item(unsigned index)
{
    return elements()->item(index);
}

void HTMLFormElement::submitImplicitly(Event* event, bool fromImplicitSubmissionTrigger)
{
    int submissionTriggerCount = 0;
    for (unsigned i = 0; i < m_associatedElements.size(); ++i) {
        FormAssociatedElement* formAssociatedElement = m_associatedElements[i];
        if (!formAssociatedElement->isFormControlElement())
            continue;
        HTMLFormControlElement* formElement = static_cast<HTMLFormControlElement*>(formAssociatedElement);
        if (formElement->isSuccessfulSubmitButton()) {
            if (formElement->renderer()) {
                formElement->dispatchSimulatedClick(event);
                return;
            }
        } else if (formElement->canTriggerImplicitSubmission())
            ++submissionTriggerCount;
    }
    if (fromImplicitSubmissionTrigger && submissionTriggerCount == 1)
        prepareForSubmission(event);
}

static inline HTMLFormControlElement* submitElementFromEvent(const Event* event)
{
    Node* targetNode = event->target()->toNode();
    if (!targetNode || !targetNode->isElementNode())
        return 0;
    Element* targetElement = static_cast<Element*>(targetNode);
    if (!targetElement->isFormControlElement())
        return 0;
    return static_cast<HTMLFormControlElement*>(targetElement);
}

bool HTMLFormElement::validateInteractively(Event* event)
{
    ASSERT(event);
    if (!document()->page() || !document()->page()->settings()->interactiveFormValidationEnabled() || noValidate())
        return true;

    HTMLFormControlElement* submitElement = submitElementFromEvent(event);
    if (submitElement && submitElement->formNoValidate())
        return true;

    for (unsigned i = 0; i < m_associatedElements.size(); ++i) {
        if (m_associatedElements[i]->isFormControlElement())
            static_cast<HTMLFormControlElement*>(m_associatedElements[i])->hideVisibleValidationMessage();
    }

    Vector<RefPtr<FormAssociatedElement> > unhandledInvalidControls;
    if (!checkInvalidControlsAndCollectUnhandled(unhandledInvalidControls))
        return true;
    // Because the form has invalid controls, we abort the form submission and
    // show a validation message on a focusable form control.

    // Needs to update layout now because we'd like to call isFocusable(), which
    // has !renderer()->needsLayout() assertion.
    document()->updateLayoutIgnorePendingStylesheets();

    RefPtr<HTMLFormElement> protector(this);
    // Focus on the first focusable control and show a validation message.
    for (unsigned i = 0; i < unhandledInvalidControls.size(); ++i) {
        FormAssociatedElement* unhandledAssociatedElement = unhandledInvalidControls[i].get();
        HTMLElement* unhandled = toHTMLElement(unhandledAssociatedElement);
        if (unhandled->isFocusable() && unhandled->inDocument()) {
            unhandled->scrollIntoViewIfNeeded(false);
            unhandled->focus();
            if (unhandled->isFormControlElement())
                static_cast<HTMLFormControlElement*>(unhandled)->updateVisibleValidationMessage();
            break;
        }
    }
    // Warn about all of unfocusable controls.
    if (Frame* frame = document()->frame()) {
        for (unsigned i = 0; i < unhandledInvalidControls.size(); ++i) {
            FormAssociatedElement* unhandledAssociatedElement = unhandledInvalidControls[i].get();
            HTMLElement* unhandled = toHTMLElement(unhandledAssociatedElement);
            if (unhandled->isFocusable() && unhandled->inDocument())
                continue;
            String message("An invalid form control with name='%name' is not focusable.");
            message.replace("%name", unhandledAssociatedElement->name());
            frame->domWindow()->console()->addMessage(HTMLMessageSource, LogMessageType, ErrorMessageLevel, message, 0, document()->url().string());
        }
    }
    return false;
}

bool HTMLFormElement::prepareForSubmission(Event* event)
{
    Frame* frame = document()->frame();
    if (m_isSubmittingOrPreparingForSubmission || !frame)
        return m_isSubmittingOrPreparingForSubmission;

    m_isSubmittingOrPreparingForSubmission = true;
    m_shouldSubmit = false;

    // Interactive validation must be done before dispatching the submit event.
    if (!validateInteractively(event)) {
        m_isSubmittingOrPreparingForSubmission = false;
        return false;
    }

    frame->loader()->client()->dispatchWillSendSubmitEvent(this);

    if (dispatchEvent(Event::create(eventNames().submitEvent, true, true)))
        m_shouldSubmit = true;

    m_isSubmittingOrPreparingForSubmission = false;

    if (m_shouldSubmit)
        submit(event, true, true, NotSubmittedByJavaScript);

    return m_shouldSubmit;
}

void HTMLFormElement::submit()
{
    submit(0, false, true, NotSubmittedByJavaScript);
}

void HTMLFormElement::submitFromJavaScript()
{
    Frame* frame = document()->frame();
    if (!frame)
        return;
    submit(0, false, frame->script()->anyPageIsProcessingUserGesture(), SubmittedByJavaScript);
}

void HTMLFormElement::submit(Event* event, bool activateSubmitButton, bool processingUserGesture, FormSubmissionTrigger formSubmissionTrigger)
{
    FrameView* view = document()->view();
    Frame* frame = document()->frame();
    if (!view || !frame)
        return;

    if (m_isSubmittingOrPreparingForSubmission) {
        m_shouldSubmit = true;
        return;
    }

    m_isSubmittingOrPreparingForSubmission = true;
    m_wasUserSubmitted = processingUserGesture;

    HTMLFormControlElement* firstSuccessfulSubmitButton = 0;
    bool needButtonActivation = activateSubmitButton; // do we need to activate a submit button?

    for (unsigned i = 0; i < m_associatedElements.size(); ++i) {
        FormAssociatedElement* associatedElement = m_associatedElements[i];
        if (!associatedElement->isFormControlElement())
            continue;
        if (needButtonActivation) {
            HTMLFormControlElement* control = static_cast<HTMLFormControlElement*>(associatedElement);
            if (control->isActivatedSubmit())
                needButtonActivation = false;
            else if (firstSuccessfulSubmitButton == 0 && control->isSuccessfulSubmitButton())
                firstSuccessfulSubmitButton = control;
        }
    }

    if (needButtonActivation && firstSuccessfulSubmitButton)
        firstSuccessfulSubmitButton->setActivatedSubmit(true);

    frame->loader()->submitForm(FormSubmission::create(this, m_attributes, event, !processingUserGesture, formSubmissionTrigger));

    if (needButtonActivation && firstSuccessfulSubmitButton)
        firstSuccessfulSubmitButton->setActivatedSubmit(false);

    m_shouldSubmit = false;
    m_isSubmittingOrPreparingForSubmission = false;
}

void HTMLFormElement::reset()
{
    Frame* frame = document()->frame();
    if (m_isInResetFunction || !frame)
        return;

    m_isInResetFunction = true;

    if (!dispatchEvent(Event::create(eventNames().resetEvent, true, true))) {
        m_isInResetFunction = false;
        return;
    }

    for (unsigned i = 0; i < m_associatedElements.size(); ++i) {
        if (m_associatedElements[i]->isFormControlElement())
            static_cast<HTMLFormControlElement*>(m_associatedElements[i])->reset();
    }

    m_isInResetFunction = false;
}

void HTMLFormElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == actionAttr)
        m_attributes.parseAction(attr->value());
    else if (attr->name() == targetAttr)
        m_attributes.setTarget(attr->value());
    else if (attr->name() == methodAttr)
        m_attributes.parseMethodType(attr->value());
    else if (attr->name() == enctypeAttr)
        m_attributes.parseEncodingType(attr->value());
    else if (attr->name() == accept_charsetAttr)
        m_attributes.setAcceptCharset(attr->value());
    else if (attr->name() == autocompleteAttr) {
        if (!autoComplete())
            document()->registerForDocumentActivationCallbacks(this);
        else
            document()->unregisterForDocumentActivationCallbacks(this);
    } else if (attr->name() == onsubmitAttr)
        setAttributeEventListener(eventNames().submitEvent, createAttributeEventListener(this, attr));
    else if (attr->name() == onresetAttr)
        setAttributeEventListener(eventNames().resetEvent, createAttributeEventListener(this, attr));
    else if (attr->name() == nameAttr) {
        const AtomicString& newName = attr->value();
        if (inDocument() && document()->isHTMLDocument()) {
            HTMLDocument* document = static_cast<HTMLDocument*>(this->document());
            document->removeNamedItem(m_name);
            document->addNamedItem(newName);
        }
        m_name = newName;
    } else
        HTMLElement::parseMappedAttribute(attr);
}

template<class T, size_t n> static void removeFromVector(Vector<T*, n> & vec, T* item)
{
    size_t size = vec.size();
    for (size_t i = 0; i != size; ++i)
        if (vec[i] == item) {
            vec.remove(i);
            break;
        }
}

unsigned HTMLFormElement::formElementIndexWithFormAttribute(Element* element)
{
    // Compares the position of the form element and the inserted element.
    // Updates the indeces in order to the relation of the position:
    unsigned short position = compareDocumentPosition(element);
    if (position & (DOCUMENT_POSITION_CONTAINS | DOCUMENT_POSITION_CONTAINED_BY))
        ++m_associatedElementsAfterIndex;
    else if (position & DOCUMENT_POSITION_PRECEDING) {
        ++m_associatedElementsBeforeIndex;
        ++m_associatedElementsAfterIndex;
    }

    if (m_associatedElements.isEmpty())
        return 0;

    // Does binary search on m_associatedElements in order to find the index
    // to be inserted.
    unsigned left = 0, right = m_associatedElements.size() - 1;
    while (left != right) {
        unsigned middle = left + ((right - left) / 2);
        position = element->compareDocumentPosition(toHTMLElement(m_associatedElements[middle]));
        if (position & DOCUMENT_POSITION_FOLLOWING)
            right = middle;
        else
            left = middle + 1;
    }

    position = element->compareDocumentPosition(toHTMLElement(m_associatedElements[left]));
    if (position & DOCUMENT_POSITION_FOLLOWING)
        return left;
    return left + 1;
}

unsigned HTMLFormElement::formElementIndex(FormAssociatedElement* associatedElement)
{
    HTMLElement* element = toHTMLElement(associatedElement);
    // Treats separately the case where this element has the form attribute
    // for performance consideration.
    if (element->fastHasAttribute(formAttr))
        return formElementIndexWithFormAttribute(element);

    // Check for the special case where this element is the very last thing in
    // the form's tree of children; we don't want to walk the entire tree in that
    // common case that occurs during parsing; instead we'll just return a value
    // that says "add this form element to the end of the array".
    if (element->traverseNextNode(this)) {
        unsigned i = m_associatedElementsBeforeIndex;
        for (Node* node = this; node; node = node->traverseNextNode(this)) {
            if (node == element) {
                ++m_associatedElementsAfterIndex;
                return i;
            }
            if (node->isHTMLElement()
                    && (static_cast<Element*>(node)->isFormControlElement()
                        || node->hasTagName(objectTag))
                    && toHTMLElement(node)->form() == this)
                ++i;
        }
    }
    return m_associatedElementsAfterIndex++;
}

void HTMLFormElement::registerFormElement(FormAssociatedElement* e)
{
    if (e->isFormControlElement()) {
        HTMLFormControlElement* element = static_cast<HTMLFormControlElement*>(e);
        document()->checkedRadioButtons().removeButton(element);
        m_checkedRadioButtons.addButton(element);
    }
    m_associatedElements.insert(formElementIndex(e), e);
}

void HTMLFormElement::removeFormElement(FormAssociatedElement* e)
{
    if (e->isFormControlElement())
        m_checkedRadioButtons.removeButton(static_cast<HTMLFormControlElement*>(e));
    unsigned index;
    for (index = 0; index < m_associatedElements.size(); ++index) {
        if (m_associatedElements[index] == e)
            break;
    }
    ASSERT(index < m_associatedElements.size());
    if (index < m_associatedElementsBeforeIndex)
        --m_associatedElementsBeforeIndex;
    if (index < m_associatedElementsAfterIndex)
        --m_associatedElementsAfterIndex;
    removeFromVector(m_associatedElements, e);
}

bool HTMLFormElement::isURLAttribute(Attribute* attr) const
{
    return attr->name() == actionAttr;
}

void HTMLFormElement::registerImgElement(HTMLImageElement* e)
{
    ASSERT(m_imageElements.find(e) == notFound);
    m_imageElements.append(e);
}

void HTMLFormElement::removeImgElement(HTMLImageElement* e)
{
    ASSERT(m_imageElements.find(e) != notFound);
    removeFromVector(m_imageElements, e);
}

PassRefPtr<HTMLCollection> HTMLFormElement::elements()
{
    return HTMLFormCollection::create(this);
}

String HTMLFormElement::name() const
{
    return getAttribute(nameAttr);
}

bool HTMLFormElement::noValidate() const
{
    return fastHasAttribute(novalidateAttr);
}

// FIXME: This function should be removed because it does not do the same thing as the
// JavaScript binding for action, which treats action as a URL attribute. Last time I
// (Darin Adler) removed this, someone added it back, so I am leaving it in for now.
String HTMLFormElement::action() const
{
    return getAttribute(actionAttr);
}

void HTMLFormElement::setAction(const String &value)
{
    setAttribute(actionAttr, value);
}

void HTMLFormElement::setEnctype(const String &value)
{
    setAttribute(enctypeAttr, value);
}

String HTMLFormElement::method() const
{
    return getAttribute(methodAttr);
}

void HTMLFormElement::setMethod(const String &value)
{
    setAttribute(methodAttr, value);
}

String HTMLFormElement::target() const
{
    return getAttribute(targetAttr);
}

bool HTMLFormElement::wasUserSubmitted() const
{
    return m_wasUserSubmitted;
}

HTMLFormControlElement* HTMLFormElement::defaultButton() const
{
    for (unsigned i = 0; i < m_associatedElements.size(); ++i) {
        if (!m_associatedElements[i]->isFormControlElement())
            continue;
        HTMLFormControlElement* control = static_cast<HTMLFormControlElement*>(m_associatedElements[i]);
        if (control->isSuccessfulSubmitButton())
            return control;
    }

    return 0;
}

bool HTMLFormElement::checkValidity()
{
    Vector<RefPtr<FormAssociatedElement> > controls;
    return !checkInvalidControlsAndCollectUnhandled(controls);
}

bool HTMLFormElement::checkInvalidControlsAndCollectUnhandled(Vector<RefPtr<FormAssociatedElement> >& unhandledInvalidControls)
{
    RefPtr<HTMLFormElement> protector(this);
    // Copy m_associatedElements because event handlers called from
    // HTMLFormControlElement::checkValidity() might change m_associatedElements.
    Vector<RefPtr<FormAssociatedElement> > elements;
    elements.reserveCapacity(m_associatedElements.size());
    for (unsigned i = 0; i < m_associatedElements.size(); ++i)
        elements.append(m_associatedElements[i]);
    bool hasInvalidControls = false;
    for (unsigned i = 0; i < elements.size(); ++i) {
        if (elements[i]->form() == this && elements[i]->isFormControlElement()) {
            HTMLFormControlElement* control = static_cast<HTMLFormControlElement*>(elements[i].get());
            if (!control->checkValidity(&unhandledInvalidControls) && control->form() == this)
                hasInvalidControls = true;
        }
    }
    return hasInvalidControls;
}

HTMLFormControlElement* HTMLFormElement::elementForAlias(const AtomicString& alias)
{
    if (alias.isEmpty() || !m_elementAliases)
        return 0;
    return m_elementAliases->get(alias.impl()).get();
}

void HTMLFormElement::addElementAlias(HTMLFormControlElement* element, const AtomicString& alias)
{
    if (alias.isEmpty())
        return;
    if (!m_elementAliases)
        m_elementAliases = adoptPtr(new AliasMap);
    m_elementAliases->set(alias.impl(), element);
}

void HTMLFormElement::getNamedElements(const AtomicString& name, Vector<RefPtr<Node> >& namedItems)
{
    elements()->namedItems(name, namedItems);

    HTMLFormControlElement* aliasElement = elementForAlias(name);
    if (aliasElement) {
        if (namedItems.find(aliasElement) == notFound) {
            // We have seen it before but it is gone now. Still, we need to return it.
            // FIXME: The above comment is not clear enough; it does not say why we need to do this.
            namedItems.append(aliasElement);
        }
    }
    if (namedItems.size() && namedItems.first() != aliasElement)
        addElementAlias(static_cast<HTMLFormControlElement*>(namedItems.first().get()), name);
}

void HTMLFormElement::documentDidBecomeActive()
{
    ASSERT(!autoComplete());

    for (unsigned i = 0; i < m_associatedElements.size(); ++i) {
        if (m_associatedElements[i]->isFormControlElement())
            static_cast<HTMLFormControlElement*>(m_associatedElements[i])->reset();
    }
}

void HTMLFormElement::willMoveToNewOwnerDocument()
{
    if (!autoComplete())
        document()->unregisterForDocumentActivationCallbacks(this);
    HTMLElement::willMoveToNewOwnerDocument();
}

void HTMLFormElement::didMoveToNewOwnerDocument()
{
    if (!autoComplete())
        document()->registerForDocumentActivationCallbacks(this);
    HTMLElement::didMoveToNewOwnerDocument();
}

bool HTMLFormElement::autoComplete() const
{
    return !equalIgnoringCase(fastGetAttribute(autocompleteAttr), "off");
}

} // namespace
