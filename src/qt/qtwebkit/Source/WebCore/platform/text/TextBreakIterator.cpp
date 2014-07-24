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
#include "TextBreakIterator.h"

namespace WebCore {

unsigned numGraphemeClusters(const String& s)
{
    unsigned stringLength = s.length();
    
    if (!stringLength)
        return 0;

    // The only Latin-1 Extended Grapheme Cluster is CR LF
    if (s.is8Bit() && !s.contains('\r'))
        return stringLength;

    NonSharedCharacterBreakIterator it(s.characters(), stringLength);
    if (!it)
        return stringLength;

    unsigned num = 0;
    while (textBreakNext(it) != TextBreakDone)
        ++num;
    return num;
}

unsigned numCharactersInGraphemeClusters(const String& s, unsigned numGraphemeClusters)
{
    unsigned stringLength = s.length();

    if (!stringLength)
        return 0;

    // The only Latin-1 Extended Grapheme Cluster is CR LF
    if (s.is8Bit() && !s.contains('\r'))
        return std::min(stringLength, numGraphemeClusters);

    NonSharedCharacterBreakIterator it(s.characters(), stringLength);
    if (!it)
        return std::min(stringLength, numGraphemeClusters);

    for (unsigned i = 0; i < numGraphemeClusters; ++i) {
        if (textBreakNext(it) == TextBreakDone)
            return stringLength;
    }
    return textBreakCurrent(it);
}

#if !USE(ICU_UNICODE)
TextBreakIterator* acquireLineBreakIterator(const LChar* string, int length, const AtomicString& locale, const UChar* priorContext, unsigned priorContextLength)
{
    Vector<UChar> utf16string(length);
    for (int i = 0; i < length; ++i)
        utf16string[i] = string[i];
    return acquireLineBreakIterator(utf16string.data(), length, locale, priorContext, priorContextLength);
}
#endif

} // namespace WebCore
