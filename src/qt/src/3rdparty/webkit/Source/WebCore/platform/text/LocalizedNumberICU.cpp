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
#include <unicode/numfmt.h>
#include <unicode/parsepos.h>
#include <wtf/MathExtras.h>
#include <wtf/PassOwnPtr.h>

using namespace icu;
using namespace std;

namespace WebCore {

static PassOwnPtr<NumberFormat> createFormatterForCurrentLocale()
{
    UErrorCode status = U_ZERO_ERROR;
    OwnPtr<NumberFormat> formatter = adoptPtr(NumberFormat::createInstance(status));
    return U_SUCCESS(status) ? formatter.release() : nullptr;
}

// This might return 0.
static NumberFormat* numberFormatter()
{
    ASSERT(isMainThread());
    static NumberFormat* formatter = createFormatterForCurrentLocale().leakPtr();
    return formatter;
}

double parseLocalizedNumber(const String& numberString)
{
    if (numberString.isEmpty())
        return numeric_limits<double>::quiet_NaN();
    NumberFormat* formatter = numberFormatter();
    if (!formatter)
        return numeric_limits<double>::quiet_NaN();
    UnicodeString numberUnicodeString(numberString.characters(), numberString.length());
    Formattable result;
    ParsePosition position(0);
    formatter->parse(numberUnicodeString, result, position);
    if (position.getIndex() != numberUnicodeString.length())
        return numeric_limits<double>::quiet_NaN();
    UErrorCode status = U_ZERO_ERROR;
    double numericResult = result.getDouble(status);
    return U_SUCCESS(status) ? numericResult : numeric_limits<double>::quiet_NaN();
}

String formatLocalizedNumber(double number, unsigned fractionDigits)
{
    NumberFormat* formatter = numberFormatter();
    if (!formatter)
        return String();
    UnicodeString result;
    formatter->setMaximumFractionDigits(clampToInteger(fractionDigits));
    formatter->format(number, result);
    return String(result.getBuffer(), result.length());
}

} // namespace WebCore
