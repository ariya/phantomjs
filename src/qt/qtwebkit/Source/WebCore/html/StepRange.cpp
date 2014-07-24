/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
#include "StepRange.h"

#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include <wtf/MathExtras.h>
#include <wtf/text/WTFString.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

StepRange::StepRange()
    : m_maximum(100)
    , m_minimum(0)
    , m_step(1)
    , m_stepBase(0)
    , m_hasStep(false)
{
}

StepRange::StepRange(const StepRange& stepRange)
    : m_maximum(stepRange.m_maximum)
    , m_minimum(stepRange.m_minimum)
    , m_step(stepRange.m_step)
    , m_stepBase(stepRange.m_stepBase)
    , m_stepDescription(stepRange.m_stepDescription)
    , m_hasStep(stepRange.m_hasStep)
{
}

StepRange::StepRange(const Decimal& stepBase, const Decimal& minimum, const Decimal& maximum, const Decimal& step, const StepDescription& stepDescription)
    : m_maximum(maximum)
    , m_minimum(minimum)
    , m_step(step.isFinite() ? step : 1)
    , m_stepBase(stepBase.isFinite() ? stepBase : 1)
    , m_stepDescription(stepDescription)
    , m_hasStep(step.isFinite())
{
    ASSERT(m_maximum.isFinite());
    ASSERT(m_minimum.isFinite());
    ASSERT(m_step.isFinite());
    ASSERT(m_stepBase.isFinite());
}

Decimal StepRange::acceptableError() const
{
    // FIXME: We should use DBL_MANT_DIG instead of FLT_MANT_DIG regarding to HTML5 specification.
    DEFINE_STATIC_LOCAL(const Decimal, twoPowerOfFloatMantissaBits, (Decimal::Positive, 0, UINT64_C(1) << FLT_MANT_DIG));
    return m_stepDescription.stepValueShouldBe == StepValueShouldBeReal ? m_step / twoPowerOfFloatMantissaBits : Decimal(0);
}

Decimal StepRange::alignValueForStep(const Decimal& currentValue, const Decimal& newValue) const
{
    DEFINE_STATIC_LOCAL(const Decimal, tenPowerOf21, (Decimal::Positive, 21, 1));
    if (newValue >= tenPowerOf21)
        return newValue;

    return stepMismatch(currentValue) ? newValue : roundByStep(newValue, m_stepBase);
}

Decimal StepRange::clampValue(const Decimal& value) const
{
    const Decimal inRangeValue = max(m_minimum, min(value, m_maximum));
    if (!m_hasStep)
        return inRangeValue;
    // Rounds inRangeValue to minimum + N * step.
    const Decimal roundedValue = roundByStep(inRangeValue, m_minimum);
    const Decimal clampedValue = roundedValue > m_maximum ? roundedValue - m_step : roundedValue;
    ASSERT(clampedValue >= m_minimum);
    ASSERT(clampedValue <= m_maximum);
    return clampedValue;
}

Decimal StepRange::parseStep(AnyStepHandling anyStepHandling, const StepDescription& stepDescription, const String& stepString)
{
    if (stepString.isEmpty())
        return stepDescription.defaultValue();

    if (equalIgnoringCase(stepString, "any")) {
        switch (anyStepHandling) {
        case RejectAny:
            return Decimal::nan();
        case AnyIsDefaultStep:
            return stepDescription.defaultValue();
        default:
            ASSERT_NOT_REACHED();
        }
    }

    Decimal step = parseToDecimalForNumberType(stepString);
    if (!step.isFinite() || step <= 0)
        return stepDescription.defaultValue();

    switch (stepDescription.stepValueShouldBe) {
    case StepValueShouldBeReal:
        step *= stepDescription.stepScaleFactor;
        break;
    case ParsedStepValueShouldBeInteger:
        // For date, month, and week, the parsed value should be an integer for some types.
        step = max(step.round(), Decimal(1));
        step *= stepDescription.stepScaleFactor;
        break;
    case ScaledStepValueShouldBeInteger:
        // For datetime, datetime-local, time, the result should be an integer.
        step *= stepDescription.stepScaleFactor;
        step = max(step.round(), Decimal(1));
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    ASSERT(step > 0);
    return step;
}

Decimal StepRange::roundByStep(const Decimal& value, const Decimal& base) const
{
    return base + ((value - base) / m_step).round() * m_step;
}

bool StepRange::stepMismatch(const Decimal& valueForCheck) const
{
    if (!m_hasStep)
        return false;
    if (!valueForCheck.isFinite())
        return false;
    const Decimal value = (valueForCheck - m_stepBase).abs();
    if (!value.isFinite())
        return false;
    // Decimal's fractional part size is DBL_MAN_DIG-bit. If the current value
    // is greater than step*2^DBL_MANT_DIG, the following computation for
    // remainder makes no sense.
    DEFINE_STATIC_LOCAL(const Decimal, twoPowerOfDoubleMantissaBits, (Decimal::Positive, 0, UINT64_C(1) << DBL_MANT_DIG));
    if (value / twoPowerOfDoubleMantissaBits > m_step)
        return false;
    // The computation follows HTML5 4.10.7.2.10 `The step attribute' :
    // ... that number subtracted from the step base is not an integral multiple
    // of the allowed value step, the element is suffering from a step mismatch.
    const Decimal remainder = (value - m_step * (value / m_step).round()).abs();
    // Accepts erros in lower fractional part which IEEE 754 single-precision
    // can't represent.
    const Decimal computedAcceptableError = acceptableError();
    return computedAcceptableError < remainder && remainder < (m_step - computedAcceptableError);
}

} // namespace WebCore
