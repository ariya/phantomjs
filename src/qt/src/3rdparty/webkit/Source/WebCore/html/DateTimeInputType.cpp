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
#include "DateTimeInputType.h"

#include "DateComponents.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include <wtf/CurrentTime.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

using namespace HTMLNames;

static const double dateTimeDefaultStep = 60.0;
static const double dateTimeStepScaleFactor = 1000.0;

PassOwnPtr<InputType> DateTimeInputType::create(HTMLInputElement* element)
{
    return adoptPtr(new DateTimeInputType(element));
}

const AtomicString& DateTimeInputType::formControlType() const
{
    return InputTypeNames::datetime();
}

DateComponents::Type DateTimeInputType::dateType() const
{
    return DateComponents::DateTime;
}

double DateTimeInputType::defaultValueForStepUp() const
{
    return currentTimeMS();
}

double DateTimeInputType::minimum() const
{
    return parseToDouble(element()->fastGetAttribute(minAttr), DateComponents::minimumDateTime());
}

double DateTimeInputType::maximum() const
{
    return parseToDouble(element()->fastGetAttribute(maxAttr), DateComponents::maximumDateTime());
}

double DateTimeInputType::defaultStep() const
{
    return dateTimeDefaultStep;
}

double DateTimeInputType::stepScaleFactor() const
{
    return dateTimeStepScaleFactor;
}

bool DateTimeInputType::scaledStepValueShouldBeInteger() const
{
    return true;
}

bool DateTimeInputType::parseToDateComponentsInternal(const UChar* characters, unsigned length, DateComponents* out) const
{
    ASSERT(out);
    unsigned end;
    return out->parseDateTime(characters, length, 0, end) && end == length;
}

bool DateTimeInputType::setMillisecondToDateComponents(double value, DateComponents* date) const
{
    ASSERT(date);
    return date->setMillisecondsSinceEpochForDateTime(value);
}

} // namespace WebCore
