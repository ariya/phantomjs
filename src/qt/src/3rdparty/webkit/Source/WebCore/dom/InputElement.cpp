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

#include "config.h"
#include "InputElement.h"

#include "BeforeTextInsertedEvent.h"

#if ENABLE(WCSS)
#include "CSSPropertyNames.h"
#include "CSSRule.h"
#include "CSSRuleList.h"
#include "CSSStyleRule.h"
#include "CSSStyleSelector.h"
#endif

#include "Attribute.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Document.h"
#include "Event.h"
#include "EventNames.h"
#include "Frame.h"
#include "Page.h"
#include "RenderTextControlSingleLine.h"
#include "SelectionController.h"
#include "TextIterator.h"

namespace WebCore {

// FIXME: According to HTML4, the length attribute's value can be arbitrarily
// large. However, due to https://bugs.webkit.org/show_bug.cgi?id=14536 things
// get rather sluggish when a text field has a larger number of characters than
// this, even when just clicking in the text field.
const int InputElement::s_maximumLength = 524288;
const int InputElement::s_defaultSize = 20;

void InputElement::dispatchFocusEvent(InputElement* inputElement, Element* element)
{
    if (!inputElement->isTextField())
        return;

    Document* document = element->document();
    if (inputElement->isPasswordField() && document->frame())
        document->setUseSecureKeyboardEntryWhenActive(true);
}

void InputElement::dispatchBlurEvent(InputElement* inputElement, Element* element)
{
    if (!inputElement->isTextField())
        return;

    Document* document = element->document();
    Frame* frame = document->frame();
    if (!frame)
        return;

    if (inputElement->isPasswordField())
        document->setUseSecureKeyboardEntryWhenActive(false);

    frame->editor()->textFieldDidEndEditing(element);
}

void InputElement::updateFocusAppearance(InputElementData& data, InputElement* inputElement, Element* element, bool restorePreviousSelection)
{
    ASSERT(inputElement->isTextField());

    if (!restorePreviousSelection || data.cachedSelectionStart() == -1)
        inputElement->select();
    else
        // Restore the cached selection.
        updateSelectionRange(inputElement, element, data.cachedSelectionStart(), data.cachedSelectionEnd());

    Document* document = element->document();
    if (document && document->frame())
        document->frame()->selection()->revealSelection();
}

void InputElement::updateSelectionRange(InputElement* inputElement, Element* element, int start, int end)
{
    if (!inputElement->isTextField())
        return;

    setSelectionRange(element, start, end);
}

void InputElement::aboutToUnload(InputElement* inputElement, Element* element)
{
    if (!inputElement->isTextField() || !element->focused())
        return;

    Document* document = element->document();
    Frame* frame = document->frame();
    if (!frame)
        return;

    frame->editor()->textFieldDidEndEditing(element);
}

void InputElement::setValueFromRenderer(InputElementData& data, InputElement* inputElement, Element* element, const String& value)
{
    // Renderer and our event handler are responsible for sanitizing values.
    ASSERT_UNUSED(inputElement, value == inputElement->sanitizeValue(value) || inputElement->sanitizeValue(value).isEmpty());

    // Workaround for bug where trailing \n is included in the result of textContent.
    // The assert macro above may also be simplified to:  value == constrainValue(value)
    // http://bugs.webkit.org/show_bug.cgi?id=9661
    if (value == "\n")
        data.setValue("");
    else
        data.setValue(value);

    element->setFormControlValueMatchesRenderer(true);

    // Input event is fired by the Node::defaultEventHandler for editable controls.
    if (!inputElement->isTextField())
        element->dispatchInputEvent();
    notifyFormStateChanged(element);
}

static String replaceEOLAndLimitLength(const InputElement* inputElement, const String& proposedValue, int maxLength)
{
    if (!inputElement->isTextField())
        return proposedValue;

    String string = proposedValue;
    string.replace("\r\n", " ");
    string.replace('\r', ' ');
    string.replace('\n', ' ');

    unsigned newLength = numCharactersInGraphemeClusters(string, maxLength);
    for (unsigned i = 0; i < newLength; ++i) {
        const UChar current = string[i];
        if (current < ' ' && current != '\t') {
            newLength = i;
            break;
        }
    }
    return string.left(newLength);
}

String InputElement::sanitizeValueForTextField(const InputElement* inputElement, const String& proposedValue)
{
#if ENABLE(WCSS)
    InputElementData data = const_cast<InputElement*>(inputElement)->data();
    if (!isConformToInputMask(data, proposedValue)) {
        if (isConformToInputMask(data, data.value()))
            return data.value();
        return String();
    }
#endif
    return replaceEOLAndLimitLength(inputElement, proposedValue, s_maximumLength);
}

String InputElement::sanitizeUserInputValue(const InputElement* inputElement, const String& proposedValue, int maxLength)
{
    return replaceEOLAndLimitLength(inputElement, proposedValue, maxLength);
}

void InputElement::handleBeforeTextInsertedEvent(InputElementData& data, InputElement* inputElement, Element* element, Event* event)
{
    ASSERT(event->isBeforeTextInsertedEvent());
    // Make sure that the text to be inserted will not violate the maxLength.

    // We use RenderTextControlSingleLine::text() instead of InputElement::value()
    // because they can be mismatched by sanitizeValue() in
    // RenderTextControlSingleLine::subtreeHasChanged() in some cases.
    unsigned oldLength = numGraphemeClusters(toRenderTextControlSingleLine(element->renderer())->text());

    // selectionLength represents the selection length of this text field to be
    // removed by this insertion.
    // If the text field has no focus, we don't need to take account of the
    // selection length. The selection is the source of text drag-and-drop in
    // that case, and nothing in the text field will be removed.
    unsigned selectionLength = element->focused() ? numGraphemeClusters(plainText(element->document()->frame()->selection()->selection().toNormalizedRange().get())) : 0;
    ASSERT(oldLength >= selectionLength);

    // Selected characters will be removed by the next text event.
    unsigned baseLength = oldLength - selectionLength;
    unsigned maxLength = static_cast<unsigned>(inputElement->supportsMaxLength() ? data.maxLength() : s_maximumLength); // maxLength() can never be negative.
    unsigned appendableLength = maxLength > baseLength ? maxLength - baseLength : 0;

    // Truncate the inserted text to avoid violating the maxLength and other constraints.
    BeforeTextInsertedEvent* textEvent = static_cast<BeforeTextInsertedEvent*>(event);
#if ENABLE(WCSS)
    RefPtr<Range> range = element->document()->frame()->selection()->selection().toNormalizedRange();
    String candidateString = toRenderTextControlSingleLine(element->renderer())->text();
    if (selectionLength)
        candidateString.replace(range->startOffset(), range->endOffset(), textEvent->text());
    else
        candidateString.insert(textEvent->text(), range->startOffset());
    if (!isConformToInputMask(inputElement->data(), candidateString)) {
        textEvent->setText("");
        return;
    }
#endif
    textEvent->setText(sanitizeUserInputValue(inputElement, textEvent->text(), appendableLength));
}

void InputElement::parseSizeAttribute(InputElementData& data, Element* element, Attribute* attribute)
{
    data.setSize(attribute->isNull() ? InputElement::s_defaultSize : attribute->value().toInt());

    if (RenderObject* renderer = element->renderer())
        renderer->setNeedsLayoutAndPrefWidthsRecalc();
}

void InputElement::parseMaxLengthAttribute(InputElementData& data, InputElement* inputElement, Element* element, Attribute* attribute)
{
    int maxLength = attribute->isNull() ? InputElement::s_maximumLength : attribute->value().toInt();
    if (maxLength <= 0 || maxLength > InputElement::s_maximumLength)
        maxLength = InputElement::s_maximumLength;

    int oldMaxLength = data.maxLength();
    data.setMaxLength(maxLength);

    if (oldMaxLength != maxLength)
        updateValueIfNeeded(data, inputElement);

    element->setNeedsStyleRecalc();
}

void InputElement::updateValueIfNeeded(InputElementData& data, InputElement* inputElement)
{
    String oldValue = data.value();
    String newValue = inputElement->sanitizeValue(oldValue);
    if (newValue != oldValue)
        inputElement->setValue(newValue);
}

void InputElement::notifyFormStateChanged(Element* element)
{
    Document* document = element->document();
    Frame* frame = document->frame();
    if (!frame)
        return;

    if (Page* page = frame->page())
        page->chrome()->client()->formStateDidChange(element);
}

// InputElementData
InputElementData::InputElementData()
    : m_size(InputElement::s_defaultSize)
    , m_maxLength(InputElement::s_maximumLength)
    , m_cachedSelectionStart(-1)
    , m_cachedSelectionEnd(-1)
#if ENABLE(WCSS)
    , m_inputFormatMask("*m")
    , m_maxInputCharsAllowed(InputElement::s_maximumLength)
#endif
{
}

InputElementData::~InputElementData()
{
}

const AtomicString& InputElementData::name() const
{
    return m_name.isNull() ? emptyAtom : m_name;
}

#if ENABLE(WCSS)
static inline const AtomicString& formatCodes()
{
    DEFINE_STATIC_LOCAL(AtomicString, codes, ("AaNnXxMm"));
    return codes;
}

static unsigned cursorPositionToMaskIndex(const String& inputFormatMask, unsigned cursorPosition)
{
    UChar mask;
    int index = -1;
    do {
        mask = inputFormatMask[++index];
        if (mask == '\\')
            ++index;
        else if (mask == '*' || (isASCIIDigit(mask) && mask != '0')) {
            index = inputFormatMask.length() - 1;
            break;
        }
    } while (cursorPosition--);

    return index;
}

bool InputElement::isConformToInputMask(const InputElementData& data, const String& inputChars)
{
    for (unsigned i = 0; i < inputChars.length(); ++i)
        if (!isConformToInputMask(data, inputChars[i], i))
            return false;
    return true;
}

bool InputElement::isConformToInputMask(const InputElementData& data, UChar inChar, unsigned cursorPosition)
{
    String inputFormatMask = data.inputFormatMask();

    if (inputFormatMask.isEmpty() || inputFormatMask == "*M" || inputFormatMask == "*m")
        return true;

    if (cursorPosition >= data.maxInputCharsAllowed())
        return false;

    unsigned maskIndex = cursorPositionToMaskIndex(inputFormatMask, cursorPosition);
    bool ok = true;
    UChar mask = inputFormatMask[maskIndex];
    // Match the inputed character with input mask
    switch (mask) {
    case 'A':
        ok = !isASCIIDigit(inChar) && !isASCIILower(inChar) && isASCIIPrintable(inChar);
        break;
    case 'a':
        ok = !isASCIIDigit(inChar) && !isASCIIUpper(inChar) && isASCIIPrintable(inChar);
        break;
    case 'N':
        ok = isASCIIDigit(inChar);
        break;
    case 'n':
        ok = !isASCIIAlpha(inChar) && isASCIIPrintable(inChar);
        break;
    case 'X':
        ok = !isASCIILower(inChar) && isASCIIPrintable(inChar);
        break;
    case 'x':
        ok = !isASCIIUpper(inChar) && isASCIIPrintable(inChar);
        break;
    case 'M':
    case 'm':
        ok = isASCIIPrintable(inChar);
        break;
    default:
        ok = (mask == inChar);
        break;
    }

    return ok;
}

String InputElement::validateInputMask(InputElementData& data, String& inputMask)
{
    inputMask.replace("\\\\", "\\");

    bool isValid = true;
    bool hasWildcard = false;
    unsigned escapeCharCount = 0;
    unsigned maskLength = inputMask.length();
    UChar formatCode;
    for (unsigned i = 0; i < maskLength; ++i) {
        formatCode = inputMask[i];
        if (formatCodes().find(formatCode) == -1) {
            if (formatCode == '*' || (isASCIIDigit(formatCode) && formatCode != '0')) {
                // Validate codes which ends with '*f' or 'nf'
                formatCode = inputMask[++i];
                if ((i + 1 != maskLength) || formatCodes().find(formatCode) == -1) {
                    isValid = false;
                    break;
                }
                hasWildcard = true;
            } else if (formatCode == '\\') {
                // skip over the next mask character
                ++i;
                ++escapeCharCount;
            } else {
                isValid = false;
                break;
            }
        }
    }

    if (!isValid)
        return String();
    // calculate the number of characters allowed to be entered by input mask
    unsigned allowedLength = maskLength;
    if (escapeCharCount)
        allowedLength -= escapeCharCount;

    if (hasWildcard) {
        formatCode = inputMask[maskLength - 2];
        if (formatCode == '*')
            allowedLength = data.maxInputCharsAllowed();
        else {
            unsigned leftLen = String(&formatCode).toInt();
            allowedLength = leftLen + allowedLength - 2;
        }
    }

    if (allowedLength < data.maxInputCharsAllowed())
        data.setMaxInputCharsAllowed(allowedLength);

    return inputMask;
}

#endif

}
