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
#include "DFGVariableEventStream.h"

#if ENABLE(DFG_JIT)

#include "CodeBlock.h"
#include "DFGValueSource.h"
#include "Operations.h"
#include <wtf/DataLog.h>
#include <wtf/HashMap.h>

namespace JSC { namespace DFG {

void VariableEventStream::logEvent(const VariableEvent& event)
{
    dataLogF("seq#%u:", static_cast<unsigned>(size()));
    event.dump(WTF::dataFile());
    dataLogF(" ");
}

namespace {

struct MinifiedGenerationInfo {
    bool filled; // true -> in gpr/fpr/pair, false -> spilled
    VariableRepresentation u;
    DataFormat format;
    
    MinifiedGenerationInfo()
        : format(DataFormatNone)
    {
    }
    
    void update(const VariableEvent& event)
    {
        switch (event.kind()) {
        case BirthToFill:
        case Fill:
            filled = true;
            break;
        case BirthToSpill:
        case Spill:
            filled = false;
            break;
        case Death:
            format = DataFormatNone;
            return;
        default:
            return;
        }
        
        u = event.variableRepresentation();
        format = event.dataFormat();
    }
};

} // namespace

bool VariableEventStream::tryToSetConstantRecovery(ValueRecovery& recovery, CodeBlock* codeBlock, MinifiedNode* node) const
{
    if (!node)
        return false;
    
    if (node->hasConstantNumber()) {
        recovery = ValueRecovery::constant(
            codeBlock->constantRegister(
                FirstConstantRegisterIndex + node->constantNumber()).get());
        return true;
    }
    
    if (node->hasWeakConstant()) {
        recovery = ValueRecovery::constant(node->weakConstant());
        return true;
    }
    
    if (node->op() == PhantomArguments) {
        recovery = ValueRecovery::argumentsThatWereNotCreated();
        return true;
    }
    
    return false;
}

void VariableEventStream::reconstruct(
    CodeBlock* codeBlock, CodeOrigin codeOrigin, MinifiedGraph& graph,
    unsigned index, Operands<ValueRecovery>& valueRecoveries) const
{
    ASSERT(codeBlock->getJITType() == JITCode::DFGJIT);
    CodeBlock* baselineCodeBlock = codeBlock->baselineVersion();
    
    unsigned numVariables;
    if (codeOrigin.inlineCallFrame)
        numVariables = baselineCodeBlockForInlineCallFrame(codeOrigin.inlineCallFrame)->m_numCalleeRegisters + codeOrigin.inlineCallFrame->stackOffset;
    else
        numVariables = baselineCodeBlock->m_numCalleeRegisters;
    
    // Crazy special case: if we're at index == 0 then this must be an argument check
    // failure, in which case all variables are already set up. The recoveries should
    // reflect this.
    if (!index) {
        valueRecoveries = Operands<ValueRecovery>(codeBlock->numParameters(), numVariables);
        for (size_t i = 0; i < valueRecoveries.size(); ++i)
            valueRecoveries[i] = ValueRecovery::alreadyInJSStack();
        return;
    }
    
    // Step 1: Find the last checkpoint, and figure out the number of virtual registers as we go.
    unsigned startIndex = index - 1;
    while (at(startIndex).kind() != Reset)
        startIndex--;
    
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLogF("Computing OSR exit recoveries starting at seq#%u.\n", startIndex);
#endif

    // Step 2: Create a mock-up of the DFG's state and execute the events.
    Operands<ValueSource> operandSources(codeBlock->numParameters(), numVariables);
    HashMap<MinifiedID, MinifiedGenerationInfo> generationInfos;
    for (unsigned i = startIndex; i < index; ++i) {
        const VariableEvent& event = at(i);
        switch (event.kind()) {
        case Reset:
            // nothing to do.
            break;
        case BirthToFill:
        case BirthToSpill: {
            MinifiedGenerationInfo info;
            info.update(event);
            generationInfos.add(event.id(), info);
            break;
        }
        case Fill:
        case Spill:
        case Death: {
            HashMap<MinifiedID, MinifiedGenerationInfo>::iterator iter = generationInfos.find(event.id());
            ASSERT(iter != generationInfos.end());
            iter->value.update(event);
            break;
        }
        case MovHintEvent:
            if (operandSources.hasOperand(event.operand()))
                operandSources.setOperand(event.operand(), ValueSource(event.id()));
            break;
        case SetLocalEvent:
            if (operandSources.hasOperand(event.operand()))
                operandSources.setOperand(event.operand(), ValueSource::forDataFormat(event.dataFormat()));
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }
    
    // Step 3: Compute value recoveries!
    valueRecoveries = Operands<ValueRecovery>(codeBlock->numParameters(), numVariables);
    for (unsigned i = 0; i < operandSources.size(); ++i) {
        ValueSource& source = operandSources[i];
        if (source.isTriviallyRecoverable()) {
            valueRecoveries[i] = source.valueRecovery();
            continue;
        }
        
        ASSERT(source.kind() == HaveNode);
        MinifiedNode* node = graph.at(source.id());
        if (tryToSetConstantRecovery(valueRecoveries[i], codeBlock, node))
            continue;
        
        MinifiedGenerationInfo info = generationInfos.get(source.id());
        if (info.format == DataFormatNone) {
            // Try to see if there is an alternate node that would contain the value we want.
            // There are four possibilities:
            //
            // Int32ToDouble: We can use this in place of the original node, but
            //    we'd rather not; so we use it only if it is the only remaining
            //    live version.
            //
            // ValueToInt32: If the only remaining live version of the value is
            //    ValueToInt32, then we can use it.
            //
            // UInt32ToNumber: If the only live version of the value is a UInt32ToNumber
            //    then the only remaining uses are ones that want a properly formed number
            //    rather than a UInt32 intermediate.
            //
            // DoubleAsInt32: Same as UInt32ToNumber.
            //
            // The reverse of the above: This node could be a UInt32ToNumber, but its
            //    alternative is still alive. This means that the only remaining uses of
            //    the number would be fine with a UInt32 intermediate.
            
            bool found = false;
            
            if (node && node->op() == UInt32ToNumber) {
                MinifiedID id = node->child1();
                if (tryToSetConstantRecovery(valueRecoveries[i], codeBlock, graph.at(id)))
                    continue;
                info = generationInfos.get(id);
                if (info.format != DataFormatNone)
                    found = true;
            }
            
            if (!found) {
                MinifiedID int32ToDoubleID;
                MinifiedID valueToInt32ID;
                MinifiedID uint32ToNumberID;
                MinifiedID doubleAsInt32ID;
                
                HashMap<MinifiedID, MinifiedGenerationInfo>::iterator iter = generationInfos.begin();
                HashMap<MinifiedID, MinifiedGenerationInfo>::iterator end = generationInfos.end();
                for (; iter != end; ++iter) {
                    MinifiedID id = iter->key;
                    node = graph.at(id);
                    if (!node)
                        continue;
                    if (!node->hasChild1())
                        continue;
                    if (node->child1() != source.id())
                        continue;
                    if (iter->value.format == DataFormatNone)
                        continue;
                    switch (node->op()) {
                    case Int32ToDouble:
                    case ForwardInt32ToDouble:
                        int32ToDoubleID = id;
                        break;
                    case ValueToInt32:
                        valueToInt32ID = id;
                        break;
                    case UInt32ToNumber:
                        uint32ToNumberID = id;
                        break;
                    case DoubleAsInt32:
                        doubleAsInt32ID = id;
                        break;
                    default:
                        break;
                    }
                }
                
                MinifiedID idToUse;
                if (!!doubleAsInt32ID)
                    idToUse = doubleAsInt32ID;
                else if (!!int32ToDoubleID)
                    idToUse = int32ToDoubleID;
                else if (!!valueToInt32ID)
                    idToUse = valueToInt32ID;
                else if (!!uint32ToNumberID)
                    idToUse = uint32ToNumberID;
                
                if (!!idToUse) {
                    info = generationInfos.get(idToUse);
                    ASSERT(info.format != DataFormatNone);
                    found = true;
                }
            }
            
            if (!found) {
                valueRecoveries[i] = ValueRecovery::constant(jsUndefined());
                continue;
            }
        }
        
        ASSERT(info.format != DataFormatNone);
        
        if (info.filled) {
            if (info.format == DataFormatDouble) {
                valueRecoveries[i] = ValueRecovery::inFPR(info.u.fpr);
                continue;
            }
#if USE(JSVALUE32_64)
            if (info.format & DataFormatJS) {
                valueRecoveries[i] = ValueRecovery::inPair(info.u.pair.tagGPR, info.u.pair.payloadGPR);
                continue;
            }
#endif
            valueRecoveries[i] = ValueRecovery::inGPR(info.u.gpr, info.format);
            continue;
        }
        
        valueRecoveries[i] =
            ValueRecovery::displacedInJSStack(static_cast<VirtualRegister>(info.u.virtualReg), info.format);
    }
    
    // Step 4: Make sure that for locals that coincide with true call frame headers, the exit compiler knows
    // that those values don't have to be recovered. Signal this by using ValueRecovery::alreadyInJSStack()
    for (InlineCallFrame* inlineCallFrame = codeOrigin.inlineCallFrame; inlineCallFrame; inlineCallFrame = inlineCallFrame->caller.inlineCallFrame) {
        for (unsigned i = JSStack::CallFrameHeaderSize; i--;)
            valueRecoveries.setLocal(inlineCallFrame->stackOffset - i - 1, ValueRecovery::alreadyInJSStack());
    }
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

