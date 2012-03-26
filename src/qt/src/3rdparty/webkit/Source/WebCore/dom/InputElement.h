/*
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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

#ifndef InputElement_h
#define InputElement_h

#include "PlatformString.h"
#include <wtf/text/AtomicString.h>

namespace WebCore {

class Attribute;
class Document;
class Element;
class Event;
class InputElementData;

class InputElement {
public:
    virtual ~InputElement() { }

    virtual bool isAutofilled() const = 0;
    virtual bool isChecked() const = 0;
    virtual bool isIndeterminate() const = 0;
    virtual bool isInputTypeHidden() const = 0;
    virtual bool isPasswordField() const = 0;
    virtual bool isSearchField() const = 0;
    virtual bool isTextField() const = 0;
    virtual bool isRadioButton() const = 0;
    virtual bool isCheckbox() const = 0;
    virtual bool isRangeControl() const = 0;
    
    virtual bool supportsMaxLength() const = 0;
    virtual bool isSteppable() const = 0;
#if ENABLE(INPUT_SPEECH)
    virtual bool isSpeechEnabled() const = 0;
#endif    

    virtual bool searchEventsShouldBeDispatched() const = 0;

    virtual int size() const = 0;
    virtual const String& suggestedValue() const = 0;
    virtual String value() const = 0;
    virtual void setValue(const String&, bool sendChangeEvent = false) = 0;
    virtual void setValueForUser(const String&) = 0;
    // The value which is drawn by a renderer.
    virtual String visibleValue() const = 0;
    virtual String convertFromVisibleValue(const String&) const = 0;

    // Returns true if the specified string can be set as the value of InputElement.
    virtual bool isAcceptableValue(const String&) const = 0;
    virtual String sanitizeValue(const String&) const = 0;
    virtual void setValueFromRenderer(const String&) = 0;

    virtual void cacheSelection(int start, int end) = 0;
    virtual void select() = 0;
  
#if ENABLE(WCSS)
    virtual InputElementData data() const = 0; 
#endif

    static const int s_maximumLength;
    static const int s_defaultSize;

    // Replaces CRs and LFs, shrinks the value for s_maximumLength.
    // This should be applied to values from the HTML value attribute and the DOM value property.
    // This function should be called only by sanitizeValue() implementations.
    // Public so it can be called by InputType.
    static String sanitizeValueForTextField(const InputElement*, const String&);

protected:
    static void dispatchFocusEvent(InputElement*, Element*);
    static void dispatchBlurEvent(InputElement*, Element*);
    static void updateFocusAppearance(InputElementData&, InputElement*, Element*, bool restorePreviousSelection);
    static void updateSelectionRange(InputElement*, Element*, int start, int end);
    static void aboutToUnload(InputElement*, Element*);
    static void setValueFromRenderer(InputElementData&, InputElement*, Element*, const String&);
    // Replaces CRs and LFs, shrinks the value for the specified maximum length.
    // This should be applied to values specified by users.
    // The input string may be a fragment of the whole value.
    static String sanitizeUserInputValue(const InputElement*, const String&, int);
    static void handleBeforeTextInsertedEvent(InputElementData&, InputElement*, Element*, Event*);
    static void parseSizeAttribute(InputElementData&, Element*, Attribute*);
    static void parseMaxLengthAttribute(InputElementData&, InputElement*, Element*, Attribute*);
    static void updateValueIfNeeded(InputElementData&, InputElement*);
    static void notifyFormStateChanged(Element*);
#if ENABLE(WCSS)
    static bool isConformToInputMask(const InputElementData&, const String&);
    static bool isConformToInputMask(const InputElementData&, UChar, unsigned);
    static String validateInputMask(InputElementData&, String&);
#endif
};

// HTMLInputElement hold this struct as member variable
// and pass it to the static helper functions in InputElement
class InputElementData {
public:
    InputElementData();
    ~InputElementData();

    const AtomicString& name() const;
    void setName(const AtomicString& value) { m_name = value; }

    // The null String represents "use the default value," and the empty String
    // represents the empty value.
    String value() const { return m_value; }
    void setValue(const String& value) { m_value = value; }

    const String& suggestedValue() const { return m_suggestedValue; }
    void setSuggestedValue(const String& value) { m_suggestedValue = value; }

    int size() const { return m_size; }
    void setSize(int value) { m_size = value; }

    int maxLength() const { return m_maxLength; }
    void setMaxLength(int value) { m_maxLength = value; }

    int cachedSelectionStart() const { return m_cachedSelectionStart; }
    void setCachedSelectionStart(int value) { m_cachedSelectionStart = value; }

    int cachedSelectionEnd() const { return m_cachedSelectionEnd; }
    void setCachedSelectionEnd(int value) { m_cachedSelectionEnd = value; }

#if ENABLE(WCSS)
    String inputFormatMask() const { return m_inputFormatMask; }
    void setInputFormatMask(const String& mask) { m_inputFormatMask = mask; }
 
    unsigned maxInputCharsAllowed() const { return m_maxInputCharsAllowed; }
    void setMaxInputCharsAllowed(unsigned maxLength) { m_maxInputCharsAllowed = maxLength; }
  
#endif

private:
    AtomicString m_name;
    String m_value;
    String m_suggestedValue;
    int m_size;
    int m_maxLength;
    int m_cachedSelectionStart;
    int m_cachedSelectionEnd;
#if ENABLE(WCSS)
    String m_inputFormatMask;
    unsigned m_maxInputCharsAllowed;
#endif
};

}

#endif
