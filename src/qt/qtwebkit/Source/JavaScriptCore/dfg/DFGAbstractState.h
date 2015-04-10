/*
 * Copyright (C) 2011, 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef DFGAbstractState_h
#define DFGAbstractState_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGAbstractValue.h"
#include "DFGBranchDirection.h"
#include "DFGGraph.h"
#include "DFGNode.h"
#include <wtf/Vector.h>

namespace JSC {

class CodeBlock;

namespace DFG {

struct BasicBlock;

// This implements the notion of an abstract state for flow-sensitive intraprocedural
// control flow analysis (CFA), with a focus on the elimination of redundant type checks.
// It also implements most of the mechanisms of abstract interpretation that such an
// analysis would use. This class should be used in two idioms:
//
// 1) Performing the CFA. In this case, AbstractState should be run over all basic
//    blocks repeatedly until convergence is reached. Convergence is defined by
//    endBasicBlock(AbstractState::MergeToSuccessors) returning false for all blocks.
//
// 2) Rematerializing the results of a previously executed CFA. In this case,
//    AbstractState should be run over whatever basic block you're interested in up
//    to the point of the node at which you'd like to interrogate the known type
//    of all other nodes. At this point it's safe to discard the AbstractState entirely,
//    call reset(), or to run it to the end of the basic block and call
//    endBasicBlock(AbstractState::DontMerge). The latter option is safest because
//    it performs some useful integrity checks.
//
// After the CFA is run, the inter-block state is saved at the heads and tails of all
// basic blocks. This allows the intra-block state to be rematerialized by just
// executing the CFA for that block. If you need to know inter-block state only, then
// you only need to examine the BasicBlock::m_valuesAtHead or m_valuesAtTail fields.
//
// Running this analysis involves the following, modulo the inter-block state
// merging and convergence fixpoint:
//
// AbstractState state(codeBlock, graph);
// state.beginBasicBlock(basicBlock);
// bool endReached = true;
// for (unsigned i = 0; i < basicBlock->size(); ++i) {
//     if (!state.execute(i))
//         break;
// }
// bool result = state.endBasicBlock(<either Merge or DontMerge>);

class AbstractState {
public:
    enum MergeMode {
        // Don't merge the state in AbstractState with basic blocks.
        DontMerge,
        
        // Merge the state in AbstractState with the tail of the basic
        // block being analyzed.
        MergeToTail,
        
        // Merge the state in AbstractState with the tail of the basic
        // block, and with the heads of successor blocks.
        MergeToSuccessors
    };
    
    AbstractState(Graph&);
    
    ~AbstractState();
    
    AbstractValue& forNode(Node* node)
    {
        return node->value;
    }
    
    AbstractValue& forNode(Edge edge)
    {
        return forNode(edge.node());
    }
    
    Operands<AbstractValue>& variables()
    {
        return m_variables;
    }
    
    // Call this before beginning CFA to initialize the abstract values of
    // arguments, and to indicate which blocks should be listed for CFA
    // execution.
    static void initialize(Graph&);

    // Start abstractly executing the given basic block. Initializes the
    // notion of abstract state to what we believe it to be at the head
    // of the basic block, according to the basic block's data structures.
    // This method also sets cfaShouldRevisit to false.
    void beginBasicBlock(BasicBlock*);
    
    // Finish abstractly executing a basic block. If MergeToTail or
    // MergeToSuccessors is passed, then this merges everything we have
    // learned about how the state changes during this block's execution into
    // the block's data structures. There are three return modes, depending
    // on the value of mergeMode:
    //
    // DontMerge:
    //    Always returns false.
    //
    // MergeToTail:
    //    Returns true if the state of the block at the tail was changed.
    //    This means that you must call mergeToSuccessors(), and if that
    //    returns true, then you must revisit (at least) the successor
    //    blocks. False will always be returned if the block is terminal
    //    (i.e. ends in Throw or Return, or has a ForceOSRExit inside it).
    //
    // MergeToSuccessors:
    //    Returns true if the state of the block at the tail was changed,
    //    and, if the state at the heads of successors was changed.
    //    A true return means that you must revisit (at least) the successor
    //    blocks. This also sets cfaShouldRevisit to true for basic blocks
    //    that must be visited next.
    bool endBasicBlock(MergeMode);
    
    // Reset the AbstractState. This throws away any results, and at this point
    // you can safely call beginBasicBlock() on any basic block.
    void reset();
    
    // Abstractly executes the given node. The new abstract state is stored into an
    // abstract stack stored in *this. Loads of local variables (that span
    // basic blocks) interrogate the basic block's notion of the state at the head.
    // Stores to local variables are handled in endBasicBlock(). This returns true
    // if execution should continue past this node. Notably, it will return true
    // for block terminals, so long as those terminals are not Return or variants
    // of Throw.
    //
    // This is guaranteed to be equivalent to doing:
    //
    // if (state.startExecuting(index)) {
    //     state.executeEdges(index);
    //     result = state.executeEffects(index);
    // } else
    //     result = true;
    bool execute(unsigned indexInBlock);
    
    // Indicate the start of execution of the node. It resets any state in the node,
    // that is progressively built up by executeEdges() and executeEffects(). In
    // particular, this resets canExit(), so if you want to "know" between calls of
    // startExecuting() and executeEdges()/Effects() whether the last run of the
    // analysis concluded that the node can exit, you should probably set that
    // information aside prior to calling startExecuting().
    bool startExecuting(Node*);
    bool startExecuting(unsigned indexInBlock);
    
    // Abstractly execute the edges of the given node. This runs filterEdgeByUse()
    // on all edges of the node. You can skip this step, if you have already used
    // filterEdgeByUse() (or some equivalent) on each edge.
    void executeEdges(Node*);
    void executeEdges(unsigned indexInBlock);
    
    ALWAYS_INLINE void filterEdgeByUse(Node* node, Edge& edge)
    {
#if !ASSERT_DISABLED
        switch (edge.useKind()) {
        case KnownInt32Use:
        case KnownNumberUse:
        case KnownCellUse:
        case KnownStringUse:
            ASSERT(!(forNode(edge).m_type & ~typeFilterFor(edge.useKind())));
            break;
        default:
            break;
        }
#endif // !ASSERT_DISABLED
        
        filterByType(node, edge, typeFilterFor(edge.useKind()));
    }
    
    // Abstractly execute the effects of the given node. This changes the abstract
    // state assuming that edges have already been filtered.
    bool executeEffects(unsigned indexInBlock);
    bool executeEffects(unsigned indexInBlock, Node*);
    
    // Did the last executed node clobber the world?
    bool didClobber() const { return m_didClobber; }
    
    // Is the execution state still valid? This will be false if execute() has
    // returned false previously.
    bool isValid() const { return m_isValid; }
    
    // Merge the abstract state stored at the first block's tail into the second
    // block's head. Returns true if the second block's state changed. If so,
    // that block must be abstractly interpreted again. This also sets
    // to->cfaShouldRevisit to true, if it returns true, or if to has not been
    // visited yet.
    bool merge(BasicBlock* from, BasicBlock* to);
    
    // Merge the abstract state stored at the block's tail into all of its
    // successors. Returns true if any of the successors' states changed. Note
    // that this is automatically called in endBasicBlock() if MergeMode is
    // MergeToSuccessors.
    bool mergeToSuccessors(Graph&, BasicBlock*);
    
    void dump(PrintStream& out);
    
private:
    void clobberWorld(const CodeOrigin&, unsigned indexInBlock);
    void clobberCapturedVars(const CodeOrigin&);
    void clobberStructures(unsigned indexInBlock);
    
    bool mergeStateAtTail(AbstractValue& destination, AbstractValue& inVariable, Node*);
    
    static bool mergeVariableBetweenBlocks(AbstractValue& destination, AbstractValue& source, Node* destinationNode, Node* sourceNode);
    
    enum BooleanResult {
        UnknownBooleanResult,
        DefinitelyFalse,
        DefinitelyTrue
    };
    BooleanResult booleanResult(Node*, AbstractValue&);
    
    bool trySetConstant(Node* node, JSValue value)
    {
        // Make sure we don't constant fold something that will produce values that contravene
        // predictions. If that happens then we know that the code will OSR exit, forcing
        // recompilation. But if we tried to constant fold then we'll have a very degenerate
        // IR: namely we'll have a JSConstant that contravenes its own prediction. There's a
        // lot of subtle code that assumes that
        // speculationFromValue(jsConstant) == jsConstant.prediction(). "Hardening" that code
        // is probably less sane than just pulling back on constant folding.
        SpeculatedType oldType = node->prediction();
        if (mergeSpeculations(speculationFromValue(value), oldType) != oldType)
            return false;
        
        forNode(node).set(value);
        return true;
    }
    
    ALWAYS_INLINE void filterByType(Node* node, Edge& edge, SpeculatedType type)
    {
        AbstractValue& value = forNode(edge);
        if (value.m_type & ~type) {
            node->setCanExit(true);
            edge.setProofStatus(NeedsCheck);
        } else
            edge.setProofStatus(IsProved);
        
        value.filter(type);
    }
    
    void verifyEdge(Node*, Edge);
    void verifyEdges(Node*);
    
    CodeBlock* m_codeBlock;
    Graph& m_graph;
    
    Operands<AbstractValue> m_variables;
    BasicBlock* m_block;
    bool m_haveStructures;
    bool m_foundConstants;
    
    bool m_isValid;
    bool m_didClobber;
    
    BranchDirection m_branchDirection; // This is only set for blocks that end in Branch and that execute to completion (i.e. m_isValid == true).
};

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGAbstractState_h

