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

#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include <wtf/MathExtras.h>
#include <wtf/text/WTFString.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

StepRange::StepRange(const HTMLInputElement* element)
{
    if (element->hasAttribute(precisionAttr)) {
        step = 1.0;
        hasStep = !equalIgnoringCase(element->getAttribute(precisionAttr), "float");
    } else
        hasStep = element->getAllowedValueStep(&step);

    maximum = element->maximum();
    minimum = element->minimum();
}

double StepRange::clampValue(double value)
{
    double clampedValue = max(minimum, min(value, maximum));
    if (!hasStep)
        return clampedValue;
    // Rounds clampedValue to minimum + N * step.
    clampedValue = minimum + round((clampedValue - minimum) / step) * step;
    if (clampedValue > maximum)
       clampedValue -= step;
    ASSERT(clampedValue >= minimum);
    ASSERT(clampedValue <= maximum);
    return clampedValue;
}

double StepRange::clampValue(const String& stringValue)
{
    double value;
    bool parseSuccess = parseToDoubleForNumberType(stringValue, &value);
    if (!parseSuccess)
        value = (minimum + maximum) / 2;
    return clampValue(value);
}

double StepRange::valueFromElement(HTMLInputElement* element, bool* wasClamped)
{
    double oldValue;
    bool parseSuccess = parseToDoubleForNumberType(element->value(), &oldValue);
    if (!parseSuccess)
        oldValue = (minimum + maximum) / 2;
    double newValue = clampValue(oldValue);

    if (wasClamped)
        *wasClamped = !parseSuccess || newValue != oldValue;

    return newValue;
}

}
