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
#include "DFGNodeFlags.h"

#if ENABLE(DFG_JIT)

#include <wtf/CommaPrinter.h>

namespace JSC { namespace DFG {

void dumpNodeFlags(PrintStream& out, NodeFlags flags)
{
    if (!(flags ^ NodeDoesNotExit)) {
        out.print("<empty>");
        return;
    }

    CommaPrinter comma("|");
    
    if (flags & NodeResultMask) {
        switch (flags & NodeResultMask) {
        case NodeResultJS:
            out.print(comma, "JS");
            break;
        case NodeResultNumber:
            out.print(comma, "Number");
            break;
        case NodeResultInt32:
            out.print(comma, "Int32");
            break;
        case NodeResultBoolean:
            out.print(comma, "Boolean");
            break;
        case NodeResultStorage:
            out.print(comma, "Storage");
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }
    
    if (flags & NodeMustGenerate)
        out.print(comma, "MustGen");
    
    if (flags & NodeHasVarArgs)
        out.print(comma, "VarArgs");
    
    if (flags & NodeClobbersWorld)
        out.print(comma, "Clobbers");
    
    if (flags & NodeMightClobber)
        out.print(comma, "MightClobber");
    
    if (flags & NodeResultMask) {
        if (!(flags & NodeUsedAsNumber) && !(flags & NodeNeedsNegZero))
            out.print(comma, "PureInt");
        else if (!(flags & NodeUsedAsNumber))
            out.print(comma, "PureInt(w/ neg zero)");
        else if (!(flags & NodeNeedsNegZero))
            out.print(comma, "PureNum");
        if (flags & NodeUsedAsOther)
            out.print(comma, "UseAsOther");
    }
    
    if (flags & NodeMayOverflow)
        out.print(comma, "MayOverflow");
    
    if (flags & NodeMayNegZero)
        out.print(comma, "MayNegZero");
    
    if (flags & NodeUsedAsInt)
        out.print(comma, "UseAsInt");
    
    if (!(flags & NodeDoesNotExit))
        out.print(comma, "CanExit");
    
    if (flags & NodeExitsForward)
        out.print(comma, "NodeExitsForward");
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

