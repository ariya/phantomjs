/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef InputType_h
#define InputType_h

#include "FeatureObserver.h"
#include "HTMLTextFormControlElement.h"
#include "StepRange.h"
#include <wtf/Forward.h>
#include <wtf/FastAllocBase.h>
#include <wtf/Noncopyable.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class BeforeTextInsertedEvent;
class Chrome;
class Color;
class DateComponents;
class DragData;
class Event;
class FileList;
class FormDataList;
class HTMLElement;
class HTMLFormElement;
class HTMLInputElement;
class Icon;
class KeyboardEvent;
class MouseEvent;
class Node;
class RenderArena;
class RenderObject;
class RenderStyle;
class TouchEvent;

typedef int ExceptionCode;

struct ClickHandlingState {
    WTF_MAKE_FAST_ALLOCATED;
  
public:
    bool checked;
    bool indeterminate;
    RefPtr<HTMLInputElement> checkedRadioButton;
};

// An InputType object represents the type-specific part of an HTMLInputElement.
// Do not expose instances of InputType and classes derived from it to classes
// other than HTMLInputElement.
class InputType {
    WTF_MAKE_NONCOPYABLE(InputType);
    WTF_MAKE_FAST_ALLOCATED;

public:
    static PassOwnPtr<InputType> create(HTMLInputElement*, const AtomicString&);
    static PassOwnPtr<InputType> createText(HTMLInputElement*);
    virtual ~InputType();

    static bool themeSupportsDataListUI(InputType*);

    virtual const AtomicString& formControlType() const = 0;
    virtual bool canChangeFromAnotherType() const;

    // Type query functions

    // Any time we are using one of these functions it's best to refactor
    // to add a virtual function to allow the input type object to do the
    // work instead, or at least make a query function that asks a higher
    // level question. These functions make the HTMLInputElement class
    // inflexible because it's harder to add new input types if there is
    // scattered code with special cases for various types.

#if ENABLE(INPUT_TYPE_COLOR)
    virtual bool isColorControl() const;
#endif
    virtual bool isCheckbox() const;
    virtual bool isDateField() const;
    virtual bool isDateTimeField() const;
    virtual bool isDateTimeLocalField() const;
    virtual bool isEmailField() const;
    virtual bool isFileUpload() const;
    virtual bool isHiddenType() const;
    virtual bool isImageButton() const;
    virtual bool supportLabels() const;
    virtual bool isMonthField() const;
    virtual bool isNumberField() const;
    virtual bool isPasswordField() const;
    virtual bool isRadioButton() const;
    virtual bool isRangeControl() const;
    virtual bool isSearchField() const;
    virtual bool isSubmitButton() const;
    virtual bool isTelephoneField() const;
    virtual bool isTextButton() const;
    virtual bool isTextField() const;
    virtual bool isTextType() const;
    virtual bool isTimeField() const;
    virtual bool isURLField() const;
    virtual bool isWeekField() const;

    // Form value functions

    virtual bool shouldSaveAndRestoreFormControlState() const;
    virtual FormControlState saveFormControlState() const;
    virtual void restoreFormControlState(const FormControlState&);
    virtual bool isFormDataAppendable() const;
    virtual bool appendFormData(FormDataList&, bool multipart) const;

    // DOM property functions

    virtual bool getTypeSpecificValue(String&); // Checked first, before internal storage or the value attribute.
    virtual String fallbackValue() const; // Checked last, if both internal storage and value attribute are missing.
    virtual String defaultValue() const; // Checked after even fallbackValue, only when the valueWithDefault function is called.
    virtual double valueAsDate() const;
    virtual void setValueAsDate(double, ExceptionCode&) const;
    virtual double valueAsDouble() const;
    virtual void setValueAsDouble(double, TextFieldEventBehavior, ExceptionCode&) const;
    virtual void setValueAsDecimal(const Decimal&, TextFieldEventBehavior, ExceptionCode&) const;

    // Validation functions
    virtual String validationMessage() const;
    virtual bool supportsValidation() const;
    virtual bool typeMismatchFor(const String&) const;
    // Type check for the current input value. We do nothing for some types
    // though typeMismatchFor() does something for them because of value
    // sanitization.
    virtual bool typeMismatch() const;
    virtual bool supportsRequired() const;
    virtual bool valueMissing(const String&) const;
    virtual bool hasBadInput() const;
    virtual bool patternMismatch(const String&) const;
    bool rangeUnderflow(const String&) const;
    bool rangeOverflow(const String&) const;
    bool isInRange(const String&) const;
    bool isOutOfRange(const String&) const;
    virtual Decimal defaultValueForStepUp() const;
    double minimum() const;
    double maximum() const;
    virtual bool sizeShouldIncludeDecoration(int defaultSize, int& preferredSize) const;
    bool stepMismatch(const String&) const;
    virtual bool getAllowedValueStep(Decimal*) const;
    virtual StepRange createStepRange(AnyStepHandling) const;
    virtual void stepUp(int, ExceptionCode&);
    virtual void stepUpFromRenderer(int);
    virtual String badInputText() const;
    virtual String typeMismatchText() const;
    virtual String valueMissingText() const;
    virtual bool canSetStringValue() const;
    virtual String localizeValue(const String&) const;
    virtual String visibleValue() const;
    // Returing the null string means "use the default value."
    // This function must be called only by HTMLInputElement::sanitizeValue().
    virtual String sanitizeValue(const String&) const;

    // Event handlers

    virtual void handleClickEvent(MouseEvent*);
    virtual void handleMouseDownEvent(MouseEvent*);
    virtual PassOwnPtr<ClickHandlingState> willDispatchClick();
    virtual void didDispatchClick(Event*, const ClickHandlingState&);
    virtual void handleDOMActivateEvent(Event*);
    virtual void handleKeydownEvent(KeyboardEvent*);
    virtual void handleKeypressEvent(KeyboardEvent*);
    virtual void handleKeyupEvent(KeyboardEvent*);
    virtual void handleBeforeTextInsertedEvent(BeforeTextInsertedEvent*);
#if ENABLE(TOUCH_EVENTS)
    virtual void handleTouchEvent(TouchEvent*);
#endif
    virtual void forwardEvent(Event*);
    // Helpers for event handlers.
    virtual bool shouldSubmitImplicitly(Event*);
    virtual PassRefPtr<HTMLFormElement> formForSubmission() const;
    virtual bool hasCustomFocusLogic() const;
    virtual bool isKeyboardFocusable(KeyboardEvent*) const;
    virtual bool isMouseFocusable() const;
    virtual bool shouldUseInputMethod() const;
    virtual void handleFocusEvent(Node* oldFocusedNode, FocusDirection);
    virtual void handleBlurEvent();
    virtual void accessKeyAction(bool sendMouseEvents);
    virtual bool canBeSuccessfulSubmitButton();
    virtual void subtreeHasChanged();
#if ENABLE(TOUCH_EVENTS)
    virtual bool hasTouchEventHandler() const;
#endif

    virtual void blur();

    // Shadow tree handling

    virtual void createShadowSubtree();
    virtual void destroyShadowSubtree();

    virtual HTMLElement* containerElement() const { return 0; }
    virtual HTMLElement* innerBlockElement() const { return 0; }
    virtual HTMLElement* innerTextElement() const { return 0; }
    virtual HTMLElement* innerSpinButtonElement() const { return 0; }
    virtual HTMLElement* resultsButtonElement() const { return 0; }
    virtual HTMLElement* cancelButtonElement() const { return 0; }
#if ENABLE(INPUT_SPEECH)
    virtual HTMLElement* speechButtonElement() const { return 0; }
#endif
    virtual HTMLElement* sliderThumbElement() const { return 0; }
    virtual HTMLElement* sliderTrackElement() const { return 0; }
    virtual HTMLElement* placeholderElement() const;

    // Miscellaneous functions

    virtual bool rendererIsNeeded();
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*) const;
    virtual void addSearchResult();
    virtual void attach();
    virtual void detach();
    virtual void minOrMaxAttributeChanged();
    virtual void stepAttributeChanged();
    virtual void altAttributeChanged();
    virtual void srcAttributeChanged();
    virtual bool shouldRespectAlignAttribute();
    virtual FileList* files();
    virtual void setFiles(PassRefPtr<FileList>);
    // Should return true if the given DragData has more than one dropped files.
    virtual bool receiveDroppedFiles(const DragData*);
#if ENABLE(FILE_SYSTEM)
    virtual String droppedFileSystemId();
#endif
    virtual Icon* icon() const;
    // Should return true if the corresponding renderer for a type can display a suggested value.
    virtual bool canSetSuggestedValue();
    virtual bool shouldSendChangeEventAfterCheckedChanged();
    virtual bool canSetValue(const String&);
    virtual bool storesValueSeparateFromAttribute();
    virtual void setValue(const String&, bool valueChanged, TextFieldEventBehavior);
    virtual bool shouldResetOnDocumentActivation();
    virtual bool shouldRespectListAttribute();
    virtual bool shouldRespectSpeechAttribute();
    virtual bool isEnumeratable();
    virtual bool isCheckable();
    virtual bool isSteppable() const;
    virtual bool shouldRespectHeightAndWidthAttributes();
    virtual bool supportsPlaceholder() const;
    virtual bool supportsReadOnly() const;
    virtual void updateInnerTextValue();
    virtual void updatePlaceholderText();
    virtual void attributeChanged();
    virtual void multipleAttributeChanged();
    virtual void disabledAttributeChanged();
    virtual void readonlyAttributeChanged();
    virtual void requiredAttributeChanged();
    virtual void valueAttributeChanged();
    virtual String defaultToolTip() const;
#if ENABLE(DATALIST_ELEMENT)
    virtual void listAttributeTargetChanged();
    virtual Decimal findClosestTickMarkValue(const Decimal&);
#endif
    virtual void updateClearButtonVisibility();

    // Parses the specified string for the type, and return
    // the Decimal value for the parsing result if the parsing
    // succeeds; Returns defaultValue otherwise. This function can
    // return NaN or Infinity only if defaultValue is NaN or Infinity.
    virtual Decimal parseToNumber(const String&, const Decimal& defaultValue) const;

    // Parses the specified string for this InputType, and returns true if it
    // is successfully parsed. An instance pointed by the DateComponents*
    // parameter will have parsed values and be modified even if the parsing
    // fails. The DateComponents* parameter may be 0.
    virtual bool parseToDateComponents(const String&, DateComponents*) const;

    // Create a string representation of the specified Decimal value for the
    // input type. If NaN or Infinity is specified, this returns an empty
    // string. This should not be called for types without valueAsNumber.
    virtual String serialize(const Decimal&) const;

    virtual bool supportsIndeterminateAppearance() const;

    virtual bool supportsSelectionAPI() const;

    // Gets width and height of the input element if the type of the
    // element is image. It returns 0 if the element is not image type.
    virtual unsigned height() const;
    virtual unsigned width() const;

    void dispatchSimulatedClickIfActive(KeyboardEvent*) const;

protected:
    InputType(HTMLInputElement* element) : m_element(element) { }
    HTMLInputElement* element() const { return m_element; }
    Chrome* chrome() const;
    Decimal parseToNumberOrNaN(const String&) const;
    void observeFeatureIfVisible(FeatureObserver::Feature) const;

private:
    // Helper for stepUp()/stepDown(). Adds step value * count to the current value.
    void applyStep(int count, AnyStepHandling, TextFieldEventBehavior, ExceptionCode&);

    // Raw pointer because the HTMLInputElement object owns this InputType object.
    HTMLInputElement* m_element;
};

} // namespace WebCore
#endif
