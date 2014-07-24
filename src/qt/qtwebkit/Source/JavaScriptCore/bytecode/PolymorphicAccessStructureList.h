/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef PolymorphicAccessStructureList_h
#define PolymorphicAccessStructureList_h

#include "JITStubRoutine.h"
#include "Structure.h"
#include "StructureChain.h"
#include <wtf/Platform.h>

#define POLYMORPHIC_LIST_CACHE_SIZE 8

namespace JSC {

// *Sigh*, If the JIT is enabled we need to track the stubRountine (of type CodeLocationLabel),
// If the JIT is not in use we don't actually need the variable (that said, if the JIT is not in use we don't
// curently actually use PolymorphicAccessStructureLists, which we should).  Anyway, this seems like the best
// solution for now - will need to something smarter if/when we actually want mixed-mode operation.

#if ENABLE(JIT)
// Structure used by op_get_by_id_self_list and op_get_by_id_proto_list instruction to hold data off the main opcode stream.
struct PolymorphicAccessStructureList {
    WTF_MAKE_FAST_ALLOCATED;
public:
    struct PolymorphicStubInfo {
        bool isChain;
        bool isDirect;
        RefPtr<JITStubRoutine> stubRoutine;
        WriteBarrier<Structure> base;
        union {
            WriteBarrierBase<Structure> proto;
            WriteBarrierBase<StructureChain> chain;
        } u;

        PolymorphicStubInfo()
        {
            u.proto.clear();
        }

        void set(VM& vm, JSCell* owner, PassRefPtr<JITStubRoutine> _stubRoutine, Structure* _base, bool isDirect)
        {
            stubRoutine = _stubRoutine;
            base.set(vm, owner, _base);
            u.proto.clear();
            isChain = false;
            this->isDirect = isDirect;
        }
            
        void set(VM& vm, JSCell* owner, PassRefPtr<JITStubRoutine> _stubRoutine, Structure* _base, Structure* _proto, bool isDirect)
        {
            stubRoutine = _stubRoutine;
            base.set(vm, owner, _base);
            u.proto.set(vm, owner, _proto);
            isChain = false;
            this->isDirect = isDirect;
        }
            
        void set(VM& vm, JSCell* owner, PassRefPtr<JITStubRoutine> _stubRoutine, Structure* _base, StructureChain* _chain, bool isDirect)
        {
            stubRoutine = _stubRoutine;
            base.set(vm, owner, _base);
            u.chain.set(vm, owner, _chain);
            isChain = true;
            this->isDirect = isDirect;
        }
    } list[POLYMORPHIC_LIST_CACHE_SIZE];
        
    PolymorphicAccessStructureList()
    {
    }
        
    PolymorphicAccessStructureList(VM& vm, JSCell* owner, PassRefPtr<JITStubRoutine> stubRoutine, Structure* firstBase, bool isDirect)
    {
        list[0].set(vm, owner, stubRoutine, firstBase, isDirect);
    }

    PolymorphicAccessStructureList(VM& vm, JSCell* owner, PassRefPtr<JITStubRoutine> stubRoutine, Structure* firstBase, Structure* firstProto, bool isDirect)
    {
        list[0].set(vm, owner, stubRoutine, firstBase, firstProto, isDirect);
    }

    PolymorphicAccessStructureList(VM& vm, JSCell* owner, PassRefPtr<JITStubRoutine> stubRoutine, Structure* firstBase, StructureChain* firstChain, bool isDirect)
    {
        list[0].set(vm, owner, stubRoutine, firstBase, firstChain, isDirect);
    }

    bool visitWeak(int count)
    {
        for (int i = 0; i < count; ++i) {
            PolymorphicStubInfo& info = list[i];
            if (!info.base) {
                // We're being marked during initialisation of an entry
                ASSERT(!info.u.proto);
                continue;
            }
                
            if (!Heap::isMarked(info.base.get()))
                return false;
            if (info.u.proto && !info.isChain
                && !Heap::isMarked(info.u.proto.get()))
                return false;
            if (info.u.chain && info.isChain
                && !Heap::isMarked(info.u.chain.get()))
                return false;
        }
            
        return true;
    }
};

#endif // ENABLE(JIT)

} // namespace JSC

#endif // PolymorphicAccessStructureList_h

