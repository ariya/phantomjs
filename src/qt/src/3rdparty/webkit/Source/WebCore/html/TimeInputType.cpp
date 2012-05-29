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
#include "TimeInputType.h"

#include "DateComponents.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include <wtf/CurrentTime.h>
#include <wtf/DateMath.h>
#include <wtf/MathExtras.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

using namespace HTMLNames;

static const double timeDefaultStep = 60.0;
static const double timeStepScaleFactor = 1000.0;

PassOwnPtr<InputType> TimeInputType::create(HTMLInputElement* element)
{
    return adoptPtr(new TimeInputType(element));
}

const AtomicString& TimeInputType::formControlType() const
{
    return InputTypeNames::time();
}

DateComponents::Type TimeInputType::dateType() const
{
    return DateComponents::Time;
}

double TimeInputType::defaultValueForStepUp() const
{
    double current = currentTimeMS();
    double utcOffset = calculateUTCOffset();
    double dstOffset = calculateDSTOffset(current, utcOffset);
    int offset = static_cast<int>((utcOffset + dstOffset) / msPerMinute);
    current += offset * msPerMinute;

    DateComponents date;
    date.setMillisecondsSinceMidnight(current);
    double milliseconds = date.millisecondsSinceEpoch();
    ASSERT(isfinite(milliseconds));
    return milliseconds;
}

double TimeInputType::minimum() const
{
    return parseToDouble(element()->fastGetAttribute(minAttr), DateComponents::minimumTime());
}

double TimeInputType::maximum() const
{
    return parseToDouble(element()->fastGetAttribute(maxAttr), DateComponents::maximumTime());
}

double TimeInputType::defaultStep() const
{
    return timeDefaultStep;
}

double TimeInputType::stepScaleFactor() const
{
    return timeStepScaleFactor;
}

bool TimeInputType::scaledStepValueShouldBeInteger() const
{
    return true;
}

bool TimeInputType::parseToDateComponentsInternal(const UChar* characters, unsigned length, DateComponents* out) const
{
    ASSERT(out);
    unsigned end;
    return out->parseTime(characters, length, 0, end) && end == length;
}

bool TimeInputType::setMillisecondToDateComponents(double value, DateComponents* date) const
{
    ASSERT(date);
    return date->setMillisecondsSinceMidnight(value);
}

} // namespace WebCore
