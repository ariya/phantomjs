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

#ifndef StepRange_h
#define StepRange_h

#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>

namespace WebCore {

class HTMLInputElement;

class StepRange {
    WTF_MAKE_NONCOPYABLE(StepRange);
public:
    bool hasStep;
    double step;
    double minimum;
    double maximum; // maximum must be >= minimum.

    explicit StepRange(const HTMLInputElement*);
    double clampValue(double value);
    double clampValue(const String& stringValue);

    // Clamp the middle value according to the step
    double defaultValue()
    {
        return clampValue((minimum + maximum) / 2);
    }

    // Map value into 0-1 range
    double proportionFromValue(double value)
    {
        if (minimum == maximum)
            return 0;

        return (value - minimum) / (maximum - minimum);
    }

    // Map from 0-1 range to value
    double valueFromProportion(double proportion)
    {
        return minimum + proportion * (maximum - minimum);
    }

    double valueFromElement(HTMLInputElement*, bool* wasClamped = 0);
};

}

#endif // StepRange_h
