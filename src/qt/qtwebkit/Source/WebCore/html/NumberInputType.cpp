/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#include "config.h"
#include "NumberInputType.h"

#include "ExceptionCode.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "InputTypeNames.h"
#include "KeyboardEvent.h"
#include "LocalizedStrings.h"
#include "PlatformLocale.h"
#include "RenderTextControl.h"
#include <limits>
#include <wtf/ASCIICType.h>
#include <wtf/MathExtras.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

using namespace HTMLNames;
using namespace std;

static const int numberDefaultStep = 1;
static const int numberDefaultStepBase = 0;
static const int numberStepScaleFactor = 1;

struct RealNumberRenderSize
{
    unsigned sizeBeforeDecimalPoint;
    unsigned sizeAfteDecimalPoint;

    RealNumberRenderSize(unsigned before, unsigned after)
        : sizeBeforeDecimalPoint(before)
        , sizeAfteDecimalPoint(after)
    {
    }

    RealNumberRenderSize max(const RealNumberRenderSize& other) const
    {
        return RealNumberRenderSize(
            std::max(sizeBeforeDecimalPoint, other.sizeBeforeDecimalPoint),
            std::max(sizeAfteDecimalPoint, other.sizeAfteDecimalPoint));
    }
};

static RealNumberRenderSize calculateRenderSize(const Decimal& value)
{
    ASSERT(value.isFinite());
    const unsigned sizeOfDigits = String::number(value.value().coefficient()).length();
    const unsigned sizeOfSign = value.isNegative() ? 1 : 0;
    const int exponent = value.exponent();
    if (exponent >= 0)
        return RealNumberRenderSize(sizeOfSign + sizeOfDigits, 0);

    const int sizeBeforeDecimalPoint = exponent + sizeOfDigits;
    if (sizeBeforeDecimalPoint > 0) {
        // In case of "123.456"
        return RealNumberRenderSize(sizeOfSign + sizeBeforeDecimalPoint, sizeOfDigits - sizeBeforeDecimalPoint);
    }

    // In case of "0.00012345"
    const unsigned sizeOfZero = 1;
    const unsigned numberOfZeroAfterDecimalPoint = -sizeBeforeDecimalPoint;
    return RealNumberRenderSize(sizeOfSign + sizeOfZero , numberOfZeroAfterDecimalPoint + sizeOfDigits);
}

PassOwnPtr<InputType> NumberInputType::create(HTMLInputElement* element)
{
    return adoptPtr(new NumberInputType(element));
}

void NumberInputType::attach()
{
    TextFieldInputType::attach();
    observeFeatureIfVisible(FeatureObserver::InputTypeNumber);
}

const AtomicString& NumberInputType::formControlType() const
{
    return InputTypeNames::number();
}

void NumberInputType::setValue(const String& sanitizedValue, bool valueChanged, TextFieldEventBehavior eventBehavior)
{
    if (!valueChanged && sanitizedValue.isEmpty() && !element()->innerTextValue().isEmpty())
        updateInnerTextValue();
    TextFieldInputType::setValue(sanitizedValue, valueChanged, eventBehavior);
}

double NumberInputType::valueAsDouble() const
{
    return parseToDoubleForNumberType(element()->value());
}

void NumberInputType::setValueAsDouble(double newValue, TextFieldEventBehavior eventBehavior, ExceptionCode& ec) const
{
    // FIXME: We should use numeric_limits<double>::max for number input type.
    const double floatMax = numeric_limits<float>::max();
    if (newValue < -floatMax) {
        ec = INVALID_STATE_ERR;
        return;
    }
    if (newValue > floatMax) {
        ec = INVALID_STATE_ERR;
        return;
    }
    element()->setValue(serializeForNumberType(newValue), eventBehavior);
}

void NumberInputType::setValueAsDecimal(const Decimal& newValue, TextFieldEventBehavior eventBehavior, ExceptionCode& ec) const
{
    // FIXME: We should use numeric_limits<double>::max for number input type.
    const Decimal floatMax = Decimal::fromDouble(numeric_limits<float>::max());
    if (newValue < -floatMax) {
        ec = INVALID_STATE_ERR;
        return;
    }
    if (newValue > floatMax) {
        ec = INVALID_STATE_ERR;
        return;
    }
    element()->setValue(serializeForNumberType(newValue), eventBehavior);
}

bool NumberInputType::typeMismatchFor(const String& value) const
{
    return !value.isEmpty() && !std::isfinite(parseToDoubleForNumberType(value));
}

bool NumberInputType::typeMismatch() const
{
    ASSERT(!typeMismatchFor(element()->value()));
    return false;
}

StepRange NumberInputType::createStepRange(AnyStepHandling anyStepHandling) const
{
    DEFINE_STATIC_LOCAL(const StepRange::StepDescription, stepDescription, (numberDefaultStep, numberDefaultStepBase, numberStepScaleFactor));
    const Decimal stepBase = parseToDecimalForNumberType(element()->fastGetAttribute(minAttr), numberDefaultStepBase);
    // FIXME: We should use numeric_limits<double>::max for number input type.
    const Decimal floatMax = Decimal::fromDouble(numeric_limits<float>::max());
    const Decimal minimum = parseToNumber(element()->fastGetAttribute(minAttr), -floatMax);
    const Decimal maximum = parseToNumber(element()->fastGetAttribute(maxAttr), floatMax);
    const Decimal step = StepRange::parseStep(anyStepHandling, stepDescription, element()->fastGetAttribute(stepAttr));
    return StepRange(stepBase, minimum, maximum, step, stepDescription);
}

bool NumberInputType::sizeShouldIncludeDecoration(int defaultSize, int& preferredSize) const
{
    preferredSize = defaultSize;

    const String stepString = element()->fastGetAttribute(stepAttr);
    if (equalIgnoringCase(stepString, "any"))
        return false;

    const Decimal minimum = parseToDecimalForNumberType(element()->fastGetAttribute(minAttr));
    if (!minimum.isFinite())
        return false;

    const Decimal maximum = parseToDecimalForNumberType(element()->fastGetAttribute(maxAttr));
    if (!maximum.isFinite())
        return false;

    const Decimal step = parseToDecimalForNumberType(stepString, 1);
    ASSERT(step.isFinite());

    RealNumberRenderSize size = calculateRenderSize(minimum).max(calculateRenderSize(maximum).max(calculateRenderSize(step)));

    preferredSize = size.sizeBeforeDecimalPoint + size.sizeAfteDecimalPoint + (size.sizeAfteDecimalPoint ? 1 : 0);

    return true;
}

bool NumberInputType::isSteppable() const
{
    return true;
}

void NumberInputType::handleKeydownEvent(KeyboardEvent* event)
{
    handleKeydownEventForSpinButton(event);
    if (!event->defaultHandled())
        TextFieldInputType::handleKeydownEvent(event);
}

Decimal NumberInputType::parseToNumber(const String& src, const Decimal& defaultValue) const
{
    return parseToDecimalForNumberType(src, defaultValue);
}

String NumberInputType::serialize(const Decimal& value) const
{
    if (!value.isFinite())
        return String();
    return serializeForNumberType(value);
}

static bool isE(UChar ch)
{
    return ch == 'e' || ch == 'E';
}

String NumberInputType::localizeValue(const String& proposedValue) const
{
    if (proposedValue.isEmpty())
        return proposedValue;
    // We don't localize scientific notations.
    if (proposedValue.find(isE) != notFound)
        return proposedValue;
    return element()->locale().convertToLocalizedNumber(proposedValue);
}

String NumberInputType::visibleValue() const
{
    return localizeValue(element()->value());
}

String NumberInputType::convertFromVisibleValue(const String& visibleValue) const
{
    if (visibleValue.isEmpty())
        return visibleValue;
    // We don't localize scientific notations.
    if (visibleValue.find(isE) != notFound)
        return visibleValue;
    return element()->locale().convertFromLocalizedNumber(visibleValue);
}

String NumberInputType::sanitizeValue(const String& proposedValue) const
{
    if (proposedValue.isEmpty())
        return proposedValue;
    return std::isfinite(parseToDoubleForNumberType(proposedValue)) ? proposedValue : emptyString();
}

bool NumberInputType::hasBadInput() const
{
    String standardValue = convertFromVisibleValue(element()->innerTextValue());
    return !standardValue.isEmpty() && !std::isfinite(parseToDoubleForNumberType(standardValue));
}

String NumberInputType::badInputText() const
{
    return validationMessageBadInputForNumberText();
}

bool NumberInputType::shouldRespectSpeechAttribute()
{
    return true;
}

bool NumberInputType::supportsPlaceholder() const
{
    return true;
}

bool NumberInputType::isNumberField() const
{
    return true;
}

void NumberInputType::minOrMaxAttributeChanged()
{
    InputType::minOrMaxAttributeChanged();

    if (element()->renderer())
        element()->renderer()->setNeedsLayoutAndPrefWidthsRecalc();
}

void NumberInputType::stepAttributeChanged()
{
    InputType::stepAttributeChanged();

    if (element()->renderer())
        element()->renderer()->setNeedsLayoutAndPrefWidthsRecalc();
}

} // namespace WebCore
