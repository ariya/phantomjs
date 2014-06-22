/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2010 Apple Inc. All rights reserved.
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

#ifndef HTMLInputElement_h
#define HTMLInputElement_h

#include "FileChooser.h"
#include "HTMLTextFormControlElement.h"
#include "StepRange.h"

namespace WebCore {

class CheckedRadioButtons;
class DragData;
class FileList;
class HTMLDataListElement;
class HTMLImageLoader;
class HTMLOptionElement;
class Icon;
class InputType;
class KURL;
class ListAttributeTargetObserver;
struct DateTimeChooserParameters;

class HTMLInputElement : public HTMLTextFormControlElement {
public:
    static PassRefPtr<HTMLInputElement> create(const QualifiedName&, Document*, HTMLFormElement*, bool createdByParser);
    virtual ~HTMLInputElement();

    DEFINE_ATTRIBUTE_EVENT_LISTENER(webkitspeechchange);

    virtual HTMLInputElement* toInputElement() { return this; }

    virtual bool shouldAutocomplete() const;

    // For ValidityState
    virtual bool hasBadInput() const OVERRIDE;
    virtual bool patternMismatch() const OVERRIDE;
    virtual bool rangeUnderflow() const OVERRIDE;
    virtual bool rangeOverflow() const;
    virtual bool stepMismatch() const OVERRIDE;
    virtual bool tooLong() const OVERRIDE;
    virtual bool typeMismatch() const OVERRIDE;
    virtual bool valueMissing() const OVERRIDE;
    virtual String validationMessage() const OVERRIDE;

    // Returns the minimum value for type=date, number, or range.  Don't call this for other types.
    double minimum() const;
    // Returns the maximum value for type=date, number, or range.  Don't call this for other types.
    // This always returns a value which is >= minimum().
    double maximum() const;
    // Sets the "allowed value step" defined in the HTML spec to the specified double pointer.
    // Returns false if there is no "allowed value step."
    bool getAllowedValueStep(Decimal*) const;
    StepRange createStepRange(AnyStepHandling) const;

#if ENABLE(DATALIST_ELEMENT)
    Decimal findClosestTickMarkValue(const Decimal&);
#endif

    // Implementations of HTMLInputElement::stepUp() and stepDown().
    void stepUp(int, ExceptionCode&);
    void stepDown(int, ExceptionCode&);
    void stepUp(ExceptionCode& ec) { stepUp(1, ec); }
    void stepDown(ExceptionCode& ec) { stepDown(1, ec); }
    // stepUp()/stepDown() for user-interaction.
    bool isSteppable() const;

    bool isTextButton() const;

    bool isRadioButton() const;
    bool isTextField() const;
    bool isSearchField() const;
    bool isInputTypeHidden() const;
    bool isPasswordField() const;
    bool isCheckbox() const;
    bool isRangeControl() const;

#if ENABLE(INPUT_TYPE_COLOR)
    bool isColorControl() const;
#endif

    // FIXME: It's highly likely that any call site calling this function should instead
    // be using a different one. Many input elements behave like text fields, and in addition
    // any unknown input type is treated as text. Consider, for example, isTextField or
    // isTextField && !isPasswordField.
    bool isText() const;

    bool isEmailField() const;
    bool isFileUpload() const;
    bool isImageButton() const;
    bool isNumberField() const;
    bool isSubmitButton() const;
    bool isTelephoneField() const;
    bool isURLField() const;
    bool isDateField() const;
    bool isDateTimeField() const;
    bool isDateTimeLocalField() const;
    bool isMonthField() const;
    bool isTimeField() const;
    bool isWeekField() const;

#if ENABLE(INPUT_SPEECH)
    bool isSpeechEnabled() const;
#endif

    HTMLElement* containerElement() const;
    virtual HTMLElement* innerTextElement() const;
    HTMLElement* innerBlockElement() const;
    HTMLElement* innerSpinButtonElement() const;
    HTMLElement* resultsButtonElement() const;
    HTMLElement* cancelButtonElement() const;
#if ENABLE(INPUT_SPEECH)
    HTMLElement* speechButtonElement() const;
#endif
    HTMLElement* sliderThumbElement() const;
    HTMLElement* sliderTrackElement() const;
    virtual HTMLElement* placeholderElement() const;

    bool checked() const { return m_isChecked; }
    void setChecked(bool, TextFieldEventBehavior = DispatchNoEvent);

    // 'indeterminate' is a state independent of the checked state that causes the control to draw in a way that hides the actual state.
    bool indeterminate() const { return m_isIndeterminate; }
    void setIndeterminate(bool);
    // shouldAppearChecked is used by the rendering tree/CSS while checked() is used by JS to determine checked state
    bool shouldAppearChecked() const;
    virtual bool shouldAppearIndeterminate() const OVERRIDE;

    int size() const;
    bool sizeShouldIncludeDecoration(int& preferredSize) const;

    void setType(const String&);

    String value() const;
    void setValue(const String&, ExceptionCode&, TextFieldEventBehavior = DispatchNoEvent);
    void setValue(const String&, TextFieldEventBehavior = DispatchNoEvent);
    void setValueForUser(const String&);
    // Checks if the specified string would be a valid value.
    // We should not call this for types with no string value such as CHECKBOX and RADIO.
    bool isValidValue(const String&) const;
    bool hasDirtyValue() const { return !m_valueIfDirty.isNull(); };

    String sanitizeValue(const String&) const;

    String localizeValue(const String&) const;

    // The value which is drawn by a renderer.
    String visibleValue() const;

    const String& suggestedValue() const;
    void setSuggestedValue(const String&);

    void setEditingValue(const String&);

    double valueAsDate() const;
    void setValueAsDate(double, ExceptionCode&);

    double valueAsNumber() const;
    void setValueAsNumber(double, ExceptionCode&, TextFieldEventBehavior = DispatchNoEvent);

    String valueWithDefault() const;

    void setValueFromRenderer(const String&);

    bool canHaveSelection() const;

    virtual bool rendererIsNeeded(const NodeRenderingContext&);
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
    virtual void detach(const AttachContext& = AttachContext()) OVERRIDE;

    // FIXME: For isActivatedSubmit and setActivatedSubmit, we should use the NVI-idiom here by making
    // it private virtual in all classes and expose a public method in HTMLFormControlElement to call
    // the private virtual method.
    virtual bool isActivatedSubmit() const;
    virtual void setActivatedSubmit(bool flag);

    String altText() const;

    int maxResults() const { return m_maxResults; }

    String defaultValue() const;
    void setDefaultValue(const String&);

    Vector<String> acceptMIMETypes();
    Vector<String> acceptFileExtensions();
    String accept() const;
    String alt() const;

    void setSize(unsigned);
    void setSize(unsigned, ExceptionCode&);

    KURL src() const;

    virtual int maxLength() const;
    void setMaxLength(int, ExceptionCode&);

    bool multiple() const;

    bool isAutofilled() const { return m_isAutofilled; }
    void setAutofilled(bool = true);

    FileList* files();
    void setFiles(PassRefPtr<FileList>);

    // Returns true if the given DragData has more than one dropped files.
    bool receiveDroppedFiles(const DragData*);

#if ENABLE(FILE_SYSTEM)
    String droppedFileSystemId();
#endif

    Icon* icon() const;
    // These functions are used for rendering the input active during a
    // drag-and-drop operation.
    bool canReceiveDroppedFiles() const;
    void setCanReceiveDroppedFiles(bool);

    void addSearchResult();
    void onSearch();

    void updateClearButtonVisibility();

    virtual bool willRespondToMouseClickEvents() OVERRIDE;

#if ENABLE(DATALIST_ELEMENT)
    HTMLElement* list() const;
    HTMLDataListElement* dataList() const;
    void listAttributeTargetChanged();
#endif

    HTMLInputElement* checkedRadioButtonForGroup() const;
    bool isInRequiredRadioButtonGroup();

    // Functions for InputType classes.
    void setValueInternal(const String&, TextFieldEventBehavior);
    bool isTextFormControlFocusable() const;
    bool isTextFormControlKeyboardFocusable(KeyboardEvent*) const;
    bool isTextFormControlMouseFocusable() const;
    bool valueAttributeWasUpdatedAfterParsing() const { return m_valueAttributeWasUpdatedAfterParsing; }

    void cacheSelectionInResponseToSetValue(int caretOffset) { cacheSelection(caretOffset, caretOffset, SelectionHasNoDirection); }

#if ENABLE(INPUT_TYPE_COLOR)
    // For test purposes.
    void selectColorInColorChooser(const Color&);
#endif

    String defaultToolTip() const;

#if ENABLE(MEDIA_CAPTURE)
    String capture() const;
    void setCapture(const String& value);
#endif

    static const int maximumLength;

    unsigned height() const;
    unsigned width() const;
    void setHeight(unsigned);
    void setWidth(unsigned);

    virtual void blur() OVERRIDE;
    void defaultBlur();

    virtual const AtomicString& name() const OVERRIDE;

    void endEditing();

    static Vector<FileChooserFileInfo> filesFromFileInputFormControlState(const FormControlState&);

    virtual bool matchesReadOnlyPseudoClass() const OVERRIDE;
    virtual bool matchesReadWritePseudoClass() const OVERRIDE;
    virtual void setRangeText(const String& replacement, ExceptionCode&) OVERRIDE;
    virtual void setRangeText(const String& replacement, unsigned start, unsigned end, const String& selectionMode, ExceptionCode&) OVERRIDE;

    bool hasImageLoader() const { return m_imageLoader; }
    HTMLImageLoader* imageLoader();

#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
    bool setupDateTimeChooserParameters(DateTimeChooserParameters&);
#endif

protected:
    HTMLInputElement(const QualifiedName&, Document*, HTMLFormElement*, bool createdByParser);

    virtual void defaultEventHandler(Event*);

private:
    enum AutoCompleteSetting { Uninitialized, On, Off };

    // FIXME: Author shadows should be allowed
    // https://bugs.webkit.org/show_bug.cgi?id=92608
    virtual bool areAuthorShadowsAllowed() const OVERRIDE { return false; }

    virtual void didAddUserAgentShadowRoot(ShadowRoot*) OVERRIDE;

    virtual void willChangeForm() OVERRIDE;
    virtual void didChangeForm() OVERRIDE;
    virtual InsertionNotificationRequest insertedInto(ContainerNode*) OVERRIDE;
    virtual void removedFrom(ContainerNode*) OVERRIDE;
    virtual void didMoveToNewDocument(Document* oldDocument) OVERRIDE;

    virtual bool hasCustomFocusLogic() const OVERRIDE;
    virtual bool isKeyboardFocusable(KeyboardEvent*) const OVERRIDE;
    virtual bool isMouseFocusable() const OVERRIDE;
    virtual bool isEnumeratable() const;
    virtual bool supportLabels() const OVERRIDE;
    virtual void updateFocusAppearance(bool restorePreviousSelection);
    virtual bool shouldUseInputMethod() OVERRIDE FINAL;

    virtual bool isTextFormControl() const { return isTextField(); }

    virtual bool canTriggerImplicitSubmission() const { return isTextField(); }

    virtual const AtomicString& formControlType() const;

    virtual bool shouldSaveAndRestoreFormControlState() const OVERRIDE;
    virtual FormControlState saveFormControlState() const OVERRIDE;
    virtual void restoreFormControlState(const FormControlState&) OVERRIDE;

    virtual bool canStartSelection() const;

    virtual void accessKeyAction(bool sendMouseEvents);

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual bool isPresentationAttribute(const QualifiedName&) const OVERRIDE;
    virtual void collectStyleForPresentationAttribute(const QualifiedName&, const AtomicString&, MutableStylePropertySet*) OVERRIDE;
    virtual void finishParsingChildren();

    virtual void copyNonAttributePropertiesFromElement(const Element&);

    virtual void attach(const AttachContext& = AttachContext()) OVERRIDE;

    virtual bool appendFormData(FormDataList&, bool);

    virtual bool isSuccessfulSubmitButton() const;

    virtual void reset();

    virtual void* preDispatchEventHandler(Event*);
    virtual void postDispatchEventHandler(Event*, void* dataFromPreDispatch);

    virtual bool isURLAttribute(const Attribute&) const OVERRIDE;
    virtual bool isInRange() const;
    virtual bool isOutOfRange() const;

    virtual void documentDidResumeFromPageCache();

    virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;

    bool needsSuspensionCallback();
    void registerForSuspensionCallbackIfNeeded();
    void unregisterForSuspensionCallbackIfNeeded();

    bool supportsMaxLength() const { return isTextType(); }
    bool isTextType() const;
    bool tooLong(const String&, NeedsToCheckDirtyFlag) const;

    virtual bool supportsPlaceholder() const;
    virtual void updatePlaceholderText();
    virtual bool isEmptyValue() const OVERRIDE { return innerTextValue().isEmpty(); }
    virtual bool isEmptySuggestedValue() const { return suggestedValue().isEmpty(); }
    virtual void handleFocusEvent(Node* oldFocusedNode, FocusDirection) OVERRIDE;
    virtual void handleBlurEvent();

    virtual bool isOptionalFormControl() const { return !isRequiredFormControl(); }
    virtual bool isRequiredFormControl() const;
    virtual bool recalcWillValidate() const;
    virtual void requiredAttributeChanged() OVERRIDE;

    void updateType();
    
    virtual void subtreeHasChanged();

#if ENABLE(DATALIST_ELEMENT)
    void resetListAttributeTargetObserver();
#endif
    void parseMaxLengthAttribute(const AtomicString&);
    void updateValueIfNeeded();

    // Returns null if this isn't associated with any radio button group.
    CheckedRadioButtons* checkedRadioButtons() const;
    void addToRadioButtonGroup();
    void removeFromRadioButtonGroup();

    AtomicString m_name;
    String m_valueIfDirty;
    String m_suggestedValue;
    int m_size;
    int m_maxLength;
    short m_maxResults;
    bool m_isChecked : 1;
    bool m_reflectsCheckedAttribute : 1;
    bool m_isIndeterminate : 1;
    bool m_hasType : 1;
    bool m_isActivatedSubmit : 1;
    unsigned m_autocomplete : 2; // AutoCompleteSetting
    bool m_isAutofilled : 1;
#if ENABLE(DATALIST_ELEMENT)
    bool m_hasNonEmptyList : 1;
#endif
    bool m_stateRestored : 1;
    bool m_parsingInProgress : 1;
    bool m_valueAttributeWasUpdatedAfterParsing : 1;
    bool m_wasModifiedByUser : 1;
    bool m_canReceiveDroppedFiles : 1;
#if ENABLE(TOUCH_EVENTS)
    bool m_hasTouchEventHandler : 1;
#endif
    OwnPtr<InputType> m_inputType;
    // The ImageLoader must be owned by this element because the loader code assumes
    // that it lives as long as its owning element lives. If we move the loader into
    // the ImageInput object we may delete the loader while this element lives on.
    OwnPtr<HTMLImageLoader> m_imageLoader;
#if ENABLE(DATALIST_ELEMENT)
    OwnPtr<ListAttributeTargetObserver> m_listAttributeTargetObserver;
#endif
};

inline bool isHTMLInputElement(Node* node)
{
    return node->hasTagName(HTMLNames::inputTag);
}

inline bool isHTMLInputElement(Element* element)
{
    return element->hasTagName(HTMLNames::inputTag);
}

inline HTMLInputElement* toHTMLInputElement(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || isHTMLInputElement(node));
    return static_cast<HTMLInputElement*>(node);
}

} //namespace
#endif
