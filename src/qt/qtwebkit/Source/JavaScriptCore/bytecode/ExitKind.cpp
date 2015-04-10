/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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
#include "ExitKind.h"

#include <wtf/Assertions.h>
#include <wtf/PrintStream.h>

namespace JSC {

const char* exitKindToString(ExitKind kind)
{
    switch (kind) {
    case ExitKindUnset:
        return "Unset";
    case BadType:
        return "BadType";
    case BadFunction:
        return "BadFunction";
    case BadExecutable:
        return "BadExecutable";
    case BadCache:
        return "BadCache";
    case BadWeakConstantCache:
        return "BadWeakConstantCache";
    case BadIndexingType:
        return "BadIndexingType";
    case Overflow:
        return "Overflow";
    case NegativeZero:
        return "NegativeZero";
    case StoreToHole:
        return "StoreToHole";
    case LoadFromHole:
        return "LoadFromHole";
    case OutOfBounds:
        return "OutOfBounds";
    case StoreToHoleOrOutOfBounds:
        return "StoreToHoleOrOutOfBounds";
    case InadequateCoverage:
        return "InadequateCoverage";
    case ArgumentsEscaped:
        return "ArgumentsEscaped";
    case NotStringObject:
        return "NotStringObject";
    case Uncountable:
        return "Uncountable";
    case UncountableWatchpoint:
        return "UncountableWatchpoint";
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return "Unknown";
    }
}

bool exitKindIsCountable(ExitKind kind)
{
    switch (kind) {
    case ExitKindUnset:
        RELEASE_ASSERT_NOT_REACHED();
    case BadType:
    case Uncountable:
    case UncountableWatchpoint:
    case LoadFromHole: // Already counted directly by the baseline JIT.
    case StoreToHole: // Already counted directly by the baseline JIT.
    case OutOfBounds: // Already counted directly by the baseline JIT.
    case StoreToHoleOrOutOfBounds: // Already counted directly by the baseline JIT.
        return false;
    default:
        return true;
    }
}

} // namespace JSC

namespace WTF {

void printInternal(PrintStream& out, JSC::ExitKind kind)
{
    out.print(exitKindToString(kind));
}

} // namespace WTF

