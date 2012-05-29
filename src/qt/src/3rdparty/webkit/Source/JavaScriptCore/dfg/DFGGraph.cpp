/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "DFGGraph.h"

#include "CodeBlock.h"

#if ENABLE(DFG_JIT)

namespace JSC { namespace DFG {

#ifndef NDEBUG

// Creates an array of stringized names.
static const char* dfgOpNames[] = {
#define STRINGIZE_DFG_OP_ENUM(opcode, flags) #opcode ,
    FOR_EACH_DFG_OP(STRINGIZE_DFG_OP_ENUM)
#undef STRINGIZE_DFG_OP_ENUM
};

void Graph::dump(NodeIndex nodeIndex, CodeBlock* codeBlock)
{
    Node& node = at(nodeIndex);
    NodeType op = node.op;

    unsigned refCount = node.refCount();
    if (!refCount)
        return;
    bool mustGenerate = node.mustGenerate();
    if (mustGenerate)
        --refCount;

    // Example/explanation of dataflow dump output
    //
    //   14:   <!2:7>  GetByVal(@3, @13)
    //   ^1     ^2 ^3     ^4       ^5
    //
    // (1) The nodeIndex of this operation.
    // (2) The reference count. The number printed is the 'real' count,
    //     not including the 'mustGenerate' ref. If the node is
    //     'mustGenerate' then the count it prefixed with '!'.
    // (3) The virtual register slot assigned to this node.
    // (4) The name of the operation.
    // (5) The arguments to the operation. The may be of the form:
    //         @#   - a NodeIndex referencing a prior node in the graph.
    //         arg# - an argument number.
    //         $#   - the index in the CodeBlock of a constant { for numeric constants the value is displayed | for integers, in both decimal and hex }.
    //         id#  - the index in the CodeBlock of an identifier { if codeBlock is passed to dump(), the string representation is displayed }.
    //         var# - the index of a var on the global object, used by GetGlobalVar/PutGlobalVar operations.
    printf("% 4d:\t<%c%u:", (int)nodeIndex, mustGenerate ? '!' : ' ', refCount);
    if (node.hasResult())
        printf("%u", node.virtualRegister());
    else
        printf("-");
    printf(">\t%s(", dfgOpNames[op & NodeIdMask]);
    if (node.child1 != NoNode)
        printf("@%u", node.child1);
    if (node.child2 != NoNode)
        printf(", @%u", node.child2);
    if (node.child3 != NoNode)
        printf(", @%u", node.child3);
    bool hasPrinted = node.child1 != NoNode;

    if (node.hasVarNumber()) {
        printf("%svar%u", hasPrinted ? ", " : "", node.varNumber());
        hasPrinted = true;
    }
    if (node.hasIdentifier()) {
        if (codeBlock)
            printf("%sid%u{%s}", hasPrinted ? ", " : "", node.identifierNumber(), codeBlock->identifier(node.identifierNumber()).ustring().utf8().data());
        else
            printf("%sid%u", hasPrinted ? ", " : "", node.identifierNumber());
        hasPrinted = true;
    }
    if (node.hasLocal()) {
        int local = node.local();
        if (operandIsArgument(local))
            printf("%sarg%u", hasPrinted ? ", " : "", local - codeBlock->thisRegister());
        else
            printf("%sr%u", hasPrinted ? ", " : "", local);
        hasPrinted = true;
    }
    if (op == Int32Constant) {
        printf("%s$%u{%d|0x%08x}", hasPrinted ? ", " : "", node.constantNumber(), node.int32Constant(), node.int32Constant());
        hasPrinted = true;
    }
    if (op == DoubleConstant) {
        printf("%s$%u{%f})", hasPrinted ? ", " : "", node.constantNumber(), node.numericConstant());
        hasPrinted = true;
    }
    if (op == JSConstant) {
        printf("%s$%u", hasPrinted ? ", " : "", node.constantNumber());
        hasPrinted = true;
    }
    if  (node.isBranch() || node.isJump()) {
        printf("%sT:#%u", hasPrinted ? ", " : "", blockIndexForBytecodeOffset(node.takenBytecodeOffset()));
        hasPrinted = true;
    }
    if  (node.isBranch()) {
        printf("%sF:#%u", hasPrinted ? ", " : "", blockIndexForBytecodeOffset(node.notTakenBytecodeOffset()));
        hasPrinted = true;
    }

    printf(")\n");
}

void Graph::dump(CodeBlock* codeBlock)
{
    for (size_t b = 0; b < m_blocks.size(); ++b) {
        printf("Block #%u:\n", (int)b);
        for (size_t i = m_blocks[b]->begin; i < m_blocks[b]->end; ++i)
            dump(i, codeBlock);
    }
    printf("Phi Nodes:\n");
    for (size_t i = m_blocks.last()->end; i < size(); ++i)
        dump(i, codeBlock);
}

#endif

// FIXME: Convert this method to be iterative, not recursive.
void Graph::refChildren(NodeIndex op)
{
    Node& node = at(op);

    if (node.child1 == NoNode) {
        ASSERT(node.child2 == NoNode && node.child3 == NoNode);
        return;
    }
    ref(node.child1);

    if (node.child2 == NoNode) {
        ASSERT(node.child3 == NoNode);
        return;
    }
    ref(node.child2);

    if (node.child3 == NoNode)
        return;
    ref(node.child3);
}

} } // namespace JSC::DFG

#endif
