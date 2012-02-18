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
#include "HTMLFormControlElement.h"

#include "Attribute.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Document.h"
#include "DocumentParser.h"
#include "ElementRareData.h"
#include "Event.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "Frame.h"
#include "HTMLFormElement.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "LabelsNodeList.h"
#include "Page.h"
#include "RenderBox.h"
#include "RenderTextControl.h"
#include "RenderTheme.h"
#include "ScriptEventListener.h"
#include "ValidationMessage.h"
#include "ValidityState.h"
#include <limits>
#include <wtf/Vector.h>
#include <wtf/unicode/CharacterNames.h>

namespace WebCore {

using namespace HTMLNames;
using namespace std;

HTMLFormControlElement::HTMLFormControlElement(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
    : HTMLElement(tagName, document)
    , FormAssociatedElement(form)
    , m_disabled(false)
    , m_readOnly(false)
    , m_required(false)
    , m_valueMatchesRenderer(false)
    , m_willValidateInitialized(false)
    , m_willValidate(true)
    , m_isValid(true)
    , m_wasChangedSinceLastFormControlChangeEvent(false)
{
    if (!this->form())
        setForm(findFormAncestor());
    if (this->form())
        this->form()->registerFormElement(this);
}

HTMLFormControlElement::~HTMLFormControlElement()
{
    if (form())
        form()->removeFormElement(this);
}

void HTMLFormControlElement::detach()
{
    m_validationMessage = nullptr;
    HTMLElement::detach();
}

bool HTMLFormControlElement::formNoValidate() const
{
    return fastHasAttribute(formnovalidateAttr);
}

void HTMLFormControlElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == disabledAttr) {
        bool oldDisabled = m_disabled;
        m_disabled = !attr->isNull();
        if (oldDisabled != m_disabled) {
            setNeedsStyleRecalc();
            if (renderer() && renderer()->style()->hasAppearance())
                renderer()->theme()->stateChanged(renderer(), EnabledState);
        }
    } else if (attr->name() == readonlyAttr) {
        bool oldReadOnly = m_readOnly;
        m_readOnly = !attr->isNull();
        if (oldReadOnly != m_readOnly) {
            setNeedsStyleRecalc();
            if (renderer() && renderer()->style()->hasAppearance())
                renderer()->theme()->stateChanged(renderer(), ReadOnlyState);
        }
    } else if (attr->name() == requiredAttr) {
        bool oldRequired = m_required;
        m_required = !attr->isNull();
        if (oldRequired != m_required) {
            setNeedsValidityCheck();
            setNeedsStyleRecalc(); // Updates for :required :optional classes.
        }
    } else
        HTMLElement::parseMappedAttribute(attr);
    setNeedsWillValidateCheck();
}

static bool shouldAutofocus(HTMLFormControlElement* element)
{
    if (!element->autofocus())
        return false;
    if (!element->renderer())
        return false;
    if (element->document()->ignoreAutofocus())
        return false;
    if (element->isReadOnlyFormControl())
        return false;

    // FIXME: Should this set of hasTagName checks be replaced by a
    // virtual member function?
    if (element->hasTagName(inputTag))
        return !static_cast<HTMLInputElement*>(element)->isInputTypeHidden();
    if (element->hasTagName(selectTag))
        return true;
    if (element->hasTagName(keygenTag))
        return true;
    if (element->hasTagName(buttonTag))
        return true;
    if (element->hasTagName(textareaTag))
        return true;

    return false;
}

static void focusPostAttach(Node* element) 
{ 
    static_cast<Element*>(element)->focus(); 
    element->deref(); 
}

void HTMLFormControlElement::attach()
{
    ASSERT(!attached());

    suspendPostAttachCallbacks();

    HTMLElement::attach();

    // The call to updateFromElement() needs to go after the call through
    // to the base class's attach() because that can sometimes do a close
    // on the renderer.
    if (renderer())
        renderer()->updateFromElement();

    if (shouldAutofocus(this)) {
        ref();
        queuePostAttachCallback(focusPostAttach, this);
    }

    resumePostAttachCallbacks();
}

void HTMLFormControlElement::willMoveToNewOwnerDocument()
{
    FormAssociatedElement::willMoveToNewOwnerDocument();
    HTMLElement::willMoveToNewOwnerDocument();
}

void HTMLFormControlElement::insertedIntoTree(bool deep)
{
    FormAssociatedElement::insertedIntoTree();
    if (!form())
        document()->checkedRadioButtons().addButton(this);

    HTMLElement::insertedIntoTree(deep);
}

void HTMLFormControlElement::removedFromTree(bool deep)
{
    FormAssociatedElement::removedFromTree();
    HTMLElement::removedFromTree(deep);
}

void HTMLFormControlElement::insertedIntoDocument()
{
    HTMLElement::insertedIntoDocument();
    FormAssociatedElement::insertedIntoDocument();
}

void HTMLFormControlElement::removedFromDocument()
{
    HTMLElement::removedFromDocument();
    FormAssociatedElement::removedFromDocument();
}

const AtomicString& HTMLFormControlElement::formControlName() const
{
    const AtomicString& name = fastGetAttribute(nameAttr);
    return name.isNull() ? emptyAtom : name;
}

void HTMLFormControlElement::setName(const AtomicString& value)
{
    setAttribute(nameAttr, value);
}

bool HTMLFormControlElement::wasChangedSinceLastFormControlChangeEvent() const
{
    return m_wasChangedSinceLastFormControlChangeEvent;
}

void HTMLFormControlElement::setChangedSinceLastFormControlChangeEvent(bool changed)
{
    m_wasChangedSinceLastFormControlChangeEvent = changed;
}

void HTMLFormControlElement::dispatchFormControlChangeEvent()
{
    HTMLElement::dispatchChangeEvent();
    setChangedSinceLastFormControlChangeEvent(false);
}

void HTMLFormControlElement::dispatchFormControlInputEvent()
{
    setChangedSinceLastFormControlChangeEvent(true);
    HTMLElement::dispatchInputEvent();
}

void HTMLFormControlElement::setDisabled(bool b)
{
    setAttribute(disabledAttr, b ? "" : 0);
}

bool HTMLFormControlElement::autofocus() const
{
    return hasAttribute(autofocusAttr);
}

bool HTMLFormControlElement::required() const
{
    return m_required;
}

static void updateFromElementCallback(Node* node)
{
    ASSERT_ARG(node, node->isElementNode());
    ASSERT_ARG(node, static_cast<Element*>(node)->isFormControlElement());
    ASSERT(node->renderer());
    if (RenderObject* renderer = node->renderer())
        renderer->updateFromElement();
}

void HTMLFormControlElement::recalcStyle(StyleChange change)
{
    HTMLElement::recalcStyle(change);

    // updateFromElement() can cause the selection to change, and in turn
    // trigger synchronous layout, so it must not be called during style recalc.
    if (renderer())
        queuePostAttachCallback(updateFromElementCallback, this);
}

bool HTMLFormControlElement::supportsFocus() const
{
    return !m_disabled;
}

bool HTMLFormControlElement::isFocusable() const
{
    if (!renderer() || 
        !renderer()->isBox() || toRenderBox(renderer())->size().isEmpty())
        return false;
    // HTMLElement::isFocusable handles visibility and calls suportsFocus which
    // will cover the disabled case.
    return HTMLElement::isFocusable();
}

bool HTMLFormControlElement::isKeyboardFocusable(KeyboardEvent* event) const
{
    if (isFocusable())
        if (document()->frame())
            return document()->frame()->eventHandler()->tabsToAllFormControls(event);
    return false;
}

bool HTMLFormControlElement::isMouseFocusable() const
{
#if PLATFORM(GTK) || PLATFORM(QT)
    return HTMLElement::isMouseFocusable();
#else
    return false;
#endif
}

short HTMLFormControlElement::tabIndex() const
{
    // Skip the supportsFocus check in HTMLElement.
    return Element::tabIndex();
}

bool HTMLFormControlElement::recalcWillValidate() const
{
    // FIXME: Should return false if this element has a datalist element as an
    // ancestor. See HTML5 4.10.10 'The datalist element.'
    return !m_disabled && !m_readOnly;
}

bool HTMLFormControlElement::willValidate() const
{
    if (!m_willValidateInitialized) {
        m_willValidateInitialized = true;
        m_willValidate = recalcWillValidate();
    } else {
        // If the following assertion fails, setNeedsWillValidateCheck() is not
        // called correctly when something which changes recalcWillValidate() result
        // is updated.
        ASSERT(m_willValidate == recalcWillValidate());
    }
    return m_willValidate;
}

void HTMLFormControlElement::setNeedsWillValidateCheck()
{
    // We need to recalculate willValidte immediately because willValidate
    // change can causes style change.
    bool newWillValidate = recalcWillValidate();
    if (m_willValidateInitialized && m_willValidate == newWillValidate)
        return;
    m_willValidateInitialized = true;
    m_willValidate = newWillValidate;
    setNeedsStyleRecalc();
    if (!m_willValidate)
        hideVisibleValidationMessage();
}

String HTMLFormControlElement::validationMessage()
{
    return validity()->validationMessage();
}

void HTMLFormControlElement::updateVisibleValidationMessage()
{
    Page* page = document()->page();
    if (!page)
        return;
    String message;
    if (renderer() && willValidate()) {
        message = validationMessage().stripWhiteSpace();
        // HTML5 specification doesn't ask UA to show the title attribute value
        // with the validationMessage.  However, this behavior is same as Opera
        // and the specification describes such behavior as an example.
        const AtomicString& title = getAttribute(titleAttr);
        if (!message.isEmpty() && !title.isEmpty()) {
            message.append('\n');
            message.append(title);
        }
    }
    if (message.isEmpty()) {
        hideVisibleValidationMessage();
        return;
    }
    if (!m_validationMessage) {
        m_validationMessage = ValidationMessage::create(this);
        m_validationMessage->setMessage(message);
    } else {
        // Call setMessage() even if m_validationMesage->message() == message
        // because the existing message might be to be hidden.
        m_validationMessage->setMessage(message);
    }
}

void HTMLFormControlElement::hideVisibleValidationMessage()
{
    if (m_validationMessage)
        m_validationMessage->requestToHideMessage();
}

String HTMLFormControlElement::visibleValidationMessage() const
{
    return m_validationMessage ? m_validationMessage->message() : String();
}

bool HTMLFormControlElement::checkValidity(Vector<RefPtr<FormAssociatedElement> >* unhandledInvalidControls)
{
    if (!willValidate() || isValidFormControlElement())
        return true;
    // An event handler can deref this object.
    RefPtr<HTMLFormControlElement> protector(this);
    RefPtr<Document> originalDocument(document());
    bool needsDefaultAction = dispatchEvent(Event::create(eventNames().invalidEvent, false, true));
    if (needsDefaultAction && unhandledInvalidControls && inDocument() && originalDocument == document())
        unhandledInvalidControls->append(this);
    return false;
}

bool HTMLFormControlElement::isValidFormControlElement()
{
    // If the following assertion fails, setNeedsValidityCheck() is not called
    // correctly when something which changes validity is updated.
    ASSERT(m_isValid == validity()->valid());
    return m_isValid;
}

void HTMLFormControlElement::setNeedsValidityCheck()
{
    bool newIsValid = validity()->valid();
    if (willValidate() && newIsValid != m_isValid) {
        // Update style for pseudo classes such as :valid :invalid.
        setNeedsStyleRecalc();
    }
    m_isValid = newIsValid;

    // Updates only if this control already has a validtion message.
    if (!visibleValidationMessage().isEmpty()) {
        // Calls updateVisibleValidationMessage() even if m_isValid is not
        // changed because a validation message can be chagned.
        updateVisibleValidationMessage();
    }
}

void HTMLFormControlElement::setCustomValidity(const String& error)
{
    validity()->setCustomErrorMessage(error);
}

void HTMLFormControlElement::dispatchFocusEvent()
{
    if (document()->page())
        document()->page()->chrome()->client()->formDidFocus(this);

    HTMLElement::dispatchFocusEvent();
}

void HTMLFormControlElement::dispatchBlurEvent()
{
    if (document()->page())
        document()->page()->chrome()->client()->formDidBlur(this);

    HTMLElement::dispatchBlurEvent();
    hideVisibleValidationMessage();
}

HTMLFormElement* HTMLFormControlElement::virtualForm() const
{
    return FormAssociatedElement::form();
}

bool HTMLFormControlElement::isDefaultButtonForForm() const
{
    return isSuccessfulSubmitButton() && form() && form()->defaultButton() == this;
}

void HTMLFormControlElement::attributeChanged(Attribute* attr, bool preserveDecls)
{
    if (attr->name() == formAttr) {
        formAttributeChanged();
        if (!form())
            document()->checkedRadioButtons().addButton(this);
    } else
        HTMLElement::attributeChanged(attr, preserveDecls);
}

bool HTMLFormControlElement::isLabelable() const
{
    // FIXME: Add meterTag and outputTag to the list once we support them.
    return hasTagName(buttonTag) || hasTagName(inputTag) || hasTagName(keygenTag)
#if ENABLE(METER_TAG)
        || hasTagName(meterTag)
#endif
#if ENABLE(PROGRESS_TAG)
        || hasTagName(progressTag)
#endif
        || hasTagName(selectTag) || hasTagName(textareaTag);
}

PassRefPtr<NodeList> HTMLFormControlElement::labels()
{
    if (!isLabelable())
        return 0;
    if (!document())
        return 0;

    NodeRareData* data = Node::ensureRareData();
    if (!data->nodeLists()) {
        data->setNodeLists(NodeListsNodeData::create());
        document()->addNodeListCache();
    }

    if (data->nodeLists()->m_labelsNodeListCache)
        return data->nodeLists()->m_labelsNodeListCache;

    RefPtr<LabelsNodeList> list = LabelsNodeList::create(this);
    data->nodeLists()->m_labelsNodeListCache = list.get();
    return list.release();
}

HTMLFormControlElementWithState::HTMLFormControlElementWithState(const QualifiedName& tagName, Document* doc, HTMLFormElement* f)
    : HTMLFormControlElement(tagName, doc, f)
{
    document()->registerFormElementWithState(this);
}

HTMLFormControlElementWithState::~HTMLFormControlElementWithState()
{
    document()->unregisterFormElementWithState(this);
}

void HTMLFormControlElementWithState::willMoveToNewOwnerDocument()
{
    document()->unregisterFormElementWithState(this);
    HTMLFormControlElement::willMoveToNewOwnerDocument();
}

void HTMLFormControlElementWithState::didMoveToNewOwnerDocument()
{
    document()->registerFormElementWithState(this);
    HTMLFormControlElement::didMoveToNewOwnerDocument();
}

bool HTMLFormControlElementWithState::autoComplete() const
{
    if (!form())
        return true;
    return form()->autoComplete();
}

bool HTMLFormControlElementWithState::shouldSaveAndRestoreFormControlState() const
{
    // We don't save/restore control state in a form with autocomplete=off.
    return attached() && autoComplete();
}

void HTMLFormControlElementWithState::finishParsingChildren()
{
    HTMLFormControlElement::finishParsingChildren();

    // We don't save state of a control with shouldSaveAndRestoreFormControlState()=false.
    // But we need to skip restoring process too because a control in another
    // form might have the same pair of name and type and saved its state.
    if (!shouldSaveAndRestoreFormControlState())
        return;

    Document* doc = document();
    if (doc->hasStateForNewFormElements()) {
        String state;
        if (doc->takeStateForFormElement(name().impl(), type().impl(), state))
            restoreFormControlState(state);
    }
}

void HTMLFormControlElementWithState::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().webkitEditableContentChangedEvent && renderer() && renderer()->isTextControl()) {
        toRenderTextControl(renderer())->subtreeHasChanged();
        return;
    }

    HTMLFormControlElement::defaultEventHandler(event);
}

HTMLTextFormControlElement::HTMLTextFormControlElement(const QualifiedName& tagName, Document* doc, HTMLFormElement* form)
    : HTMLFormControlElementWithState(tagName, doc, form)
{
}

HTMLTextFormControlElement::~HTMLTextFormControlElement()
{
}

void HTMLTextFormControlElement::insertedIntoDocument()
{
    HTMLFormControlElement::insertedIntoDocument();
    String initialValue = value();
    setTextAsOfLastFormControlChangeEvent(initialValue.isNull() ? String("") : initialValue);
}

void HTMLTextFormControlElement::dispatchFocusEvent()
{
    if (supportsPlaceholder())
        updatePlaceholderVisibility(false);
    handleFocusEvent();
    HTMLFormControlElementWithState::dispatchFocusEvent();
}

void HTMLTextFormControlElement::dispatchBlurEvent()
{
    if (supportsPlaceholder())
        updatePlaceholderVisibility(false);
    handleBlurEvent();
    HTMLFormControlElementWithState::dispatchBlurEvent();
}

String HTMLTextFormControlElement::strippedPlaceholder() const
{
    // According to the HTML5 specification, we need to remove CR and LF from
    // the attribute value.
    const AtomicString& attributeValue = getAttribute(placeholderAttr);
    if (!attributeValue.contains(newlineCharacter) && !attributeValue.contains(carriageReturn))
        return attributeValue;

    Vector<UChar> stripped;
    unsigned length = attributeValue.length();
    stripped.reserveCapacity(length);
    for (unsigned i = 0; i < length; ++i) {
        UChar character = attributeValue[i];
        if (character == newlineCharacter || character == carriageReturn)
            continue;
        stripped.append(character);
    }
    return String::adopt(stripped);
}

static bool isNotLineBreak(UChar ch) { return ch != newlineCharacter && ch != carriageReturn; }

bool HTMLTextFormControlElement::isPlaceholderEmpty() const
{
    const AtomicString& attributeValue = getAttribute(placeholderAttr);
    return attributeValue.string().find(isNotLineBreak) == notFound;
}

bool HTMLTextFormControlElement::placeholderShouldBeVisible() const
{
    return supportsPlaceholder()
        && isEmptyValue()
        && isEmptySuggestedValue()
        && !isPlaceholderEmpty()
        && (document()->focusedNode() != this || (renderer() && renderer()->theme()->shouldShowPlaceholderWhenFocused()));
}

void HTMLTextFormControlElement::updatePlaceholderVisibility(bool placeholderValueChanged)
{
    if (supportsPlaceholder() && renderer())
        toRenderTextControl(renderer())->updatePlaceholderVisibility(placeholderShouldBeVisible(), placeholderValueChanged);
}

RenderTextControl* HTMLTextFormControlElement::textRendererAfterUpdateLayout()
{
    if (!isTextFormControl())
        return 0;
    document()->updateLayoutIgnorePendingStylesheets();
    return toRenderTextControl(renderer());
}

void HTMLTextFormControlElement::setSelectionStart(int start)
{
    setSelectionRange(start, max(start, selectionEnd()));
}

void HTMLTextFormControlElement::setSelectionEnd(int end)
{
    setSelectionRange(min(end, selectionStart()), end);
}

void HTMLTextFormControlElement::select()
{
    setSelectionRange(0, numeric_limits<int>::max());
}

void HTMLTextFormControlElement::dispatchFormControlChangeEvent()
{
    if (m_textAsOfLastFormControlChangeEvent != value()) {
        HTMLElement::dispatchChangeEvent();
        setTextAsOfLastFormControlChangeEvent(value());
    }
    setChangedSinceLastFormControlChangeEvent(false);
}

void HTMLTextFormControlElement::setSelectionRange(int start, int end)
{
    WebCore::setSelectionRange(this, start, end);
}

int HTMLTextFormControlElement::selectionStart() const
{
    if (!isTextFormControl())
        return 0;
    if (document()->focusedNode() != this && cachedSelectionStart() >= 0)
        return cachedSelectionStart();
    if (!renderer())
        return 0;
    return toRenderTextControl(renderer())->selectionStart();
}

int HTMLTextFormControlElement::selectionEnd() const
{
    if (!isTextFormControl())
        return 0;
    if (document()->focusedNode() != this && cachedSelectionEnd() >= 0)
        return cachedSelectionEnd();
    if (!renderer())
        return 0;
    return toRenderTextControl(renderer())->selectionEnd();
}

PassRefPtr<Range> HTMLTextFormControlElement::selection() const
{
    if (!renderer() || !isTextFormControl() || cachedSelectionStart() < 0 || cachedSelectionEnd() < 0)
        return 0;
    return toRenderTextControl(renderer())->selection(cachedSelectionStart(), cachedSelectionEnd());
}

void HTMLTextFormControlElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == placeholderAttr)
        updatePlaceholderVisibility(true);
    else if (attr->name() == onselectAttr)
        setAttributeEventListener(eventNames().selectEvent, createAttributeEventListener(this, attr));
    else if (attr->name() == onchangeAttr)
        setAttributeEventListener(eventNames().changeEvent, createAttributeEventListener(this, attr));
    else
        HTMLFormControlElementWithState::parseMappedAttribute(attr);
}

} // namespace Webcore
