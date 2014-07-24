/*
 * Copyright (C) 2008, 2012, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Instruction_h
#define Instruction_h

#include "MacroAssembler.h"
#include "Opcode.h"
#include "PropertySlot.h"
#include "ResolveOperation.h"
#include "SpecialPointer.h"
#include "Structure.h"
#include "StructureChain.h"
#include <wtf/VectorTraits.h>

namespace JSC {

class ArrayAllocationProfile;
class ArrayProfile;
class ObjectAllocationProfile;
struct LLIntCallLinkInfo;
struct ValueProfile;

struct Instruction {
    Instruction()
    {
        u.jsCell.clear();
    }
        
    Instruction(Opcode opcode)
    {
#if !ENABLE(COMPUTED_GOTO_OPCODES)
        // We have to initialize one of the pointer members to ensure that
        // the entire struct is initialized, when opcode is not a pointer.
        u.jsCell.clear();
#endif
        u.opcode = opcode;
    }

    Instruction(int operand)
    {
        // We have to initialize one of the pointer members to ensure that
        // the entire struct is initialized in 64-bit.
        u.jsCell.clear();
        u.operand = operand;
    }

    Instruction(VM& vm, JSCell* owner, Structure* structure)
    {
        u.structure.clear();
        u.structure.set(vm, owner, structure);
    }
    Instruction(VM& vm, JSCell* owner, StructureChain* structureChain)
    {
        u.structureChain.clear();
        u.structureChain.set(vm, owner, structureChain);
    }
    Instruction(VM& vm, JSCell* owner, JSCell* jsCell)
    {
        u.jsCell.clear();
        u.jsCell.set(vm, owner, jsCell);
    }

    Instruction(PropertySlot::GetValueFunc getterFunc) { u.getterFunc = getterFunc; }
        
    Instruction(LLIntCallLinkInfo* callLinkInfo) { u.callLinkInfo = callLinkInfo; }
        
    Instruction(ValueProfile* profile) { u.profile = profile; }
    Instruction(ArrayProfile* profile) { u.arrayProfile = profile; }
    Instruction(ArrayAllocationProfile* profile) { u.arrayAllocationProfile = profile; }
    Instruction(ObjectAllocationProfile* profile) { u.objectAllocationProfile = profile; }
        
    Instruction(WriteBarrier<Unknown>* registerPointer) { u.registerPointer = registerPointer; }
        
    Instruction(Special::Pointer pointer) { u.specialPointer = pointer; }
        
    Instruction(bool* predicatePointer) { u.predicatePointer = predicatePointer; }

    union {
        Opcode opcode;
        int operand;
        WriteBarrierBase<Structure> structure;
        WriteBarrierBase<StructureChain> structureChain;
        WriteBarrierBase<JSCell> jsCell;
        WriteBarrier<Unknown>* registerPointer;
        Special::Pointer specialPointer;
        PropertySlot::GetValueFunc getterFunc;
        LLIntCallLinkInfo* callLinkInfo;
        ValueProfile* profile;
        ArrayProfile* arrayProfile;
        ArrayAllocationProfile* arrayAllocationProfile;
        ObjectAllocationProfile* objectAllocationProfile;
        void* pointer;
        bool* predicatePointer;
        ResolveOperations* resolveOperations;
        PutToBaseOperation* putToBaseOperation;
    } u;
        
private:
    Instruction(StructureChain*);
    Instruction(Structure*);
};

} // namespace JSC

namespace WTF {

template<> struct VectorTraits<JSC::Instruction> : VectorTraitsBase<true, JSC::Instruction> { };

} // namespace WTF

#endif // Instruction_h
