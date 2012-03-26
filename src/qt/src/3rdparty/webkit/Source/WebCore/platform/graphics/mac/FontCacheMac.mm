/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Nicholas Shanks <webkit@nickshanks.com>
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
#import "FontCache.h"

#import "Font.h"
#import "SimpleFontData.h"
#import "FontPlatformData.h"
#import "WebCoreSystemInterface.h"
#import "WebFontCache.h"
#import <AppKit/AppKit.h>
#import <wtf/MainThread.h>
#import <wtf/StdLibExtras.h>


namespace WebCore {

// The "void*" parameter makes the function match the prototype for callbacks from callOnMainThread.
static void invalidateFontCache(void*)
{
    if (!isMainThread()) {
        callOnMainThread(&invalidateFontCache, 0);
        return;
    }
    fontCache()->invalidate();
}

#if !defined(BUILDING_ON_LEOPARD)
static void fontCacheRegisteredFontsChangedNotificationCallback(CFNotificationCenterRef, void* observer, CFStringRef name, const void *, CFDictionaryRef)
{
    ASSERT_UNUSED(observer, observer == fontCache());
    ASSERT_UNUSED(name, CFEqual(name, kCTFontManagerRegisteredFontsChangedNotification));
    invalidateFontCache(0);
}
#else
static void fontCacheATSNotificationCallback(ATSFontNotificationInfoRef, void*)
{
    invalidateFontCache(0);
}
#endif

void FontCache::platformInit()
{
    wkSetUpFontCache();
#if !defined(BUILDING_ON_LEOPARD)
    CFNotificationCenterAddObserver(CFNotificationCenterGetLocalCenter(), this, fontCacheRegisteredFontsChangedNotificationCallback, kCTFontManagerRegisteredFontsChangedNotification, 0, CFNotificationSuspensionBehaviorDeliverImmediately);
#else
    // kCTFontManagerRegisteredFontsChangedNotification does not exist on Leopard and earlier.
    // FIXME: Passing kATSFontNotifyOptionReceiveWhileSuspended may be an overkill and does not seem to work anyway.
    ATSFontNotificationSubscribe(fontCacheATSNotificationCallback, kATSFontNotifyOptionReceiveWhileSuspended, 0, 0);
#endif
}

static int toAppKitFontWeight(FontWeight fontWeight)
{
    static int appKitFontWeights[] = {
        2,  // FontWeight100
        3,  // FontWeight200
        4,  // FontWeight300
        5,  // FontWeight400
        6,  // FontWeight500
        8,  // FontWeight600
        9,  // FontWeight700
        10, // FontWeight800
        12, // FontWeight900
    };
    return appKitFontWeights[fontWeight];
}

static inline bool isAppKitFontWeightBold(NSInteger appKitFontWeight)
{
    return appKitFontWeight >= 7;
}

const SimpleFontData* FontCache::getFontDataForCharacters(const Font& font, const UChar* characters, int length)
{
    const FontPlatformData& platformData = font.fontDataAt(0)->fontDataForCharacter(characters[0])->platformData();
    NSFont *nsFont = platformData.font();

    NSString *string = [[NSString alloc] initWithCharactersNoCopy:const_cast<UChar*>(characters) length:length freeWhenDone:NO];
    NSFont *substituteFont = wkGetFontInLanguageForRange(nsFont, string, NSMakeRange(0, length));
    [string release];

    if (!substituteFont && length == 1)
        substituteFont = wkGetFontInLanguageForCharacter(nsFont, characters[0]);
    if (!substituteFont)
        return 0;

    // Use the family name from the AppKit-supplied substitute font, requesting the
    // traits, weight, and size we want. One way this does better than the original
    // AppKit request is that it takes synthetic bold and oblique into account.
    // But it does create the possibility that we could end up with a font that
    // doesn't actually cover the characters we need.

    NSFontManager *fontManager = [NSFontManager sharedFontManager];

    NSFontTraitMask traits;
    NSInteger weight;
    CGFloat size;

    if (nsFont) {
        traits = [fontManager traitsOfFont:nsFont];
        if (platformData.m_syntheticBold)
            traits |= NSBoldFontMask;
        if (platformData.m_syntheticOblique)
            traits |= NSFontItalicTrait;
        weight = [fontManager weightOfFont:nsFont];
        size = [nsFont pointSize];
    } else {
        // For custom fonts nsFont is nil.
        traits = font.italic() ? NSFontItalicTrait : 0;
        weight = toAppKitFontWeight(font.weight());
        size = font.pixelSize();
    }

    if (NSFont *bestVariation = [fontManager fontWithFamily:[substituteFont familyName] traits:traits weight:weight size:size])
        substituteFont = bestVariation;

    substituteFont = font.fontDescription().usePrinterFont() ? [substituteFont printerFont] : [substituteFont screenFont];

    NSFontTraitMask substituteFontTraits = [fontManager traitsOfFont:substituteFont];
    NSInteger substituteFontWeight = [fontManager weightOfFont:substituteFont];

    FontPlatformData alternateFont(substituteFont, platformData.size(),
        !font.isPlatformFont() && isAppKitFontWeightBold(weight) && !isAppKitFontWeightBold(substituteFontWeight),
        !font.isPlatformFont() && (traits & NSFontItalicTrait) && !(substituteFontTraits & NSFontItalicTrait),
        platformData.m_orientation);
    return getCachedFontData(&alternateFont);
}

SimpleFontData* FontCache::getSimilarFontPlatformData(const Font& font)
{
    // Attempt to find an appropriate font using a match based on 
    // the presence of keywords in the the requested names.  For example, we'll
    // match any name that contains "Arabic" to Geeza Pro.
    SimpleFontData* simpleFontData = 0;
    const FontFamily* currFamily = &font.fontDescription().family();
    while (currFamily && !simpleFontData) {
        if (currFamily->family().length()) {
            static String* matchWords[3] = { new String("Arabic"), new String("Pashto"), new String("Urdu") };
            DEFINE_STATIC_LOCAL(AtomicString, geezaStr, ("Geeza Pro"));
            for (int j = 0; j < 3 && !simpleFontData; ++j)
                if (currFamily->family().contains(*matchWords[j], false))
                    simpleFontData = getCachedFontData(font.fontDescription(), geezaStr);
        }
        currFamily = currFamily->next();
    }

    return simpleFontData;
}

SimpleFontData* FontCache::getLastResortFallbackFont(const FontDescription& fontDescription)
{
    DEFINE_STATIC_LOCAL(AtomicString, timesStr, ("Times"));

    // FIXME: Would be even better to somehow get the user's default font here.  For now we'll pick
    // the default that the user would get without changing any prefs.
    SimpleFontData* simpleFontData = getCachedFontData(fontDescription, timesStr);
    if (simpleFontData)
        return simpleFontData;

    // The Times fallback will almost always work, but in the highly unusual case where
    // the user doesn't have it, we fall back on Lucida Grande because that's
    // guaranteed to be there, according to Nathan Taylor. This is good enough
    // to avoid a crash at least.
    DEFINE_STATIC_LOCAL(AtomicString, lucidaGrandeStr, ("Lucida Grande"));
    return getCachedFontData(fontDescription, lucidaGrandeStr);
}

void FontCache::getTraitsInFamily(const AtomicString& familyName, Vector<unsigned>& traitsMasks)
{
    [WebFontCache getTraits:traitsMasks inFamily:familyName];
}

FontPlatformData* FontCache::createFontPlatformData(const FontDescription& fontDescription, const AtomicString& family)
{
    NSFontTraitMask traits = fontDescription.italic() ? NSFontItalicTrait : 0;
    NSInteger weight = toAppKitFontWeight(fontDescription.weight());
    float size = fontDescription.computedPixelSize();

    NSFont *nsFont = [WebFontCache fontWithFamily:family traits:traits weight:weight size:size];
    if (!nsFont)
        return 0;

    NSFontManager *fontManager = [NSFontManager sharedFontManager];
    NSFontTraitMask actualTraits = 0;
    if (fontDescription.italic())
        actualTraits = [fontManager traitsOfFont:nsFont];
    NSInteger actualWeight = [fontManager weightOfFont:nsFont];

    NSFont *platformFont = fontDescription.usePrinterFont() ? [nsFont printerFont] : [nsFont screenFont];
    bool syntheticBold = isAppKitFontWeightBold(weight) && !isAppKitFontWeightBold(actualWeight);
    bool syntheticOblique = (traits & NSFontItalicTrait) && !(actualTraits & NSFontItalicTrait);

    return new FontPlatformData(platformFont, size, syntheticBold, syntheticOblique, fontDescription.orientation(), fontDescription.textOrientation(), fontDescription.widthVariant());
}

} // namespace WebCore
