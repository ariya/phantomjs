/*
 * Copyright (C) 2005, 2006, 2007 Apple Inc.  All rights reserved.
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
#include "StringTruncator.h"

#include "Font.h"
#include "TextBreakIterator.h"
#include "TextRun.h"
#include <wtf/Assertions.h>
#include <wtf/Vector.h>
#include <wtf/unicode/CharacterNames.h>

namespace WebCore {

#define STRING_BUFFER_SIZE 2048

typedef unsigned TruncationFunction(const String&, unsigned length, unsigned keepCount, UChar* buffer);

static inline int textBreakAtOrPreceding(TextBreakIterator* it, int offset)
{
    if (isTextBreak(it, offset))
        return offset;

    int result = textBreakPreceding(it, offset);
    return result == TextBreakDone ? 0 : result;
}

static inline int boundedTextBreakFollowing(TextBreakIterator* it, int offset, int length)
{
    int result = textBreakFollowing(it, offset);
    return result == TextBreakDone ? length : result;
}

static unsigned centerTruncateToBuffer(const String& string, unsigned length, unsigned keepCount, UChar* buffer)
{
    ASSERT(keepCount < length);
    ASSERT(keepCount < STRING_BUFFER_SIZE);
    
    unsigned omitStart = (keepCount + 1) / 2;
    NonSharedCharacterBreakIterator it(string.characters(), length);
    unsigned omitEnd = boundedTextBreakFollowing(it, omitStart + (length - keepCount) - 1, length);
    omitStart = textBreakAtOrPreceding(it, omitStart);
    
    unsigned truncatedLength = omitStart + 1 + (length - omitEnd);
    ASSERT(truncatedLength <= length);

    memcpy(buffer, string.characters(), sizeof(UChar) * omitStart);
    buffer[omitStart] = horizontalEllipsis;
    memcpy(&buffer[omitStart + 1], &string.characters()[omitEnd], sizeof(UChar) * (length - omitEnd));
    
    return truncatedLength;
}

static unsigned rightTruncateToBuffer(const String& string, unsigned length, unsigned keepCount, UChar* buffer)
{
    ASSERT(keepCount < length);
    ASSERT(keepCount < STRING_BUFFER_SIZE);
    
    NonSharedCharacterBreakIterator it(string.characters(), length);
    unsigned keepLength = textBreakAtOrPreceding(it, keepCount);
    unsigned truncatedLength = keepLength + 1;
    
    memcpy(buffer, string.characters(), sizeof(UChar) * keepLength);
    buffer[keepLength] = horizontalEllipsis;
    
    return truncatedLength;
}

static float stringWidth(const Font& renderer, const UChar* characters, unsigned length, bool disableRoundingHacks)
{
    TextRun run(characters, length);
    if (disableRoundingHacks)
        run.disableRoundingHacks();
    return renderer.width(run);
}

static String truncateString(const String& string, float maxWidth, const Font& font, TruncationFunction truncateToBuffer, bool disableRoundingHacks)
{
    if (string.isEmpty())
        return string;
    
    ASSERT(maxWidth >= 0);
    
    float currentEllipsisWidth = stringWidth(font, &horizontalEllipsis, 1, disableRoundingHacks);
    
    UChar stringBuffer[STRING_BUFFER_SIZE];
    unsigned truncatedLength;
    unsigned keepCount;
    unsigned length = string.length();

    if (length > STRING_BUFFER_SIZE) {
        keepCount = STRING_BUFFER_SIZE - 1; // need 1 character for the ellipsis
        truncatedLength = centerTruncateToBuffer(string, length, keepCount, stringBuffer);
    } else {
        keepCount = length;
        memcpy(stringBuffer, string.characters(), sizeof(UChar) * length);
        truncatedLength = length;
    }

    float width = stringWidth(font, stringBuffer, truncatedLength, disableRoundingHacks);
    if (width <= maxWidth)
        return string;

    unsigned keepCountForLargestKnownToFit = 0;
    float widthForLargestKnownToFit = currentEllipsisWidth;
    
    unsigned keepCountForSmallestKnownToNotFit = keepCount;
    float widthForSmallestKnownToNotFit = width;
    
    if (currentEllipsisWidth >= maxWidth) {
        keepCountForLargestKnownToFit = 1;
        keepCountForSmallestKnownToNotFit = 2;
    }
    
    while (keepCountForLargestKnownToFit + 1 < keepCountForSmallestKnownToNotFit) {
        ASSERT(widthForLargestKnownToFit <= maxWidth);
        ASSERT(widthForSmallestKnownToNotFit > maxWidth);

        float ratio = (keepCountForSmallestKnownToNotFit - keepCountForLargestKnownToFit)
            / (widthForSmallestKnownToNotFit - widthForLargestKnownToFit);
        keepCount = static_cast<unsigned>(maxWidth * ratio);
        
        if (keepCount <= keepCountForLargestKnownToFit) {
            keepCount = keepCountForLargestKnownToFit + 1;
        } else if (keepCount >= keepCountForSmallestKnownToNotFit) {
            keepCount = keepCountForSmallestKnownToNotFit - 1;
        }
        
        ASSERT(keepCount < length);
        ASSERT(keepCount > 0);
        ASSERT(keepCount < keepCountForSmallestKnownToNotFit);
        ASSERT(keepCount > keepCountForLargestKnownToFit);
        
        truncatedLength = truncateToBuffer(string, length, keepCount, stringBuffer);

        width = stringWidth(font, stringBuffer, truncatedLength, disableRoundingHacks);
        if (width <= maxWidth) {
            keepCountForLargestKnownToFit = keepCount;
            widthForLargestKnownToFit = width;
        } else {
            keepCountForSmallestKnownToNotFit = keepCount;
            widthForSmallestKnownToNotFit = width;
        }
    }
    
    if (keepCountForLargestKnownToFit == 0) {
        keepCountForLargestKnownToFit = 1;
    }
    
    if (keepCount != keepCountForLargestKnownToFit) {
        keepCount = keepCountForLargestKnownToFit;
        truncatedLength = truncateToBuffer(string, length, keepCount, stringBuffer);
    }
    
    return String(stringBuffer, truncatedLength);
}

String StringTruncator::centerTruncate(const String& string, float maxWidth, const Font& font, EnableRoundingHacksOrNot enableRoundingHacks)
{
    return truncateString(string, maxWidth, font, centerTruncateToBuffer, !enableRoundingHacks);
}

String StringTruncator::rightTruncate(const String& string, float maxWidth, const Font& font, EnableRoundingHacksOrNot enableRoundingHacks)
{
    return truncateString(string, maxWidth, font, rightTruncateToBuffer, !enableRoundingHacks);
}

float StringTruncator::width(const String& string, const Font& font, EnableRoundingHacksOrNot enableRoundingHacks)
{
    return stringWidth(font, string.characters(), string.length(), !enableRoundingHacks);
}

} // namespace WebCore
