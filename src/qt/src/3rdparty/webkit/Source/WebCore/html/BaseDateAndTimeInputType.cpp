/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "BaseDateAndTimeInputType.h"

#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "KeyboardEvent.h"
#include "LocalizedDate.h"
#include <limits>
#include <wtf/CurrentTime.h>
#include <wtf/DateMath.h>
#include <wtf/MathExtras.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

using namespace HTMLNames;
using namespace std;

static const double msecPerMinute = 60 * 1000;
static const double msecPerSecond = 1000;

double BaseDateAndTimeInputType::valueAsDate() const
{
    return parseToDouble(element()->value(), DateComponents::invalidMilliseconds());
}

void BaseDateAndTimeInputType::setValueAsDate(double value, ExceptionCode&) const
{
    element()->setValue(serializeWithMilliseconds(value));
}

double BaseDateAndTimeInputType::valueAsNumber() const
{
    return parseToDouble(element()->value(), numeric_limits<double>::quiet_NaN());
}

void BaseDateAndTimeInputType::setValueAsNumber(double newValue, ExceptionCode&) const
{
    element()->setValue(serialize(newValue));
}

bool BaseDateAndTimeInputType::typeMismatchFor(const String& value) const
{
    return !value.isEmpty() && !parseToDateComponents(value, 0);
}

bool BaseDateAndTimeInputType::typeMismatch() const
{
    return typeMismatchFor(element()->value());
}

bool BaseDateAndTimeInputType::rangeUnderflow(const String& value) const
{
    const double nan = numeric_limits<double>::quiet_NaN();
    double doubleValue = parseToDouble(value, nan);
    return isfinite(doubleValue) && doubleValue < minimum();
}

bool BaseDateAndTimeInputType::rangeOverflow(const String& value) const
{
    const double nan = numeric_limits<double>::quiet_NaN();
    double doubleValue = parseToDouble(value, nan);
    return isfinite(doubleValue) && doubleValue > maximum();
}

bool BaseDateAndTimeInputType::supportsRangeLimitation() const
{
    return true;
}

double BaseDateAndTimeInputType::defaultValueForStepUp() const
{
    double ms = currentTimeMS();
    double utcOffset = calculateUTCOffset();
    double dstOffset = calculateDSTOffset(ms, utcOffset);
    int offset = static_cast<int>((utcOffset + dstOffset) / msPerMinute);
    return ms + (offset * msPerMinute);
}

bool BaseDateAndTimeInputType::isSteppable() const
{
    return true;
}

bool BaseDateAndTimeInputType::stepMismatch(const String& value, double step) const
{
    const double nan = numeric_limits<double>::quiet_NaN();
    double doubleValue = parseToDouble(value, nan);
    doubleValue = fabs(doubleValue - stepBase());
    if (!isfinite(doubleValue))
        return false;
    ASSERT(round(doubleValue) == doubleValue);
    ASSERT(round(step) == step);
    return fmod(doubleValue, step);
}

double BaseDateAndTimeInputType::stepBase() const
{
    return parseToDouble(element()->fastGetAttribute(minAttr), defaultStepBase());
}

void BaseDateAndTimeInputType::handleKeydownEvent(KeyboardEvent* event)
{
    handleKeydownEventForSpinButton(event);
    if (!event->defaultHandled())
        TextFieldInputType::handleKeydownEvent(event);
}

void BaseDateAndTimeInputType::handleWheelEvent(WheelEvent* event)
{
    handleWheelEventForSpinButton(event);
}

double BaseDateAndTimeInputType::parseToDouble(const String& src, double defaultValue) const
{
    DateComponents date;
    if (!parseToDateComponents(src, &date))
        return defaultValue;
    double msec = date.millisecondsSinceEpoch();
    ASSERT(isfinite(msec));
    return msec;
}

bool BaseDateAndTimeInputType::parseToDateComponents(const String& source, DateComponents* out) const
{
    if (source.isEmpty())
        return false;
    DateComponents ignoredResult;
    if (!out)
        out = &ignoredResult;
    return parseToDateComponentsInternal(source.characters(), source.length(), out);
}

String BaseDateAndTimeInputType::serialize(double value) const
{
    if (!isfinite(value))
        return String();
    DateComponents date;
    if (!setMillisecondToDateComponents(value, &date))
        return String();
    return serializeWithComponents(date);
}

String BaseDateAndTimeInputType::serializeWithComponents(const DateComponents& date) const
{
    double step;
    if (!element()->getAllowedValueStep(&step))
        return date.toString();
    if (!fmod(step, msecPerMinute))
        return date.toString(DateComponents::None);
    if (!fmod(step, msecPerSecond))
        return date.toString(DateComponents::Second);
    return date.toString(DateComponents::Millisecond);
}

String BaseDateAndTimeInputType::serializeWithMilliseconds(double value) const
{
    return serialize(value);
}

String BaseDateAndTimeInputType::visibleValue() const
{
    String currentValue = element()->value();
    DateComponents date;
    if (!parseToDateComponents(currentValue, &date))
        return currentValue;

    String localized = formatLocalizedDate(date);
    return localized.isEmpty() ? currentValue : localized;
}

String BaseDateAndTimeInputType::convertFromVisibleValue(const String& visibleValue) const
{
    if (visibleValue.isEmpty())
        return visibleValue;

    double parsedValue = parseLocalizedDate(visibleValue, dateType());
    if (!isfinite(parsedValue))
        return visibleValue;

    return serializeWithMilliseconds(parsedValue);
}

} // namespace WebCore
