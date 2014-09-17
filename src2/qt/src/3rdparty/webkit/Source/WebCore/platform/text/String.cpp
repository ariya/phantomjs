/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
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
 */

#include "config.h"
#include "PlatformString.h"

#include "SharedBuffer.h"
#include "TextBreakIterator.h"
#include <wtf/unicode/UTF8.h>
#include <wtf/unicode/Unicode.h>

using namespace WTF;
using namespace WTF::Unicode;

namespace WebCore {

PassRefPtr<SharedBuffer> utf8Buffer(const String& string)
{
    // Allocate a buffer big enough to hold all the characters.
    const int length = string.length();
    Vector<char> buffer(length * 3);

    // Convert to runs of 8-bit characters.
    char* p = buffer.data();
    const UChar* d = string.characters();
    ConversionResult result = convertUTF16ToUTF8(&d, d + length, &p, p + buffer.size(), true);
    if (result != conversionOK)
        return 0;

    buffer.shrink(p - buffer.data());
    return SharedBuffer::adoptVector(buffer);
}

unsigned numGraphemeClusters(const String& s)
{
    TextBreakIterator* it = characterBreakIterator(s.characters(), s.length());
    if (!it)
        return s.length();

    unsigned num = 0;
    while (textBreakNext(it) != TextBreakDone)
        ++num;
    return num;
}

unsigned numCharactersInGraphemeClusters(const String& s, unsigned numGraphemeClusters)
{
    TextBreakIterator* it = characterBreakIterator(s.characters(), s.length());
    if (!it)
        return min(s.length(), numGraphemeClusters);

    for (unsigned i = 0; i < numGraphemeClusters; ++i) {
        if (textBreakNext(it) == TextBreakDone)
            return s.length();
    }
    return textBreakCurrent(it);
}

} // namespace WebCore
