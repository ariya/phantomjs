/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef DFGUseKind_h
#define DFGUseKind_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "SpeculatedType.h"
#include <wtf/PrintStream.h>

namespace JSC { namespace DFG {

enum UseKind {
    UntypedUse,
    Int32Use,
    KnownInt32Use,
    RealNumberUse,
    NumberUse,
    KnownNumberUse,
    BooleanUse,
    CellUse,
    KnownCellUse,
    ObjectUse,
    ObjectOrOtherUse,
    StringUse,
    KnownStringUse,
    StringObjectUse,
    StringOrStringObjectUse,
    NotCellUse,
    OtherUse,
    LastUseKind // Must always be the last entry in the enum, as it is used to denote the number of enum elements.
};

ALWAYS_INLINE SpeculatedType typeFilterFor(UseKind useKind)
{
    switch (useKind) {
    case UntypedUse:
        return SpecEmptyOrTop; // TOP isn't good enough; untyped uses may use the normally unseen empty value, in the case of lazy registers.
    case Int32Use:
    case KnownInt32Use:
        return SpecInt32;
    case RealNumberUse:
        return SpecRealNumber;
    case NumberUse:
    case KnownNumberUse:
        return SpecNumber;
    case BooleanUse:
        return SpecBoolean;
    case CellUse:
    case KnownCellUse:
        return SpecCell;
    case ObjectUse:
        return SpecObject;
    case ObjectOrOtherUse:
        return SpecObject | SpecOther;
    case StringUse:
    case KnownStringUse:
        return SpecString;
    case StringObjectUse:
        return SpecStringObject;
    case StringOrStringObjectUse:
        return SpecString | SpecStringObject;
    case NotCellUse:
        return ~SpecCell;
    case OtherUse:
        return SpecOther;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return SpecTop;
    }
}

ALWAYS_INLINE bool isNumerical(UseKind kind)
{
    switch (kind) {
    case Int32Use:
    case KnownInt32Use:
    case RealNumberUse:
    case NumberUse:
        return true;
    default:
        return false;
    }
}

} } // namespace JSC::DFG

namespace WTF {

void printInternal(PrintStream&, JSC::DFG::UseKind);

} // namespace WTF

#endif // ENABLE(DFG_JIT)

#endif // DFGUseKind_h

