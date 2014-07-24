/*
 * Copyright (C) 2008, 2012 Apple Inc. All rights reserved.
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

#ifndef StructureStubInfo_h
#define StructureStubInfo_h

#include <wtf/Platform.h>

#if ENABLE(JIT)

#include "CodeOrigin.h"
#include "DFGRegisterSet.h"
#include "Instruction.h"
#include "JITStubRoutine.h"
#include "MacroAssembler.h"
#include "Opcode.h"
#include "PolymorphicAccessStructureList.h"
#include "Structure.h"
#include "StructureStubClearingWatchpoint.h"
#include <wtf/OwnPtr.h>

namespace JSC {

class PolymorphicPutByIdList;

enum AccessType {
    access_get_by_id_self,
    access_get_by_id_proto,
    access_get_by_id_chain,
    access_get_by_id_self_list,
    access_get_by_id_proto_list,
    access_put_by_id_transition_normal,
    access_put_by_id_transition_direct,
    access_put_by_id_replace,
    access_put_by_id_list,
    access_unset,
    access_get_by_id_generic,
    access_put_by_id_generic,
    access_get_array_length,
    access_get_string_length,
};

inline bool isGetByIdAccess(AccessType accessType)
{
    switch (accessType) {
    case access_get_by_id_self:
    case access_get_by_id_proto:
    case access_get_by_id_chain:
    case access_get_by_id_self_list:
    case access_get_by_id_proto_list:
    case access_get_by_id_generic:
    case access_get_array_length:
    case access_get_string_length:
        return true;
    default:
        return false;
    }
}
    
inline bool isPutByIdAccess(AccessType accessType)
{
    switch (accessType) {
    case access_put_by_id_transition_normal:
    case access_put_by_id_transition_direct:
    case access_put_by_id_replace:
    case access_put_by_id_list:
    case access_put_by_id_generic:
        return true;
    default:
        return false;
    }
}

struct StructureStubInfo {
    StructureStubInfo()
        : accessType(access_unset)
        , seen(false)
        , resetByGC(false)
    {
    }

    void initGetByIdSelf(VM& vm, JSCell* owner, Structure* baseObjectStructure)
    {
        accessType = access_get_by_id_self;

        u.getByIdSelf.baseObjectStructure.set(vm, owner, baseObjectStructure);
    }

    void initGetByIdProto(VM& vm, JSCell* owner, Structure* baseObjectStructure, Structure* prototypeStructure, bool isDirect)
    {
        accessType = access_get_by_id_proto;

        u.getByIdProto.baseObjectStructure.set(vm, owner, baseObjectStructure);
        u.getByIdProto.prototypeStructure.set(vm, owner, prototypeStructure);
        u.getByIdProto.isDirect = isDirect;
    }

    void initGetByIdChain(VM& vm, JSCell* owner, Structure* baseObjectStructure, StructureChain* chain, unsigned count, bool isDirect)
    {
        accessType = access_get_by_id_chain;

        u.getByIdChain.baseObjectStructure.set(vm, owner, baseObjectStructure);
        u.getByIdChain.chain.set(vm, owner, chain);
        u.getByIdChain.count = count;
        u.getByIdChain.isDirect = isDirect;
    }

    void initGetByIdSelfList(PolymorphicAccessStructureList* structureList, int listSize)
    {
        accessType = access_get_by_id_self_list;

        u.getByIdSelfList.structureList = structureList;
        u.getByIdSelfList.listSize = listSize;
    }

    void initGetByIdProtoList(PolymorphicAccessStructureList* structureList, int listSize)
    {
        accessType = access_get_by_id_proto_list;

        u.getByIdProtoList.structureList = structureList;
        u.getByIdProtoList.listSize = listSize;
    }

    // PutById*

    void initPutByIdTransition(VM& vm, JSCell* owner, Structure* previousStructure, Structure* structure, StructureChain* chain, bool isDirect)
    {
        if (isDirect)
            accessType = access_put_by_id_transition_direct;
        else
            accessType = access_put_by_id_transition_normal;

        u.putByIdTransition.previousStructure.set(vm, owner, previousStructure);
        u.putByIdTransition.structure.set(vm, owner, structure);
        u.putByIdTransition.chain.set(vm, owner, chain);
    }

    void initPutByIdReplace(VM& vm, JSCell* owner, Structure* baseObjectStructure)
    {
        accessType = access_put_by_id_replace;
    
        u.putByIdReplace.baseObjectStructure.set(vm, owner, baseObjectStructure);
    }
        
    void initPutByIdList(PolymorphicPutByIdList* list)
    {
        accessType = access_put_by_id_list;
        u.putByIdList.list = list;
    }
        
    void reset()
    {
        deref();
        accessType = access_unset;
        stubRoutine.clear();
        watchpoints.clear();
    }

    void deref();

    bool visitWeakReferences();
        
    bool seenOnce()
    {
        return seen;
    }

    void setSeen()
    {
        seen = true;
    }
        
    StructureStubClearingWatchpoint* addWatchpoint(CodeBlock* codeBlock)
    {
        return WatchpointsOnStructureStubInfo::ensureReferenceAndAddWatchpoint(
            watchpoints, codeBlock, this);
    }
        
    unsigned bytecodeIndex;

    int8_t accessType;
    bool seen : 1;
    bool resetByGC : 1;

#if ENABLE(DFG_JIT)
    CodeOrigin codeOrigin;
#endif // ENABLE(DFG_JIT)

    union {
        struct {
            int8_t registersFlushed;
            int8_t baseGPR;
#if USE(JSVALUE32_64)
            int8_t valueTagGPR;
#endif
            int8_t valueGPR;
            DFG::RegisterSetPOD usedRegisters;
            int32_t deltaCallToDone;
            int32_t deltaCallToStorageLoad;
            int32_t deltaCallToStructCheck;
            int32_t deltaCallToSlowCase;
            int32_t deltaCheckImmToCall;
#if USE(JSVALUE64)
            int32_t deltaCallToLoadOrStore;
#else
            int32_t deltaCallToTagLoadOrStore;
            int32_t deltaCallToPayloadLoadOrStore;
#endif
        } dfg;
        struct {
            union {
                struct {
                    int16_t structureToCompare;
                    int16_t structureCheck;
                    int16_t propertyStorageLoad;
#if USE(JSVALUE64)
                    int16_t displacementLabel;
#else
                    int16_t displacementLabel1;
                    int16_t displacementLabel2;
#endif
                    int16_t putResult;
                    int16_t coldPathBegin;
                } get;
                struct {
                    int16_t structureToCompare;
                    int16_t propertyStorageLoad;
#if USE(JSVALUE64)
                    int16_t displacementLabel;
#else
                    int16_t displacementLabel1;
                    int16_t displacementLabel2;
#endif
                } put;
            } u;
            int16_t methodCheckProtoObj;
            int16_t methodCheckProtoStructureToCompare;
            int16_t methodCheckPutFunction;
        } baseline;
    } patch;

    union {
        struct {
            // It would be unwise to put anything here, as it will surely be overwritten.
        } unset;
        struct {
            WriteBarrierBase<Structure> baseObjectStructure;
        } getByIdSelf;
        struct {
            WriteBarrierBase<Structure> baseObjectStructure;
            WriteBarrierBase<Structure> prototypeStructure;
            bool isDirect;
        } getByIdProto;
        struct {
            WriteBarrierBase<Structure> baseObjectStructure;
            WriteBarrierBase<StructureChain> chain;
            unsigned count : 31;
            bool isDirect : 1;
        } getByIdChain;
        struct {
            PolymorphicAccessStructureList* structureList;
            int listSize;
        } getByIdSelfList;
        struct {
            PolymorphicAccessStructureList* structureList;
            int listSize;
        } getByIdProtoList;
        struct {
            WriteBarrierBase<Structure> previousStructure;
            WriteBarrierBase<Structure> structure;
            WriteBarrierBase<StructureChain> chain;
        } putByIdTransition;
        struct {
            WriteBarrierBase<Structure> baseObjectStructure;
        } putByIdReplace;
        struct {
            PolymorphicPutByIdList* list;
        } putByIdList;
    } u;

    RefPtr<JITStubRoutine> stubRoutine;
    CodeLocationCall callReturnLocation;
    CodeLocationLabel hotPathBegin;
    RefPtr<WatchpointsOnStructureStubInfo> watchpoints;
};

inline void* getStructureStubInfoReturnLocation(StructureStubInfo* structureStubInfo)
{
    return structureStubInfo->callReturnLocation.executableAddress();
}

inline unsigned getStructureStubInfoBytecodeIndex(StructureStubInfo* structureStubInfo)
{
    return structureStubInfo->bytecodeIndex;
}

} // namespace JSC

#endif // ENABLE(JIT)

#endif // StructureStubInfo_h
