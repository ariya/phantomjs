/*
 * This file is part of the internal font implementation.
 *
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (c) 2010 Google Inc. All rights reserved.
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

#import "config.h"
#import "FontPlatformData.h"

#import "PlatformString.h"
#import "WebCoreSystemInterface.h"
#import <AppKit/NSFont.h>

namespace WebCore {

// These CoreText Text Spacing feature selectors are not defined in CoreText.
enum TextSpacingCTFeatureSelector { TextSpacingProportional, TextSpacingFullWidth, TextSpacingHalfWidth, TextSpacingThirdWidth, TextSpacingQuarterWidth };

#if PLATFORM(MAC)
void FontPlatformData::loadFont(NSFont* nsFont, float, NSFont*& outNSFont, CGFontRef& cgFont)
{
    outNSFont = nsFont;
    cgFont = CTFontCopyGraphicsFont(toCTFontRef(nsFont), 0);
}
#endif  // PLATFORM(MAC)

FontPlatformData::FontPlatformData(NSFont *nsFont, float size, bool syntheticBold, bool syntheticOblique, FontOrientation orientation,
                                   TextOrientation textOrientation, FontWidthVariant widthVariant)
    : m_syntheticBold(syntheticBold)
    , m_syntheticOblique(syntheticOblique)
    , m_orientation(orientation)
    , m_textOrientation(textOrientation)
    , m_size(size)
    , m_widthVariant(widthVariant)
    , m_font(nsFont)
#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
    // FIXME: Chromium: The following code isn't correct for the Chromium port since the sandbox might
    // have blocked font loading, in which case we'll only have the real loaded font file after the call to loadFont().
    , m_isColorBitmapFont(CTFontGetSymbolicTraits(toCTFontRef(nsFont)) & kCTFontColorGlyphsTrait)
#else
    , m_isColorBitmapFont(false)
#endif
{
    ASSERT_ARG(nsFont, nsFont);

    CGFontRef cgFont = 0;
    loadFont(nsFont, size, m_font, cgFont);

    if (m_font)
        CFRetain(m_font);

    m_cgFont.adoptCF(cgFont);
}

FontPlatformData:: ~FontPlatformData()
{
    if (m_font && m_font != reinterpret_cast<NSFont *>(-1))
        CFRelease(m_font);
}

void FontPlatformData::platformDataInit(const FontPlatformData& f)
{
    m_font = f.m_font && f.m_font != reinterpret_cast<NSFont *>(-1) ? const_cast<NSFont *>(static_cast<const NSFont *>(CFRetain(f.m_font))) : f.m_font;

    m_cgFont = f.m_cgFont;
    m_CTFont = f.m_CTFont;

#if PLATFORM(CHROMIUM) && OS(DARWIN)
    m_inMemoryFont = f.m_inMemoryFont;
#endif
}

const FontPlatformData& FontPlatformData::platformDataAssign(const FontPlatformData& f)
{
    m_cgFont = f.m_cgFont;
    if (m_font == f.m_font)
        return *this;
    if (f.m_font && f.m_font != reinterpret_cast<NSFont *>(-1))
        CFRetain(f.m_font);
    if (m_font && m_font != reinterpret_cast<NSFont *>(-1))
        CFRelease(m_font);
    m_font = f.m_font;
    m_CTFont = f.m_CTFont;
#if PLATFORM(CHROMIUM) && OS(DARWIN)
    m_inMemoryFont = f.m_inMemoryFont;
#endif
    return *this;
}

bool FontPlatformData::platformIsEqual(const FontPlatformData& other) const
{
    return m_font == other.m_font
        && m_cgFont == other.m_cgFont;
}

void FontPlatformData::setFont(NSFont *font)
{
    ASSERT_ARG(font, font);
    ASSERT(m_font != reinterpret_cast<NSFont *>(-1));

    if (m_font == font)
        return;

    CFRetain(font);
    if (m_font)
        CFRelease(m_font);
    m_font = font;
    m_size = [font pointSize];
    
    CGFontRef cgFont = 0;
    NSFont* loadedFont = 0;
    loadFont(m_font, m_size, loadedFont, cgFont);
    
#if PLATFORM(CHROMIUM) && OS(DARWIN)
    // If loadFont replaced m_font with a fallback font, then release the
    // previous font to counter the retain above. Then retain the new font.
    if (loadedFont != m_font) {
        CFRelease(m_font);
        CFRetain(loadedFont);
        m_font = loadedFont;
    }
#endif
    
    m_cgFont.adoptCF(cgFont);
#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
    m_isColorBitmapFont = CTFontGetSymbolicTraits(toCTFontRef(m_font)) & kCTFontColorGlyphsTrait;
#endif
    m_CTFont = 0;
}

bool FontPlatformData::roundsGlyphAdvances() const
{
    return [m_font renderingMode] == NSFontAntialiasedIntegerAdvancementsRenderingMode;
}

bool FontPlatformData::allowsLigatures() const
{
    return ![[m_font coveredCharacterSet] characterIsMember:'a'];
}

inline int mapFontWidthVariantToCTFeatureSelector(FontWidthVariant variant)
{
    switch(variant) {
    case RegularWidth:
        return TextSpacingProportional;

    case HalfWidth:
        return TextSpacingHalfWidth;

    case ThirdWidth:
        return TextSpacingThirdWidth;

    case QuarterWidth:
        return TextSpacingQuarterWidth;
    }

    ASSERT_NOT_REACHED();
    return TextSpacingProportional;
}

CTFontRef FontPlatformData::ctFont() const
{
    if (m_widthVariant == RegularWidth) {
        if (m_font)
            return toCTFontRef(m_font);
        if (!m_CTFont)
            m_CTFont.adoptCF(CTFontCreateWithGraphicsFont(m_cgFont.get(), m_size, 0, 0));
        return m_CTFont.get();
    }
    
    if (!m_CTFont) {
        int featureTypeValue = kTextSpacingType;
        int featureSelectorValue = mapFontWidthVariantToCTFeatureSelector(m_widthVariant);
        RetainPtr<CTFontRef> sourceFont(AdoptCF, CTFontCreateWithGraphicsFont(m_cgFont.get(), m_size, 0, 0));
        RetainPtr<CTFontDescriptorRef> sourceDescriptor(AdoptCF, CTFontCopyFontDescriptor(sourceFont.get()));
        RetainPtr<CFNumberRef> featureType(AdoptCF, CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &featureTypeValue));
        RetainPtr<CFNumberRef> featureSelector(AdoptCF, CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &featureSelectorValue));
        RetainPtr<CTFontDescriptorRef> newDescriptor(AdoptCF, CTFontDescriptorCreateCopyWithFeature(sourceDescriptor.get(), featureType.get(), featureSelector.get()));
        RetainPtr<CTFontRef> newFont(AdoptCF, CTFontCreateWithFontDescriptor(newDescriptor.get(), m_size, 0));

        m_CTFont = newFont.get() ? newFont : sourceFont;
    }
    return m_CTFont.get();
}

#ifndef NDEBUG
String FontPlatformData::description() const
{
    RetainPtr<CFStringRef> cgFontDescription(AdoptCF, CFCopyDescription(cgFont()));
    return String(cgFontDescription.get()) + " " + String::number(m_size)
            + (m_syntheticBold ? " synthetic bold" : "") + (m_syntheticOblique ? " synthetic oblique" : "") + (m_orientation ? " vertical orientation" : "");
}
#endif

} // namespace WebCore
