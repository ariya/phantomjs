/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#ifndef CharacterReferenceParserInlines_h
#define CharacterReferenceParserInlines_h

#include <wtf/text/StringBuilder.h>

namespace WebCore {

inline bool isHexDigit(UChar cc)
{
    return (cc >= '0' && cc <= '9') || (cc >= 'a' && cc <= 'f') || (cc >= 'A' && cc <= 'F');
}

inline void unconsumeCharacters(SegmentedString& source, const StringBuilder& consumedCharacters)
{
    if (consumedCharacters.length() == 1)
        source.push(consumedCharacters[0]);
    else if (consumedCharacters.length() == 2) {
        source.push(consumedCharacters[0]);
        source.push(consumedCharacters[1]);
    } else
        source.prepend(SegmentedString(consumedCharacters.toStringPreserveCapacity()));
}

template <typename ParserFunctions>
bool consumeCharacterReference(SegmentedString& source, StringBuilder& decodedCharacter, bool& notEnoughCharacters, UChar additionalAllowedCharacter)
{
    ASSERT(!additionalAllowedCharacter || additionalAllowedCharacter == '"' || additionalAllowedCharacter == '\'' || additionalAllowedCharacter == '>');
    ASSERT(!notEnoughCharacters);
    ASSERT(decodedCharacter.isEmpty());
    
    enum EntityState {
        Initial,
        Number,
        MaybeHexLowerCaseX,
        MaybeHexUpperCaseX,
        Hex,
        Decimal,
        Named
    };
    EntityState entityState = Initial;
    UChar32 result = 0;
    StringBuilder consumedCharacters;
    
    while (!source.isEmpty()) {
        UChar cc = source.currentChar();
        switch (entityState) {
        case Initial: {
            if (cc == '\x09' || cc == '\x0A' || cc == '\x0C' || cc == ' ' || cc == '<' || cc == '&')
                return false;
            if (additionalAllowedCharacter && cc == additionalAllowedCharacter)
                return false;
            if (cc == '#') {
                entityState = Number;
                break;
            }
            if ((cc >= 'a' && cc <= 'z') || (cc >= 'A' && cc <= 'Z')) {
                entityState = Named;
                continue;
            }
            return false;
        }
        case Number: {
            if (cc == 'x') {
                entityState = MaybeHexLowerCaseX;
                break;
            }
            if (cc == 'X') {
                entityState = MaybeHexUpperCaseX;
                break;
            }
            if (cc >= '0' && cc <= '9') {
                entityState = Decimal;
                continue;
            }
            source.push('#');
            return false;
        }
        case MaybeHexLowerCaseX: {
            if (isHexDigit(cc)) {
                entityState = Hex;
                continue;
            }
            source.push('#');
            source.push('x');
            return false;
        }
        case MaybeHexUpperCaseX: {
            if (isHexDigit(cc)) {
                entityState = Hex;
                continue;
            }
            source.push('#');
            source.push('X');
            return false;
        }
        case Hex: {
            if (cc >= '0' && cc <= '9')
                result = result * 16 + cc - '0';
            else if (cc >= 'a' && cc <= 'f')
                result = result * 16 + 10 + cc - 'a';
            else if (cc >= 'A' && cc <= 'F')
                result = result * 16 + 10 + cc - 'A';
            else if (cc == ';') {
                source.advanceAndASSERT(cc);
                decodedCharacter.append(ParserFunctions::legalEntityFor(result));
                return true;
            } else if (ParserFunctions::acceptMalformed()) {
                decodedCharacter.append(ParserFunctions::legalEntityFor(result));
                return true;
            } else {
                unconsumeCharacters(source, consumedCharacters);
                return false;
            }
            break;
        }
        case Decimal: {
            if (cc >= '0' && cc <= '9')
                result = result * 10 + cc - '0';
            else if (cc == ';') {
                source.advanceAndASSERT(cc);
                decodedCharacter.append(ParserFunctions::legalEntityFor(result));
                return true;
            } else if (ParserFunctions::acceptMalformed()) {
                decodedCharacter.append(ParserFunctions::legalEntityFor(result));
                return true;
            } else {
                unconsumeCharacters(source, consumedCharacters);
                return false;
            }
            break;
        }
        case Named: {
            return ParserFunctions::consumeNamedEntity(source, decodedCharacter, notEnoughCharacters, additionalAllowedCharacter, cc);
        }
        }
        consumedCharacters.append(cc);
        source.advanceAndASSERT(cc);
    }
    ASSERT(source.isEmpty());
    notEnoughCharacters = true;
    unconsumeCharacters(source, consumedCharacters);
    return false;
}

}

#endif // CharacterReferenceParserInlines_h
