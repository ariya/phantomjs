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
#include "CodeBlockWithJITType.h"
#include "DFGVariableAccessDataDump.h"
#include "FunctionExecutableDump.h"
#include "Operations.h"
#include <wtf/CommaPrinter.h>

#if ENABLE(DFG_JIT)

namespace JSC { namespace DFG {

// Creates an array of stringized names.
static const char* dfgOpNames[] = {
#define STRINGIZE_DFG_OP_ENUM(opcode, flags) #opcode ,
    FOR_EACH_DFG_OP(STRINGIZE_DFG_OP_ENUM)
#undef STRINGIZE_DFG_OP_ENUM
};

Graph::Graph(VM& vm, CodeBlock* codeBlock, unsigned osrEntryBytecodeIndex, const Operands<JSValue>& mustHandleValues)
    : m_vm(vm)
    , m_codeBlock(codeBlock)
    , m_compilation(vm.m_perBytecodeProfiler ? vm.m_perBytecodeProfiler->newCompilation(codeBlock, Profiler::DFG) : 0)
    , m_profiledBlock(codeBlock->alternative())
    , m_allocator(vm.m_dfgState->m_allocator)
    , m_hasArguments(false)
    , m_osrEntryBytecodeIndex(osrEntryBytecodeIndex)
    , m_mustHandleValues(mustHandleValues)
    , m_fixpointState(BeforeFixpoint)
    , m_form(LoadStore)
    , m_unificationState(LocallyUnified)
    , m_refCountState(EverythingIsLive)
{
    ASSERT(m_profiledBlock);
}

Graph::~Graph()
{
    m_allocator.freeAll();
}

const char *Graph::opName(NodeType op)
{
    return dfgOpNames[op];
}

static void printWhiteSpace(PrintStream& out, unsigned amount)
{
    while (amount-- > 0)
        out.print(" ");
}

bool Graph::dumpCodeOrigin(PrintStream& out, const char* prefix, Node* previousNode, Node* currentNode)
{
    if (!previousNode)
        return false;
    
    if (previousNode->codeOrigin.inlineCallFrame == currentNode->codeOrigin.inlineCallFrame)
        return false;
    
    Vector<CodeOrigin> previousInlineStack = previousNode->codeOrigin.inlineStack();
    Vector<CodeOrigin> currentInlineStack = currentNode->codeOrigin.inlineStack();
    unsigned commonSize = std::min(previousInlineStack.size(), currentInlineStack.size());
    unsigned indexOfDivergence = commonSize;
    for (unsigned i = 0; i < commonSize; ++i) {
        if (previousInlineStack[i].inlineCallFrame != currentInlineStack[i].inlineCallFrame) {
            indexOfDivergence = i;
            break;
        }
    }
    
    bool hasPrinted = false;
    
    // Print the pops.
    for (unsigned i = previousInlineStack.size(); i-- > indexOfDivergence;) {
        out.print(prefix);
        printWhiteSpace(out, i * 2);
        out.print("<-- ", *previousInlineStack[i].inlineCallFrame, "\n");
        hasPrinted = true;
    }
    
    // Print the pushes.
    for (unsigned i = indexOfDivergence; i < currentInlineStack.size(); ++i) {
        out.print(prefix);
        printWhiteSpace(out, i * 2);
        out.print("--> ", *currentInlineStack[i].inlineCallFrame, "\n");
        hasPrinted = true;
    }
    
    return hasPrinted;
}

int Graph::amountOfNodeWhiteSpace(Node* node)
{
    return (node->codeOrigin.inlineDepth() - 1) * 2;
}

void Graph::printNodeWhiteSpace(PrintStream& out, Node* node)
{
    printWhiteSpace(out, amountOfNodeWhiteSpace(node));
}

void Graph::dump(PrintStream& out, const char* prefix, Node* node)
{
    NodeType op = node->op();

    unsigned refCount = node->refCount();
    bool skipped = !refCount;
    bool mustGenerate = node->mustGenerate();
    if (mustGenerate)
        --refCount;

    out.print(prefix);
    printNodeWhiteSpace(out, node);

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
    out.printf("% 4d:%s<%c%u:", (int)node->index(), skipped ? "  skipped  " : "           ", mustGenerate ? '!' : ' ', refCount);
    if (node->hasResult() && !skipped && node->hasVirtualRegister())
        out.print(node->virtualRegister());
    else
        out.print("-");
    out.print(">\t", opName(op), "(");
    CommaPrinter comma;
    if (node->flags() & NodeHasVarArgs) {
        for (unsigned childIdx = node->firstChild(); childIdx < node->firstChild() + node->numChildren(); childIdx++) {
            if (!m_varArgChildren[childIdx])
                continue;
            out.print(comma, m_varArgChildren[childIdx]);
        }
    } else {
        if (!!node->child1() || !!node->child2() || !!node->child3())
            out.print(comma, node->child1());
        if (!!node->child2() || !!node->child3())
            out.print(comma, node->child2());
        if (!!node->child3())
            out.print(comma, node->child3());
    }

    if (toCString(NodeFlagsDump(node->flags())) != "<empty>")
        out.print(comma, NodeFlagsDump(node->flags()));
    if (node->hasArrayMode())
        out.print(comma, node->arrayMode());
    if (node->hasVarNumber())
        out.print(comma, node->varNumber());
    if (node->hasRegisterPointer())
        out.print(comma, "global", globalObjectFor(node->codeOrigin)->findRegisterIndex(node->registerPointer()), "(", RawPointer(node->registerPointer()), ")");
    if (node->hasIdentifier())
        out.print(comma, "id", node->identifierNumber(), "{", m_codeBlock->identifier(node->identifierNumber()).string(), "}");
    if (node->hasStructureSet()) {
        for (size_t i = 0; i < node->structureSet().size(); ++i)
            out.print(comma, "struct(", RawPointer(node->structureSet()[i]), ": ", IndexingTypeDump(node->structureSet()[i]->indexingType()), ")");
    }
    if (node->hasStructure())
        out.print(comma, "struct(", RawPointer(node->structure()), ": ", IndexingTypeDump(node->structure()->indexingType()), ")");
    if (node->hasStructureTransitionData())
        out.print(comma, "struct(", RawPointer(node->structureTransitionData().previousStructure), " -> ", RawPointer(node->structureTransitionData().newStructure), ")");
    if (node->hasFunction()) {
        out.print(comma, "function(", RawPointer(node->function()), ", ");
        if (node->function()->inherits(&JSFunction::s_info)) {
            JSFunction* function = jsCast<JSFunction*>(node->function());
            if (function->isHostFunction())
                out.print("<host function>");
            else
                out.print(FunctionExecutableDump(function->jsExecutable()));
        } else
            out.print("<not JSFunction>");
        out.print(")");
    }
    if (node->hasExecutable()) {
        if (node->executable()->inherits(&FunctionExecutable::s_info))
            out.print(comma, "executable(", FunctionExecutableDump(jsCast<FunctionExecutable*>(node->executable())), ")");
        else
            out.print(comma, "executable(not function: ", RawPointer(node->executable()), ")");
    }
    if (node->hasFunctionDeclIndex()) {
        FunctionExecutable* executable = m_codeBlock->functionDecl(node->functionDeclIndex());
        out.print(comma, executable->inferredName().string(), "#", executable->hashFor(CodeForCall));
    }
    if (node->hasFunctionExprIndex()) {
        FunctionExecutable* executable = m_codeBlock->functionExpr(node->functionExprIndex());
        out.print(comma, executable->inferredName().string(), "#", executable->hashFor(CodeForCall));
    }
    if (node->hasStorageAccessData()) {
        StorageAccessData& storageAccessData = m_storageAccessData[node->storageAccessDataIndex()];
        out.print(comma, "id", storageAccessData.identifierNumber, "{", m_codeBlock->identifier(storageAccessData.identifierNumber).string(), "}");
        out.print(", ", static_cast<ptrdiff_t>(storageAccessData.offset));
    }
    ASSERT(node->hasVariableAccessData() == node->hasLocal());
    if (node->hasVariableAccessData()) {
        VariableAccessData* variableAccessData = node->variableAccessData();
        int operand = variableAccessData->operand();
        if (operandIsArgument(operand))
            out.print(comma, "arg", operandToArgument(operand), "(", VariableAccessDataDump(*this, variableAccessData), ")");
        else
            out.print(comma, "r", operand, "(", VariableAccessDataDump(*this, variableAccessData), ")");
    }
    if (node->hasConstantBuffer()) {
        out.print(comma);
        out.print(node->startConstant(), ":[");
        CommaPrinter anotherComma;
        for (unsigned i = 0; i < node->numConstants(); ++i)
            out.print(anotherComma, m_codeBlock->constantBuffer(node->startConstant())[i]);
        out.print("]");
    }
    if (node->hasIndexingType())
        out.print(comma, IndexingTypeDump(node->indexingType()));
    if (node->hasExecutionCounter())
        out.print(comma, RawPointer(node->executionCounter()));
    if (op == JSConstant) {
        out.print(comma, "$", node->constantNumber());
        JSValue value = valueOfJSConstant(node);
        out.print(" = ", value);
    }
    if (op == WeakJSConstant)
        out.print(comma, RawPointer(node->weakConstant()));
    if (node->isBranch() || node->isJump())
        out.print(comma, "T:#", node->takenBlockIndex());
    if (node->isBranch())
        out.print(comma, "F:#", node->notTakenBlockIndex());
    out.print(comma, "bc#", node->codeOrigin.bytecodeIndex);
    
    out.print(")");

    if (!skipped) {
        if (node->hasVariableAccessData())
            out.print("  predicting ", SpeculationDump(node->variableAccessData()->prediction()), node->variableAccessData()->shouldUseDoubleFormat() ? ", forcing double" : "");
        else if (node->hasHeapPrediction())
            out.print("  predicting ", SpeculationDump(node->getHeapPrediction()));
    }
    
    out.print("\n");
}

void Graph::dumpBlockHeader(PrintStream& out, const char* prefix, BlockIndex blockIndex, PhiNodeDumpMode phiNodeDumpMode)
{
    BasicBlock* block = m_blocks[blockIndex].get();

    out.print(prefix, "Block #", blockIndex, " (", block->at(0)->codeOrigin, "): ", block->isReachable ? "" : "(skipped)", block->isOSRTarget ? " (OSR target)" : "", "\n");
    out.print(prefix, "  Predecessors:");
    for (size_t i = 0; i < block->m_predecessors.size(); ++i)
        out.print(" #", block->m_predecessors[i]);
    out.print("\n");
    if (m_dominators.isValid()) {
        out.print(prefix, "  Dominated by:");
        for (size_t i = 0; i < m_blocks.size(); ++i) {
            if (!m_dominators.dominates(i, blockIndex))
                continue;
            out.print(" #", i);
        }
        out.print("\n");
        out.print(prefix, "  Dominates:");
        for (size_t i = 0; i < m_blocks.size(); ++i) {
            if (!m_dominators.dominates(blockIndex, i))
                continue;
            out.print(" #", i);
        }
        out.print("\n");
    }
    out.print(prefix, "  Phi Nodes:");
    for (size_t i = 0; i < block->phis.size(); ++i) {
        Node* phiNode = block->phis[i];
        if (!phiNode->shouldGenerate() && phiNodeDumpMode == DumpLivePhisOnly)
            continue;
        out.print(" @", phiNode->index(), "<", phiNode->refCount(), ">->(");
        if (phiNode->child1()) {
            out.print("@", phiNode->child1()->index());
            if (phiNode->child2()) {
                out.print(", @", phiNode->child2()->index());
                if (phiNode->child3())
                    out.print(", @", phiNode->child3()->index());
            }
        }
        out.print(")", i + 1 < block->phis.size() ? "," : "");
    }
    out.print("\n");
}

void Graph::dump(PrintStream& out)
{
    dataLog("DFG for ", CodeBlockWithJITType(m_codeBlock, JITCode::DFGJIT), ":\n");
    dataLog("  Fixpoint state: ", m_fixpointState, "; Form: ", m_form, "; Unification state: ", m_unificationState, "; Ref count state: ", m_refCountState, "\n");

    out.print("  ArgumentPosition size: ", m_argumentPositions.size(), "\n");
    for (size_t i = 0; i < m_argumentPositions.size(); ++i) {
        out.print("    #", i, ": ");
        ArgumentPosition& arguments = m_argumentPositions[i];
        arguments.dump(out, this);
    }

    Node* lastNode = 0;
    for (size_t b = 0; b < m_blocks.size(); ++b) {
        BasicBlock* block = m_blocks[b].get();
        if (!block)
            continue;
        dumpBlockHeader(out, "", b, DumpAllPhis);
        out.print("  vars before: ");
        if (block->cfaHasVisited)
            dumpOperands(block->valuesAtHead, out);
        else
            out.print("<empty>");
        out.print("\n");
        out.print("  var links: ");
        dumpOperands(block->variablesAtHead, out);
        out.print("\n");
        for (size_t i = 0; i < block->size(); ++i) {
            dumpCodeOrigin(out, "", lastNode, block->at(i));
            dump(out, "", block->at(i));
            lastNode = block->at(i);
        }
        out.print("  vars after: ");
        if (block->cfaHasVisited)
            dumpOperands(block->valuesAtTail, out);
        else
            out.print("<empty>");
        out.print("\n");
        out.print("  var links: ");
        dumpOperands(block->variablesAtTail, out);
        out.print("\n");
    }
}

void Graph::dethread()
{
    if (m_form == LoadStore)
        return;
    
    if (logCompilationChanges())
        dataLog("Dethreading DFG graph.\n");
    
    SamplingRegion samplingRegion("DFG Dethreading");
    
    for (BlockIndex blockIndex = m_blocks.size(); blockIndex--;) {
        BasicBlock* block = m_blocks[blockIndex].get();
        if (!block)
            continue;
        for (unsigned phiIndex = block->phis.size(); phiIndex--;) {
            Node* phi = block->phis[phiIndex];
            phi->children.reset();
        }
    }
    
    m_form = LoadStore;
}

void Graph::handleSuccessor(Vector<BlockIndex, 16>& worklist, BlockIndex blockIndex, BlockIndex successorIndex)
{
    BasicBlock* successor = m_blocks[successorIndex].get();
    if (!successor->isReachable) {
        successor->isReachable = true;
        worklist.append(successorIndex);
    }
    
    successor->m_predecessors.append(blockIndex);
}

void Graph::determineReachability()
{
    Vector<BlockIndex, 16> worklist;
    worklist.append(0);
    m_blocks[0]->isReachable = true;
    while (!worklist.isEmpty()) {
        BlockIndex index = worklist.last();
        worklist.removeLast();
        
        BasicBlock* block = m_blocks[index].get();
        ASSERT(block->isLinked);
        
        Node* node = block->last();
        ASSERT(node->isTerminal());
        
        if (node->isJump())
            handleSuccessor(worklist, index, node->takenBlockIndex());
        else if (node->isBranch()) {
            handleSuccessor(worklist, index, node->takenBlockIndex());
            handleSuccessor(worklist, index, node->notTakenBlockIndex());
        }
    }
}

void Graph::resetReachability()
{
    for (BlockIndex blockIndex = m_blocks.size(); blockIndex--;) {
        BasicBlock* block = m_blocks[blockIndex].get();
        if (!block)
            continue;
        block->isReachable = false;
        block->m_predecessors.clear();
    }
    
    determineReachability();
}

void Graph::resetExitStates()
{
    for (BlockIndex blockIndex = 0; blockIndex < m_blocks.size(); ++blockIndex) {
        BasicBlock* block = m_blocks[blockIndex].get();
        if (!block)
            continue;
        for (unsigned indexInBlock = block->size(); indexInBlock--;)
            block->at(indexInBlock)->setCanExit(true);
    }
}

} } // namespace JSC::DFG

#endif
