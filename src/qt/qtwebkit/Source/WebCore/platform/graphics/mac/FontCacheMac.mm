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

static void fontCacheRegisteredFontsChangedNotificationCallback(CFNotificationCenterRef, void* observer, CFStringRef name, const void *, CFDictionaryRef)
{
    ASSERT_UNUSED(observer, observer == fontCache());
    ASSERT_UNUSED(name, CFEqual(name, kCTFontManagerRegisteredFontsChangedNotification));
    invalidateFontCache(0);
}

void FontCache::platformInit()
{
    wkSetUpFontCache();
    CFNotificationCenterAddObserver(CFNotificationCenterGetLocalCenter(), this, fontCacheRegisteredFontsChangedNotificationCallback, kCTFontManagerRegisteredFontsChangedNotification, 0, CFNotificationSuspensionBehaviorDeliverImmediately);
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

PassRefPtr<SimpleFontData> FontCache::systemFallbackForCharacters(const FontDescription& description, const SimpleFontData* originalFontData, bool isPlatformFont, const UChar* characters, int length)
{
    UChar32 character;
    U16_GET(characters, 0, 0, length, character);
    const FontPlatformData& platformData = originalFontData->platformData();
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
        traits = description.italic() ? NSFontItalicTrait : 0;
        weight = toAppKitFontWeight(description.weight());
        size = description.computedPixelSize();
    }

    NSFontTraitMask substituteFontTraits = [fontManager traitsOfFont:substituteFont];
    NSInteger substituteFontWeight = [fontManager weightOfFont:substituteFont];

    if (traits != substituteFontTraits || weight != substituteFontWeight || !nsFont) {
        if (NSFont *bestVariation = [fontManager fontWithFamily:[substituteFont familyName] traits:traits weight:weight size:size]) {
            if (!nsFont || (([fontManager traitsOfFont:bestVariation] != substituteFontTraits || [fontManager weightOfFont:bestVariation] != substituteFontWeight)
                && [[bestVariation coveredCharacterSet] longCharacterIsMember:character]))
                substituteFont = bestVariation;
        }
    }

    substituteFont = description.usePrinterFont() ? [substituteFont printerFont] : [substituteFont screenFont];

    substituteFontTraits = [fontManager traitsOfFont:substituteFont];
    substituteFontWeight = [fontManager weightOfFont:substituteFont];

    FontPlatformData alternateFont(substituteFont, platformData.size(), platformData.isPrinterFont(),
        !isPlatformFont && isAppKitFontWeightBold(weight) && !isAppKitFontWeightBold(substituteFontWeight),
        !isPlatformFont && (traits & NSFontItalicTrait) && !(substituteFontTraits & NSFontItalicTrait),
        platformData.m_orientation);

    return getCachedFontData(&alternateFont, DoNotRetain);
}

PassRefPtr<SimpleFontData> FontCache::similarFontPlatformData(const FontDescription& description)
{
    // Attempt to find an appropriate font using a match based on 
    // the presence of keywords in the the requested names.  For example, we'll
    // match any name that contains "Arabic" to Geeza Pro.
    RefPtr<SimpleFontData> simpleFontData;
    for (unsigned i = 0; i < description.familyCount(); ++i) {
        const AtomicString& family = description.familyAt(i);
        if (family.isEmpty())
            continue;
        static String* matchWords[3] = { new String("Arabic"), new String("Pashto"), new String("Urdu") };
        DEFINE_STATIC_LOCAL(AtomicString, geezaStr, ("Geeza Pro", AtomicString::ConstructFromLiteral));
        for (int j = 0; j < 3 && !simpleFontData; ++j)
            if (family.contains(*matchWords[j], false))
                simpleFontData = getCachedFontData(description, geezaStr);
    }
    return simpleFontData.release();
}

PassRefPtr<SimpleFontData> FontCache::getLastResortFallbackFont(const FontDescription& fontDescription, ShouldRetain shouldRetain)
{
    DEFINE_STATIC_LOCAL(AtomicString, timesStr, ("Times", AtomicString::ConstructFromLiteral));

    // FIXME: Would be even better to somehow get the user's default font here.  For now we'll pick
    // the default that the user would get without changing any prefs.
    RefPtr<SimpleFontData> simpleFontData = getCachedFontData(fontDescription, timesStr, false, shouldRetain);
    if (simpleFontData)
        return simpleFontData.release();

    // The Times fallback will almost always work, but in the highly unusual case where
    // the user doesn't have it, we fall back on Lucida Grande because that's
    // guaranteed to be there, according to Nathan Taylor. This is good enough
    // to avoid a crash at least.
    DEFINE_STATIC_LOCAL(AtomicString, lucidaGrandeStr, ("Lucida Grande", AtomicString::ConstructFromLiteral));
    return getCachedFontData(fontDescription, lucidaGrandeStr, false, shouldRetain);
}

void FontCache::getTraitsInFamily(const AtomicString& familyName, Vector<unsigned>& traitsMasks)
{
    [WebFontCache getTraits:traitsMasks inFamily:familyName];
}

PassOwnPtr<FontPlatformData> FontCache::createFontPlatformData(const FontDescription& fontDescription, const AtomicString& family)
{
    NSFontTraitMask traits = fontDescription.italic() ? NSFontItalicTrait : 0;
    NSInteger weight = toAppKitFontWeight(fontDescription.weight());
    float size = fontDescription.computedPixelSize();

    NSFont *nsFont = [WebFontCache fontWithFamily:family traits:traits weight:weight size:size];
    if (!nsFont)
        return nullptr;

    NSFontManager *fontManager = [NSFontManager sharedFontManager];
    NSFontTraitMask actualTraits = 0;
    if (fontDescription.italic())
        actualTraits = [fontManager traitsOfFont:nsFont];
    NSInteger actualWeight = [fontManager weightOfFont:nsFont];

    NSFont *platformFont = fontDescription.usePrinterFont() ? [nsFont printerFont] : [nsFont screenFont];
    bool syntheticBold = isAppKitFontWeightBold(weight) && !isAppKitFontWeightBold(actualWeight);
    bool syntheticOblique = (traits & NSFontItalicTrait) && !(actualTraits & NSFontItalicTrait);

    OwnPtr<FontPlatformData> platformData = adoptPtr(new FontPlatformData(platformFont, size, fontDescription.usePrinterFont(), syntheticBold, syntheticOblique, fontDescription.orientation(), fontDescription.widthVariant()));
    return platformData.release();
}

} // namespace WebCore
