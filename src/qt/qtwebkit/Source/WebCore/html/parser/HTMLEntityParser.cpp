/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Torch Mobile, Inc. http://www.torchmobile.com/
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "HTMLEntityParser.h"

#include "CharacterReferenceParserInlines.h"
#include "HTMLEntitySearch.h"
#include "HTMLEntityTable.h"
#include <wtf/text/StringBuilder.h>

using namespace WTF;

namespace WebCore {

static const UChar windowsLatin1ExtensionArray[32] = {
    0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, // 80-87
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F, // 88-8F
    0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, // 90-97
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178, // 98-9F
};

static inline bool isAlphaNumeric(UChar cc)
{
    return (cc >= '0' && cc <= '9') || (cc >= 'a' && cc <= 'z') || (cc >= 'A' && cc <= 'Z');
}

class HTMLEntityParser {
public:
    inline static UChar adjustEntity(UChar32 value)
    {
        if ((value & ~0x1F) != 0x0080)
            return value;
        return windowsLatin1ExtensionArray[value - 0x80];
    }

    inline static UChar32 legalEntityFor(UChar32 value)
    {
        // FIXME: A number of specific entity values generate parse errors.
        if (!value || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF))
            return 0xFFFD;
        if (U_IS_BMP(value))
            return adjustEntity(value);
        return value;
    }

    inline static bool acceptMalformed() { return true; }

    inline static bool consumeNamedEntity(SegmentedString& source, StringBuilder& decodedEntity, bool& notEnoughCharacters, UChar additionalAllowedCharacter, UChar& cc)
    {
        StringBuilder consumedCharacters;
        HTMLEntitySearch entitySearch;
        while (!source.isEmpty()) {
            cc = source.currentChar();
            entitySearch.advance(cc);
            if (!entitySearch.isEntityPrefix())
                break;
            consumedCharacters.append(cc);
            source.advanceAndASSERT(cc);
        }
        notEnoughCharacters = source.isEmpty();
        if (notEnoughCharacters) {
            // We can't an entity because there might be a longer entity
            // that we could match if we had more data.
            unconsumeCharacters(source, consumedCharacters);
            return false;
        }
        if (!entitySearch.mostRecentMatch()) {
            unconsumeCharacters(source, consumedCharacters);
            return false;
        }
        if (entitySearch.mostRecentMatch()->length != entitySearch.currentLength()) {
            // We've consumed too many characters. We need to walk the
            // source back to the point at which we had consumed an
            // actual entity.
            unconsumeCharacters(source, consumedCharacters);
            consumedCharacters.clear();
            const int length = entitySearch.mostRecentMatch()->length;
            const UChar* reference = entitySearch.mostRecentMatch()->entity;
            for (int i = 0; i < length; ++i) {
                cc = source.currentChar();
                ASSERT_UNUSED(reference, cc == *reference++);
                consumedCharacters.append(cc);
                source.advanceAndASSERT(cc);
                ASSERT(!source.isEmpty());
            }
            cc = source.currentChar();
        }
        if (entitySearch.mostRecentMatch()->lastCharacter() == ';'
            || !additionalAllowedCharacter
            || !(isAlphaNumeric(cc) || cc == '=')) {
            decodedEntity.append(entitySearch.mostRecentMatch()->firstValue);
            if (entitySearch.mostRecentMatch()->secondValue)
                decodedEntity.append(entitySearch.mostRecentMatch()->secondValue);
            return true;
        }
        unconsumeCharacters(source, consumedCharacters);
        return false;
    }
};

bool consumeHTMLEntity(SegmentedString& source, StringBuilder& decodedEntity, bool& notEnoughCharacters, UChar additionalAllowedCharacter)
{
    return consumeCharacterReference<HTMLEntityParser>(source, decodedEntity, notEnoughCharacters, additionalAllowedCharacter);
}

static size_t appendUChar32ToUCharArray(UChar32 value, UChar* result)
{
    if (U_IS_BMP(value)) {
        UChar character = static_cast<UChar>(value);
        ASSERT(character == value);
        result[0] = character;
        return 1;
    }

    result[0] = U16_LEAD(value);
    result[1] = U16_TRAIL(value);
    return 2;
}

size_t decodeNamedEntityToUCharArray(const char* name, UChar result[4])
{
    HTMLEntitySearch search;
    while (*name) {
        search.advance(*name++);
        if (!search.isEntityPrefix())
            return 0;
    }
    search.advance(';');
    if (!search.isEntityPrefix())
        return 0;

    size_t numberOfCodePoints = appendUChar32ToUCharArray(search.mostRecentMatch()->firstValue, result);
    if (!search.mostRecentMatch()->secondValue)
        return numberOfCodePoints;
    return numberOfCodePoints + appendUChar32ToUCharArray(search.mostRecentMatch()->secondValue, result + numberOfCodePoints);
}

} // namespace WebCore
