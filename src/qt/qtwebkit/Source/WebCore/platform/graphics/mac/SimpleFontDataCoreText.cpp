/*
 * Copyright (C) 2005, 2006, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SimpleFontData.h"

#include <ApplicationServices/ApplicationServices.h>

namespace WebCore {

CFDictionaryRef SimpleFontData::getCFStringAttributes(TypesettingFeatures typesettingFeatures, FontOrientation orientation) const
{
    unsigned key = typesettingFeatures + 1;
    HashMap<unsigned, RetainPtr<CFDictionaryRef> >::AddResult addResult = m_CFStringAttributes.add(key, RetainPtr<CFDictionaryRef>());
    RetainPtr<CFDictionaryRef>& attributesDictionary = addResult.iterator->value;
    if (!addResult.isNewEntry)
        return attributesDictionary.get();

    attributesDictionary = adoptCF(CFDictionaryCreateMutable(kCFAllocatorDefault, 4, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
    CFMutableDictionaryRef mutableAttributes = (CFMutableDictionaryRef)attributesDictionary.get();

    CFDictionarySetValue(mutableAttributes, kCTFontAttributeName, platformData().ctFont());

    if (!(typesettingFeatures & Kerning)) {
        const float zero = 0;
        static CFNumberRef zeroKerningValue = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloatType, &zero);
        CFDictionarySetValue(mutableAttributes, kCTKernAttributeName, zeroKerningValue);
    }

    bool allowLigatures = (orientation == Horizontal && platformData().allowsLigatures()) || (typesettingFeatures & Ligatures);
    if (!allowLigatures) {
        const int zero = 0;
        static CFNumberRef essentialLigaturesValue = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &zero);
        CFDictionarySetValue(mutableAttributes, kCTLigatureAttributeName, essentialLigaturesValue);
    }

    if (orientation == Vertical)
        CFDictionarySetValue(mutableAttributes, kCTVerticalFormsAttributeName, kCFBooleanTrue);

    return attributesDictionary.get();
}

} // namespace WebCore
