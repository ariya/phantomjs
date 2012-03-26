/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
#include "StructureStubInfo.h"

#include "JSObject.h"
#include "ScopeChain.h"

namespace JSC {

#if ENABLE(JIT)
void StructureStubInfo::deref()
{
    switch (accessType) {
    case access_get_by_id_self_list: {
        PolymorphicAccessStructureList* polymorphicStructures = u.getByIdSelfList.structureList;
        delete polymorphicStructures;
        return;
    }
    case access_get_by_id_proto_list: {
        PolymorphicAccessStructureList* polymorphicStructures = u.getByIdProtoList.structureList;
        delete polymorphicStructures;
        return;
    }
    case access_get_by_id_self:
    case access_get_by_id_proto:
    case access_get_by_id_chain:
    case access_put_by_id_transition:
    case access_put_by_id_replace:
    case access_get_by_id:
    case access_put_by_id:
    case access_get_by_id_generic:
    case access_put_by_id_generic:
    case access_get_array_length:
    case access_get_string_length:
        // These instructions don't have to release any allocated memory
        return;
    default:
        ASSERT_NOT_REACHED();
    }
}

void StructureStubInfo::visitAggregate(SlotVisitor& visitor)
{
    switch (accessType) {
    case access_get_by_id_self:
        visitor.append(&u.getByIdSelf.baseObjectStructure);
        return;
    case access_get_by_id_proto:
        visitor.append(&u.getByIdProto.baseObjectStructure);
        visitor.append(&u.getByIdProto.prototypeStructure);
        return;
    case access_get_by_id_chain:
        visitor.append(&u.getByIdChain.baseObjectStructure);
        visitor.append(&u.getByIdChain.chain);
        return;
    case access_get_by_id_self_list: {
        PolymorphicAccessStructureList* polymorphicStructures = u.getByIdSelfList.structureList;
        polymorphicStructures->visitAggregate(visitor, u.getByIdSelfList.listSize);
        return;
    }
    case access_get_by_id_proto_list: {
        PolymorphicAccessStructureList* polymorphicStructures = u.getByIdProtoList.structureList;
        polymorphicStructures->visitAggregate(visitor, u.getByIdProtoList.listSize);
        return;
    }
    case access_put_by_id_transition:
        visitor.append(&u.putByIdTransition.previousStructure);
        visitor.append(&u.putByIdTransition.structure);
        visitor.append(&u.putByIdTransition.chain);
        return;
    case access_put_by_id_replace:
        visitor.append(&u.putByIdReplace.baseObjectStructure);
        return;
    case access_get_by_id:
    case access_put_by_id:
    case access_get_by_id_generic:
    case access_put_by_id_generic:
    case access_get_array_length:
    case access_get_string_length:
        // These instructions don't need to mark anything
        return;
    default:
        ASSERT_NOT_REACHED();
    }
}
#endif

} // namespace JSC
