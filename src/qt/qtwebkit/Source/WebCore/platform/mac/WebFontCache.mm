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
#import "WebFontCache.h"

#import "FontTraitsMask.h"
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <math.h>

using namespace WebCore;

#define SYNTHESIZED_FONT_TRAITS (NSBoldFontMask | NSItalicFontMask)

#define IMPORTANT_FONT_TRAITS (0 \
    | NSCompressedFontMask \
    | NSCondensedFontMask \
    | NSExpandedFontMask \
    | NSItalicFontMask \
    | NSNarrowFontMask \
    | NSPosterFontMask \
    | NSSmallCapsFontMask \
)

static BOOL acceptableChoice(NSFontTraitMask desiredTraits, NSFontTraitMask candidateTraits)
{
    desiredTraits &= ~SYNTHESIZED_FONT_TRAITS;
    return (candidateTraits & desiredTraits) == desiredTraits;
}

static BOOL betterChoice(NSFontTraitMask desiredTraits, int desiredWeight,
    NSFontTraitMask chosenTraits, int chosenWeight,
    NSFontTraitMask candidateTraits, int candidateWeight)
{
    if (!acceptableChoice(desiredTraits, candidateTraits))
        return NO;

    // A list of the traits we care about.
    // The top item in the list is the worst trait to mismatch; if a font has this
    // and we didn't ask for it, we'd prefer any other font in the family.
    const NSFontTraitMask masks[] = {
        NSPosterFontMask,
        NSSmallCapsFontMask,
        NSItalicFontMask,
        NSCompressedFontMask,
        NSCondensedFontMask,
        NSExpandedFontMask,
        NSNarrowFontMask,
        0
    };

    int i = 0;
    NSFontTraitMask mask;
    while ((mask = masks[i++])) {
        BOOL desired = (desiredTraits & mask) != 0;
        BOOL chosenHasUnwantedTrait = desired != ((chosenTraits & mask) != 0);
        BOOL candidateHasUnwantedTrait = desired != ((candidateTraits & mask) != 0);
        if (!candidateHasUnwantedTrait && chosenHasUnwantedTrait)
            return YES;
        if (!chosenHasUnwantedTrait && candidateHasUnwantedTrait)
            return NO;
    }

    int chosenWeightDeltaMagnitude = abs(chosenWeight - desiredWeight);
    int candidateWeightDeltaMagnitude = abs(candidateWeight - desiredWeight);

    // If both are the same distance from the desired weight, prefer the candidate if it is further from medium.
    if (chosenWeightDeltaMagnitude == candidateWeightDeltaMagnitude)
        return abs(candidateWeight - 6) > abs(chosenWeight - 6);

    // Otherwise, prefer the one closer to the desired weight.
    return candidateWeightDeltaMagnitude < chosenWeightDeltaMagnitude;
}

static inline FontTraitsMask toTraitsMask(NSFontTraitMask appKitTraits, NSInteger appKitWeight)
{
    return static_cast<FontTraitsMask>(((appKitTraits & NSFontItalicTrait) ? FontStyleItalicMask : FontStyleNormalMask)
        | FontVariantNormalMask
        | (appKitWeight == 1 ? FontWeight100Mask :
              appKitWeight == 2 ? FontWeight200Mask :
              appKitWeight <= 4 ? FontWeight300Mask :
              appKitWeight == 5 ? FontWeight400Mask :
              appKitWeight == 6 ? FontWeight500Mask :
              appKitWeight <= 8 ? FontWeight600Mask :
              appKitWeight == 9 ? FontWeight700Mask :
              appKitWeight <= 11 ? FontWeight800Mask :
                                   FontWeight900Mask));
}

@implementation WebFontCache

+ (void)getTraits:(Vector<unsigned>&)traitsMasks inFamily:(NSString *)desiredFamily
{
    NSFontManager *fontManager = [NSFontManager sharedFontManager];

    NSEnumerator *e = [[fontManager availableFontFamilies] objectEnumerator];
    NSString *availableFamily;
    while ((availableFamily = [e nextObject])) {
        if ([desiredFamily caseInsensitiveCompare:availableFamily] == NSOrderedSame)
            break;
    }

    if (!availableFamily) {
        // Match by PostScript name.
        NSEnumerator *availableFonts = [[fontManager availableFonts] objectEnumerator];
        NSString *availableFont;
        while ((availableFont = [availableFonts nextObject])) {
            if ([desiredFamily caseInsensitiveCompare:availableFont] == NSOrderedSame) {
                NSFont *font = [NSFont fontWithName:availableFont size:10];
                NSInteger weight = [fontManager weightOfFont:font];
                traitsMasks.append(toTraitsMask([fontManager traitsOfFont:font], weight));
                break;
            }
        }
        return;
    }

    NSArray *fonts = [fontManager availableMembersOfFontFamily:availableFamily];    
    unsigned n = [fonts count];
    unsigned i;
    for (i = 0; i < n; i++) {
        NSArray *fontInfo = [fonts objectAtIndex:i];
        // Array indices must be hard coded because of lame AppKit API.
        NSInteger fontWeight = [[fontInfo objectAtIndex:2] intValue];
        NSFontTraitMask fontTraits = [[fontInfo objectAtIndex:3] unsignedIntValue];
        traitsMasks.append(toTraitsMask(fontTraits, fontWeight));
    }
}

// Family name is somewhat of a misnomer here.  We first attempt to find an exact match
// comparing the desiredFamily to the PostScript name of the installed fonts.  If that fails
// we then do a search based on the family names of the installed fonts.
+ (NSFont *)internalFontWithFamily:(NSString *)desiredFamily traits:(NSFontTraitMask)desiredTraits weight:(int)desiredWeight size:(float)size
{

    if ([desiredFamily compare:@"-webkit-system-font" options:NSCaseInsensitiveSearch] == NSOrderedSame) {
        // We ignore italic for system font.
        return (desiredWeight >= 7) ? [NSFont boldSystemFontOfSize:size] : [NSFont systemFontOfSize:size];
    }

    NSFontManager *fontManager = [NSFontManager sharedFontManager];

    // Do a simple case insensitive search for a matching font family.
    // NSFontManager requires exact name matches.
    // This addresses the problem of matching arial to Arial, etc., but perhaps not all the issues.
    NSEnumerator *e = [[fontManager availableFontFamilies] objectEnumerator];
    NSString *availableFamily;
    while ((availableFamily = [e nextObject])) {
        if ([desiredFamily caseInsensitiveCompare:availableFamily] == NSOrderedSame)
            break;
    }

    if (!availableFamily) {
        // Match by PostScript name.
        NSEnumerator *availableFonts = [[fontManager availableFonts] objectEnumerator];
        NSString *availableFont;
        NSFont *nameMatchedFont = nil;
        NSFontTraitMask desiredTraitsForNameMatch = desiredTraits | (desiredWeight >= 7 ? NSBoldFontMask : 0);
        while ((availableFont = [availableFonts nextObject])) {
            if ([desiredFamily caseInsensitiveCompare:availableFont] == NSOrderedSame) {
                nameMatchedFont = [NSFont fontWithName:availableFont size:size];

                // Special case Osaka-Mono.  According to <rdar://problem/3999467>, we need to 
                // treat Osaka-Mono as fixed pitch.
                if ([desiredFamily caseInsensitiveCompare:@"Osaka-Mono"] == NSOrderedSame && desiredTraitsForNameMatch == 0)
                    return nameMatchedFont;

                NSFontTraitMask traits = [fontManager traitsOfFont:nameMatchedFont];
                if ((traits & desiredTraitsForNameMatch) == desiredTraitsForNameMatch)
                    return [fontManager convertFont:nameMatchedFont toHaveTrait:desiredTraitsForNameMatch];

                availableFamily = [nameMatchedFont familyName];
                break;
            }
        }
    }

    // Found a family, now figure out what weight and traits to use.
    BOOL choseFont = false;
    int chosenWeight = 0;
    NSFontTraitMask chosenTraits = 0;
    NSString *chosenFullName = 0;

    NSArray *fonts = [fontManager availableMembersOfFontFamily:availableFamily];    
    unsigned n = [fonts count];
    unsigned i;
    for (i = 0; i < n; i++) {
        NSArray *fontInfo = [fonts objectAtIndex:i];

        // Array indices must be hard coded because of lame AppKit API.
        NSString *fontFullName = [fontInfo objectAtIndex:0];
        NSInteger fontWeight = [[fontInfo objectAtIndex:2] intValue];
        NSFontTraitMask fontTraits = [[fontInfo objectAtIndex:3] unsignedIntValue];

        BOOL newWinner;
        if (!choseFont)
            newWinner = acceptableChoice(desiredTraits, fontTraits);
        else
            newWinner = betterChoice(desiredTraits, desiredWeight, chosenTraits, chosenWeight, fontTraits, fontWeight);

        if (newWinner) {
            choseFont = YES;
            chosenWeight = fontWeight;
            chosenTraits = fontTraits;
            chosenFullName = fontFullName;

            if (chosenWeight == desiredWeight && (chosenTraits & IMPORTANT_FONT_TRAITS) == (desiredTraits & IMPORTANT_FONT_TRAITS))
                break;
        }
    }

    if (!choseFont)
        return nil;

    NSFont *font = [NSFont fontWithName:chosenFullName size:size];

    if (!font)
        return nil;

    NSFontTraitMask actualTraits = 0;
    if (desiredTraits & NSFontItalicTrait)
        actualTraits = [fontManager traitsOfFont:font];
    int actualWeight = [fontManager weightOfFont:font];

    bool syntheticBold = desiredWeight >= 7 && actualWeight < 7;
    bool syntheticOblique = (desiredTraits & NSFontItalicTrait) && !(actualTraits & NSFontItalicTrait);

    // There are some malformed fonts that will be correctly returned by -fontWithFamily:traits:weight:size: as a match for a particular trait,
    // though -[NSFontManager traitsOfFont:] incorrectly claims the font does not have the specified trait. This could result in applying 
    // synthetic bold on top of an already-bold font, as reported in <http://bugs.webkit.org/show_bug.cgi?id=6146>. To work around this
    // problem, if we got an apparent exact match, but the requested traits aren't present in the matched font, we'll try to get a font from 
    // the same family without those traits (to apply the synthetic traits to later).
    NSFontTraitMask nonSyntheticTraits = desiredTraits;

    if (syntheticBold)
        nonSyntheticTraits &= ~NSBoldFontMask;

    if (syntheticOblique)
        nonSyntheticTraits &= ~NSItalicFontMask;

    if (nonSyntheticTraits != desiredTraits) {
        NSFont *fontWithoutSyntheticTraits = [fontManager fontWithFamily:availableFamily traits:nonSyntheticTraits weight:chosenWeight size:size];
        if (fontWithoutSyntheticTraits)
            font = fontWithoutSyntheticTraits;
    }

    return font;
}

+ (NSFont *)fontWithFamily:(NSString *)desiredFamily traits:(NSFontTraitMask)desiredTraits weight:(int)desiredWeight size:(float)size
{
    NSFont *font = [self internalFontWithFamily:desiredFamily traits:desiredTraits weight:desiredWeight size:size];
    if (font)
        return font;

    // Auto activate the font before looking for it a second time.
    // Ignore the result because we want to use our own algorithm to actually find the font.
    [NSFont fontWithName:desiredFamily size:size];

    return [self internalFontWithFamily:desiredFamily traits:desiredTraits weight:desiredWeight size:size];
}

#if !PLATFORM(IOS)
+ (NSFont *)fontWithFamily:(NSString *)desiredFamily traits:(NSFontTraitMask)desiredTraits size:(float)size
{
    int desiredWeight = (desiredTraits & NSBoldFontMask) ? 9 : 5;
    return [self fontWithFamily:desiredFamily traits:desiredTraits weight:desiredWeight size:size];
}
#endif

@end
