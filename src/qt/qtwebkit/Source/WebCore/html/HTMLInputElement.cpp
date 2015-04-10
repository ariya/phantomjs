/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 * Copyright (C) 2007 Samuel Weinig (sam@webkit.org)
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
#include "BeforeTextInsertedEvent.h"
#include "CSSPropertyNames.h"
#include "DateTimeChooser.h"
#include "Document.h"
#include "Editor.h"
#include "ElementShadow.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "FeatureObserver.h"
#include "FileInputType.h"
#include "FileList.h"
#include "FormController.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "FrameView.h"
#include "HTMLCollection.h"
#include "HTMLDataListElement.h"
#include "HTMLFormElement.h"
#include "HTMLImageLoader.h"
#include "HTMLNames.h"
#include "HTMLOptionElement.h"
#include "HTMLParserIdioms.h"
#include "IdTargetObserver.h"
#include "InputType.h"
#include "InsertionPoint.h"
#include "KeyboardEvent.h"
#include "Language.h"
#include "LocalizedStrings.h"
#include "MouseEvent.h"
#include "PlatformMouseEvent.h"
#include "RenderTextControlSingleLine.h"
#include "RenderTheme.h"
#include "RuntimeEnabledFeatures.h"
#include "ScopedEventQueue.h"
#include "SearchInputType.h"
#include "ShadowRoot.h"
#include "ScriptEventListener.h"
#include "StyleResolver.h"
#include <wtf/MathExtras.h>

#if ENABLE(INPUT_TYPE_COLOR)
#include "ColorInputType.h"
#endif

#if ENABLE(INPUT_SPEECH)
#include "RuntimeEnabledFeatures.h"
#endif

#if ENABLE(TOUCH_EVENTS)
#include "TouchEvent.h"
#endif

using namespace std;

namespace WebCore {

using namespace HTMLNames;

#if ENABLE(DATALIST_ELEMENT)
class ListAttributeTargetObserver : IdTargetObserver {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<ListAttributeTargetObserver> create(const AtomicString& id, HTMLInputElement*);
    virtual void idTargetChanged() OVERRIDE;

private:
    ListAttributeTargetObserver(const AtomicString& id, HTMLInputElement*);

    HTMLInputElement* m_element;
};
#endif

// FIXME: According to HTML4, the length attribute's value can be arbitrarily
// large. However, due to https://bugs.webkit.org/show_bug.cgi?id=14536 things
// get rather sluggish when a text field has a larger number of characters than
// this, even when just clicking in the text field.
const int HTMLInputElement::maximumLength = 524288;
const int defaultSize = 20;
const int maxSavedResults = 256;

HTMLInputElement::HTMLInputElement(const QualifiedName& tagName, Document* document, HTMLFormElement* form, bool createdByParser)
    : HTMLTextFormControlElement(tagName, document, form)
    , m_size(defaultSize)
    , m_maxLength(maximumLength)
    , m_maxResults(-1)
    , m_isChecked(false)
    , m_reflectsCheckedAttribute(true)
    , m_isIndeterminate(false)
    , m_hasType(false)
    , m_isActivatedSubmit(false)
    , m_autocomplete(Uninitialized)
    , m_isAutofilled(false)
#if ENABLE(DATALIST_ELEMENT)
    , m_hasNonEmptyList(false)
#endif
    , m_stateRestored(false)
    , m_parsingInProgress(createdByParser)
    , m_valueAttributeWasUpdatedAfterParsing(false)
    , m_wasModifiedByUser(false)
    , m_canReceiveDroppedFiles(false)
#if ENABLE(TOUCH_EVENTS)
    , m_hasTouchEventHandler(false)
#endif
    , m_inputType(InputType::createText(this))
{
    ASSERT(hasTagName(inputTag) || hasTagName(isindexTag));
}

PassRefPtr<HTMLInputElement> HTMLInputElement::create(const QualifiedName& tagName, Document* document, HTMLFormElement* form, bool createdByParser)
{
    RefPtr<HTMLInputElement> inputElement = adoptRef(new HTMLInputElement(tagName, document, form, createdByParser));
    inputElement->ensureUserAgentShadowRoot();
    return inputElement.release();
}

HTMLImageLoader* HTMLInputElement::imageLoader()
{
    if (!m_imageLoader)
        m_imageLoader = adoptPtr(new HTMLImageLoader(this));
    return m_imageLoader.get();
}

void HTMLInputElement::didAddUserAgentShadowRoot(ShadowRoot*)
{
    m_inputType->createShadowSubtree();
}

HTMLInputElement::~HTMLInputElement()
{
    if (needsSuspensionCallback())
        document()->unregisterForPageCacheSuspensionCallbacks(this);

    // Need to remove form association while this is still an HTMLInputElement
    // so that virtual functions are called correctly.
    setForm(0);
    // setForm(0) may register this to a document-level radio button group.
    // We should unregister it to avoid accessing a deleted object.
    if (isRadioButton())
        document()->formController().checkedRadioButtons().removeButton(this);
#if ENABLE(TOUCH_EVENTS)
    if (m_hasTouchEventHandler)
        document()->didRemoveEventTargetNode(this);
#endif
}

const AtomicString& HTMLInputElement::name() const
{
    return m_name.isNull() ? emptyAtom : m_name;
}

Vector<FileChooserFileInfo> HTMLInputElement::filesFromFileInputFormControlState(const FormControlState& state)
{
    return FileInputType::filesFromFormControlState(state);
}

HTMLElement* HTMLInputElement::containerElement() const
{
    return m_inputType->containerElement();
}

HTMLElement* HTMLInputElement::innerTextElement() const
{
    return m_inputType->innerTextElement();
}

HTMLElement* HTMLInputElement::innerBlockElement() const
{
    return m_inputType->innerBlockElement();
}

HTMLElement* HTMLInputElement::innerSpinButtonElement() const
{
    return m_inputType->innerSpinButtonElement();
}

HTMLElement* HTMLInputElement::resultsButtonElement() const
{
    return m_inputType->resultsButtonElement();
}

HTMLElement* HTMLInputElement::cancelButtonElement() const
{
    return m_inputType->cancelButtonElement();
}

#if ENABLE(INPUT_SPEECH)
HTMLElement* HTMLInputElement::speechButtonElement() const
{
    return m_inputType->speechButtonElement();
}
#endif

HTMLElement* HTMLInputElement::sliderThumbElement() const
{
    return m_inputType->sliderThumbElement();
}

HTMLElement* HTMLInputElement::sliderTrackElement() const
{
    return m_inputType->sliderTrackElement();
}

HTMLElement* HTMLInputElement::placeholderElement() const
{
    return m_inputType->placeholderElement();
}

bool HTMLInputElement::shouldAutocomplete() const
{
    if (m_autocomplete != Uninitialized)
        return m_autocomplete == On;
    return HTMLTextFormControlElement::shouldAutocomplete();
}

bool HTMLInputElement::isValidValue(const String& value) const
{
    if (!m_inputType->canSetStringValue()) {
        ASSERT_NOT_REACHED();
        return false;
    }
    return !m_inputType->typeMismatchFor(value)
        && !m_inputType->stepMismatch(value)
        && !m_inputType->rangeUnderflow(value)
        && !m_inputType->rangeOverflow(value)
        && !tooLong(value, IgnoreDirtyFlag)
        && !m_inputType->patternMismatch(value)
        && !m_inputType->valueMissing(value);
}

bool HTMLInputElement::tooLong() const
{
    return willValidate() && tooLong(value(), CheckDirtyFlag);
}

bool HTMLInputElement::typeMismatch() const
{
    return willValidate() && m_inputType->typeMismatch();
}

bool HTMLInputElement::valueMissing() const
{
    return willValidate() && m_inputType->valueMissing(value());
}

bool HTMLInputElement::hasBadInput() const
{
    return willValidate() && m_inputType->hasBadInput();
}

bool HTMLInputElement::patternMismatch() const
{
    return willValidate() && m_inputType->patternMismatch(value());
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
        // Return false for the default value or a value set by a script even if
        // it is longer than maxLength.
        if (!hasDirtyValue() || !m_wasModifiedByUser)
            return false;
    }
    return numGraphemeClusters(value) > static_cast<unsigned>(max);
}

bool HTMLInputElement::rangeUnderflow() const
{
    return willValidate() && m_inputType->rangeUnderflow(value());
}

bool HTMLInputElement::rangeOverflow() const
{
    return willValidate() && m_inputType->rangeOverflow(value());
}

String HTMLInputElement::validationMessage() const
{
    if (!willValidate())
        return String();

    if (customError())
        return customValidationMessage();

    return m_inputType->validationMessage();
}

double HTMLInputElement::minimum() const
{
    return m_inputType->minimum();
}

double HTMLInputElement::maximum() const
{
    return m_inputType->maximum();
}

bool HTMLInputElement::stepMismatch() const
{
    return willValidate() && m_inputType->stepMismatch(value());
}

bool HTMLInputElement::getAllowedValueStep(Decimal* step) const
{
    return m_inputType->getAllowedValueStep(step);
}

StepRange HTMLInputElement::createStepRange(AnyStepHandling anyStepHandling) const
{
    return m_inputType->createStepRange(anyStepHandling);
}

#if ENABLE(DATALIST_ELEMENT)
Decimal HTMLInputElement::findClosestTickMarkValue(const Decimal& value)
{
    return m_inputType->findClosestTickMarkValue(value);
}
#endif

void HTMLInputElement::stepUp(int n, ExceptionCode& ec)
{
    m_inputType->stepUp(n, ec);
}

void HTMLInputElement::stepDown(int n, ExceptionCode& ec)
{
    m_inputType->stepUp(-n, ec);
}

void HTMLInputElement::blur()
{
    m_inputType->blur();
}

void HTMLInputElement::defaultBlur()
{
    HTMLTextFormControlElement::blur();
}

bool HTMLInputElement::hasCustomFocusLogic() const
{
    return m_inputType->hasCustomFocusLogic();
}

bool HTMLInputElement::isKeyboardFocusable(KeyboardEvent* event) const
{
    return m_inputType->isKeyboardFocusable(event);
}

bool HTMLInputElement::isMouseFocusable() const
{
    return m_inputType->isMouseFocusable();
}

bool HTMLInputElement::isTextFormControlFocusable() const
{
    return HTMLTextFormControlElement::isFocusable();
}

bool HTMLInputElement::isTextFormControlKeyboardFocusable(KeyboardEvent* event) const
{
    return HTMLTextFormControlElement::isKeyboardFocusable(event);
}

bool HTMLInputElement::isTextFormControlMouseFocusable() const
{
    return HTMLTextFormControlElement::isMouseFocusable();
}

void HTMLInputElement::updateFocusAppearance(bool restorePreviousSelection)
{
    if (isTextField()) {
        if (!restorePreviousSelection || !hasCachedSelection())
            select();
        else
            restoreCachedSelection();
        if (document()->frame())
            document()->frame()->selection()->revealSelection();
    } else
        HTMLTextFormControlElement::updateFocusAppearance(restorePreviousSelection);
}

void HTMLInputElement::endEditing()
{
    if (!isTextField())
        return;

    if (Frame* frame = document()->frame())
        frame->editor().textFieldDidEndEditing(this);
}

bool HTMLInputElement::shouldUseInputMethod()
{
    return m_inputType->shouldUseInputMethod();
}

void HTMLInputElement::handleFocusEvent(Node* oldFocusedNode, FocusDirection direction)
{
    m_inputType->handleFocusEvent(oldFocusedNode, direction);
}

void HTMLInputElement::handleBlurEvent()
{
    m_inputType->handleBlurEvent();
}

void HTMLInputElement::setType(const String& type)
{
    // FIXME: This should just call setAttribute. No reason to handle the empty string specially.
    // We should write a test case to show that setting to the empty string does not remove the
    // attribute in other browsers and then fix this. Note that setting to null *does* remove
    // the attribute and setAttribute implements that.
    if (type.isEmpty())
        removeAttribute(typeAttr);
    else
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
        // Useful in case we were called from inside parseAttribute.
        setAttribute(typeAttr, type());
        return;
    }

    removeFromRadioButtonGroup();

    bool didStoreValue = m_inputType->storesValueSeparateFromAttribute();
    bool neededSuspensionCallback = needsSuspensionCallback();
    bool didRespectHeightAndWidth = m_inputType->shouldRespectHeightAndWidthAttributes();

    m_inputType->destroyShadowSubtree();

    bool wasAttached = attached();
    if (wasAttached)
        detach();

    m_inputType = newType.release();
    m_inputType->createShadowSubtree();

#if ENABLE(TOUCH_EVENTS)
    bool hasTouchEventHandler = m_inputType->hasTouchEventHandler();
    if (hasTouchEventHandler != m_hasTouchEventHandler) {
        if (hasTouchEventHandler)
            document()->didAddTouchEventHandler(this);
        else
            document()->didRemoveTouchEventHandler(this);
        m_hasTouchEventHandler = hasTouchEventHandler;
    }
#endif

    setNeedsWillValidateCheck();

    bool willStoreValue = m_inputType->storesValueSeparateFromAttribute();

    if (didStoreValue && !willStoreValue && hasDirtyValue()) {
        setAttribute(valueAttr, m_valueIfDirty);
        m_valueIfDirty = String();
    }
    if (!didStoreValue && willStoreValue) {
        AtomicString valueString = fastGetAttribute(valueAttr);
        m_valueIfDirty = sanitizeValue(valueString);
    } else
        updateValueIfNeeded();

    setFormControlValueMatchesRenderer(false);
    m_inputType->updateInnerTextValue();

    m_wasModifiedByUser = false;

    if (neededSuspensionCallback)
        unregisterForSuspensionCallbackIfNeeded();
    else
        registerForSuspensionCallbackIfNeeded();

    if (didRespectHeightAndWidth != m_inputType->shouldRespectHeightAndWidthAttributes()) {
        ASSERT(elementData());
        if (const Attribute* height = getAttributeItem(heightAttr))
            attributeChanged(heightAttr, height->value());
        if (const Attribute* width = getAttributeItem(widthAttr))
            attributeChanged(widthAttr, width->value());
        if (const Attribute* align = getAttributeItem(alignAttr))
            attributeChanged(alignAttr, align->value());
    }

    if (wasAttached) {
        attach();
        if (document()->focusedElement() == this)
            updateFocusAppearance(true);
    }

    if (ElementShadow* elementShadow = shadowOfParentForDistribution(this))
        elementShadow->invalidateDistribution();

    setChangedSinceLastFormControlChangeEvent(false);

    addToRadioButtonGroup();

    setNeedsValidityCheck();
    notifyFormStateChanged();
}

void HTMLInputElement::subtreeHasChanged()
{
    m_inputType->subtreeHasChanged();
    // When typing in an input field, childrenChanged is not called, so we need to force the directionality check.
    calculateAndAdjustDirectionality();
}

const AtomicString& HTMLInputElement::formControlType() const
{
    return m_inputType->formControlType();
}

bool HTMLInputElement::shouldSaveAndRestoreFormControlState() const
{
    if (!m_inputType->shouldSaveAndRestoreFormControlState())
        return false;
    return HTMLTextFormControlElement::shouldSaveAndRestoreFormControlState();
}

FormControlState HTMLInputElement::saveFormControlState() const
{
    return m_inputType->saveFormControlState();
}

void HTMLInputElement::restoreFormControlState(const FormControlState& state)
{
    m_inputType->restoreFormControlState(state);
    m_stateRestored = true;
}

bool HTMLInputElement::canStartSelection() const
{
    if (!isTextField())
        return false;
    return HTMLTextFormControlElement::canStartSelection();
}

bool HTMLInputElement::canHaveSelection() const
{
    return isTextField();
}

void HTMLInputElement::accessKeyAction(bool sendMouseEvents)
{
    m_inputType->accessKeyAction(sendMouseEvents);
}

bool HTMLInputElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (name == vspaceAttr || name == hspaceAttr || name == alignAttr || name == widthAttr || name == heightAttr || (name == borderAttr && isImageButton()))
        return true;
    return HTMLTextFormControlElement::isPresentationAttribute(name);
}

void HTMLInputElement::collectStyleForPresentationAttribute(const QualifiedName& name, const AtomicString& value, MutableStylePropertySet* style)
{
    if (name == vspaceAttr) {
        addHTMLLengthToStyle(style, CSSPropertyMarginTop, value);
        addHTMLLengthToStyle(style, CSSPropertyMarginBottom, value);
    } else if (name == hspaceAttr) {
        addHTMLLengthToStyle(style, CSSPropertyMarginLeft, value);
        addHTMLLengthToStyle(style, CSSPropertyMarginRight, value);
    } else if (name == alignAttr) {
        if (m_inputType->shouldRespectAlignAttribute())
            applyAlignmentAttributeToStyle(value, style);
    } else if (name == widthAttr) {
        if (m_inputType->shouldRespectHeightAndWidthAttributes())
            addHTMLLengthToStyle(style, CSSPropertyWidth, value);
    } else if (name == heightAttr) {
        if (m_inputType->shouldRespectHeightAndWidthAttributes())
            addHTMLLengthToStyle(style, CSSPropertyHeight, value);
    } else if (name == borderAttr && isImageButton())
        applyBorderAttributeToStyle(value, style);
    else
        HTMLTextFormControlElement::collectStyleForPresentationAttribute(name, value, style);
}

void HTMLInputElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == nameAttr) {
        removeFromRadioButtonGroup();
        m_name = value;
        addToRadioButtonGroup();
        HTMLTextFormControlElement::parseAttribute(name, value);
    } else if (name == autocompleteAttr) {
        if (equalIgnoringCase(value, "off")) {
            m_autocomplete = Off;
            registerForSuspensionCallbackIfNeeded();
        } else {
            bool needsToUnregister = m_autocomplete == Off;

            if (value.isEmpty())
                m_autocomplete = Uninitialized;
            else
                m_autocomplete = On;

            if (needsToUnregister)
                unregisterForSuspensionCallbackIfNeeded();
        }
    } else if (name == typeAttr)
        updateType();
    else if (name == valueAttr) {
        // Changes to the value attribute may change whether or not this element has a default value.
        // If this field is autocomplete=off that might affect the return value of needsSuspensionCallback.
        if (m_autocomplete == Off) {
            unregisterForSuspensionCallbackIfNeeded();
            registerForSuspensionCallbackIfNeeded();
        }
        // We only need to setChanged if the form is looking at the default value right now.
        if (!hasDirtyValue()) {
            updatePlaceholderVisibility(false);
            setNeedsStyleRecalc();
        }
        setFormControlValueMatchesRenderer(false);
        setNeedsValidityCheck();
        m_valueAttributeWasUpdatedAfterParsing = !m_parsingInProgress;
        m_inputType->valueAttributeChanged();
    } else if (name == checkedAttr) {
        // Another radio button in the same group might be checked by state
        // restore. We shouldn't call setChecked() even if this has the checked
        // attribute. So, delay the setChecked() call until
        // finishParsingChildren() is called if parsing is in progress.
        if (!m_parsingInProgress && m_reflectsCheckedAttribute) {
            setChecked(!value.isNull());
            m_reflectsCheckedAttribute = true;
        }
    } else if (name == maxlengthAttr)
        parseMaxLengthAttribute(value);
    else if (name == sizeAttr) {
        int oldSize = m_size;
        int valueAsInteger = value.toInt();
        m_size = valueAsInteger > 0 ? valueAsInteger : defaultSize;
        if (m_size != oldSize && renderer())
            renderer()->setNeedsLayoutAndPrefWidthsRecalc();
    } else if (name == altAttr)
        m_inputType->altAttributeChanged();
    else if (name == srcAttr)
        m_inputType->srcAttributeChanged();
    else if (name == usemapAttr || name == accesskeyAttr) {
        // FIXME: ignore for the moment
    } else if (name == onsearchAttr) {
        // Search field and slider attributes all just cause updateFromElement to be called through style recalcing.
        setAttributeEventListener(eventNames().searchEvent, createAttributeEventListener(this, name, value));
    } else if (name == resultsAttr) {
        int oldResults = m_maxResults;
        m_maxResults = !value.isNull() ? std::min(value.toInt(), maxSavedResults) : -1;
        // FIXME: Detaching just for maxResults change is not ideal.  We should figure out the right
        // time to relayout for this change.
        if (m_maxResults != oldResults && (m_maxResults <= 0 || oldResults <= 0))
            reattachIfAttached();
        setNeedsStyleRecalc();
        FeatureObserver::observe(document(), FeatureObserver::ResultsAttribute);
    } else if (name == autosaveAttr) {
        setNeedsStyleRecalc();
        FeatureObserver::observe(document(), FeatureObserver::AutoSaveAttribute);
    } else if (name == incrementalAttr) {
        setNeedsStyleRecalc();
        FeatureObserver::observe(document(), FeatureObserver::IncrementalAttribute);
    } else if (name == minAttr) {
        m_inputType->minOrMaxAttributeChanged();
        setNeedsValidityCheck();
        FeatureObserver::observe(document(), FeatureObserver::MinAttribute);
    } else if (name == maxAttr) {
        m_inputType->minOrMaxAttributeChanged();
        setNeedsValidityCheck();
        FeatureObserver::observe(document(), FeatureObserver::MaxAttribute);
    } else if (name == multipleAttr) {
        m_inputType->multipleAttributeChanged();
        setNeedsValidityCheck();
    } else if (name == stepAttr) {
        m_inputType->stepAttributeChanged();
        setNeedsValidityCheck();
        FeatureObserver::observe(document(), FeatureObserver::StepAttribute);
    } else if (name == patternAttr) {
        setNeedsValidityCheck();
        FeatureObserver::observe(document(), FeatureObserver::PatternAttribute);
    } else if (name == precisionAttr) {
        setNeedsValidityCheck();
        FeatureObserver::observe(document(), FeatureObserver::PrecisionAttribute);
    } else if (name == disabledAttr) {
        HTMLTextFormControlElement::parseAttribute(name, value);
        m_inputType->disabledAttributeChanged();
    } else if (name == readonlyAttr) {
        HTMLTextFormControlElement::parseAttribute(name, value);
        m_inputType->readonlyAttributeChanged();
    }
#if ENABLE(DATALIST_ELEMENT)
    else if (name == listAttr) {
        m_hasNonEmptyList = !value.isEmpty();
        if (m_hasNonEmptyList) {
            resetListAttributeTargetObserver();
            listAttributeTargetChanged();
        }
        FeatureObserver::observe(document(), FeatureObserver::ListAttribute);
    }
#endif
#if ENABLE(INPUT_SPEECH)
    else if (name == webkitspeechAttr) {
        if (renderer()) {
            // This renderer and its children have quite different layouts and styles depending on
            // whether the speech button is visible or not. So we reset the whole thing and recreate
            // to get the right styles and layout.
            detach();
            m_inputType->destroyShadowSubtree();
            m_inputType->createShadowSubtree();
            if (!attached())
                attach();
        } else {
            m_inputType->destroyShadowSubtree();
            m_inputType->createShadowSubtree();
        }
        setFormControlValueMatchesRenderer(false);
        setNeedsStyleRecalc();
        FeatureObserver::observe(document(), FeatureObserver::PrefixedSpeechAttribute);
    } else if (name == onwebkitspeechchangeAttr)
        setAttributeEventListener(eventNames().webkitspeechchangeEvent, createAttributeEventListener(this, name, value));
#endif
#if ENABLE(DIRECTORY_UPLOAD)
    else if (name == webkitdirectoryAttr) {
        HTMLTextFormControlElement::parseAttribute(name, value);
        FeatureObserver::observe(document(), FeatureObserver::PrefixedDirectoryAttribute);
    }
#endif
    else
        HTMLTextFormControlElement::parseAttribute(name, value);
    m_inputType->attributeChanged();
}

void HTMLInputElement::finishParsingChildren()
{
    m_parsingInProgress = false;
    HTMLTextFormControlElement::finishParsingChildren();
    if (!m_stateRestored) {
        bool checked = hasAttribute(checkedAttr);
        if (checked)
            setChecked(checked);
        m_reflectsCheckedAttribute = true;
    }
}

bool HTMLInputElement::rendererIsNeeded(const NodeRenderingContext& context)
{
    return m_inputType->rendererIsNeeded() && HTMLTextFormControlElement::rendererIsNeeded(context);
}

RenderObject* HTMLInputElement::createRenderer(RenderArena* arena, RenderStyle* style)
{
    return m_inputType->createRenderer(arena, style);
}

void HTMLInputElement::attach(const AttachContext& context)
{
    PostAttachCallbackDisabler disabler(this);

    if (!m_hasType)
        updateType();

    HTMLTextFormControlElement::attach(context);

    m_inputType->attach();

    if (document()->focusedElement() == this)
        document()->updateFocusAppearanceSoon(true /* restore selection */);
}

void HTMLInputElement::detach(const AttachContext& context)
{
    HTMLTextFormControlElement::detach(context);
    setFormControlValueMatchesRenderer(false);
    m_inputType->detach();
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
    return !isDisabledFormControl() && m_inputType->canBeSuccessfulSubmitButton();
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

void HTMLInputElement::setChecked(bool nowChecked, TextFieldEventBehavior eventBehavior)
{
    if (checked() == nowChecked)
        return;

    m_reflectsCheckedAttribute = false;
    m_isChecked = nowChecked;
    setNeedsStyleRecalc();

    if (CheckedRadioButtons* buttons = checkedRadioButtons())
            buttons->updateCheckedState(this);
    if (renderer() && renderer()->style()->hasAppearance())
        renderer()->theme()->stateChanged(renderer(), CheckedState);
    setNeedsValidityCheck();

    // Ideally we'd do this from the render tree (matching
    // RenderTextView), but it's not possible to do it at the moment
    // because of the way the code is structured.
    if (renderer()) {
        if (AXObjectCache* cache = renderer()->document()->existingAXObjectCache())
            cache->checkedStateChanged(this);
    }

    // Only send a change event for items in the document (avoid firing during
    // parsing) and don't send a change event for a radio button that's getting
    // unchecked to match other browsers. DOM is not a useful standard for this
    // because it says only to fire change events at "lose focus" time, which is
    // definitely wrong in practice for these types of elements.
    if (eventBehavior != DispatchNoEvent && inDocument() && m_inputType->shouldSendChangeEventAfterCheckedChanged()) {
        setTextAsOfLastFormControlChangeEvent(String());
        dispatchFormControlChangeEvent();
    }

    didAffectSelector(AffectedSelectorChecked);
}

void HTMLInputElement::setIndeterminate(bool newValue)
{
    if (indeterminate() == newValue)
        return;

    m_isIndeterminate = newValue;

    didAffectSelector(AffectedSelectorIndeterminate);

    if (renderer() && renderer()->style()->hasAppearance())
        renderer()->theme()->stateChanged(renderer(), CheckedState);
}

int HTMLInputElement::size() const
{
    return m_size;
}

bool HTMLInputElement::sizeShouldIncludeDecoration(int& preferredSize) const
{
    return m_inputType->sizeShouldIncludeDecoration(defaultSize, preferredSize);
}

void HTMLInputElement::copyNonAttributePropertiesFromElement(const Element& source)
{
    const HTMLInputElement& sourceElement = static_cast<const HTMLInputElement&>(source);

    m_valueIfDirty = sourceElement.m_valueIfDirty;
    m_wasModifiedByUser = false;
    setChecked(sourceElement.m_isChecked);
    m_reflectsCheckedAttribute = sourceElement.m_reflectsCheckedAttribute;
    m_isIndeterminate = sourceElement.m_isIndeterminate;

    HTMLTextFormControlElement::copyNonAttributePropertiesFromElement(source);

    setFormControlValueMatchesRenderer(false);
    m_inputType->updateInnerTextValue();
}

String HTMLInputElement::value() const
{
    String value;
    if (m_inputType->getTypeSpecificValue(value))
        return value;

    value = m_valueIfDirty;
    if (!value.isNull())
        return value;

    AtomicString valueString = fastGetAttribute(valueAttr);
    value = sanitizeValue(valueString);
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
    setValue(value, DispatchChangeEvent);
}

const String& HTMLInputElement::suggestedValue() const
{
    return m_suggestedValue;
}

void HTMLInputElement::setSuggestedValue(const String& value)
{
    if (!m_inputType->canSetSuggestedValue())
        return;
    setFormControlValueMatchesRenderer(false);
    m_suggestedValue = sanitizeValue(value);
    setNeedsStyleRecalc();
    m_inputType->updateInnerTextValue();
}

void HTMLInputElement::setEditingValue(const String& value)
{
    if (!renderer() || !isTextField())
        return;
    setInnerTextValue(value);
    subtreeHasChanged();

    unsigned max = value.length();
    if (focused())
        setSelectionRange(max, max);
    else
        cacheSelectionInResponseToSetValue(max);

    dispatchInputEvent();
}

void HTMLInputElement::setValue(const String& value, ExceptionCode& ec, TextFieldEventBehavior eventBehavior)
{
    if (isFileUpload() && !value.isEmpty()) {
        ec = INVALID_STATE_ERR;
        return;
    }
    setValue(value, eventBehavior);
}

void HTMLInputElement::setValue(const String& value, TextFieldEventBehavior eventBehavior)
{
    if (!m_inputType->canSetValue(value))
        return;

    RefPtr<HTMLInputElement> protector(this);
    EventQueueScope scope;
    String sanitizedValue = sanitizeValue(value);
    bool valueChanged = sanitizedValue != this->value();

    setLastChangeWasNotUserEdit();
    setFormControlValueMatchesRenderer(false);
    m_suggestedValue = String(); // Prevent TextFieldInputType::setValue from using the suggested value.
    m_inputType->setValue(sanitizedValue, valueChanged, eventBehavior);

    if (!valueChanged)
        return;

    notifyFormStateChanged();
}

void HTMLInputElement::setValueInternal(const String& sanitizedValue, TextFieldEventBehavior eventBehavior)
{
    m_valueIfDirty = sanitizedValue;
    m_wasModifiedByUser = eventBehavior != DispatchNoEvent;
    setNeedsValidityCheck();
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
    return m_inputType->valueAsDouble();
}

void HTMLInputElement::setValueAsNumber(double newValue, ExceptionCode& ec, TextFieldEventBehavior eventBehavior)
{
    if (!std::isfinite(newValue)) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }
    m_inputType->setValueAsDouble(newValue, eventBehavior, ec);
}

void HTMLInputElement::setValueFromRenderer(const String& value)
{
    // File upload controls will never use this.
    ASSERT(!isFileUpload());

    m_suggestedValue = String();

    // Renderer and our event handler are responsible for sanitizing values.
    ASSERT(value == sanitizeValue(value) || sanitizeValue(value).isEmpty());

    // Workaround for bug where trailing \n is included in the result of textContent.
    // The assert macro above may also be simplified to: value == constrainValue(value)
    // http://bugs.webkit.org/show_bug.cgi?id=9661
    m_valueIfDirty = value == "\n" ? emptyString() : value;

    setFormControlValueMatchesRenderer(true);
    m_wasModifiedByUser = true;

    // Input event is fired by the Node::defaultEventHandler for editable controls.
    if (!isTextField())
        dispatchInputEvent();
    notifyFormStateChanged();

    setNeedsValidityCheck();

    // Clear autofill flag (and yellow background) on user edit.
    setAutofilled(false);
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

#if ENABLE(TOUCH_EVENTS)
    if (evt->isTouchEvent()) {
        m_inputType->handleTouchEvent(static_cast<TouchEvent*>(evt));
        if (evt->defaultHandled())
            return;
    }
#endif

    if (evt->isKeyboardEvent() && evt->type() == eventNames().keydownEvent) {
        m_inputType->handleKeydownEvent(static_cast<KeyboardEvent*>(evt));
        if (evt->defaultHandled())
            return;
    }

    // Call the base event handler before any of our own event handling for almost all events in text fields.
    // Makes editing keyboard handling take precedence over the keydown and keypress handling in this function.
    bool callBaseClassEarly = isTextField() && (evt->type() == eventNames().keydownEvent || evt->type() == eventNames().keypressEvent);
    if (callBaseClassEarly) {
        HTMLTextFormControlElement::defaultEventHandler(evt);
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

    if (evt->isMouseEvent() && evt->type() == eventNames().mousedownEvent) {
        m_inputType->handleMouseDownEvent(static_cast<MouseEvent*>(evt));
        if (evt->defaultHandled())
            return;
    }

    m_inputType->forwardEvent(evt);

    if (!callBaseClassEarly && !evt->defaultHandled())
        HTMLTextFormControlElement::defaultEventHandler(evt);
}

bool HTMLInputElement::willRespondToMouseClickEvents()
{
    // FIXME: Consider implementing willRespondToMouseClickEvents() in InputType if more accurate results are necessary.
    if (!isDisabledFormControl())
        return true;

    return HTMLTextFormControlElement::willRespondToMouseClickEvents();
}

bool HTMLInputElement::isURLAttribute(const Attribute& attribute) const
{
    return attribute.name() == srcAttr || attribute.name() == formactionAttr || HTMLTextFormControlElement::isURLAttribute(attribute);
}

String HTMLInputElement::defaultValue() const
{
    return fastGetAttribute(valueAttr);
}

void HTMLInputElement::setDefaultValue(const String &value)
{
    setAttribute(valueAttr, value);
}

static inline bool isRFC2616TokenCharacter(UChar ch)
{
    return isASCII(ch) && ch > ' ' && ch != '"' && ch != '(' && ch != ')' && ch != ',' && ch != '/' && (ch < ':' || ch > '@') && (ch < '[' || ch > ']') && ch != '{' && ch != '}' && ch != 0x7f;
}

static bool isValidMIMEType(const String& type)
{
    size_t slashPosition = type.find('/');
    if (slashPosition == notFound || !slashPosition || slashPosition == type.length() - 1)
        return false;
    for (size_t i = 0; i < type.length(); ++i) {
        if (!isRFC2616TokenCharacter(type[i]) && i != slashPosition)
            return false;
    }
    return true;
}

static bool isValidFileExtension(const String& type)
{
    if (type.length() < 2)
        return false;
    return type[0] == '.';
}

static Vector<String> parseAcceptAttribute(const String& acceptString, bool (*predicate)(const String&))
{
    Vector<String> types;
    if (acceptString.isEmpty())
        return types;

    Vector<String> splitTypes;
    acceptString.split(',', false, splitTypes);
    for (size_t i = 0; i < splitTypes.size(); ++i) {
        String trimmedType = stripLeadingAndTrailingHTMLSpaces(splitTypes[i]);
        if (trimmedType.isEmpty())
            continue;
        if (!predicate(trimmedType))
            continue;
        types.append(trimmedType.lower());
    }

    return types;
}

Vector<String> HTMLInputElement::acceptMIMETypes()
{
    return parseAcceptAttribute(fastGetAttribute(acceptAttr), isValidMIMEType);
}

Vector<String> HTMLInputElement::acceptFileExtensions()
{
    return parseAcceptAttribute(fastGetAttribute(acceptAttr), isValidFileExtension);
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
    return m_maxLength;
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

void HTMLInputElement::setSize(unsigned size, ExceptionCode& ec)
{
    if (!size)
        ec = INDEX_SIZE_ERR;
    else
        setSize(size);
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

void HTMLInputElement::setFiles(PassRefPtr<FileList> files)
{
    m_inputType->setFiles(files);
}

bool HTMLInputElement::receiveDroppedFiles(const DragData* dragData)
{
    return m_inputType->receiveDroppedFiles(dragData);
}

#if ENABLE(FILE_SYSTEM)
String HTMLInputElement::droppedFileSystemId()
{
    return m_inputType->droppedFileSystemId();
}
#endif

Icon* HTMLInputElement::icon() const
{
    return m_inputType->icon();
}

bool HTMLInputElement::canReceiveDroppedFiles() const
{
    return m_canReceiveDroppedFiles;
}

void HTMLInputElement::setCanReceiveDroppedFiles(bool canReceiveDroppedFiles)
{
    if (m_canReceiveDroppedFiles == canReceiveDroppedFiles)
        return;
    m_canReceiveDroppedFiles = canReceiveDroppedFiles;
    if (renderer())
        renderer()->updateFromElement();
}

String HTMLInputElement::visibleValue() const
{
    return m_inputType->visibleValue();
}

String HTMLInputElement::sanitizeValue(const String& proposedValue) const
{
    if (proposedValue.isNull())
        return proposedValue;
    return m_inputType->sanitizeValue(proposedValue);
}

String HTMLInputElement::localizeValue(const String& proposedValue) const
{
    if (proposedValue.isNull())
        return proposedValue;
    return m_inputType->localizeValue(proposedValue);
}

bool HTMLInputElement::isInRange() const
{
    return m_inputType->isInRange(value());
}

bool HTMLInputElement::isOutOfRange() const
{
    return m_inputType->isOutOfRange(value());
}

bool HTMLInputElement::needsSuspensionCallback()
{
    if (m_inputType->shouldResetOnDocumentActivation())
        return true;

    // Sensitive input elements are marked with autocomplete=off, and we want to wipe them out
    // when going back; returning true here arranges for us to call reset at the time
    // the page is restored. Non-empty textual default values indicate that the field
    // is not really sensitive -- there's no default value for an account number --
    // and we would see unexpected results if we reset to something other than blank.
    bool isSensitive = m_autocomplete == Off && !(m_inputType->isTextType() && !defaultValue().isEmpty());

    return isSensitive;
}

void HTMLInputElement::registerForSuspensionCallbackIfNeeded()
{
    if (needsSuspensionCallback())
        document()->registerForPageCacheSuspensionCallbacks(this);
}

void HTMLInputElement::unregisterForSuspensionCallbackIfNeeded()
{
    if (!needsSuspensionCallback())
        document()->unregisterForPageCacheSuspensionCallbacks(this);
}

bool HTMLInputElement::isRequiredFormControl() const
{
    return m_inputType->supportsRequired() && isRequired();
}

bool HTMLInputElement::matchesReadOnlyPseudoClass() const
{
    return m_inputType->supportsReadOnly() && isReadOnly();
}

bool HTMLInputElement::matchesReadWritePseudoClass() const
{
    return m_inputType->supportsReadOnly() && !isReadOnly();
}

void HTMLInputElement::addSearchResult()
{
    m_inputType->addSearchResult();
}

void HTMLInputElement::onSearch()
{
    ASSERT(isSearchField());
    if (m_inputType)
        static_cast<SearchInputType*>(m_inputType.get())->stopSearchEventTimer();
    dispatchEvent(Event::create(eventNames().searchEvent, true, false));
}

void HTMLInputElement::updateClearButtonVisibility()
{
    m_inputType->updateClearButtonVisibility();
}

void HTMLInputElement::documentDidResumeFromPageCache()
{
    ASSERT(needsSuspensionCallback());
    reset();
}

void HTMLInputElement::willChangeForm()
{
    removeFromRadioButtonGroup();
    HTMLTextFormControlElement::willChangeForm();
}

void HTMLInputElement::didChangeForm()
{
    HTMLTextFormControlElement::didChangeForm();
    addToRadioButtonGroup();
}

Node::InsertionNotificationRequest HTMLInputElement::insertedInto(ContainerNode* insertionPoint)
{
    HTMLTextFormControlElement::insertedInto(insertionPoint);
    if (insertionPoint->inDocument() && !form())
        addToRadioButtonGroup();
#if ENABLE(DATALIST_ELEMENT)
    resetListAttributeTargetObserver();
#endif
    return InsertionDone;
}

void HTMLInputElement::removedFrom(ContainerNode* insertionPoint)
{
    if (insertionPoint->inDocument() && !form())
        removeFromRadioButtonGroup();
    HTMLTextFormControlElement::removedFrom(insertionPoint);
    ASSERT(!inDocument());
#if ENABLE(DATALIST_ELEMENT)
    resetListAttributeTargetObserver();
#endif
}

void HTMLInputElement::didMoveToNewDocument(Document* oldDocument)
{
    if (hasImageLoader())
        imageLoader()->elementDidMoveToNewDocument();

    bool needsSuspensionCallback = this->needsSuspensionCallback();
    if (oldDocument) {
        // Always unregister for cache callbacks when leaving a document, even if we would otherwise like to be registered
        if (needsSuspensionCallback)
            oldDocument->unregisterForPageCacheSuspensionCallbacks(this);
        if (isRadioButton())
            oldDocument->formController().checkedRadioButtons().removeButton(this);
#if ENABLE(TOUCH_EVENTS)
        if (m_hasTouchEventHandler)
            oldDocument->didRemoveEventTargetNode(this);
#endif
    }

    if (needsSuspensionCallback)
        document()->registerForPageCacheSuspensionCallbacks(this);

#if ENABLE(TOUCH_EVENTS)
    if (m_hasTouchEventHandler)
        document()->didAddTouchEventHandler(this);
#endif

    HTMLTextFormControlElement::didMoveToNewDocument(oldDocument);
}

void HTMLInputElement::addSubresourceAttributeURLs(ListHashSet<KURL>& urls) const
{
    HTMLTextFormControlElement::addSubresourceAttributeURLs(urls);

    addSubresourceURL(urls, src());
}

bool HTMLInputElement::recalcWillValidate() const
{
    return m_inputType->supportsValidation() && HTMLTextFormControlElement::recalcWillValidate();
}

void HTMLInputElement::requiredAttributeChanged()
{
    HTMLTextFormControlElement::requiredAttributeChanged();
    if (CheckedRadioButtons* buttons = checkedRadioButtons())
        buttons->requiredAttributeChanged(this);
    m_inputType->requiredAttributeChanged();
}

#if ENABLE(INPUT_TYPE_COLOR)
void HTMLInputElement::selectColorInColorChooser(const Color& color)
{
    if (!m_inputType->isColorControl())
        return;
    static_cast<ColorInputType*>(m_inputType.get())->didChooseColor(color);
}
#endif
    
#if ENABLE(DATALIST_ELEMENT)
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

void HTMLInputElement::resetListAttributeTargetObserver()
{
    if (inDocument())
        m_listAttributeTargetObserver = ListAttributeTargetObserver::create(fastGetAttribute(listAttr), this);
    else
        m_listAttributeTargetObserver = nullptr;
}

void HTMLInputElement::listAttributeTargetChanged()
{
    m_inputType->listAttributeTargetChanged();
}
#endif // ENABLE(DATALIST_ELEMENT)

bool HTMLInputElement::isSteppable() const
{
    return m_inputType->isSteppable();
}

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

#if ENABLE(INPUT_TYPE_COLOR)
bool HTMLInputElement::isColorControl() const
{
    return m_inputType->isColorControl();
}
#endif

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

bool HTMLInputElement::isDateField() const
{
    return m_inputType->isDateField();
}

bool HTMLInputElement::isDateTimeField() const
{
    return m_inputType->isDateTimeField();
}

bool HTMLInputElement::isDateTimeLocalField() const
{
    return m_inputType->isDateTimeLocalField();
}

bool HTMLInputElement::isMonthField() const
{
    return m_inputType->isMonthField();
}

bool HTMLInputElement::isTimeField() const
{
    return m_inputType->isTimeField();
}

bool HTMLInputElement::isWeekField() const
{
    return m_inputType->isWeekField();
}

bool HTMLInputElement::isEnumeratable() const
{
    return m_inputType->isEnumeratable();
}

bool HTMLInputElement::supportLabels() const
{
    return m_inputType->supportLabels();
}

bool HTMLInputElement::shouldAppearChecked() const
{
    return checked() && m_inputType->isCheckable();
}

bool HTMLInputElement::supportsPlaceholder() const
{
    return m_inputType->supportsPlaceholder();
}

void HTMLInputElement::updatePlaceholderText()
{
    return m_inputType->updatePlaceholderText();
}

void HTMLInputElement::parseMaxLengthAttribute(const AtomicString& value)
{
    int maxLength;
    if (!parseHTMLInteger(value, maxLength))
        maxLength = maximumLength;
    if (maxLength < 0 || maxLength > maximumLength)
        maxLength = maximumLength;
    int oldMaxLength = m_maxLength;
    m_maxLength = maxLength;
    if (oldMaxLength != maxLength)
        updateValueIfNeeded();
    setNeedsStyleRecalc();
    setNeedsValidityCheck();
}

void HTMLInputElement::updateValueIfNeeded()
{
    String newValue = sanitizeValue(m_valueIfDirty);
    ASSERT(!m_valueIfDirty.isNull() || newValue.isNull());
    if (newValue != m_valueIfDirty)
        setValue(newValue);
}

String HTMLInputElement::defaultToolTip() const
{
    return m_inputType->defaultToolTip();
}

bool HTMLInputElement::shouldAppearIndeterminate() const 
{
    return m_inputType->supportsIndeterminateAppearance() && indeterminate();
}

#if ENABLE(MEDIA_CAPTURE)
String HTMLInputElement::capture() const
{
    if (!isFileUpload())
        return String();

    String capture = fastGetAttribute(captureAttr).lower();
    if (capture == "camera"
        || capture == "camcorder"
        || capture == "microphone"
        || capture == "filesystem")
        return capture;

    return "filesystem";
}

void HTMLInputElement::setCapture(const String& value)
{
    setAttribute(captureAttr, value);
}

#endif

bool HTMLInputElement::isInRequiredRadioButtonGroup()
{
    ASSERT(isRadioButton());
    if (CheckedRadioButtons* buttons = checkedRadioButtons())
        return buttons->isInRequiredGroup(this);
    return false;
}

HTMLInputElement* HTMLInputElement::checkedRadioButtonForGroup() const
{
    if (CheckedRadioButtons* buttons = checkedRadioButtons())
        return buttons->checkedButtonForGroup(name());
    return 0;
}

CheckedRadioButtons* HTMLInputElement::checkedRadioButtons() const
{
    if (!isRadioButton())
        return 0;
    if (HTMLFormElement* formElement = form())
        return &formElement->checkedRadioButtons();
    if (inDocument())
        return &document()->formController().checkedRadioButtons();
    return 0;
}

inline void HTMLInputElement::addToRadioButtonGroup()
{
    if (CheckedRadioButtons* buttons = checkedRadioButtons())
        buttons->addButton(this);
}

inline void HTMLInputElement::removeFromRadioButtonGroup()
{
    if (CheckedRadioButtons* buttons = checkedRadioButtons())
        buttons->removeButton(this);
}

unsigned HTMLInputElement::height() const
{
    return m_inputType->height();
}

unsigned HTMLInputElement::width() const
{
    return m_inputType->width();
}

void HTMLInputElement::setHeight(unsigned height)
{
    setAttribute(heightAttr, String::number(height));
}

void HTMLInputElement::setWidth(unsigned width)
{
    setAttribute(widthAttr, String::number(width));
}

#if ENABLE(DATALIST_ELEMENT)
PassOwnPtr<ListAttributeTargetObserver> ListAttributeTargetObserver::create(const AtomicString& id, HTMLInputElement* element)
{
    return adoptPtr(new ListAttributeTargetObserver(id, element));
}

ListAttributeTargetObserver::ListAttributeTargetObserver(const AtomicString& id, HTMLInputElement* element)
    : IdTargetObserver(element->treeScope()->idTargetObserverRegistry(), id)
    , m_element(element)
{
}

void ListAttributeTargetObserver::idTargetChanged()
{
    m_element->listAttributeTargetChanged();
}
#endif

void HTMLInputElement::setRangeText(const String& replacement, ExceptionCode& ec)
{
    if (!m_inputType->supportsSelectionAPI()) {
        ec = INVALID_STATE_ERR;
        return;
    }

    HTMLTextFormControlElement::setRangeText(replacement, ec);
}

void HTMLInputElement::setRangeText(const String& replacement, unsigned start, unsigned end, const String& selectionMode, ExceptionCode& ec)
{
    if (!m_inputType->supportsSelectionAPI()) {
        ec = INVALID_STATE_ERR;
        return;
    }

    HTMLTextFormControlElement::setRangeText(replacement, start, end, selectionMode, ec);
}

#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
bool HTMLInputElement::setupDateTimeChooserParameters(DateTimeChooserParameters& parameters)
{
    if (!document()->view())
        return false;

    parameters.type = type();
    parameters.minimum = minimum();
    parameters.maximum = maximum();
    parameters.required = isRequired();
    if (!RuntimeEnabledFeatures::langAttributeAwareFormControlUIEnabled())
        parameters.locale = defaultLanguage();
    else {
        AtomicString computedLocale = computeInheritedLanguage();
        parameters.locale = computedLocale.isEmpty() ? AtomicString(defaultLanguage()) : computedLocale;
    }

    StepRange stepRange = createStepRange(RejectAny);
    if (stepRange.hasStep()) {
        parameters.step = stepRange.step().toDouble();
        parameters.stepBase = stepRange.stepBase().toDouble();
    } else {
        parameters.step = 1.0;
        parameters.stepBase = 0;
    }

    parameters.anchorRectInRootView = document()->view()->contentsToRootView(pixelSnappedBoundingBox());
    parameters.currentValue = value();
    parameters.isAnchorElementRTL = computedStyle()->direction() == RTL;
#if ENABLE(DATALIST_ELEMENT)
    if (HTMLDataListElement* dataList = this->dataList()) {
        RefPtr<HTMLCollection> options = dataList->options();
        for (unsigned i = 0; HTMLOptionElement* option = toHTMLOptionElement(options->item(i)); ++i) {
            if (!isValidValue(option->value()))
                continue;
            parameters.suggestionValues.append(sanitizeValue(option->value()));
            parameters.localizedSuggestionValues.append(localizeValue(option->value()));
            parameters.suggestionLabels.append(option->value() == option->label() ? String() : option->label());
        }
    }
#endif
    return true;
}
#endif

} // namespace
