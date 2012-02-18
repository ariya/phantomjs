/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2009 Michelangelo De Simone <micdesim@gmail.com>
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "ValidityState.h"

#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLSelectElement.h"
#include "HTMLTextAreaElement.h"
#include "HTMLTreeBuilder.h"
#include "LocalizedStrings.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

using namespace HTMLNames;

String ValidityState::validationMessage() const
{
    if (!toHTMLElement(m_control)->willValidate())
        return String();

    if (customError())
        return m_customErrorMessage;
    HTMLElement* element = toHTMLElement(m_control);
    bool isInputElement = element->isFormControlElement() && element->hasTagName(inputTag);
    bool isTextAreaElement = element->isFormControlElement() && element->hasTagName(textareaTag);
    // The order of the following checks is meaningful. e.g. We'd like to show the
    // valueMissing message even if the control has other validation errors.
    if (valueMissing()) {
        if (element->hasTagName(selectTag))
            return validationMessageValueMissingForSelectText();
        if (isInputElement)
            return static_cast<HTMLInputElement*>(element)->valueMissingText();
        return validationMessageValueMissingText();
    }
    if (typeMismatch()) {
        if (isInputElement)
            return static_cast<HTMLInputElement*>(element)->typeMismatchText();
        return validationMessageTypeMismatchText();
    }
    if (patternMismatch())
        return validationMessagePatternMismatchText();
    if (tooLong()) {
        if (!isInputElement && !isTextAreaElement) {
            ASSERT_NOT_REACHED();
            return String();
        }
        HTMLTextFormControlElement* text = static_cast<HTMLTextFormControlElement*>(element);
        return validationMessageTooLongText(numGraphemeClusters(text->value()), text->maxLength());
    }
    if (rangeUnderflow()) {
        if (!isInputElement) {
            ASSERT_NOT_REACHED();
            return String();
        }
        return validationMessageRangeUnderflowText(static_cast<HTMLInputElement*>(element)->minimumString());
    }
    if (rangeOverflow()) {
        if (!isInputElement) {
            ASSERT_NOT_REACHED();
            return String();
        }
        return validationMessageRangeOverflowText(static_cast<HTMLInputElement*>(element)->maximumString());
    }
    if (stepMismatch()) {
        if (!isInputElement) {
            ASSERT_NOT_REACHED();
            return String();
        }
        HTMLInputElement* input = static_cast<HTMLInputElement*>(element);
        return validationMessageStepMismatchText(input->stepBaseString(), input->stepString());
    }

    return String();
}

void ValidityState::setCustomErrorMessage(const String& message)
{
    m_customErrorMessage = message;
    if (m_control->isFormControlElement())
        static_cast<HTMLFormControlElement*>(m_control)->setNeedsValidityCheck();
}

bool ValidityState::valueMissing() const
{
    HTMLElement* element = toHTMLElement(m_control);
    if (!element->willValidate())
        return false;

    if (element->hasTagName(inputTag)) {
        HTMLInputElement* input = static_cast<HTMLInputElement*>(element);
        return input->valueMissing(input->value());
    }
    if (element->hasTagName(textareaTag)) {
        HTMLTextAreaElement* textArea = static_cast<HTMLTextAreaElement*>(element);
        return textArea->valueMissing(textArea->value());
    }
    if (element->hasTagName(selectTag)) {
        HTMLSelectElement* select = static_cast<HTMLSelectElement*>(element);
        return select->valueMissing();
    }
    return false;
}

bool ValidityState::typeMismatch() const
{
    HTMLElement* element = toHTMLElement(m_control);
    if (!element->willValidate())
        return false;

    if (!element->hasTagName(inputTag))
        return false;
    return static_cast<HTMLInputElement*>(element)->typeMismatch();
}

bool ValidityState::patternMismatch() const
{
    HTMLElement* element = toHTMLElement(m_control);
    if (!element->willValidate())
        return false;

    if (!element->hasTagName(inputTag))
        return false;
    HTMLInputElement* input = static_cast<HTMLInputElement*>(element);
    return input->patternMismatch(input->value());
}

bool ValidityState::tooLong() const
{
    HTMLElement* element = toHTMLElement(m_control);
    if (!element->willValidate())
        return false;

    if (element->hasTagName(inputTag)) {
        HTMLInputElement* input = static_cast<HTMLInputElement*>(element);
        return input->tooLong(input->value(), HTMLTextFormControlElement::CheckDirtyFlag);
    }
    if (element->hasTagName(textareaTag)) {
        HTMLTextAreaElement* textArea = static_cast<HTMLTextAreaElement*>(element);
        return textArea->tooLong(textArea->value(), HTMLTextFormControlElement::CheckDirtyFlag);
    }
    return false;
}

bool ValidityState::rangeUnderflow() const
{
    HTMLElement* element = toHTMLElement(m_control);
    if (!element->willValidate())
        return false;

    if (!element->hasTagName(inputTag))
        return false;
    HTMLInputElement* input = static_cast<HTMLInputElement*>(element);
    return input->rangeUnderflow(input->value());
}

bool ValidityState::rangeOverflow() const
{
    HTMLElement* element = toHTMLElement(m_control);
    if (!element->willValidate())
        return false;

    if (!element->hasTagName(inputTag))
        return false;
    HTMLInputElement* input = static_cast<HTMLInputElement*>(element);
    return input->rangeOverflow(input->value());
}

bool ValidityState::stepMismatch() const
{
    HTMLElement* element = toHTMLElement(m_control);
    if (!element->willValidate())
        return false;

    if (!element->hasTagName(inputTag))
        return false;
    HTMLInputElement* input = static_cast<HTMLInputElement*>(element);
    return input->stepMismatch(input->value());
}

bool ValidityState::customError() const
{
    HTMLElement* element = toHTMLElement(m_control);
    return element->willValidate() && !m_customErrorMessage.isEmpty();
}

bool ValidityState::valid() const
{
    bool someError = typeMismatch() || stepMismatch() || rangeUnderflow() || rangeOverflow()
        || tooLong() || patternMismatch() || valueMissing() || customError();
    return !someError;
}

} // namespace
