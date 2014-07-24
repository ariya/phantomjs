/*
 * Copyright (C) 2012 Patrick Gansterer <paroga@paroga.com>
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
#include "UnicodeWchar.h"

#include <algorithm>

namespace WTF {
namespace Unicode {

CharCategory category(UChar32)
{
    return NoCategory; // FIXME: implement!
}

unsigned char combiningClass(UChar32)
{
    return 0; // FIXME: implement!
}

Direction direction(UChar32)
{
    return LeftToRight; // FIXME: implement!
}

DecompositionType decompositionType(UChar32)
{
    return DecompositionNone; // FIXME: implement!
}

bool hasLineBreakingPropertyComplexContext(UChar32)
{
    return false; // FIXME: implement!
}

UChar32 mirroredChar(UChar32 c)
{
    return c; // FIXME: implement!
}

template<UChar Function(UChar)>
static inline int convertWithFunction(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError)
{
    UChar* resultIterator = result;
    UChar* resultEnd = result + std::min(resultLength, sourceLength);
    while (resultIterator < resultEnd)
        *resultIterator++ = Function(*source++);

    if (sourceLength < resultLength)
        *resultIterator = '\0';

    *isError = sourceLength > resultLength;
    return sourceLength;
}

int foldCase(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError)
{
    return convertWithFunction<foldCase>(result, resultLength, source, sourceLength, isError);
}

int toLower(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError)
{
    return convertWithFunction<toLower>(result, resultLength, source, sourceLength, isError);
}

int toUpper(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError)
{
    return convertWithFunction<toUpper>(result, resultLength, source, sourceLength, isError);
}

} // namespace Unicode
} // namespace WTF
