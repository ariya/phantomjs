/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "DFGVariableEvent.h"

#if ENABLE(DFG_JIT)

#include "DFGFPRInfo.h"
#include "DFGGPRInfo.h"

namespace JSC { namespace DFG {

void VariableEvent::dump(PrintStream& out) const
{
    switch (kind()) {
    case Reset:
        out.printf("Reset");
        break;
    case BirthToFill:
        dumpFillInfo("BirthToFill", out);
        break;
    case BirthToSpill:
        dumpSpillInfo("BirthToSpill", out);
        break;
    case Fill:
        dumpFillInfo("Fill", out);
        break;
    case Spill:
        dumpSpillInfo("Spill", out);
        break;
    case Death:
        out.print("Death(", id(), ")");
        break;
    case MovHintEvent:
        out.print("MovHint(", id(), ", r", operand(), ")");
        break;
    case SetLocalEvent:
        out.printf("SetLocal(r%d, %s)", operand(), dataFormatToString(dataFormat()));
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
}

void VariableEvent::dumpFillInfo(const char* name, PrintStream& out) const
{
    out.print(name, "(", id(), ", ");
    if (dataFormat() == DataFormatDouble)
        out.printf("%s", FPRInfo::debugName(fpr()));
#if USE(JSVALUE32_64)
    else if (dataFormat() & DataFormatJS)
        out.printf("%s:%s", GPRInfo::debugName(tagGPR()), GPRInfo::debugName(payloadGPR()));
#endif
    else
        out.printf("%s", GPRInfo::debugName(gpr()));
    out.printf(", %s)", dataFormatToString(dataFormat()));
}

void VariableEvent::dumpSpillInfo(const char* name, PrintStream& out) const
{
    out.print(name, "(", id(), ", r", virtualRegister(), ", ", dataFormatToString(dataFormat()), ")");
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

