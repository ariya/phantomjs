/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller ( mueller@kde.org )
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Andrew Wellington (proton@wiretapped.net)
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
#include "Length.h"

#include "PlatformString.h"
#include <wtf/ASCIICType.h>
#include <wtf/Assertions.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/text/StringBuffer.h>

using namespace WTF;

namespace WebCore {

static Length parseLength(const UChar* data, unsigned length)
{
    if (length == 0)
        return Length(1, Relative);

    unsigned i = 0;
    while (i < length && isSpaceOrNewline(data[i]))
        ++i;
    if (i < length && (data[i] == '+' || data[i] == '-'))
        ++i;
    while (i < length && isASCIIDigit(data[i]))
        ++i;
    unsigned intLength = i;
    while (i < length && (isASCIIDigit(data[i]) || data[i] == '.'))
        ++i;
    unsigned doubleLength = i;

    // IE quirk: Skip whitespace between the number and the % character (20 % => 20%).
    while (i < length && isSpaceOrNewline(data[i]))
        ++i;

    bool ok;
    UChar next = (i < length) ? data[i] : ' ';
    if (next == '%') {
        // IE quirk: accept decimal fractions for percentages.
        double r = charactersToDouble(data, doubleLength, &ok);
        if (ok)
            return Length(r, Percent);
        return Length(1, Relative);
    }
    int r = charactersToIntStrict(data, intLength, &ok);
    if (next == '*') {
        if (ok)
            return Length(r, Relative);
        return Length(1, Relative);
    }
    if (ok)
        return Length(r, Fixed);
    return Length(0, Relative);
}

static int countCharacter(const UChar* data, unsigned length, UChar character)
{
    int count = 0;
    for (int i = 0; i < static_cast<int>(length); ++i)
        count += data[i] == character;
    return count;
}

PassOwnArrayPtr<Length> newCoordsArray(const String& string, int& len)
{
    unsigned length = string.length();
    const UChar* data = string.characters();
    StringBuffer spacified(length);
    for (unsigned i = 0; i < length; i++) {
        UChar cc = data[i];
        if (cc > '9' || (cc < '0' && cc != '-' && cc != '*' && cc != '.'))
            spacified[i] = ' ';
        else
            spacified[i] = cc;
    }
    RefPtr<StringImpl> str = StringImpl::adopt(spacified);

    str = str->simplifyWhiteSpace();

    len = countCharacter(str->characters(), str->length(), ' ') + 1;
    OwnArrayPtr<Length> r = adoptArrayPtr(new Length[len]);

    int i = 0;
    unsigned pos = 0;
    size_t pos2;

    while ((pos2 = str->find(' ', pos)) != notFound) {
        r[i++] = parseLength(str->characters() + pos, pos2 - pos);
        pos = pos2+1;
    }
    r[i] = parseLength(str->characters() + pos, str->length() - pos);

    ASSERT(i == len - 1);

    return r.release();
}

PassOwnArrayPtr<Length> newLengthArray(const String& string, int& len)
{
    RefPtr<StringImpl> str = string.impl()->simplifyWhiteSpace();
    if (!str->length()) {
        len = 1;
        return nullptr;
    }

    len = countCharacter(str->characters(), str->length(), ',') + 1;
    OwnArrayPtr<Length> r = adoptArrayPtr(new Length[len]);

    int i = 0;
    unsigned pos = 0;
    size_t pos2;

    while ((pos2 = str->find(',', pos)) != notFound) {
        r[i++] = parseLength(str->characters() + pos, pos2 - pos);
        pos = pos2+1;
    }

    ASSERT(i == len - 1);

    // IE Quirk: If the last comma is the last char skip it and reduce len by one.
    if (str->length()-pos > 0)
        r[i] = parseLength(str->characters() + pos, str->length() - pos);
    else
        len--;

    return r.release();
}

} // namespace WebCore
