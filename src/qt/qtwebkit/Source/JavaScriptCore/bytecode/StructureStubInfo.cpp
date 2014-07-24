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
#include "PolymorphicPutByIdList.h"


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
    case access_put_by_id_list:
        delete u.putByIdList.list;
        return;
    case access_get_by_id_self:
    case access_get_by_id_proto:
    case access_get_by_id_chain:
    case access_put_by_id_transition_normal:
    case access_put_by_id_transition_direct:
    case access_put_by_id_replace:
    case access_unset:
    case access_get_by_id_generic:
    case access_put_by_id_generic:
    case access_get_array_length:
    case access_get_string_length:
        // These instructions don't have to release any allocated memory
        return;
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

bool StructureStubInfo::visitWeakReferences()
{
    switch (accessType) {
    case access_get_by_id_self:
        if (!Heap::isMarked(u.getByIdSelf.baseObjectStructure.get()))
            return false;
        break;
    case access_get_by_id_proto:
        if (!Heap::isMarked(u.getByIdProto.baseObjectStructure.get())
            || !Heap::isMarked(u.getByIdProto.prototypeStructure.get()))
            return false;
        break;
    case access_get_by_id_chain:
        if (!Heap::isMarked(u.getByIdChain.baseObjectStructure.get())
            || !Heap::isMarked(u.getByIdChain.chain.get()))
            return false;
        break;
    case access_get_by_id_self_list: {
        PolymorphicAccessStructureList* polymorphicStructures = u.getByIdSelfList.structureList;
        if (!polymorphicStructures->visitWeak(u.getByIdSelfList.listSize))
            return false;
        break;
    }
    case access_get_by_id_proto_list: {
        PolymorphicAccessStructureList* polymorphicStructures = u.getByIdProtoList.structureList;
        if (!polymorphicStructures->visitWeak(u.getByIdProtoList.listSize))
            return false;
        break;
    }
    case access_put_by_id_transition_normal:
    case access_put_by_id_transition_direct:
        if (!Heap::isMarked(u.putByIdTransition.previousStructure.get())
            || !Heap::isMarked(u.putByIdTransition.structure.get())
            || !Heap::isMarked(u.putByIdTransition.chain.get()))
            return false;
        break;
    case access_put_by_id_replace:
        if (!Heap::isMarked(u.putByIdReplace.baseObjectStructure.get()))
            return false;
        break;
    case access_put_by_id_list:
        if (!u.putByIdList.list->visitWeak())
            return false;
        break;
    default:
        // The rest of the instructions don't require references, so there is no need to
        // do anything.
        break;
    }
    return true;
}
#endif

} // namespace JSC
