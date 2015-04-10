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
#include "GetByIdStatus.h"

#include "CodeBlock.h"
#include "JSScope.h"
#include "LLIntData.h"
#include "LowLevelInterpreter.h"
#include "Operations.h"

namespace JSC {

GetByIdStatus GetByIdStatus::computeFromLLInt(CodeBlock* profiledBlock, unsigned bytecodeIndex, Identifier& ident)
{
    UNUSED_PARAM(profiledBlock);
    UNUSED_PARAM(bytecodeIndex);
    UNUSED_PARAM(ident);
#if ENABLE(LLINT)
    Instruction* instruction = profiledBlock->instructions().begin() + bytecodeIndex;
    
    if (instruction[0].u.opcode == LLInt::getOpcode(llint_op_get_array_length))
        return GetByIdStatus(NoInformation, false);

    Structure* structure = instruction[4].u.structure.get();
    if (!structure)
        return GetByIdStatus(NoInformation, false);
    
    unsigned attributesIgnored;
    JSCell* specificValue;
    PropertyOffset offset = structure->get(
        *profiledBlock->vm(), ident, attributesIgnored, specificValue);
    if (structure->isDictionary())
        specificValue = 0;
    if (!isValidOffset(offset))
        return GetByIdStatus(NoInformation, false);
    
    return GetByIdStatus(Simple, false, StructureSet(structure), offset, specificValue);
#else
    return GetByIdStatus(NoInformation, false);
#endif
}

void GetByIdStatus::computeForChain(GetByIdStatus& result, CodeBlock* profiledBlock, Identifier& ident, Structure* structure)
{
#if ENABLE(JIT) && ENABLE(VALUE_PROFILER)
    // Validate the chain. If the chain is invalid, then currently the best thing
    // we can do is to assume that TakesSlow is true. In the future, it might be
    // worth exploring reifying the structure chain from the structure we've got
    // instead of using the one from the cache, since that will do the right things
    // if the structure chain has changed. But that may be harder, because we may
    // then end up having a different type of access altogether. And it currently
    // does not appear to be worth it to do so -- effectively, the heuristic we
    // have now is that if the structure chain has changed between when it was
    // cached on in the baseline JIT and when the DFG tried to inline the access,
    // then we fall back on a polymorphic access.
    Structure* currentStructure = structure;
    JSObject* currentObject = 0;
    for (unsigned i = 0; i < result.m_chain.size(); ++i) {
        ASSERT(!currentStructure->isDictionary());
        currentObject = asObject(currentStructure->prototypeForLookup(profiledBlock));
        currentStructure = result.m_chain[i];
        if (currentObject->structure() != currentStructure)
            return;
    }
    
    ASSERT(currentObject);
        
    unsigned attributesIgnored;
    JSCell* specificValue;
        
    result.m_offset = currentStructure->get(
        *profiledBlock->vm(), ident, attributesIgnored, specificValue);
    if (currentStructure->isDictionary())
        specificValue = 0;
    if (!isValidOffset(result.m_offset))
        return;
        
    result.m_structureSet.add(structure);
    result.m_specificValue = JSValue(specificValue);
#else
    UNUSED_PARAM(result);
    UNUSED_PARAM(profiledBlock);
    UNUSED_PARAM(ident);
    UNUSED_PARAM(structure);
    UNREACHABLE_FOR_PLATFORM();
#endif
}

GetByIdStatus GetByIdStatus::computeFor(CodeBlock* profiledBlock, unsigned bytecodeIndex, Identifier& ident)
{
    UNUSED_PARAM(profiledBlock);
    UNUSED_PARAM(bytecodeIndex);
    UNUSED_PARAM(ident);
#if ENABLE(JIT) && ENABLE(VALUE_PROFILER)
    if (!profiledBlock->numberOfStructureStubInfos())
        return computeFromLLInt(profiledBlock, bytecodeIndex, ident);
    
    // First check if it makes either calls, in which case we want to be super careful, or
    // if it's not set at all, in which case we punt.
    StructureStubInfo& stubInfo = profiledBlock->getStubInfo(bytecodeIndex);
    if (!stubInfo.seen)
        return computeFromLLInt(profiledBlock, bytecodeIndex, ident);
    
    if (stubInfo.resetByGC)
        return GetByIdStatus(TakesSlowPath, true);

    PolymorphicAccessStructureList* list;
    int listSize;
    switch (stubInfo.accessType) {
    case access_get_by_id_self_list:
        list = stubInfo.u.getByIdSelfList.structureList;
        listSize = stubInfo.u.getByIdSelfList.listSize;
        break;
    case access_get_by_id_proto_list:
        list = stubInfo.u.getByIdProtoList.structureList;
        listSize = stubInfo.u.getByIdProtoList.listSize;
        break;
    default:
        list = 0;
        listSize = 0;
        break;
    }
    for (int i = 0; i < listSize; ++i) {
        if (!list->list[i].isDirect)
            return GetByIdStatus(MakesCalls, true);
    }
    
    // Next check if it takes slow case, in which case we want to be kind of careful.
    if (profiledBlock->likelyToTakeSlowCase(bytecodeIndex))
        return GetByIdStatus(TakesSlowPath, true);
    
    // Finally figure out if we can derive an access strategy.
    GetByIdStatus result;
    result.m_wasSeenInJIT = true; // This is interesting for bytecode dumping only.
    switch (stubInfo.accessType) {
    case access_unset:
        return computeFromLLInt(profiledBlock, bytecodeIndex, ident);
        
    case access_get_by_id_self: {
        Structure* structure = stubInfo.u.getByIdSelf.baseObjectStructure.get();
        unsigned attributesIgnored;
        JSCell* specificValue;
        result.m_offset = structure->get(
            *profiledBlock->vm(), ident, attributesIgnored, specificValue);
        if (structure->isDictionary())
            specificValue = 0;
        
        if (isValidOffset(result.m_offset)) {
            result.m_structureSet.add(structure);
            result.m_specificValue = JSValue(specificValue);
        }
        
        if (isValidOffset(result.m_offset))
            ASSERT(result.m_structureSet.size());
        break;
    }
        
    case access_get_by_id_self_list: {
        for (int i = 0; i < listSize; ++i) {
            ASSERT(list->list[i].isDirect);
            
            Structure* structure = list->list[i].base.get();
            if (result.m_structureSet.contains(structure))
                continue;
            
            unsigned attributesIgnored;
            JSCell* specificValue;
            PropertyOffset myOffset = structure->get(
                *profiledBlock->vm(), ident, attributesIgnored, specificValue);
            if (structure->isDictionary())
                specificValue = 0;
            
            if (!isValidOffset(myOffset)) {
                result.m_offset = invalidOffset;
                break;
            }
                    
            if (!i) {
                result.m_offset = myOffset;
                result.m_specificValue = JSValue(specificValue);
            } else if (result.m_offset != myOffset) {
                result.m_offset = invalidOffset;
                break;
            } else if (result.m_specificValue != JSValue(specificValue))
                result.m_specificValue = JSValue();
            
            result.m_structureSet.add(structure);
        }
                    
        if (isValidOffset(result.m_offset))
            ASSERT(result.m_structureSet.size());
        break;
    }
        
    case access_get_by_id_proto: {
        if (!stubInfo.u.getByIdProto.isDirect)
            return GetByIdStatus(MakesCalls, true);
        result.m_chain.append(stubInfo.u.getByIdProto.prototypeStructure.get());
        computeForChain(
            result, profiledBlock, ident,
            stubInfo.u.getByIdProto.baseObjectStructure.get());
        break;
    }
        
    case access_get_by_id_chain: {
        if (!stubInfo.u.getByIdChain.isDirect)
            return GetByIdStatus(MakesCalls, true);
        for (unsigned i = 0; i < stubInfo.u.getByIdChain.count; ++i)
            result.m_chain.append(stubInfo.u.getByIdChain.chain->head()[i].get());
        computeForChain(
            result, profiledBlock, ident,
            stubInfo.u.getByIdChain.baseObjectStructure.get());
        break;
    }
        
    default:
        ASSERT(!isValidOffset(result.m_offset));
        break;
    }
    
    if (!isValidOffset(result.m_offset)) {
        result.m_state = TakesSlowPath;
        result.m_structureSet.clear();
        result.m_chain.clear();
        result.m_specificValue = JSValue();
    } else
        result.m_state = Simple;
    
    return result;
#else // ENABLE(JIT)
    return GetByIdStatus(NoInformation, false);
#endif // ENABLE(JIT)
}

GetByIdStatus GetByIdStatus::computeFor(VM& vm, Structure* structure, Identifier& ident)
{
    // For now we only handle the super simple self access case. We could handle the
    // prototype case in the future.
    
    if (PropertyName(ident).asIndex() != PropertyName::NotAnIndex)
        return GetByIdStatus(TakesSlowPath);
    
    if (structure->typeInfo().overridesGetOwnPropertySlot())
        return GetByIdStatus(TakesSlowPath);
    
    if (!structure->propertyAccessesAreCacheable())
        return GetByIdStatus(TakesSlowPath);
    
    GetByIdStatus result;
    result.m_wasSeenInJIT = false; // To my knowledge nobody that uses computeFor(VM&, Structure*, Identifier&) reads this field, but I might as well be honest: no, it wasn't seen in the JIT, since I computed it statically.
    unsigned attributes;
    JSCell* specificValue;
    result.m_offset = structure->get(vm, ident, attributes, specificValue);
    if (!isValidOffset(result.m_offset))
        return GetByIdStatus(TakesSlowPath); // It's probably a prototype lookup. Give up on life for now, even though we could totally be way smarter about it.
    if (attributes & Accessor)
        return GetByIdStatus(MakesCalls);
    if (structure->isDictionary())
        specificValue = 0;
    result.m_structureSet.add(structure);
    result.m_specificValue = JSValue(specificValue);
    return result;
}

} // namespace JSC

