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
#import "SimpleFontData.h"

#import "Font.h"
#import "FontCache.h"
#import "FontDescription.h"
#import <ApplicationServices/ApplicationServices.h>

using namespace std;

namespace WebCore {

CFDictionaryRef SimpleFontData::getCFStringAttributes(TypesettingFeatures typesettingFeatures, FontOrientation orientation) const
{
    unsigned key = typesettingFeatures + 1;
    pair<HashMap<unsigned, RetainPtr<CFDictionaryRef> >::iterator, bool> addResult = m_CFStringAttributes.add(key, RetainPtr<CFDictionaryRef>());
    RetainPtr<CFDictionaryRef>& attributesDictionary = addResult.first->second;
    if (!addResult.second)
        return attributesDictionary.get();
    
    bool treatLineAsVertical = orientation == Vertical;

    bool allowLigatures = (!treatLineAsVertical && platformData().allowsLigatures()) || (typesettingFeatures & Ligatures);

    static const int ligaturesNotAllowedValue = 0;
    static CFNumberRef ligaturesNotAllowed = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &ligaturesNotAllowedValue);

    static const int ligaturesAllowedValue = 1;
    static CFNumberRef ligaturesAllowed = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &ligaturesAllowedValue);

    if (!(typesettingFeatures & Kerning)) {
        static const float kerningAdjustmentValue = 0;
        static CFNumberRef kerningAdjustment = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloatType, &kerningAdjustmentValue);
        static const void* keysWithKerningDisabled[] = { kCTFontAttributeName, kCTKernAttributeName, kCTLigatureAttributeName, kCTVerticalFormsAttributeName };
        const void* valuesWithKerningDisabled[] = { platformData().ctFont(), kerningAdjustment, allowLigatures
            ? ligaturesAllowed : ligaturesNotAllowed, treatLineAsVertical ? kCFBooleanTrue : kCFBooleanFalse };
        attributesDictionary.adoptCF(CFDictionaryCreate(0, keysWithKerningDisabled, valuesWithKerningDisabled,
            WTF_ARRAY_LENGTH(keysWithKerningDisabled), &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
    } else {
        // By omitting the kCTKernAttributeName attribute, we get Core Text's standard kerning.
        static const void* keysWithKerningEnabled[] = { kCTFontAttributeName, kCTLigatureAttributeName, kCTVerticalFormsAttributeName };
        const void* valuesWithKerningEnabled[] = { platformData().ctFont(), allowLigatures ? ligaturesAllowed : ligaturesNotAllowed, treatLineAsVertical ? kCFBooleanTrue : kCFBooleanFalse };
        attributesDictionary.adoptCF(CFDictionaryCreate(0, keysWithKerningEnabled, valuesWithKerningEnabled,
            WTF_ARRAY_LENGTH(keysWithKerningEnabled), &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
    }

    return attributesDictionary.get();
}

} // namespace WebCore
