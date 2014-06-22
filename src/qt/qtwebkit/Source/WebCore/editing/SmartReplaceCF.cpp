/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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

#include "config.h"
#include "SmartReplace.h"

#include <CoreFoundation/CFCharacterSet.h>
#include <CoreFoundation/CFString.h>

namespace WebCore {

static CFMutableCharacterSetRef getSmartSet(bool isPreviousCharacter)
{
    static CFMutableCharacterSetRef preSmartSet = NULL; 
    static CFMutableCharacterSetRef postSmartSet = NULL;
    CFMutableCharacterSetRef smartSet = isPreviousCharacter ? preSmartSet : postSmartSet;
    if (!smartSet) {
        smartSet = CFCharacterSetCreateMutable(kCFAllocatorDefault);
        CFCharacterSetAddCharactersInString(smartSet, isPreviousCharacter ? CFSTR("([\"\'#$/-`{") : CFSTR(")].,;:?\'!\"%*-/}"));
        CFCharacterSetUnion(smartSet, CFCharacterSetGetPredefined(kCFCharacterSetWhitespaceAndNewline));        
        // Adding CJK ranges
        CFCharacterSetAddCharactersInRange(smartSet, CFRangeMake(0x1100, 256)); // Hangul Jamo (0x1100 - 0x11FF)
        CFCharacterSetAddCharactersInRange(smartSet, CFRangeMake(0x2E80, 352)); // CJK & Kangxi Radicals (0x2E80 - 0x2FDF)
        CFCharacterSetAddCharactersInRange(smartSet, CFRangeMake(0x2FF0, 464)); // Ideograph Descriptions, CJK Symbols, Hiragana, Katakana, Bopomofo, Hangul Compatibility Jamo, Kanbun, & Bopomofo Ext (0x2FF0 - 0x31BF)
        CFCharacterSetAddCharactersInRange(smartSet, CFRangeMake(0x3200, 29392)); // Enclosed CJK, CJK Ideographs (Uni Han & Ext A), & Yi (0x3200 - 0xA4CF)
        CFCharacterSetAddCharactersInRange(smartSet, CFRangeMake(0xAC00, 11183)); // Hangul Syllables (0xAC00 - 0xD7AF)
        CFCharacterSetAddCharactersInRange(smartSet, CFRangeMake(0xF900, 352)); // CJK Compatibility Ideographs (0xF900 - 0xFA5F)
        CFCharacterSetAddCharactersInRange(smartSet, CFRangeMake(0xFE30, 32)); // CJK Compatibility From (0xFE30 - 0xFE4F)
        CFCharacterSetAddCharactersInRange(smartSet, CFRangeMake(0xFF00, 240)); // Half/Full Width Form (0xFF00 - 0xFFEF)
        CFCharacterSetAddCharactersInRange(smartSet, CFRangeMake(0x20000, 0xA6D7)); // CJK Ideograph Exntension B
        CFCharacterSetAddCharactersInRange(smartSet, CFRangeMake(0x2F800, 0x021E)); // CJK Compatibility Ideographs (0x2F800 - 0x2FA1D)

        if (isPreviousCharacter)
            preSmartSet = smartSet;
        else {
            CFCharacterSetUnion(smartSet, CFCharacterSetGetPredefined(kCFCharacterSetPunctuation));       
            postSmartSet = smartSet;
        }
    }
    return smartSet;
}

bool isCharacterSmartReplaceExempt(UChar32 c, bool isPreviousCharacter)
{
    return CFCharacterSetIsLongCharacterMember(getSmartSet(isPreviousCharacter), c);
}

}
