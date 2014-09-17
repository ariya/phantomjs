/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "SegmentedFontData.h"

#include "PlatformString.h"
#include "SimpleFontData.h"
#include <wtf/Assertions.h>

namespace WebCore {

SegmentedFontData::~SegmentedFontData()
{
}

const SimpleFontData* SegmentedFontData::fontDataForCharacter(UChar32 c) const
{
    Vector<FontDataRange>::const_iterator end = m_ranges.end();
    for (Vector<FontDataRange>::const_iterator it = m_ranges.begin(); it != end; ++it) {
        if (it->from() <= c && it->to() >= c)
            return it->fontData();
    }
    return m_ranges[0].fontData();
}

bool SegmentedFontData::containsCharacter(UChar32 c) const
{
    Vector<FontDataRange>::const_iterator end = m_ranges.end();
    for (Vector<FontDataRange>::const_iterator it = m_ranges.begin(); it != end; ++it) {
        if (c >= it->from() && c <= it->to())
            return true;
    }
    return false;
}

bool SegmentedFontData::containsCharacters(const UChar* characters, int length) const
{
    UChar32 c;
    for (int i = 0; i < length; ) {
        U16_NEXT(characters, i, length, c)
        if (!containsCharacter(c))
            return false;
    }
    return true;
}

bool SegmentedFontData::isCustomFont() const
{
    // All segmented fonts are custom fonts.
    return true;
}

bool SegmentedFontData::isLoading() const
{
    Vector<FontDataRange>::const_iterator end = m_ranges.end();
    for (Vector<FontDataRange>::const_iterator it = m_ranges.begin(); it != end; ++it) {
        if (it->fontData()->isLoading())
            return true;
    }
    return false;
}

bool SegmentedFontData::isSegmented() const
{
    return true;
}

#ifndef NDEBUG
String SegmentedFontData::description() const
{
    return "[segmented font]";
}
#endif

} // namespace WebCore
