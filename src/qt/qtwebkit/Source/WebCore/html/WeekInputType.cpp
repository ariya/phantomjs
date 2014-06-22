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
#if ENABLE(INPUT_TYPE_WEEK)
#include "WeekInputType.h"

#include "DateComponents.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "InputTypeNames.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

using namespace HTMLNames;

static const int weekDefaultStepBase = -259200000; // The first day of 1970-W01.
static const int weekDefaultStep = 1;
static const int weekStepScaleFactor = 604800000;

PassOwnPtr<InputType> WeekInputType::create(HTMLInputElement* element)
{
    return adoptPtr(new WeekInputType(element));
}

void WeekInputType::attach()
{
    observeFeatureIfVisible(FeatureObserver::InputTypeWeek);
}

const AtomicString& WeekInputType::formControlType() const
{
    return InputTypeNames::week();
}

DateComponents::Type WeekInputType::dateType() const
{
    return DateComponents::Week;
}

StepRange WeekInputType::createStepRange(AnyStepHandling anyStepHandling) const
{
    DEFINE_STATIC_LOCAL(const StepRange::StepDescription, stepDescription, (weekDefaultStep, weekDefaultStepBase, weekStepScaleFactor, StepRange::ParsedStepValueShouldBeInteger));

    const Decimal stepBase = parseToNumber(element()->fastGetAttribute(minAttr), weekDefaultStepBase);
    const Decimal minimum = parseToNumber(element()->fastGetAttribute(minAttr), Decimal::fromDouble(DateComponents::minimumWeek()));
    const Decimal maximum = parseToNumber(element()->fastGetAttribute(maxAttr), Decimal::fromDouble(DateComponents::maximumWeek()));
    const Decimal step = StepRange::parseStep(anyStepHandling, stepDescription, element()->fastGetAttribute(stepAttr));
    return StepRange(stepBase, minimum, maximum, step, stepDescription);
}

bool WeekInputType::parseToDateComponentsInternal(const UChar* characters, unsigned length, DateComponents* out) const
{
    ASSERT(out);
    unsigned end;
    return out->parseWeek(characters, length, 0, end) && end == length;
}

bool WeekInputType::setMillisecondToDateComponents(double value, DateComponents* date) const
{
    ASSERT(date);
    return date->setMillisecondsSinceEpochForWeek(value);
}

bool WeekInputType::isWeekField() const
{
    return true;
}

} // namespace WebCore

#endif
