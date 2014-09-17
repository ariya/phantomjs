/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(CORE_TEXT)

#include "ComplexTextController.h"

#include "Font.h"
#include "TextRun.h"
#include "WebCoreSystemInterface.h"
#include <CoreText/CoreText.h>

#if defined(BUILDING_ON_LEOPARD)
// The following symbols are SPI in 10.5.
extern "C" {
void CTRunGetAdvances(CTRunRef run, CFRange range, CGSize buffer[]);
const CGSize* CTRunGetAdvancesPtr(CTRunRef run);
extern const CFStringRef kCTTypesetterOptionForcedEmbeddingLevel;
}
#endif

namespace WebCore {

ComplexTextController::ComplexTextRun::ComplexTextRun(CTRunRef ctRun, const SimpleFontData* fontData, const UChar* characters, unsigned stringLocation, size_t stringLength, CFRange runRange)
    : m_fontData(fontData)
    , m_characters(characters)
    , m_stringLocation(stringLocation)
    , m_stringLength(stringLength)
    , m_indexEnd(runRange.location + runRange.length)
    , m_isMonotonic(true)
{
    m_glyphCount = CTRunGetGlyphCount(ctRun);
    m_coreTextIndices = CTRunGetStringIndicesPtr(ctRun);
    if (!m_coreTextIndices) {
        m_coreTextIndicesVector.grow(m_glyphCount);
        CTRunGetStringIndices(ctRun, CFRangeMake(0, 0), m_coreTextIndicesVector.data());
        m_coreTextIndices = m_coreTextIndicesVector.data();
    }

    m_glyphs = CTRunGetGlyphsPtr(ctRun);
    if (!m_glyphs) {
        m_glyphsVector.grow(m_glyphCount);
        CTRunGetGlyphs(ctRun, CFRangeMake(0, 0), m_glyphsVector.data());
        m_glyphs = m_glyphsVector.data();
    }

    m_advances = CTRunGetAdvancesPtr(ctRun);
    if (!m_advances) {
        m_advancesVector.grow(m_glyphCount);
        CTRunGetAdvances(ctRun, CFRangeMake(0, 0), m_advancesVector.data());
        m_advances = m_advancesVector.data();
    }
}

// Missing glyphs run constructor. Core Text will not generate a run of missing glyphs, instead falling back on
// glyphs from LastResort. We want to use the primary font's missing glyph in order to match the fast text code path.
void ComplexTextController::ComplexTextRun::createTextRunFromFontDataCoreText(bool ltr)
{
    m_coreTextIndicesVector.reserveInitialCapacity(m_stringLength);
    unsigned r = 0;
    while (r < m_stringLength) {
        m_coreTextIndicesVector.uncheckedAppend(r);
        if (U_IS_SURROGATE(m_characters[r])) {
            ASSERT(r + 1 < m_stringLength);
            ASSERT(U_IS_SURROGATE_LEAD(m_characters[r]));
            ASSERT(U_IS_TRAIL(m_characters[r + 1]));
            r += 2;
        } else
            r++;
    }
    m_glyphCount = m_coreTextIndicesVector.size();
    if (!ltr) {
        for (unsigned r = 0, end = m_glyphCount - 1; r < m_glyphCount / 2; ++r, --end)
            std::swap(m_coreTextIndicesVector[r], m_coreTextIndicesVector[end]);
    }
    m_coreTextIndices = m_coreTextIndicesVector.data();

    // Synthesize a run of missing glyphs.
    m_glyphsVector.fill(0, m_glyphCount);
    m_glyphs = m_glyphsVector.data();
    m_advancesVector.fill(CGSizeMake(m_fontData->widthForGlyph(0), 0), m_glyphCount);
    m_advances = m_advancesVector.data();
}

struct ProviderInfo {
    const UChar* cp;
    unsigned length;
    CFDictionaryRef attributes;
};

static const UniChar* provideStringAndAttributes(CFIndex stringIndex, CFIndex* charCount, CFDictionaryRef* attributes, void* refCon)
{
    ProviderInfo* info = static_cast<struct ProviderInfo*>(refCon);
    if (stringIndex < 0 || static_cast<unsigned>(stringIndex) >= info->length)
        return 0;

    *charCount = info->length - stringIndex;
    *attributes = info->attributes;
    return info->cp + stringIndex;
}

void ComplexTextController::collectComplexTextRunsForCharactersCoreText(const UChar* cp, unsigned length, unsigned stringLocation, const SimpleFontData* fontData)
{
    if (!fontData) {
        // Create a run of missing glyphs from the primary font.
        m_complexTextRuns.append(ComplexTextRun::create(m_font.primaryFont(), cp, stringLocation, length, m_run.ltr()));
        return;
    }

    if (m_fallbackFonts && fontData != m_font.primaryFont())
        m_fallbackFonts->add(fontData);

    RetainPtr<CTLineRef> line;

    if (!m_mayUseNaturalWritingDirection || m_run.directionalOverride()) {
        static const void* optionKeys[] = { kCTTypesetterOptionForcedEmbeddingLevel };
        const short ltrForcedEmbeddingLevelValue = 0;
        const short rtlForcedEmbeddingLevelValue = 1;
        static const void* ltrOptionValues[] = { CFNumberCreate(kCFAllocatorDefault, kCFNumberShortType, &ltrForcedEmbeddingLevelValue) };
        static const void* rtlOptionValues[] = { CFNumberCreate(kCFAllocatorDefault, kCFNumberShortType, &rtlForcedEmbeddingLevelValue) };
        static CFDictionaryRef ltrTypesetterOptions = CFDictionaryCreate(kCFAllocatorDefault, optionKeys, ltrOptionValues, WTF_ARRAY_LENGTH(optionKeys), &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        static CFDictionaryRef rtlTypesetterOptions = CFDictionaryCreate(kCFAllocatorDefault, optionKeys, rtlOptionValues, WTF_ARRAY_LENGTH(optionKeys), &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
        ProviderInfo info = { cp, length, fontData->getCFStringAttributes(m_font.typesettingFeatures(), fontData->platformData().orientation()) };
        RetainPtr<CTTypesetterRef> typesetter(AdoptCF, wkCreateCTTypesetterWithUniCharProviderAndOptions(&provideStringAndAttributes, 0, &info, m_run.ltr() ? ltrTypesetterOptions : rtlTypesetterOptions));
#else
        RetainPtr<CFStringRef> string(AdoptCF, CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault, cp, length, kCFAllocatorNull));
        RetainPtr<CFAttributedStringRef> attributedString(AdoptCF, CFAttributedStringCreate(kCFAllocatorDefault, string.get(), fontData->getCFStringAttributes(m_font.typesettingFeatures(), fontData->platformData().orientation())));
        RetainPtr<CTTypesetterRef> typesetter(AdoptCF, CTTypesetterCreateWithAttributedStringAndOptions(attributedString.get(), m_run.ltr() ? ltrTypesetterOptions : rtlTypesetterOptions));
#endif

        line.adoptCF(CTTypesetterCreateLine(typesetter.get(), CFRangeMake(0, 0)));
    } else {
        ProviderInfo info = { cp, length, fontData->getCFStringAttributes(m_font.typesettingFeatures(), fontData->platformData().orientation()) };

        line.adoptCF(wkCreateCTLineWithUniCharProvider(&provideStringAndAttributes, 0, &info));
    }

    m_coreTextLines.append(line.get());

    CFArrayRef runArray = CTLineGetGlyphRuns(line.get());

    CFIndex runCount = CFArrayGetCount(runArray);

    for (CFIndex r = 0; r < runCount; r++) {
        CTRunRef ctRun = static_cast<CTRunRef>(CFArrayGetValueAtIndex(runArray, r));
        ASSERT(CFGetTypeID(ctRun) == CTRunGetTypeID());
        CFRange runRange = CTRunGetStringRange(ctRun);
        m_complexTextRuns.append(ComplexTextRun::create(ctRun, fontData, cp, stringLocation, length, runRange));
    }
}

} // namespace WebCore

#endif // USE(CORE_TEXT)
