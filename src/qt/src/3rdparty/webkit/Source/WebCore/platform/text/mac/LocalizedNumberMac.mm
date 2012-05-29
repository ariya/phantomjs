/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "LocalizedNumber.h"

#include <limits>
#import <Foundation/NSNumberFormatter.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/CString.h>

using namespace std;

namespace WebCore {

static RetainPtr<NSNumberFormatter> createFormatterForCurrentLocale()
{
    RetainPtr<NSNumberFormatter> formatter(AdoptNS, [[NSNumberFormatter alloc] init]);
    [formatter.get() setLocalizesFormat:YES];
    [formatter.get() setNumberStyle:NSNumberFormatterDecimalStyle];
    return formatter;
}

static NSNumberFormatter *numberFormatter()
{
    ASSERT(isMainThread());
    static NSNumberFormatter *formatter = createFormatterForCurrentLocale().leakRef();
    return formatter;
}

double parseLocalizedNumber(const String& numberString)
{
    if (numberString.isEmpty())
        return numeric_limits<double>::quiet_NaN();
    NSNumber *number = [numberFormatter() numberFromString:numberString];
    if (!number)
        return numeric_limits<double>::quiet_NaN();
    return [number doubleValue];
}

String formatLocalizedNumber(double inputNumber, unsigned fractionDigits)
{
    RetainPtr<NSNumber> number(AdoptNS, [[NSNumber alloc] initWithDouble:inputNumber]);
    RetainPtr<NSNumberFormatter> formatter = numberFormatter();
    [formatter.get() setMaximumFractionDigits:fractionDigits];
    return String([formatter.get() stringFromNumber:number.get()]);
}

} // namespace WebCore

