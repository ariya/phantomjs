/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 * Copyright (C) 2007 Samuel Weinig (sam@webkit.org)
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "HTMLInputElement.h"

#include "AXObjectCache.h"
#include "Attribute.h"
#include "BeforeTextInsertedEvent.h"
#include "CSSPropertyNames.h"
#include "Document.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "FileList.h"
#include "HTMLCollection.h"
#include "HTMLDataListElement.h"
#include "HTMLFormElement.h"
#include "HTMLNames.h"
#include "HTMLOptionElement.h"
#include "HTMLParserIdioms.h"
#include "InputType.h"
#include "KeyboardEvent.h"
#include "LocalizedStrings.h"
#include "MouseEvent.h"
#include "PlatformMouseEvent.h"
#include "RenderTextControlSingleLine.h"
#include "RenderTheme.h"
#include "RuntimeEnabledFeatures.h"
#include "ScriptEventListener.h"
#include "WheelEvent.h"
#include <wtf/MathExtras.h>
#include <wtf/StdLibExtras.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

const int maxSavedResults = 256;

HTMLInputElement::HTMLInputElement(const QualifiedName& tagName, Document* document, HTMLFormElement* form, bool createdByParser)
    : HTMLTextFormControlElement(tagName, document, form)
    , m_maxResults(-1)
    , m_isChecked(false)
    , m_reflectsCheckedAttribute(true)
    , m_isIndeterminate(false)
    , m_hasType(false)
    , m_isActivatedSubmit(false)
    , m_autocomplete(Uninitialized)
    , m_isAutofilled(false)
    , m_stateRestored(false)
    , m_parsingInProgress(createdByParser)
    , m_inputType(InputType::createText(this))
{
    ASSERT(hasTagName(inputTag) || hasTagName(isindexTag));
}

PassRefPtr<HTMLInputElement> HTMLInputElement::create(const QualifiedName& tagName, Document* document, HTMLFormElement* form, bool createdByParser)
{
    return adoptRef(new HTMLInputElement(tagName, document, form, createdByParser));
}

HTMLInputElement::~HTMLInputElement()
{
    if (needsActivationCallback())
        document()->unregisterForDocumentActivationCallbacks(this);

    document()->checkedRadioButtons().removeButton(this);

    // Need to remove this from the form while it is still an HTMLInputElement,
    // so can't wait for the base class's destructor to do it.
    removeFromForm();
}

const AtomicString& HTMLInputElement::formControlName() const
{
    return m_data.name();
}

bool HTMLInputElement::autoComplete() const
{
    if (m_autocomplete != Uninitialized)
        return m_autocomplete == On;
    return HTMLTextFormControlElement::autoComplete();
}

void HTMLInputElement::updateCheckedRadioButtons()
{
    if (attached() && checked())
        checkedRadioButtons().addButton(this);

    if (form()) {
        const Vector<FormAssociatedElement*>& controls = form()->associatedElements();
        for (unsigned i = 0; i < controls.size(); ++i) {
            if (!controls[i]->isFormControlElement())
                continue;
            HTMLFormControlElement* control = static_cast<HTMLFormControlElement*>(controls[i]);
            if (control->name() != name())
                continue;
            if (control->type() != type())
                continue;
            control->setNeedsValidityCheck();
        }
    } else {
        // FIXME: Traversing the document is inefficient.
        for (Node* node = document()->body(); node; node = node->traverseNextNode()) {
            if (!node->isElementNode())
                continue;
            Element* element = static_cast<Element*>(node);
            if (element->formControlName() != name())
                continue;
            if (element->formControlType() != type())
                continue;
            HTMLFormControlElement* control = static_cast<HTMLFormControlElement*>(element);
            if (control->form())
                continue;
            control->setNeedsValidityCheck();
        }
    }

    if (renderer() && renderer()->style()->hasAppearance())
        renderer()->theme()->stateChanged(renderer(), CheckedState);
}

bool HTMLInputElement::lastChangeWasUserEdit() const
{
    if (!isTextField())
        return false;
    
    if (!renderer())
        return false;

    return toRenderTextControl(renderer())->lastChangeWasUserEdit();
}

bool HTMLInputElement::isValidValue(const String& value) const
{
    if (!m_inputType->canSetStringValue()) {
        ASSERT_NOT_REACHED();
        return false;
    }
    return !m_inputType->typeMismatchFor(value)
        && !stepMismatch(value)
        && !rangeUnderflow(value)
        && !rangeOverflow(value)
        && !tooLong(value, IgnoreDirtyFlag)
        && !patternMismatch(value)
        && !valueMissing(value);
}

bool HTMLInputElement::typeMismatch() const
{
    return m_inputType->typeMismatch();
}

bool HTMLInputElement::valueMissing(const String& value) const
{
    if (!isRequiredFormControl() || readOnly() || disabled())
        return false;
    return m_inputType->valueMissing(value);
}

bool HTMLInputElement::patternMismatch(const String& value) const
{
    return m_inputType->patternMismatch(value);
}

bool HTMLInputElement::tooLong(const String& value, NeedsToCheckDirtyFlag check) const
{
    // We use isTextType() instead of supportsMaxLength() because of the
    // 'virtual' overhead.
    if (!isTextType())
        return false;
    int max = maxLength();
    if (max < 0)
        return false;
    if (check == CheckDirtyFlag) {
        // Return false for the default value even if it is longer than maxLength.
        bool userEdited = !m_data.value().isNull();
        if (!userEdited)
            return false;
    }
    return numGraphemeClusters(value) > static_cast<unsigned>(max);
}

bool HTMLInputElement::rangeUnderflow(const String& value) const
{
    return m_inputType->rangeUnderflow(value);
}

bool HTMLInputElement::rangeOverflow(const String& value) const
{
    return m_inputType->rangeOverflow(value);
}

double HTMLInputElement::minimum() const
{
    return m_inputType->minimum();
}

double HTMLInputElement::maximum() const
{
    return m_inputType->maximum();
}

bool HTMLInputElement::stepMismatch(const String& value) const
{
    double step;
    if (!getAllowedValueStep(&step))
        return false;
    return m_inputType->stepMismatch(value, step);
}

String HTMLInputElement::minimumString() const
{
    return m_inputType->serialize(minimum());
}

String HTMLInputElement::maximumString() const
{
    return m_inputType->serialize(maximum());
}

String HTMLInputElement::stepBaseString() const
{
    return m_inputType->serialize(m_inputType->stepBase());
}

String HTMLInputElement::stepString() const
{
    double step;
    if (!getAllowedValueStep(&step)) {
        // stepString() should be called only if stepMismatch() can be true.
        ASSERT_NOT_REACHED();
        return String();
    }
    return serializeForNumberType(step / m_inputType->stepScaleFactor());
}

String HTMLInputElement::typeMismatchText() const
{
    return m_inputType->typeMismatchText();
}

String HTMLInputElement::valueMissingText() const
{
    return m_inputType->valueMissingText();
}

bool HTMLInputElement::getAllowedValueStep(double* step) const
{
    return getAllowedValueStepWithDecimalPlaces(step, 0);
}

bool HTMLInputElement::getAllowedValueStepWithDecimalPlaces(double* step, unsigned* decimalPlaces) const
{
    ASSERT(step);
    double defaultStep = m_inputType->defaultStep();
    double stepScaleFactor = m_inputType->stepScaleFactor();
    if (!isfinite(defaultStep) || !isfinite(stepScaleFactor))
        return false;
    const AtomicString& stepString = fastGetAttribute(stepAttr);
    if (stepString.isEmpty()) {
        *step = defaultStep * stepScaleFactor;
        if (decimalPlaces)
            *decimalPlaces = 0;
        return true;
    }
    if (equalIgnoringCase(stepString, "any"))
        return false;
    double parsed;
    if (!decimalPlaces) {
        if (!parseToDoubleForNumberType(stepString, &parsed) || parsed <= 0.0) {
            *step = defaultStep * stepScaleFactor;
            return true;
        }
    } else {
        if (!parseToDoubleForNumberTypeWithDecimalPlaces(stepString, &parsed, decimalPlaces) || parsed <= 0.0) {
            *step = defaultStep * stepScaleFactor;
            *decimalPlaces = 0;
            return true;
        }
    }
    // For date, month, week, the parsed value should be an integer for some types.
    if (m_inputType->parsedStepValueShouldBeInteger())
        parsed = max(round(parsed), 1.0);
    double result = parsed * stepScaleFactor;
    // For datetime, datetime-local, time, the result should be an integer.
    if (m_inputType->scaledStepValueShouldBeInteger())
        result = max(round(result), 1.0);
    ASSERT(result > 0);
    *step = result;
    return true;
}

void HTMLInputElement::applyStep(double count, ExceptionCode& ec)
{
    double step;
    unsigned stepDecimalPlaces, currentDecimalPlaces;
    if (!getAllowedValueStepWithDecimalPlaces(&step, &stepDecimalPlaces)) {
        ec = INVALID_STATE_ERR;
        return;
    }
    const double nan = numeric_limits<double>::quiet_NaN();
    double current = m_inputType->parseToDoubleWithDecimalPlaces(value(), nan, &currentDecimalPlaces);
    if (!isfinite(current)) {
        ec = INVALID_STATE_ERR;
        return;
    }
    double newValue = current + step * count;
    if (isinf(newValue)) {
        ec = INVALID_STATE_ERR;
        return;
    }
    double acceptableError = m_inputType->acceptableError(step);
    if (newValue - m_inputType->minimum() < -acceptableError) {
        ec = INVALID_STATE_ERR;
        return;
    }
    if (newValue < m_inputType->minimum())
        newValue = m_inputType->minimum();
    unsigned baseDecimalPlaces;
    double base = m_inputType->stepBaseWithDecimalPlaces(&baseDecimalPlaces);
    baseDecimalPlaces = min(baseDecimalPlaces, 16u);
    if (newValue < pow(10.0, 21.0)) {
      if (stepMismatch(value())) {
            double scale = pow(10.0, static_cast<double>(max(stepDecimalPlaces, currentDecimalPlaces)));
            newValue = round(newValue * scale) / scale;
        } else {
            double scale = pow(10.0, static_cast<double>(max(stepDecimalPlaces, baseDecimalPlaces)));
            newValue = round((base + round((newValue - base) / step) * step) * scale) / scale;
        }
    }
    if (newValue - m_inputType->maximum() > acceptableError) {
        ec = INVALID_STATE_ERR;
        return;
    }
    if (newValue > m_inputType->maximum())
        newValue = m_inputType->maximum();
    setValueAsNumber(newValue, ec);

    if (AXObjectCache::accessibilityEnabled())
         document()->axObjectCache()->postNotification(renderer(), AXObjectCache::AXValueChanged, true);
}

void HTMLInputElement::stepUp(int n, ExceptionCode& ec)
{
    applyStep(n, ec);
}

void HTMLInputElement::stepDown(int n, ExceptionCode& ec)
{
    applyStep(-n, ec);
}

bool HTMLInputElement::isKeyboardFocusable(KeyboardEvent* event) const
{
    if (isTextField())
        return HTMLFormControlElementWithState::isFocusable();
    return HTMLFormControlElementWithState::isKeyboardFocusable(event) && m_inputType->isKeyboardFocusable();
}

bool HTMLInputElement::isMouseFocusable() const
{
    if (isTextField())
        return HTMLFormControlElementWithState::isFocusable();
    return HTMLFormControlElementWithState::isMouseFocusable();
}

void HTMLInputElement::updateFocusAppearance(bool restorePreviousSelection)
{
    if (isTextField())
        InputElement::updateFocusAppearance(m_data, this, this, restorePreviousSelection);
    else
        HTMLFormControlElementWithState::updateFocusAppearance(restorePreviousSelection);
}

void HTMLInputElement::aboutToUnload()
{
    InputElement::aboutToUnload(this, this);
}

bool HTMLInputElement::shouldUseInputMethod() const
{
    return m_inputType->shouldUseInputMethod();
}

void HTMLInputElement::handleFocusEvent()
{
    InputElement::dispatchFocusEvent(this, this);
}

void HTMLInputElement::handleBlurEvent()
{
    m_inputType->handleBlurEvent();
    InputElement::dispatchBlurEvent(this, this);
}

void HTMLInputElement::setType(const String& type)
{
    // FIXME: This should just call setAttribute. No reason to handle the empty string specially.
    // We should write a test case to show that setting to the empty string does not remove the
    // attribute in other browsers and then fix this. Note that setting to null *does* remove
    // the attribute and setAttribute implements that.
    if (type.isEmpty()) {
        ExceptionCode ec;
        removeAttribute(typeAttr, ec);
    } else
        setAttribute(typeAttr, type);
}

void HTMLInputElement::updateType()
{
    OwnPtr<InputType> newType = InputType::create(this, fastGetAttribute(typeAttr));
    bool hadType = m_hasType;
    m_hasType = true;
    if (m_inputType->formControlType() == newType->formControlType())
        return;

    if (hadType && !newType->canChangeFromAnotherType()) {
        // Set the attribute back to the old value.
        // Useful in case we were called from inside parseMappedAttribute.
        setAttribute(typeAttr, type());
        return;
    }

    checkedRadioButtons().removeButton(this);

    bool wasAttached = attached();
    if (wasAttached)
        detach();

    bool didStoreValue = m_inputType->storesValueSeparateFromAttribute();
    bool neededActivationCallback = needsActivationCallback();
    bool didRespectHeightAndWidth = m_inputType->shouldRespectHeightAndWidthAttributes();

    m_inputType->destroyShadowSubtree();
    m_inputType = newType.release();
    m_inputType->createShadowSubtree();

    setNeedsWillValidateCheck();

    bool willStoreValue = m_inputType->storesValueSeparateFromAttribute();

    if (didStoreValue && !willStoreValue && !m_data.value().isNull()) {
        setAttribute(valueAttr, m_data.value());
        m_data.setValue(String());
    }
    if (!didStoreValue && willStoreValue)
        m_data.setValue(sanitizeValue(fastGetAttribute(valueAttr)));
    else
        InputElement::updateValueIfNeeded(m_data, this);

    if (neededActivationCallback)
        unregisterForActivationCallbackIfNeeded();
    else
        registerForActivationCallbackIfNeeded();

    if (didRespectHeightAndWidth != m_inputType->shouldRespectHeightAndWidthAttributes()) {
        NamedNodeMap* map = attributeMap();
        ASSERT(map);
        if (Attribute* height = map->getAttributeItem(heightAttr))
            attributeChanged(height, false);
        if (Attribute* width = map->getAttributeItem(widthAttr))
            attributeChanged(width, false);
        if (Attribute* align = map->getAttributeItem(alignAttr))
            attributeChanged(align, false);
    }

    if (wasAttached) {
        attach();
        if (document()->focusedNode() == this)
            updateFocusAppearance(true);
    }

    setChangedSinceLastFormControlChangeEvent(false);

    checkedRadioButtons().addButton(this);

    setNeedsValidityCheck();
    InputElement::notifyFormStateChanged(this);
}

const AtomicString& HTMLInputElement::formControlType() const
{
    return m_inputType->formControlType();
}

bool HTMLInputElement::saveFormControlState(String& result) const
{
    return m_inputType->saveFormControlState(result);
}

void HTMLInputElement::restoreFormControlState(const String& state)
{
    m_inputType->restoreFormControlState(state);
    m_stateRestored = true;
}

bool HTMLInputElement::canStartSelection() const
{
    if (!isTextField())
        return false;
    return HTMLFormControlElementWithState::canStartSelection();
}

bool HTMLInputElement::canHaveSelection() const
{
    return isTextField();
}

void HTMLInputElement::accessKeyAction(bool sendToAnyElement)
{
    m_inputType->accessKeyAction(sendToAnyElement);
}

bool HTMLInputElement::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (((attrName == heightAttr || attrName == widthAttr) && m_inputType->shouldRespectHeightAndWidthAttributes())
        || attrName == vspaceAttr
        || attrName == hspaceAttr) {
        result = eUniversal;
        return false;
    }

    if (attrName == alignAttr && m_inputType->shouldRespectAlignAttribute()) {
        // Share with <img> since the alignment behavior is the same.
        result = eReplaced;
        return false;
    }

    return HTMLElement::mapToEntry(attrName, result);
}

void HTMLInputElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == nameAttr) {
        checkedRadioButtons().removeButton(this);
        m_data.setName(attr->value());
        checkedRadioButtons().addButton(this);
        HTMLFormControlElementWithState::parseMappedAttribute(attr);
    } else if (attr->name() == autocompleteAttr) {
        if (equalIgnoringCase(attr->value(), "off")) {
            m_autocomplete = Off;
            registerForActivationCallbackIfNeeded();
        } else {
            bool needsToUnregister = m_autocomplete == Off;

            if (attr->isEmpty())
                m_autocomplete = Uninitialized;
            else
                m_autocomplete = On;

            if (needsToUnregister)
                unregisterForActivationCallbackIfNeeded();
        }
    } else if (attr->name() == typeAttr) {
        updateType();
    } else if (attr->name() == valueAttr) {
        // We only need to setChanged if the form is looking at the default value right now.
        if (m_data.value().isNull())
            setNeedsStyleRecalc();
        setFormControlValueMatchesRenderer(false);
        setNeedsValidityCheck();
    } else if (attr->name() == checkedAttr) {
        // Another radio button in the same group might be checked by state
        // restore. We shouldn't call setChecked() even if this has the checked
        // attribute. So, delay the setChecked() call until
        // finishParsingChildren() is called if parsing is in progress.
        if (!m_parsingInProgress && m_reflectsCheckedAttribute) {
            setChecked(!attr->isNull());
            m_reflectsCheckedAttribute = true;
        }
    } else if (attr->name() == maxlengthAttr) {
        InputElement::parseMaxLengthAttribute(m_data, this, this, attr);
        setNeedsValidityCheck();
    } else if (attr->name() == sizeAttr)
        InputElement::parseSizeAttribute(m_data, this, attr);
    else if (attr->name() == altAttr)
        m_inputType->altAttributeChanged();
    else if (attr->name() == srcAttr)
        m_inputType->srcAttributeChanged();
    else if (attr->name() == usemapAttr || attr->name() == accesskeyAttr) {
        // FIXME: ignore for the moment
    } else if (attr->name() == vspaceAttr) {
        addCSSLength(attr, CSSPropertyMarginTop, attr->value());
        addCSSLength(attr, CSSPropertyMarginBottom, attr->value());
    } else if (attr->name() == hspaceAttr) {
        addCSSLength(attr, CSSPropertyMarginLeft, attr->value());
        addCSSLength(attr, CSSPropertyMarginRight, attr->value());
    } else if (attr->name() == alignAttr) {
        if (m_inputType->shouldRespectAlignAttribute())
            addHTMLAlignment(attr);
    } else if (attr->name() == widthAttr) {
        if (m_inputType->shouldRespectHeightAndWidthAttributes())
            addCSSLength(attr, CSSPropertyWidth, attr->value());
    } else if (attr->name() == heightAttr) {
        if (m_inputType->shouldRespectHeightAndWidthAttributes())
            addCSSLength(attr, CSSPropertyHeight, attr->value());
    } else if (attr->name() == onsearchAttr) {
        // Search field and slider attributes all just cause updateFromElement to be called through style recalcing.
        setAttributeEventListener(eventNames().searchEvent, createAttributeEventListener(this, attr));
    } else if (attr->name() == resultsAttr) {
        int oldResults = m_maxResults;
        m_maxResults = !attr->isNull() ? std::min(attr->value().toInt(), maxSavedResults) : -1;
        // FIXME: Detaching just for maxResults change is not ideal.  We should figure out the right
        // time to relayout for this change.
        if (m_maxResults != oldResults && (m_maxResults <= 0 || oldResults <= 0) && attached()) {
            detach();
            attach();
        }
        setNeedsStyleRecalc();
    } else if (attr->name() == autosaveAttr || attr->name() == incrementalAttr)
        setNeedsStyleRecalc();
    else if (attr->name() == minAttr || attr->name() == maxAttr) {
        m_inputType->minOrMaxAttributeChanged();
        setNeedsValidityCheck();
    } else if (attr->name() == multipleAttr || attr->name() == patternAttr || attr->name() == precisionAttr || attr->name() == stepAttr)
        setNeedsValidityCheck();
#if ENABLE(DATALIST)
    else if (attr->name() == listAttr)
        m_hasNonEmptyList = !attr->isEmpty();
        // FIXME: we need to tell this change to a renderer if the attribute affects the appearance.
#endif
#if ENABLE(INPUT_SPEECH)
    else if (attr->name() == webkitspeechAttr) {
        if (renderer()) {
            // This renderer and its children have quite different layouts and styles depending on
            // whether the speech button is visible or not. So we reset the whole thing and recreate
            // to get the right styles and layout.
            detach();
            attach();
        }
        setNeedsStyleRecalc();
    } else if (attr->name() == onwebkitspeechchangeAttr)
        setAttributeEventListener(eventNames().webkitspeechchangeEvent, createAttributeEventListener(this, attr));
#endif
    else
        HTMLTextFormControlElement::parseMappedAttribute(attr);
}

void HTMLInputElement::finishParsingChildren()
{
    m_parsingInProgress = false;
    HTMLFormControlElementWithState::finishParsingChildren();
    if (!m_stateRestored) {
        bool checked = hasAttribute(checkedAttr);
        if (checked)
            setChecked(checked);
        m_reflectsCheckedAttribute = true;
    }
}

bool HTMLInputElement::rendererIsNeeded(RenderStyle* style)
{
    return m_inputType->rendererIsNeeded() && HTMLFormControlElementWithState::rendererIsNeeded(style);
}

RenderObject* HTMLInputElement::createRenderer(RenderArena* arena, RenderStyle* style)
{
    return m_inputType->createRenderer(arena, style);
}

void HTMLInputElement::attach()
{
    suspendPostAttachCallbacks();

    if (!m_hasType)
        updateType();

    HTMLFormControlElementWithState::attach();

    m_inputType->attach();

    if (document()->focusedNode() == this)
        document()->updateFocusAppearanceSoon(true /* restore selection */);

    resumePostAttachCallbacks();
}

void HTMLInputElement::detach()
{
    HTMLFormControlElementWithState::detach();
    setFormControlValueMatchesRenderer(false);
}

String HTMLInputElement::altText() const
{
    // http://www.w3.org/TR/1998/REC-html40-19980424/appendix/notes.html#altgen
    // also heavily discussed by Hixie on bugzilla
    // note this is intentionally different to HTMLImageElement::altText()
    String alt = fastGetAttribute(altAttr);
    // fall back to title attribute
    if (alt.isNull())
        alt = getAttribute(titleAttr);
    if (alt.isNull())
        alt = getAttribute(valueAttr);
    if (alt.isEmpty())
        alt = inputElementAltText();
    return alt;
}

bool HTMLInputElement::isSuccessfulSubmitButton() const
{
    // HTML spec says that buttons must have names to be considered successful.
    // However, other browsers do not impose this constraint. So we do not.
    return !disabled() && m_inputType->canBeSuccessfulSubmitButton();
}

bool HTMLInputElement::isActivatedSubmit() const
{
    return m_isActivatedSubmit;
}

void HTMLInputElement::setActivatedSubmit(bool flag)
{
    m_isActivatedSubmit = flag;
}

bool HTMLInputElement::appendFormData(FormDataList& encoding, bool multipart)
{
    return m_inputType->isFormDataAppendable() && m_inputType->appendFormData(encoding, multipart);
}

void HTMLInputElement::reset()
{
    if (m_inputType->storesValueSeparateFromAttribute())
        setValue(String());

    setAutofilled(false);
    setChecked(hasAttribute(checkedAttr));
    m_reflectsCheckedAttribute = true;
}

bool HTMLInputElement::isTextField() const
{
    return m_inputType->isTextField();
}

bool HTMLInputElement::isTextType() const
{
    return m_inputType->isTextType();
}

void HTMLInputElement::setChecked(bool nowChecked, bool sendChangeEvent)
{
    if (checked() == nowChecked)
        return;

    checkedRadioButtons().removeButton(this);

    m_reflectsCheckedAttribute = false;
    m_isChecked = nowChecked;
    setNeedsStyleRecalc();

    updateCheckedRadioButtons();
    setNeedsValidityCheck();

    // Ideally we'd do this from the render tree (matching
    // RenderTextView), but it's not possible to do it at the moment
    // because of the way the code is structured.
    if (renderer() && AXObjectCache::accessibilityEnabled())
        renderer()->document()->axObjectCache()->postNotification(renderer(), AXObjectCache::AXCheckedStateChanged, true);

    // Only send a change event for items in the document (avoid firing during
    // parsing) and don't send a change event for a radio button that's getting
    // unchecked to match other browsers. DOM is not a useful standard for this
    // because it says only to fire change events at "lose focus" time, which is
    // definitely wrong in practice for these types of elements.
    if (sendChangeEvent && inDocument() && m_inputType->shouldSendChangeEventAfterCheckedChanged()) {
        setTextAsOfLastFormControlChangeEvent(String());
        dispatchFormControlChangeEvent();
    }
}

void HTMLInputElement::setIndeterminate(bool newValue)
{
    if (!m_inputType->isCheckable() || indeterminate() == newValue)
        return;

    m_isIndeterminate = newValue;

    setNeedsStyleRecalc();

    if (renderer() && renderer()->style()->hasAppearance())
        renderer()->theme()->stateChanged(renderer(), CheckedState);
}

int HTMLInputElement::size() const
{
    return m_data.size();
}

void HTMLInputElement::copyNonAttributeProperties(const Element* source)
{
    const HTMLInputElement* sourceElement = static_cast<const HTMLInputElement*>(source);

    m_data.setValue(sourceElement->m_data.value());
    setChecked(sourceElement->m_isChecked);
    m_reflectsCheckedAttribute = sourceElement->m_reflectsCheckedAttribute;
    m_isIndeterminate = sourceElement->m_isIndeterminate;

    HTMLFormControlElementWithState::copyNonAttributeProperties(source);
}

String HTMLInputElement::value() const
{
    String value;
    if (m_inputType->getTypeSpecificValue(value))
        return value;

    value = m_data.value();
    if (!value.isNull())
        return value;

    value = sanitizeValue(fastGetAttribute(valueAttr));
    if (!value.isNull())
        return value;

    return m_inputType->fallbackValue();
}

String HTMLInputElement::valueWithDefault() const
{
    String value = this->value();
    if (!value.isNull())
        return value;

    return m_inputType->defaultValue();
}

void HTMLInputElement::setValueForUser(const String& value)
{
    // Call setValue and make it send a change event.
    setValue(value, true);
}

const String& HTMLInputElement::suggestedValue() const
{
    return m_data.suggestedValue();
}

void HTMLInputElement::setSuggestedValue(const String& value)
{
    if (!m_inputType->canSetSuggestedValue())
        return;
    setFormControlValueMatchesRenderer(false);
    m_data.setSuggestedValue(sanitizeValue(value));
    updatePlaceholderVisibility(false);
    if (renderer())
        renderer()->updateFromElement();
    setNeedsStyleRecalc();
}

void HTMLInputElement::setValue(const String& value, bool sendChangeEvent)
{
    if (!m_inputType->canSetValue(value))
        return;

    setFormControlValueMatchesRenderer(false);
    if (m_inputType->storesValueSeparateFromAttribute()) {
        if (files())
            files()->clear();
        else {
            m_data.setValue(sanitizeValue(value));
            if (isTextField())
                updatePlaceholderVisibility(false);
        }
        setNeedsStyleRecalc();
    } else
        setAttribute(valueAttr, sanitizeValue(value));

    setNeedsValidityCheck();

    if (isTextField()) {
        unsigned max = m_data.value().length();
        if (document()->focusedNode() == this)
            InputElement::updateSelectionRange(this, this, max, max);
        else
            cacheSelection(max, max);
        m_data.setSuggestedValue(String());
    }
    m_inputType->valueChanged();

    if (sendChangeEvent) {
        // If the user is still editing this field, dispatch an input event rather than a change event.
        // The change event will be dispatched when editing finishes.
        if (isTextField() && focused())
            dispatchFormControlInputEvent();
        else
            dispatchFormControlChangeEvent();
    }

    if (isText() && (!focused() || !sendChangeEvent))
        setTextAsOfLastFormControlChangeEvent(value);

    InputElement::notifyFormStateChanged(this);
}

double HTMLInputElement::valueAsDate() const
{
    return m_inputType->valueAsDate();
}

void HTMLInputElement::setValueAsDate(double value, ExceptionCode& ec)
{
    m_inputType->setValueAsDate(value, ec);
}

double HTMLInputElement::valueAsNumber() const
{
    return m_inputType->valueAsNumber();
}

void HTMLInputElement::setValueAsNumber(double newValue, ExceptionCode& ec)
{
    if (!isfinite(newValue)) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }
    m_inputType->setValueAsNumber(newValue, ec);
}

String HTMLInputElement::placeholder() const
{
    return fastGetAttribute(placeholderAttr).string();
}

void HTMLInputElement::setPlaceholder(const String& value)
{
    setAttribute(placeholderAttr, value);
}

bool HTMLInputElement::searchEventsShouldBeDispatched() const
{
    return hasAttribute(incrementalAttr);
}

void HTMLInputElement::setValueFromRenderer(const String& value)
{
    // File upload controls will always use setFileListFromRenderer.
    ASSERT(!isFileUpload());

    m_data.setSuggestedValue(String());
    InputElement::setValueFromRenderer(m_data, this, this, value);
    updatePlaceholderVisibility(false);
    setNeedsValidityCheck();

    // Clear autofill flag (and yellow background) on user edit.
    setAutofilled(false);
}

void HTMLInputElement::setFileListFromRenderer(const Vector<String>& paths)
{
    m_inputType->setFileList(paths);

    setFormControlValueMatchesRenderer(true);
    InputElement::notifyFormStateChanged(this);
    setNeedsValidityCheck();
}

void* HTMLInputElement::preDispatchEventHandler(Event* event)
{
    if (event->type() == eventNames().textInputEvent && m_inputType->shouldSubmitImplicitly(event)) {
        event->stopPropagation();
        return 0;
    }
    if (event->type() != eventNames().clickEvent)
        return 0;
    if (!event->isMouseEvent() || static_cast<MouseEvent*>(event)->button() != LeftButton)
        return 0;
    // FIXME: Check whether there are any cases where this actually ends up leaking.
    return m_inputType->willDispatchClick().leakPtr();
}

void HTMLInputElement::postDispatchEventHandler(Event* event, void* dataFromPreDispatch)
{
    OwnPtr<ClickHandlingState> state = adoptPtr(static_cast<ClickHandlingState*>(dataFromPreDispatch));
    if (!state)
        return;
    m_inputType->didDispatchClick(event, *state);
}

void HTMLInputElement::defaultEventHandler(Event* evt)
{
    if (evt->isMouseEvent() && evt->type() == eventNames().clickEvent && static_cast<MouseEvent*>(evt)->button() == LeftButton) {
        m_inputType->handleClickEvent(static_cast<MouseEvent*>(evt));
        if (evt->defaultHandled())
            return;
    }

    if (evt->isKeyboardEvent() && evt->type() == eventNames().keydownEvent) {
        m_inputType->handleKeydownEvent(static_cast<KeyboardEvent*>(evt));
        if (evt->defaultHandled())
            return;
    }

    // Call the base event handler before any of our own event handling for almost all events in text fields.
    // Makes editing keyboard handling take precedence over the keydown and keypress handling in this function.
    bool callBaseClassEarly = isTextField() && (evt->type() == eventNames().keydownEvent || evt->type() == eventNames().keypressEvent);
    if (callBaseClassEarly) {
        HTMLFormControlElementWithState::defaultEventHandler(evt);
        if (evt->defaultHandled())
            return;
    }

    // DOMActivate events cause the input to be "activated" - in the case of image and submit inputs, this means
    // actually submitting the form. For reset inputs, the form is reset. These events are sent when the user clicks
    // on the element, or presses enter while it is the active element. JavaScript code wishing to activate the element
    // must dispatch a DOMActivate event - a click event will not do the job.
    if (evt->type() == eventNames().DOMActivateEvent) {
        m_inputType->handleDOMActivateEvent(evt);
        if (evt->defaultHandled())
            return;
    }

    // Use key press event here since sending simulated mouse events
    // on key down blocks the proper sending of the key press event.
    if (evt->isKeyboardEvent() && evt->type() == eventNames().keypressEvent) {
        m_inputType->handleKeypressEvent(static_cast<KeyboardEvent*>(evt));
        if (evt->defaultHandled())
            return;
    }

    if (evt->isKeyboardEvent() && evt->type() == eventNames().keyupEvent) {
        m_inputType->handleKeyupEvent(static_cast<KeyboardEvent*>(evt));
        if (evt->defaultHandled())
            return;
    }

    if (m_inputType->shouldSubmitImplicitly(evt)) {
        if (isSearchField()) {
            addSearchResult();
            onSearch();
        }
        // Form submission finishes editing, just as loss of focus does.
        // If there was a change, send the event now.
        if (wasChangedSinceLastFormControlChangeEvent())
            dispatchFormControlChangeEvent();

        RefPtr<HTMLFormElement> formForSubmission = m_inputType->formForSubmission();
        // Form may never have been present, or may have been destroyed by code responding to the change event.
        if (formForSubmission)
            formForSubmission->submitImplicitly(evt, canTriggerImplicitSubmission());

        evt->setDefaultHandled();
        return;
    }

    if (evt->isBeforeTextInsertedEvent())
        m_inputType->handleBeforeTextInsertedEvent(static_cast<BeforeTextInsertedEvent*>(evt));

    if (evt->isWheelEvent()) {
        m_inputType->handleWheelEvent(static_cast<WheelEvent*>(evt));
        if (evt->defaultHandled())
            return;
    }

    if (evt->isMouseEvent() && evt->type() == eventNames().mousedownEvent) {
        m_inputType->handleMouseDownEvent(static_cast<MouseEvent*>(evt));
        if (evt->defaultHandled())
            return;
    }

    m_inputType->forwardEvent(evt);

    if (!callBaseClassEarly && !evt->defaultHandled())
        HTMLFormControlElementWithState::defaultEventHandler(evt);
}

bool HTMLInputElement::isURLAttribute(Attribute *attr) const
{
    return (attr->name() == srcAttr || attr->name() == formactionAttr);
}

String HTMLInputElement::defaultValue() const
{
    return fastGetAttribute(valueAttr);
}

void HTMLInputElement::setDefaultValue(const String &value)
{
    setAttribute(valueAttr, value);
}

void HTMLInputElement::setDefaultName(const AtomicString& name)
{
    m_data.setName(name);
}

String HTMLInputElement::accept() const
{
    return fastGetAttribute(acceptAttr);
}

String HTMLInputElement::alt() const
{
    return fastGetAttribute(altAttr);
}

int HTMLInputElement::maxLength() const
{
    return m_data.maxLength();
}

void HTMLInputElement::setMaxLength(int maxLength, ExceptionCode& ec)
{
    if (maxLength < 0)
        ec = INDEX_SIZE_ERR;
    else
        setAttribute(maxlengthAttr, String::number(maxLength));
}

bool HTMLInputElement::multiple() const
{
    return fastHasAttribute(multipleAttr);
}

void HTMLInputElement::setSize(unsigned size)
{
    setAttribute(sizeAttr, String::number(size));
}

KURL HTMLInputElement::src() const
{
    return document()->completeURL(fastGetAttribute(srcAttr));
}

void HTMLInputElement::setAutofilled(bool autofilled)
{
    if (autofilled == m_isAutofilled)
        return;

    m_isAutofilled = autofilled;
    setNeedsStyleRecalc();
}

FileList* HTMLInputElement::files()
{
    return m_inputType->files();
}

String HTMLInputElement::visibleValue() const
{
    return m_inputType->visibleValue();
}

String HTMLInputElement::convertFromVisibleValue(const String& visibleValue) const
{
    return m_inputType->convertFromVisibleValue(visibleValue);
}

bool HTMLInputElement::isAcceptableValue(const String& proposedValue) const
{
    return m_inputType->isAcceptableValue(proposedValue);
}

String HTMLInputElement::sanitizeValue(const String& proposedValue) const
{
    return m_inputType->sanitizeValue(proposedValue);
}

bool HTMLInputElement::hasUnacceptableValue() const
{
    return m_inputType->hasUnacceptableValue();
}

bool HTMLInputElement::isInRange() const
{
    return m_inputType->supportsRangeLimitation() && !rangeUnderflow(value()) && !rangeOverflow(value());
}

bool HTMLInputElement::isOutOfRange() const
{
    return m_inputType->supportsRangeLimitation() && (rangeUnderflow(value()) || rangeOverflow(value()));
}

bool HTMLInputElement::needsActivationCallback()
{
    return m_autocomplete == Off || m_inputType->shouldResetOnDocumentActivation();
}

void HTMLInputElement::registerForActivationCallbackIfNeeded()
{
    if (needsActivationCallback())
        document()->registerForDocumentActivationCallbacks(this);
}

void HTMLInputElement::unregisterForActivationCallbackIfNeeded()
{
    if (!needsActivationCallback())
        document()->unregisterForDocumentActivationCallbacks(this);
}

bool HTMLInputElement::isRequiredFormControl() const
{
    return m_inputType->supportsRequired() && required();
}

void HTMLInputElement::cacheSelection(int start, int end)
{
    m_data.setCachedSelectionStart(start);
    m_data.setCachedSelectionEnd(end);
}

void HTMLInputElement::addSearchResult()
{
    ASSERT(isSearchField());
    if (renderer())
        toRenderTextControlSingleLine(renderer())->addSearchResult();
}

void HTMLInputElement::onSearch()
{
    ASSERT(isSearchField());
    if (renderer())
        toRenderTextControlSingleLine(renderer())->stopSearchEventTimer();
    dispatchEvent(Event::create(eventNames().searchEvent, true, false));
}

void HTMLInputElement::documentDidBecomeActive()
{
    ASSERT(needsActivationCallback());
    reset();
}

void HTMLInputElement::willMoveToNewOwnerDocument()
{
    m_inputType->willMoveToNewOwnerDocument();

    // Always unregister for cache callbacks when leaving a document, even if we would otherwise like to be registered
    if (needsActivationCallback())
        document()->unregisterForDocumentActivationCallbacks(this);

    document()->checkedRadioButtons().removeButton(this);

    HTMLFormControlElementWithState::willMoveToNewOwnerDocument();
}

void HTMLInputElement::didMoveToNewOwnerDocument()
{
    registerForActivationCallbackIfNeeded();

    HTMLFormControlElementWithState::didMoveToNewOwnerDocument();
}

void HTMLInputElement::addSubresourceAttributeURLs(ListHashSet<KURL>& urls) const
{
    HTMLFormControlElementWithState::addSubresourceAttributeURLs(urls);

    addSubresourceURL(urls, src());
}

bool HTMLInputElement::recalcWillValidate() const
{
    return m_inputType->supportsValidation() && HTMLFormControlElementWithState::recalcWillValidate();
}

#if ENABLE(DATALIST)

HTMLElement* HTMLInputElement::list() const
{
    return dataList();
}

HTMLDataListElement* HTMLInputElement::dataList() const
{
    if (!m_hasNonEmptyList)
        return 0;

    if (!m_inputType->shouldRespectListAttribute())
        return 0;

    Element* element = treeScope()->getElementById(fastGetAttribute(listAttr));
    if (!element)
        return 0;
    if (!element->hasTagName(datalistTag))
        return 0;

    return static_cast<HTMLDataListElement*>(element);
}

HTMLOptionElement* HTMLInputElement::selectedOption() const
{
    String value = this->value();

    // The empty string never matches to a datalist option because it
    // doesn't represent a suggestion according to the standard.
    if (value.isEmpty())
        return 0;

    HTMLDataListElement* sourceElement = dataList();
    if (!sourceElement)
        return 0;
    RefPtr<HTMLCollection> options = sourceElement->options();
    if (!options)
        return 0;
    unsigned length = options->length();
    for (unsigned i = 0; i < length; ++i) {
        HTMLOptionElement* option = static_cast<HTMLOptionElement*>(options->item(i));
        if (!option->disabled() && value == option->value())
            return option;
    }
    return 0;
}

#endif // ENABLE(DATALIST)

bool HTMLInputElement::isSteppable() const
{
    return m_inputType->isSteppable();
}

void HTMLInputElement::stepUpFromRenderer(int n)
{
    // The differences from stepUp()/stepDown():
    //
    // Difference 1: the current value
    // If the current value is not a number, including empty, the current value is assumed as 0.
    //   * If 0 is in-range, and matches to step value
    //     - The value should be the +step if n > 0
    //     - The value should be the -step if n < 0
    //     If -step or +step is out of range, new value should be 0.
    //   * If 0 is smaller than the minimum value
    //     - The value should be the minimum value for any n
    //   * If 0 is larger than the maximum value
    //     - The value should be the maximum value for any n
    //   * If 0 is in-range, but not matched to step value
    //     - The value should be the larger matched value nearest to 0 if n > 0
    //       e.g. <input type=number min=-100 step=3> -> 2
    //     - The value should be the smaler matched value nearest to 0 if n < 0
    //       e.g. <input type=number min=-100 step=3> -> -1
    //   As for date/datetime-local/month/time/week types, the current value is assumed as "the current local date/time".
    //   As for datetime type, the current value is assumed as "the current date/time in UTC".
    // If the current value is smaller than the minimum value:
    //  - The value should be the minimum value if n > 0
    //  - Nothing should happen if n < 0
    // If the current value is larger than the maximum value:
    //  - The value should be the maximum value if n < 0
    //  - Nothing should happen if n > 0
    //
    // Difference 2: clamping steps
    // If the current value is not matched to step value:
    // - The value should be the larger matched value nearest to 0 if n > 0
    //   e.g. <input type=number value=3 min=-100 step=3> -> 5
    // - The value should be the smaler matched value nearest to 0 if n < 0
    //   e.g. <input type=number value=3 min=-100 step=3> -> 2
    //
    // n is assumed as -n if step < 0.

    ASSERT(isSteppable());
    if (!isSteppable())
        return;
    ASSERT(n);
    if (!n)
        return;

    unsigned stepDecimalPlaces, baseDecimalPlaces;
    double step, base;
    // The value will be the default value after stepping for <input value=(empty/invalid) step="any" />
    // FIXME: Not any changes after stepping, even if it is an invalid value, may be better.
    // (e.g. Stepping-up for <input type="number" value="foo" step="any" /> => "foo")
    if (equalIgnoringCase(fastGetAttribute(stepAttr), "any"))
        step = 0;
    else if (!getAllowedValueStepWithDecimalPlaces(&step, &stepDecimalPlaces))
        return;
    base = m_inputType->stepBaseWithDecimalPlaces(&baseDecimalPlaces);
    baseDecimalPlaces = min(baseDecimalPlaces, 16u);

    int sign;
    if (step > 0)
        sign = n;
    else if (step < 0)
        sign = -n;
    else
        sign = 0;

    const double nan = numeric_limits<double>::quiet_NaN();
    String currentStringValue = value();
    double current = m_inputType->parseToDouble(currentStringValue, nan);
    if (!isfinite(current)) {
        ExceptionCode ec;
        current = m_inputType->defaultValueForStepUp();
        setValueAsNumber(current, ec);
    }
    if ((sign > 0 && current < m_inputType->minimum()) || (sign < 0 && current > m_inputType->maximum()))
        setValue(m_inputType->serialize(sign > 0 ? m_inputType->minimum() : m_inputType->maximum()));
    else {
        ExceptionCode ec;
        if (stepMismatch(currentStringValue)) {
            ASSERT(step);
            double newValue;
            double scale = pow(10.0, static_cast<double>(max(stepDecimalPlaces, baseDecimalPlaces)));

            if (sign < 0)
                newValue = round((base + floor((current - base) / step) * step) * scale) / scale;
            else if (sign > 0)
                newValue = round((base + ceil((current - base) / step) * step) * scale) / scale;
            else
                newValue = current;

            if (newValue < m_inputType->minimum())
                newValue = m_inputType->minimum();
            if (newValue > m_inputType->maximum())
                newValue = m_inputType->maximum();

            setValueAsNumber(newValue, ec);
            current = newValue;
            if (n > 1)
                applyStep(n - 1, ec);
            else if (n < -1)
                applyStep(n + 1, ec);
        } else
            applyStep(n, ec);
    }

    if (currentStringValue != value()) {
        if (m_inputType->isRangeControl())
            dispatchFormControlChangeEvent();
        else
            dispatchFormControlInputEvent();
    }
}

#if ENABLE(WCSS)

void HTMLInputElement::setWapInputFormat(String& mask)
{
    String validateMask = validateInputMask(m_data, mask);
    if (!validateMask.isEmpty())
        m_data.setInputFormatMask(validateMask);
}

#endif

#if ENABLE(INPUT_SPEECH)

bool HTMLInputElement::isSpeechEnabled() const
{
    // FIXME: Add support for RANGE, EMAIL, URL, COLOR and DATE/TIME input types.
    return m_inputType->shouldRespectSpeechAttribute() && RuntimeEnabledFeatures::speechInputEnabled() && hasAttribute(webkitspeechAttr);
}

#endif

bool HTMLInputElement::isTextButton() const
{
    return m_inputType->isTextButton();
}

bool HTMLInputElement::isRadioButton() const
{
    return m_inputType->isRadioButton();
}

bool HTMLInputElement::isSearchField() const
{
    return m_inputType->isSearchField();
}

bool HTMLInputElement::isInputTypeHidden() const
{
    return m_inputType->isHiddenType();
}

bool HTMLInputElement::isPasswordField() const
{
    return m_inputType->isPasswordField();
}

bool HTMLInputElement::isCheckbox() const
{
    return m_inputType->isCheckbox();
}

bool HTMLInputElement::isRangeControl() const
{
    return m_inputType->isRangeControl();
}

bool HTMLInputElement::isText() const
{
    return m_inputType->isTextType();
}

bool HTMLInputElement::isEmailField() const
{
    return m_inputType->isEmailField();
}

bool HTMLInputElement::isFileUpload() const
{
    return m_inputType->isFileUpload();
}

bool HTMLInputElement::isImageButton() const
{
    return m_inputType->isImageButton();
}

bool HTMLInputElement::isNumberField() const
{
    return m_inputType->isNumberField();
}

bool HTMLInputElement::isSubmitButton() const
{
    return m_inputType->isSubmitButton();
}

bool HTMLInputElement::isTelephoneField() const
{
    return m_inputType->isTelephoneField();
}

bool HTMLInputElement::isURLField() const
{
    return m_inputType->isURLField();
}

bool HTMLInputElement::isEnumeratable() const
{
    return m_inputType->isEnumeratable();
}

bool HTMLInputElement::isChecked() const
{
    return checked() && m_inputType->isCheckable();
}

bool HTMLInputElement::supportsPlaceholder() const
{
    return isTextType();
}

CheckedRadioButtons& HTMLInputElement::checkedRadioButtons() const
{
    if (HTMLFormElement* formElement = form())
        return formElement->checkedRadioButtons();
    return document()->checkedRadioButtons();
}

void HTMLInputElement::handleBeforeTextInsertedEvent(Event* event)
{
    InputElement::handleBeforeTextInsertedEvent(m_data, this, this, event);
}

} // namespace
