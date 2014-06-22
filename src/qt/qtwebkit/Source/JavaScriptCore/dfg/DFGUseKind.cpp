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

#include "config.h"
#include "DFGUseKind.h"

#if ENABLE(DFG_JIT)

namespace WTF {

using namespace JSC::DFG;

void printInternal(PrintStream& out, UseKind useKind)
{
    switch (useKind) {
    case UntypedUse:
        out.print("Untyped");
        break;
    case Int32Use:
        out.print("Int32");
        break;
    case KnownInt32Use:
        out.print("KnownInt32");
        break;
    case RealNumberUse:
        out.print("RealNumber");
        break;
    case NumberUse:
        out.print("Number");
        break;
    case KnownNumberUse:
        out.print("KnownNumber");
        break;
    case BooleanUse:
        out.print("Boolean");
        break;
    case CellUse:
        out.print("Cell");
        break;
    case KnownCellUse:
        out.print("KnownCell");
        break;
    case ObjectUse:
        out.print("Object");
        break;
    case ObjectOrOtherUse:
        out.print("ObjectOrOther");
        break;
    case StringUse:
        out.print("String");
        break;
    case KnownStringUse:
        out.print("KnownString");
        break;
    case StringObjectUse:
        out.print("StringObject");
        break;
    case StringOrStringObjectUse:
        out.print("StringOrStringObject");
        break;
    case NotCellUse:
        out.print("NotCell");
        break;
    case OtherUse:
        out.print("Other");
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
}

} // namespace WTF

#endif // ENABLE(DFG_JIT)

