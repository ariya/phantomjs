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
#include "DFGMinifiedNode.h"

#if ENABLE(DFG_JIT)

#include "DFGNode.h"

namespace JSC { namespace DFG {

MinifiedNode MinifiedNode::fromNode(Node* node)
{
    ASSERT(belongsInMinifiedGraph(node->op()));
    MinifiedNode result;
    result.m_id = MinifiedID(node);
    result.m_op = node->op();
    if (hasChild(node->op()))
        result.m_childOrInfo = MinifiedID(node->child1().node()).m_id;
    else if (hasConstantNumber(node->op()))
        result.m_childOrInfo = node->constantNumber();
    else if (hasWeakConstant(node->op()))
        result.m_childOrInfo = bitwise_cast<uintptr_t>(node->weakConstant());
    else {
        ASSERT(node->op() == PhantomArguments);
        result.m_childOrInfo = 0;
    }
    return result;
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

