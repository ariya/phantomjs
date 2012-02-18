/*
 * Copyright (C) 2005, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"

// FIXME: Remove this define!
#define LOOSE_PASS_OWN_PTR

#import "SimpleFontData.h"

#import "BlockExceptions.h"
#import "Color.h"
#import "FloatRect.h"
#import "Font.h"
#import "FontCache.h"
#import "FontDescription.h"
#import "SharedBuffer.h"
#import "WebCoreSystemInterface.h"
#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <float.h>
#import <unicode/uchar.h>
#import <wtf/Assertions.h>
#import <wtf/StdLibExtras.h>
#import <wtf/RetainPtr.h>
#import <wtf/UnusedParam.h>

@interface NSFont (WebAppKitSecretAPI)
- (BOOL)_isFakeFixedPitch;
@end

using namespace std;

namespace WebCore {
  
const float smallCapsFontSizeMultiplier = 0.7f;
static inline float scaleEmToUnits(float x, unsigned unitsPerEm) { return x / unitsPerEm; }

static bool initFontData(SimpleFontData* fontData)
{
    if (!fontData->platformData().cgFont())
        return false;


    return true;
}

static NSString *webFallbackFontFamily(void)
{
    DEFINE_STATIC_LOCAL(RetainPtr<NSString>, webFallbackFontFamily, ([[NSFont systemFontOfSize:16.0f] familyName]));
    return webFallbackFontFamily.get();
}

#if !ERROR_DISABLED
#if defined(__LP64__) || (!defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD))
static NSString* pathFromFont(NSFont*)
{
    // FMGetATSFontRefFromFont is not available. As pathFromFont is only used for debugging purposes,
    // returning nil is acceptable.
    return nil;
}
#else
static NSString* pathFromFont(NSFont *font)
{
    ATSFontRef atsFont = FMGetATSFontRefFromFont(CTFontGetPlatformFont(toCTFontRef(font), 0));
    FSRef fileRef;

    OSStatus status = ATSFontGetFileReference(atsFont, &fileRef);
    if (status != noErr)
        return nil;

    UInt8 filePathBuffer[PATH_MAX];
    status = FSRefMakePath(&fileRef, filePathBuffer, PATH_MAX);
    if (status == noErr)
        return [NSString stringWithUTF8String:(const char*)filePathBuffer];

    return nil;
}
#endif // __LP64__
#endif // !ERROR_DISABLED

void SimpleFontData::platformInit()
{
#if USE(ATSUI)
    m_ATSUMirrors = false;
    m_checkedShapesArabic = false;
    m_shapesArabic = false;
#endif

    m_syntheticBoldOffset = m_platformData.m_syntheticBold ? 1.0f : 0.f;

    bool failedSetup = false;
    if (!initFontData(this)) {
        // Ack! Something very bad happened, like a corrupt font.
        // Try looking for an alternate 'base' font for this renderer.

        // Special case hack to use "Times New Roman" in place of "Times".
        // "Times RO" is a common font whose family name is "Times".
        // It overrides the normal "Times" family font.
        // It also appears to have a corrupt regular variant.
        NSString *fallbackFontFamily;
        if ([[m_platformData.font() familyName] isEqual:@"Times"])
            fallbackFontFamily = @"Times New Roman";
        else
            fallbackFontFamily = webFallbackFontFamily();
        
        // Try setting up the alternate font.
        // This is a last ditch effort to use a substitute font when something has gone wrong.
#if !ERROR_DISABLED
        RetainPtr<NSFont> initialFont = m_platformData.font();
#endif
        if (m_platformData.font())
            m_platformData.setFont([[NSFontManager sharedFontManager] convertFont:m_platformData.font() toFamily:fallbackFontFamily]);
        else
            m_platformData.setFont([NSFont fontWithName:fallbackFontFamily size:m_platformData.size()]);
#if !ERROR_DISABLED
        NSString *filePath = pathFromFont(initialFont.get());
        if (!filePath)
            filePath = @"not known";
#endif
        if (!initFontData(this)) {
            if ([fallbackFontFamily isEqual:@"Times New Roman"]) {
                // OK, couldn't setup Times New Roman as an alternate to Times, fallback
                // on the system font.  If this fails we have no alternative left.
                m_platformData.setFont([[NSFontManager sharedFontManager] convertFont:m_platformData.font() toFamily:webFallbackFontFamily()]);
                if (!initFontData(this)) {
                    // We tried, Times, Times New Roman, and the system font. No joy. We have to give up.
                    LOG_ERROR("unable to initialize with font %@ at %@", initialFont.get(), filePath);
                    failedSetup = true;
                }
            } else {
                // We tried the requested font and the system font. No joy. We have to give up.
                LOG_ERROR("unable to initialize with font %@ at %@", initialFont.get(), filePath);
                failedSetup = true;
            }
        }

        // Report the problem.
        LOG_ERROR("Corrupt font detected, using %@ in place of %@ located at \"%@\".",
            [m_platformData.font() familyName], [initialFont.get() familyName], filePath);
    }

    // If all else fails, try to set up using the system font.
    // This is probably because Times and Times New Roman are both unavailable.
    if (failedSetup) {
        m_platformData.setFont([NSFont systemFontOfSize:[m_platformData.font() pointSize]]);
        LOG_ERROR("failed to set up font, using system font %s", m_platformData.font());
        initFontData(this);
    }
    
    int iAscent;
    int iDescent;
    int iLineGap;
    unsigned unitsPerEm;
    iAscent = CGFontGetAscent(m_platformData.cgFont());
    iDescent = CGFontGetDescent(m_platformData.cgFont());
    iLineGap = CGFontGetLeading(m_platformData.cgFont());
    unitsPerEm = CGFontGetUnitsPerEm(m_platformData.cgFont());

    float pointSize = m_platformData.m_size;
    float ascent = scaleEmToUnits(iAscent, unitsPerEm) * pointSize;
    float descent = -scaleEmToUnits(iDescent, unitsPerEm) * pointSize;
    float lineGap = scaleEmToUnits(iLineGap, unitsPerEm) * pointSize;

    // We need to adjust Times, Helvetica, and Courier to closely match the
    // vertical metrics of their Microsoft counterparts that are the de facto
    // web standard. The AppKit adjustment of 20% is too big and is
    // incorrectly added to line spacing, so we use a 15% adjustment instead
    // and add it to the ascent.
    NSString *familyName = [m_platformData.font() familyName];
    if ([familyName isEqualToString:@"Times"] || [familyName isEqualToString:@"Helvetica"] || [familyName isEqualToString:@"Courier"])
        ascent += floorf(((ascent + descent) * 0.15f) + 0.5f);
#if defined(BUILDING_ON_LEOPARD)
    else if ([familyName isEqualToString:@"Geeza Pro"]) {
        // Geeza Pro has glyphs that draw slightly above the ascent or far below the descent. Adjust
        // those vertical metrics to better match reality, so that diacritics at the bottom of one line
        // do not overlap diacritics at the top of the next line.
        ascent *= 1.08f;
        descent *= 2.f;
    }
#endif

    // Compute and store line spacing, before the line metrics hacks are applied.
    m_fontMetrics.setLineSpacing(lroundf(ascent) + lroundf(descent) + lroundf(lineGap));

    // Hack Hiragino line metrics to allow room for marked text underlines.
    // <rdar://problem/5386183>
    if (descent < 3 && lineGap >= 3 && [familyName hasPrefix:@"Hiragino"]) {
        lineGap -= 3 - descent;
        descent = 3;
    }
    
    if (platformData().orientation() == Vertical && !isTextOrientationFallback()) {
        // The check doesn't look neat but this is what AppKit does for vertical writing...
        RetainPtr<CFArrayRef> tableTags(AdoptCF, CTFontCopyAvailableTables(m_platformData.ctFont(), kCTFontTableOptionExcludeSynthetic));
        CFIndex numTables = CFArrayGetCount(tableTags.get());
        for (CFIndex index = 0; index < numTables; ++index) {
            CTFontTableTag tag = (CTFontTableTag)(uintptr_t)CFArrayGetValueAtIndex(tableTags.get(), index);
            if (tag == kCTFontTableVhea || tag == kCTFontTableVORG) {
                m_hasVerticalGlyphs = true;
                break;
            }
        }
    }

    float xHeight;

    if (platformData().orientation() == Horizontal) {
        // Measure the actual character "x", since it's possible for it to extend below the baseline, and we need the
        // reported x-height to only include the portion of the glyph that is above the baseline.
        GlyphPage* glyphPageZero = GlyphPageTreeNode::getRootChild(this, 0)->page();
        NSGlyph xGlyph = glyphPageZero ? glyphPageZero->glyphDataForCharacter('x').glyph : 0;
        if (xGlyph)
            xHeight = -CGRectGetMinY(platformBoundsForGlyph(xGlyph));
        else
            xHeight = scaleEmToUnits(CGFontGetXHeight(m_platformData.cgFont()), unitsPerEm) * pointSize;
    } else
        xHeight = verticalRightOrientationFontData()->fontMetrics().xHeight();

    m_fontMetrics.setUnitsPerEm(unitsPerEm);
    m_fontMetrics.setAscent(ascent);
    m_fontMetrics.setDescent(descent);
    m_fontMetrics.setLineGap(lineGap);
    m_fontMetrics.setXHeight(xHeight);
}

static CFDataRef copyFontTableForTag(FontPlatformData& platformData, FourCharCode tableName)
{
    return CGFontCopyTableForTag(platformData.cgFont(), tableName);
}

void SimpleFontData::platformCharWidthInit()
{
    m_avgCharWidth = 0;
    m_maxCharWidth = 0;
    
    RetainPtr<CFDataRef> os2Table(AdoptCF, copyFontTableForTag(m_platformData, 'OS/2'));
    if (os2Table && CFDataGetLength(os2Table.get()) >= 4) {
        const UInt8* os2 = CFDataGetBytePtr(os2Table.get());
        SInt16 os2AvgCharWidth = os2[2] * 256 + os2[3];
        m_avgCharWidth = scaleEmToUnits(os2AvgCharWidth, m_fontMetrics.unitsPerEm()) * m_platformData.m_size;
    }

    RetainPtr<CFDataRef> headTable(AdoptCF, copyFontTableForTag(m_platformData, 'head'));
    if (headTable && CFDataGetLength(headTable.get()) >= 42) {
        const UInt8* head = CFDataGetBytePtr(headTable.get());
        ushort uxMin = head[36] * 256 + head[37];
        ushort uxMax = head[40] * 256 + head[41];
        SInt16 xMin = static_cast<SInt16>(uxMin);
        SInt16 xMax = static_cast<SInt16>(uxMax);
        float diff = static_cast<float>(xMax - xMin);
        m_maxCharWidth = scaleEmToUnits(diff, m_fontMetrics.unitsPerEm()) * m_platformData.m_size;
    }

    // Fallback to a cross-platform estimate, which will populate these values if they are non-positive.
    initCharWidths();
}

void SimpleFontData::platformDestroy()
{
    if (!isCustomFont() && m_derivedFontData) {
        // These come from the cache.
        if (m_derivedFontData->smallCaps)
            fontCache()->releaseFontData(m_derivedFontData->smallCaps.leakPtr());

        if (m_derivedFontData->emphasisMark)
            fontCache()->releaseFontData(m_derivedFontData->emphasisMark.leakPtr());
    }

#if USE(ATSUI)
    HashMap<unsigned, ATSUStyle>::iterator end = m_ATSUStyleMap.end();
    for (HashMap<unsigned, ATSUStyle>::iterator it = m_ATSUStyleMap.begin(); it != end; ++it)
        ATSUDisposeStyle(it->second);
#endif
}

SimpleFontData* SimpleFontData::scaledFontData(const FontDescription& fontDescription, float scaleFactor) const
{
    if (isCustomFont()) {
        FontPlatformData scaledFontData(m_platformData);
        scaledFontData.m_size = scaledFontData.m_size * scaleFactor;
        return new SimpleFontData(scaledFontData, true, false);
    }

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    float size = m_platformData.size() * scaleFactor;
    FontPlatformData scaledFontData([[NSFontManager sharedFontManager] convertFont:m_platformData.font() toSize:size], size, false, false, m_platformData.orientation());

    // AppKit resets the type information (screen/printer) when you convert a font to a different size.
    // We have to fix up the font that we're handed back.
    scaledFontData.setFont(fontDescription.usePrinterFont() ? [scaledFontData.font() printerFont] : [scaledFontData.font() screenFont]);

    if (scaledFontData.font()) {
        NSFontManager *fontManager = [NSFontManager sharedFontManager];
        NSFontTraitMask fontTraits = [fontManager traitsOfFont:m_platformData.font()];

        if (m_platformData.m_syntheticBold)
            fontTraits |= NSBoldFontMask;
        if (m_platformData.m_syntheticOblique)
            fontTraits |= NSItalicFontMask;

        NSFontTraitMask scaledFontTraits = [fontManager traitsOfFont:scaledFontData.font()];
        scaledFontData.m_syntheticBold = (fontTraits & NSBoldFontMask) && !(scaledFontTraits & NSBoldFontMask);
        scaledFontData.m_syntheticOblique = (fontTraits & NSItalicFontMask) && !(scaledFontTraits & NSItalicFontMask);

        return fontCache()->getCachedFontData(&scaledFontData);
    }
    END_BLOCK_OBJC_EXCEPTIONS;

    return 0;
}

SimpleFontData* SimpleFontData::smallCapsFontData(const FontDescription& fontDescription) const
{
    if (!m_derivedFontData)
        m_derivedFontData = DerivedFontData::create(isCustomFont());
    if (!m_derivedFontData->smallCaps)
        m_derivedFontData->smallCaps = scaledFontData(fontDescription, smallCapsFontSizeMultiplier);

    return m_derivedFontData->smallCaps.get();
}

SimpleFontData* SimpleFontData::emphasisMarkFontData(const FontDescription& fontDescription) const
{
    if (!m_derivedFontData)
        m_derivedFontData = DerivedFontData::create(isCustomFont());
    if (!m_derivedFontData->emphasisMark)
        m_derivedFontData->emphasisMark = scaledFontData(fontDescription, .5f);

    return m_derivedFontData->emphasisMark.get();
}

bool SimpleFontData::containsCharacters(const UChar* characters, int length) const
{
    NSString *string = [[NSString alloc] initWithCharactersNoCopy:const_cast<unichar*>(characters) length:length freeWhenDone:NO];
    NSCharacterSet *set = [[m_platformData.font() coveredCharacterSet] invertedSet];
    bool result = set && [string rangeOfCharacterFromSet:set].location == NSNotFound;
    [string release];
    return result;
}

void SimpleFontData::determinePitch()
{
    NSFont* f = m_platformData.font();
    // Special case Osaka-Mono.
    // According to <rdar://problem/3999467>, we should treat Osaka-Mono as fixed pitch.
    // Note that the AppKit does not report Osaka-Mono as fixed pitch.

    // Special case MS-PGothic.
    // According to <rdar://problem/4032938>, we should not treat MS-PGothic as fixed pitch.
    // Note that AppKit does report MS-PGothic as fixed pitch.

    // Special case MonotypeCorsiva
    // According to <rdar://problem/5454704>, we should not treat MonotypeCorsiva as fixed pitch.
    // Note that AppKit does report MonotypeCorsiva as fixed pitch.

    NSString *name = [f fontName];
    m_treatAsFixedPitch = ([f isFixedPitch] || [f _isFakeFixedPitch] ||
           [name caseInsensitiveCompare:@"Osaka-Mono"] == NSOrderedSame) &&
           [name caseInsensitiveCompare:@"MS-PGothic"] != NSOrderedSame &&
           [name caseInsensitiveCompare:@"MonotypeCorsiva"] != NSOrderedSame;
}

FloatRect SimpleFontData::platformBoundsForGlyph(Glyph glyph) const
{
    FloatRect boundingBox;
    boundingBox = CTFontGetBoundingRectsForGlyphs(m_platformData.ctFont(), platformData().orientation() == Vertical ? kCTFontVerticalOrientation : kCTFontHorizontalOrientation, &glyph, 0, 1);
    boundingBox.setY(-boundingBox.maxY());
    if (m_syntheticBoldOffset)
        boundingBox.setWidth(boundingBox.width() + m_syntheticBoldOffset);

    return boundingBox;
}

float SimpleFontData::platformWidthForGlyph(Glyph glyph) const
{
    CGSize advance;
    if (platformData().orientation() == Horizontal || m_isBrokenIdeographFallback) {
        NSFont* font = platformData().font();
        float pointSize = platformData().m_size;
        CGAffineTransform m = CGAffineTransformMakeScale(pointSize, pointSize);
        if (!wkGetGlyphTransformedAdvances(platformData().cgFont(), font, &m, &glyph, &advance)) {
            LOG_ERROR("Unable to cache glyph widths for %@ %f", [font displayName], pointSize);
            advance.width = 0;
        }
    } else
        CTFontGetAdvancesForGlyphs(m_platformData.ctFont(), kCTFontVerticalOrientation, &glyph, &advance, 1);

    return advance.width + m_syntheticBoldOffset;
}

} // namespace WebCore
