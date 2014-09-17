/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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
#include "ComplexTextController.h"

#if USE(ATSUI)

#include "Font.h"
#include "ShapeArabic.h"
#include "TextRun.h"
#include <ApplicationServices/ApplicationServices.h>
#include <wtf/unicode/CharacterNames.h>

#ifdef __LP64__
// ATSUTextInserted() is SPI in 64-bit.
extern "C" {
OSStatus ATSUTextInserted(ATSUTextLayout iTextLayout,  UniCharArrayOffset iInsertionLocation, UniCharCount iInsertionLength);
}
#endif

using namespace WTF::Unicode;

namespace WebCore {

OSStatus ComplexTextController::ComplexTextRun::overrideLayoutOperation(ATSULayoutOperationSelector, ATSULineRef atsuLineRef, URefCon refCon, void*, ATSULayoutOperationCallbackStatus* callbackStatus)
{
    ComplexTextRun* complexTextRun = reinterpret_cast<ComplexTextRun*>(refCon);
    OSStatus status;
    ItemCount count;
    ATSLayoutRecord* layoutRecords;

    status = ATSUDirectGetLayoutDataArrayPtrFromLineRef(atsuLineRef, kATSUDirectDataLayoutRecordATSLayoutRecordCurrent, true, reinterpret_cast<void**>(&layoutRecords), &count);
    if (status != noErr) {
        *callbackStatus = kATSULayoutOperationCallbackStatusContinue;
        return status;
    }

    count--;
    ItemCount j = 0;
    CFIndex indexOffset = 0;

    if (complexTextRun->m_directionalOverride) {
        j++;
        count -= 2;
        indexOffset = -1;
    }

    complexTextRun->m_glyphCount = count;
    complexTextRun->m_glyphsVector.reserveCapacity(count);
    complexTextRun->m_advancesVector.reserveCapacity(count);
    complexTextRun->m_atsuiIndices.reserveCapacity(count);

    bool atBeginning = true;
    CGFloat lastX = 0;

    for (ItemCount i = 0; i < count; ++i, ++j) {
        if (layoutRecords[j].glyphID == kATSDeletedGlyphcode) {
            complexTextRun->m_glyphCount--;
            continue;
        }
        complexTextRun->m_glyphsVector.uncheckedAppend(layoutRecords[j].glyphID);
        complexTextRun->m_atsuiIndices.uncheckedAppend(layoutRecords[j].originalOffset / 2 + indexOffset);
        CGFloat x = FixedToFloat(layoutRecords[j].realPos);
        if (!atBeginning)
            complexTextRun->m_advancesVector.uncheckedAppend(CGSizeMake(x - lastX, 0));
        lastX = x;
        atBeginning = false;
    }

    complexTextRun->m_advancesVector.uncheckedAppend(CGSizeMake(FixedToFloat(layoutRecords[j].realPos) - lastX, 0));

    complexTextRun->m_glyphs = complexTextRun->m_glyphsVector.data();
    complexTextRun->m_advances = complexTextRun->m_advancesVector.data();

    status = ATSUDirectReleaseLayoutDataArrayPtr(atsuLineRef, kATSUDirectDataLayoutRecordATSLayoutRecordCurrent, reinterpret_cast<void**>(&layoutRecords));
    *callbackStatus = kATSULayoutOperationCallbackStatusContinue;
    return noErr;
}

static inline bool isArabicLamWithAlefLigature(UChar c)
{
    return c >= 0xfef5 && c <= 0xfefc;
}

static void shapeArabic(const UChar* source, UChar* dest, unsigned totalLength)
{
    unsigned shapingStart = 0;
    while (shapingStart < totalLength) {
        unsigned shapingEnd;
        // We do not want to pass a Lam with Alef ligature followed by a space to the shaper,
        // since we want to be able to identify this sequence as the result of shaping a Lam
        // followed by an Alef and padding with a space.
        bool foundLigatureSpace = false;
        for (shapingEnd = shapingStart; !foundLigatureSpace && shapingEnd < totalLength - 1; ++shapingEnd)
            foundLigatureSpace = isArabicLamWithAlefLigature(source[shapingEnd]) && source[shapingEnd + 1] == ' ';
        shapingEnd++;

        UErrorCode shapingError = U_ZERO_ERROR;
        unsigned charsWritten = shapeArabic(source + shapingStart, shapingEnd - shapingStart, dest + shapingStart, shapingEnd - shapingStart, U_SHAPE_LETTERS_SHAPE | U_SHAPE_LENGTH_FIXED_SPACES_NEAR, &shapingError);

        if (U_SUCCESS(shapingError) && charsWritten == shapingEnd - shapingStart) {
            for (unsigned j = shapingStart; j < shapingEnd - 1; ++j) {
                if (isArabicLamWithAlefLigature(dest[j]) && dest[j + 1] == ' ')
                    dest[++j] = zeroWidthSpace;
            }
            if (foundLigatureSpace) {
                dest[shapingEnd] = ' ';
                shapingEnd++;
            } else if (isArabicLamWithAlefLigature(dest[shapingEnd - 1])) {
                // u_shapeArabic quirk: if the last two characters in the source string are a Lam and an Alef,
                // the space is put at the beginning of the string, despite U_SHAPE_LENGTH_FIXED_SPACES_NEAR.
                ASSERT(dest[shapingStart] == ' ');
                dest[shapingStart] = zeroWidthSpace;
            }
        } else {
            // Something went wrong. Abandon shaping and just copy the rest of the buffer.
            LOG_ERROR("u_shapeArabic failed(%d)", shapingError);
            shapingEnd = totalLength;
            memcpy(dest + shapingStart, source + shapingStart, (shapingEnd - shapingStart) * sizeof(UChar));
        }
        shapingStart = shapingEnd;
    }
}

ComplexTextController::ComplexTextRun::ComplexTextRun(ATSUTextLayout atsuTextLayout, const SimpleFontData* fontData, const UChar* characters, unsigned stringLocation, size_t stringLength, bool ltr, bool directionalOverride)
    : m_fontData(fontData)
    , m_characters(characters)
    , m_stringLocation(stringLocation)
    , m_stringLength(stringLength)
    , m_indexEnd(stringLength)
    , m_directionalOverride(directionalOverride)
    , m_isMonotonic(true)
{
    OSStatus status;

    status = ATSUSetTextLayoutRefCon(atsuTextLayout, reinterpret_cast<URefCon>(this));

    ATSLineLayoutOptions lineLayoutOptions = kATSLineKeepSpacesOutOfMargin | kATSLineHasNoHangers;

    Boolean rtl = !ltr;

    Vector<UChar, 256> substituteCharacters;
    bool shouldCheckForMirroring = !ltr && !fontData->m_ATSUMirrors;
    bool shouldCheckForArabic = !fontData->shapesArabic();
    bool shouldShapeArabic = false;

    bool mirrored = false;
    for (size_t i = 0; i < stringLength; ++i) {
        if (shouldCheckForMirroring) {
            UChar mirroredChar = u_charMirror(characters[i]);
            if (mirroredChar != characters[i]) {
                if (!mirrored) {
                    mirrored = true;
                    substituteCharacters.grow(stringLength);
                    memcpy(substituteCharacters.data(), characters, stringLength * sizeof(UChar));
                    ATSUTextMoved(atsuTextLayout, substituteCharacters.data());
                }
                substituteCharacters[i] = mirroredChar;
            }
        }
        if (shouldCheckForArabic && isArabicChar(characters[i])) {
            shouldCheckForArabic = false;
            shouldShapeArabic = true;
        }
    }

    if (shouldShapeArabic) {
        Vector<UChar, 256> shapedArabic(stringLength);
        shapeArabic(substituteCharacters.isEmpty() ? characters : substituteCharacters.data(), shapedArabic.data(), stringLength);
        substituteCharacters.swap(shapedArabic);
        ATSUTextMoved(atsuTextLayout, substituteCharacters.data());
    }

    if (directionalOverride) {
        UChar override = ltr ? leftToRightOverride : rightToLeftOverride;
        if (substituteCharacters.isEmpty()) {
            substituteCharacters.grow(stringLength + 2);
            substituteCharacters[0] = override;
            memcpy(substituteCharacters.data() + 1, characters, stringLength * sizeof(UChar));
            substituteCharacters[stringLength + 1] = popDirectionalFormatting;
            ATSUTextMoved(atsuTextLayout, substituteCharacters.data());
        } else {
            substituteCharacters.prepend(override);
            substituteCharacters.append(popDirectionalFormatting);
        }
        ATSUTextInserted(atsuTextLayout, 0, 2);
    }

    ATSULayoutOperationOverrideSpecifier overrideSpecifier;
    overrideSpecifier.operationSelector = kATSULayoutOperationPostLayoutAdjustment;
    overrideSpecifier.overrideUPP = overrideLayoutOperation;

    ATSUAttributeTag tags[] = { kATSULineLayoutOptionsTag, kATSULineDirectionTag, kATSULayoutOperationOverrideTag };
    ByteCount sizes[] = { sizeof(ATSLineLayoutOptions), sizeof(Boolean), sizeof(ATSULayoutOperationOverrideSpecifier) };
    ATSUAttributeValuePtr values[] = { &lineLayoutOptions, &rtl, &overrideSpecifier };

    status = ATSUSetLayoutControls(atsuTextLayout, 3, tags, sizes, values);

    ItemCount boundsCount;
    status = ATSUGetGlyphBounds(atsuTextLayout, 0, 0, 0, m_stringLength, kATSUseFractionalOrigins, 0, 0, &boundsCount);

    status = ATSUDisposeTextLayout(atsuTextLayout);
}

void ComplexTextController::ComplexTextRun::createTextRunFromFontDataATSUI(bool ltr)
{
    m_atsuiIndices.reserveCapacity(m_stringLength);
    unsigned r = 0;
    while (r < m_stringLength) {
        m_atsuiIndices.uncheckedAppend(r);
        if (U_IS_SURROGATE(m_characters[r])) {
            ASSERT(r + 1 < m_stringLength);
            ASSERT(U_IS_SURROGATE_LEAD(m_characters[r]));
            ASSERT(U_IS_TRAIL(m_characters[r + 1]));
            r += 2;
        } else
            r++;
    }
    m_glyphCount = m_atsuiIndices.size();
    if (!ltr) {
        for (unsigned r = 0, end = m_glyphCount - 1; r < m_glyphCount / 2; ++r, --end)
            std::swap(m_atsuiIndices[r], m_atsuiIndices[end]);
    }

    m_glyphsVector.fill(0, m_glyphCount);
    m_glyphs = m_glyphsVector.data();
    m_advancesVector.fill(CGSizeMake(m_fontData->widthForGlyph(0), 0), m_glyphCount);
    m_advances = m_advancesVector.data();
}

static bool fontHasMirroringInfo(ATSUFontID fontID)
{
    ByteCount propTableSize;
    OSStatus status = ATSFontGetTable(fontID, 'prop', 0, 0, 0, &propTableSize);
    if (status == noErr)    // naively assume that if a 'prop' table exists then it contains mirroring info
        return true;
    else if (status != kATSInvalidFontTableAccess) // anything other than a missing table is logged as an error
        LOG_ERROR("ATSFontGetTable failed (%d)", static_cast<int>(status));

    return false;
}

static void disableLigatures(const SimpleFontData* fontData, ATSUStyle atsuStyle, TypesettingFeatures typesettingFeatures)
{
    // Don't be too aggressive: if the font doesn't contain 'a', then assume that any ligatures it contains are
    // in characters that always go through ATSUI, and therefore allow them. Geeza Pro is an example.
    // See bugzilla 5166.
    if ((typesettingFeatures & Ligatures) || (fontData->platformData().orientation() == Horizontal && fontData->platformData().allowsLigatures()))
        return;

    ATSUFontFeatureType featureTypes[] = { kLigaturesType };
    ATSUFontFeatureSelector featureSelectors[] = { kCommonLigaturesOffSelector };
    OSStatus status = ATSUSetFontFeatures(atsuStyle, 1, featureTypes, featureSelectors);
    if (status != noErr)
        LOG_ERROR("ATSUSetFontFeatures failed (%d) -- ligatures remain enabled", static_cast<int>(status));
}

static ATSUStyle initializeATSUStyle(const SimpleFontData* fontData, TypesettingFeatures typesettingFeatures)
{
    unsigned key = typesettingFeatures + 1;
    pair<HashMap<unsigned, ATSUStyle>::iterator, bool> addResult = fontData->m_ATSUStyleMap.add(key, 0);
    ATSUStyle& atsuStyle = addResult.first->second;
    if (!addResult.second)
        return atsuStyle;

    ATSUFontID fontID = fontData->platformData().ctFont() ? CTFontGetPlatformFont(fontData->platformData().ctFont(), 0) : 0;
    if (!fontID) {
        LOG_ERROR("unable to get ATSUFontID for %p", fontData->platformData().font());
        fontData->m_ATSUStyleMap.remove(addResult.first);
        return 0;
    }

    OSStatus status = ATSUCreateStyle(&atsuStyle);
    if (status != noErr)
        LOG_ERROR("ATSUCreateStyle failed (%d)", static_cast<int>(status));

    Fixed fontSize = FloatToFixed(fontData->platformData().m_size);
    Fract kerningInhibitFactor = FloatToFract(1);
    static CGAffineTransform verticalFlip = CGAffineTransformMakeScale(1, -1);

    ByteCount styleSizes[4] = { sizeof(fontSize), sizeof(fontID), sizeof(verticalFlip), sizeof(kerningInhibitFactor) };
    ATSUAttributeTag styleTags[4] = { kATSUSizeTag, kATSUFontTag, kATSUFontMatrixTag, kATSUKerningInhibitFactorTag };
    ATSUAttributeValuePtr styleValues[4] = { &fontSize, &fontID, &verticalFlip, &kerningInhibitFactor };

    bool allowKerning = typesettingFeatures & Kerning;
    status = ATSUSetAttributes(atsuStyle, allowKerning ? 3 : 4, styleTags, styleSizes, styleValues);
    if (status != noErr)
        LOG_ERROR("ATSUSetAttributes failed (%d)", static_cast<int>(status));

    fontData->m_ATSUMirrors = fontHasMirroringInfo(fontID);

    disableLigatures(fontData, atsuStyle, typesettingFeatures);
    return atsuStyle;
}

void ComplexTextController::collectComplexTextRunsForCharactersATSUI(const UChar* cp, unsigned length, unsigned stringLocation, const SimpleFontData* fontData)
{
    if (!fontData) {
        // Create a run of missing glyphs from the primary font.
        m_complexTextRuns.append(ComplexTextRun::create(m_font.primaryFont(), cp, stringLocation, length, m_run.ltr()));
        return;
    }

    if (m_fallbackFonts && fontData != m_font.primaryFont())
        m_fallbackFonts->add(fontData);

    ATSUStyle atsuStyle = initializeATSUStyle(fontData, m_font.typesettingFeatures());

    OSStatus status;
    ATSUTextLayout atsuTextLayout;
    UniCharCount runLength = length;

    status = ATSUCreateTextLayoutWithTextPtr(cp, 0, length, length, 1, &runLength, &atsuStyle, &atsuTextLayout);
    if (status != noErr) {
        LOG_ERROR("ATSUCreateTextLayoutWithTextPtr failed with error %d", static_cast<int>(status));
        return;
    }
    m_complexTextRuns.append(ComplexTextRun::create(atsuTextLayout, fontData, cp, stringLocation, length, m_run.ltr(), m_run.directionalOverride()));
}

} // namespace WebCore

#endif // USE(ATSUI)
