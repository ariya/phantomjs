/*
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2010 Igalia S.L.
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
#include "FontCache.h"

#include "CString.h"
#include "Font.h"
#include "OwnPtrCairo.h"
#include "RefPtrCairo.h"
#include "SimpleFontData.h"
#include <cairo-ft.h>
#include <cairo.h>
#include <fontconfig/fcfreetype.h>
#include <wtf/Assertions.h>

namespace WebCore {

void FontCache::platformInit()
{
    // It's fine to call FcInit multiple times per the documentation.
    if (!FcInit())
        ASSERT_NOT_REACHED();
}

FcPattern* createFontConfigPatternForCharacters(const UChar* characters, int length)
{
    FcPattern* pattern = FcPatternCreate();

    FcCharSet* fontConfigCharSet = FcCharSetCreate();
    for (int i = 0; i < length; ++i) {
        if (U16_IS_SURROGATE(characters[i]) && U16_IS_SURROGATE_LEAD(characters[i])
                && i != length - 1 && U16_IS_TRAIL(characters[i + 1])) {
            FcCharSetAddChar(fontConfigCharSet, U16_GET_SUPPLEMENTARY(characters[i], characters[i+1]));
            i++;
        } else
            FcCharSetAddChar(fontConfigCharSet, characters[i]);
    }
    FcPatternAddCharSet(pattern, FC_CHARSET, fontConfigCharSet);
    FcCharSetDestroy(fontConfigCharSet);

    FcPatternAddBool(pattern, FC_SCALABLE, FcTrue);
    FcConfigSubstitute(0, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);
    return pattern;
}

FcPattern* findBestFontGivenFallbacks(const FontPlatformData& fontData, FcPattern* pattern)
{
    if (!fontData.m_pattern)
        return 0;

    if (!fontData.m_fallbacks) {
        FcResult fontConfigResult;
        fontData.m_fallbacks = FcFontSort(0, fontData.m_pattern.get(), FcTrue, 0, &fontConfigResult);
    }

    if (!fontData.m_fallbacks)
        return 0;

    FcFontSet* sets[] = { fontData.m_fallbacks };
    FcResult fontConfigResult;
    return FcFontSetMatch(0, sets, 1, pattern, &fontConfigResult);
}

const SimpleFontData* FontCache::getFontDataForCharacters(const Font& font, const UChar* characters, int length)
{
    RefPtr<FcPattern> pattern = adoptRef(createFontConfigPatternForCharacters(characters, length));
    const FontPlatformData& fontData = font.primaryFont()->platformData();

    RefPtr<FcPattern> fallbackPattern = adoptRef(findBestFontGivenFallbacks(fontData, pattern.get()));
    if (fallbackPattern) {
        FontPlatformData alternateFontData(fallbackPattern.get(), font.fontDescription());
        return getCachedFontData(&alternateFontData);
    }

    FcResult fontConfigResult;
    RefPtr<FcPattern> resultPattern = adoptRef(FcFontMatch(0, pattern.get(), &fontConfigResult));
    if (!resultPattern)
        return 0;
    FontPlatformData alternateFontData(resultPattern.get(), font.fontDescription());
    return getCachedFontData(&alternateFontData);
}

SimpleFontData* FontCache::getSimilarFontPlatformData(const Font& font)
{
    return 0;
}

SimpleFontData* FontCache::getLastResortFallbackFont(const FontDescription& fontDescription)
{
    // We want to return a fallback font here, otherwise the logic preventing FontConfig
    // matches for non-fallback fonts might return 0. See isFallbackFontAllowed.
    static AtomicString timesStr("serif");
    return getCachedFontData(fontDescription, timesStr);
}

void FontCache::getTraitsInFamily(const AtomicString& familyName, Vector<unsigned>& traitsMasks)
{
}

static String getFamilyNameStringFromFontDescriptionAndFamily(const FontDescription& fontDescription, const AtomicString& family)
{
    // If we're creating a fallback font (e.g. "-webkit-monospace"), convert the name into
    // the fallback name (like "monospace") that fontconfig understands.
    if (family.length() && !family.startsWith("-webkit-"))
        return family.string();

    switch (fontDescription.genericFamily()) {
    case FontDescription::StandardFamily:
    case FontDescription::SerifFamily:
        return "serif";
    case FontDescription::SansSerifFamily:
        return "sans-serif";
    case FontDescription::MonospaceFamily:
        return "monospace";
    case FontDescription::CursiveFamily:
        return "cursive";
    case FontDescription::FantasyFamily:
        return "fantasy";
    case FontDescription::NoFamily:
    default:
        return "";
    }
}

int fontWeightToFontconfigWeight(FontWeight weight)
{
    switch (weight) {
    case FontWeight100:
        return FC_WEIGHT_THIN;
    case FontWeight200:
        return FC_WEIGHT_ULTRALIGHT;
    case FontWeight300:
        return FC_WEIGHT_LIGHT;
    case FontWeight400:
        return FC_WEIGHT_REGULAR;
    case FontWeight500:
        return FC_WEIGHT_MEDIUM;
    case FontWeight600:
        return FC_WEIGHT_SEMIBOLD;
    case FontWeight700:
        return FC_WEIGHT_BOLD;
    case FontWeight800:
        return FC_WEIGHT_EXTRABOLD;
    case FontWeight900:
        return FC_WEIGHT_ULTRABLACK;
    default:
        ASSERT_NOT_REACHED();
        return FC_WEIGHT_REGULAR;
    }
}

FontPlatformData* FontCache::createFontPlatformData(const FontDescription& fontDescription, const AtomicString& family)
{
    // The CSS font matching algorithm (http://www.w3.org/TR/css3-fonts/#font-matching-algorithm)
    // says that we must find an exact match for font family, slant (italic or oblique can be used)
    // and font weight (we only match bold/non-bold here).
    RefPtr<FcPattern> pattern = adoptRef(FcPatternCreate());
    String familyNameString(getFamilyNameStringFromFontDescriptionAndFamily(fontDescription, family));
    if (!FcPatternAddString(pattern.get(), FC_FAMILY, reinterpret_cast<const FcChar8*>(familyNameString.utf8().data())))
        return 0;

    bool italic = fontDescription.italic();
    if (!FcPatternAddInteger(pattern.get(), FC_SLANT, italic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN))
        return 0;
    if (!FcPatternAddInteger(pattern.get(), FC_WEIGHT, fontWeightToFontconfigWeight(fontDescription.weight())))
        return 0;
    if (!FcPatternAddDouble(pattern.get(), FC_PIXEL_SIZE, fontDescription.computedPixelSize()))
        return 0;

    // The strategy is originally from Skia (src/ports/SkFontHost_fontconfig.cpp):

    // Allow Fontconfig to do pre-match substitution. Unless we are accessing a "fallback"
    // family like "sans," this is the only time we allow Fontconfig to substitute one
    // family name for another (i.e. if the fonts are aliased to each other).
    FcConfigSubstitute(0, pattern.get(), FcMatchPattern);
    FcDefaultSubstitute(pattern.get());

    FcChar8* fontConfigFamilyNameAfterConfiguration;
    FcPatternGetString(pattern.get(), FC_FAMILY, 0, &fontConfigFamilyNameAfterConfiguration);
    String familyNameAfterConfiguration = String::fromUTF8(reinterpret_cast<char*>(fontConfigFamilyNameAfterConfiguration));

    FcResult fontConfigResult;
    RefPtr<FcPattern> resultPattern = adoptRef(FcFontMatch(0, pattern.get(), &fontConfigResult));
    if (!resultPattern) // No match.
        return 0;

    FcChar8* fontConfigFamilyNameAfterMatching;
    FcPatternGetString(resultPattern.get(), FC_FAMILY, 0, &fontConfigFamilyNameAfterMatching);
    String familyNameAfterMatching = String::fromUTF8(reinterpret_cast<char*>(fontConfigFamilyNameAfterMatching));

    // If Fontconfig gave use a different font family than the one we requested, we should ignore it
    // and allow WebCore to give us the next font on the CSS fallback list. The only exception is if
    // this family name is a commonly used generic family.
    if (!equalIgnoringCase(familyNameAfterConfiguration, familyNameAfterMatching)
        && !(equalIgnoringCase(familyNameString, "sans") || equalIgnoringCase(familyNameString, "sans-serif")
          || equalIgnoringCase(familyNameString, "serif") || equalIgnoringCase(familyNameString, "monospace")
          || equalIgnoringCase(familyNameString, "fantasy") || equalIgnoringCase(familyNameString, "cursive")))
        return 0;

    // Verify that this font has an encoding compatible with Fontconfig. Fontconfig currently
    // supports three encodings in FcFreeTypeCharIndex: Unicode, Symbol and AppleRoman.
    // If this font doesn't have one of these three encodings, don't select it.
    FontPlatformData* platformData = new FontPlatformData(resultPattern.get(), fontDescription);
    if (!platformData->hasCompatibleCharmap()) {
        delete platformData;
        return 0;
    }

    return platformData;
}

}
