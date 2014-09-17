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

#ifndef SegmentedFontData_h
#define SegmentedFontData_h

#include "FontData.h"
#include <wtf/Vector.h>

namespace WebCore {

class SimpleFontData;

struct FontDataRange {
    FontDataRange(UChar32 from, UChar32 to, const SimpleFontData* fontData)
        : m_from(from)
        , m_to(to)
        , m_fontData(fontData)
    {
    }

    UChar32 from() const { return m_from; }
    UChar32 to() const { return m_to; }
    const SimpleFontData* fontData() const { return m_fontData; }

private:
    UChar32 m_from;
    UChar32 m_to;
    const SimpleFontData* m_fontData;
};

class SegmentedFontData : public FontData {
public:
    virtual ~SegmentedFontData();

    void appendRange(const FontDataRange& range) { m_ranges.append(range); }
    unsigned numRanges() const { return m_ranges.size(); }
    const FontDataRange& rangeAt(unsigned i) const { return m_ranges[i]; }

#ifndef NDEBUG
    virtual String description() const;
#endif

private:
    virtual const SimpleFontData* fontDataForCharacter(UChar32) const;
    virtual bool containsCharacters(const UChar*, int length) const;

    virtual bool isCustomFont() const;
    virtual bool isLoading() const;
    virtual bool isSegmented() const;

    bool containsCharacter(UChar32) const;

    Vector<FontDataRange, 1> m_ranges;
};

} // namespace WebCore

#endif // SegmentedFontData_h
