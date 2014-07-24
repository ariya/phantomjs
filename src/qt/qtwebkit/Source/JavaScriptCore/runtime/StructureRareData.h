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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef StructureRareData_h
#define StructureRareData_h

#include "ClassInfo.h"
#include "JSCell.h"
#include "JSTypeInfo.h"

namespace JSC {

class JSPropertyNameIterator;
class Structure;

class StructureRareData : public JSCell {
    friend class Structure;
public:
    static StructureRareData* create(VM&, Structure*);
    static StructureRareData* clone(VM&, const StructureRareData* other);

    static void visitChildren(JSCell*, SlotVisitor&);

    static Structure* createStructure(VM&, JSGlobalObject*, JSValue prototype);

    // Returns true if this StructureRareData should also be cloned when cloning the owner Structure.
    bool needsCloning() const { return false; }

    Structure* previousID() const;
    void setPreviousID(VM&, Structure* transition, Structure*);
    void clearPreviousID();

    JSString* objectToStringValue() const;
    void setObjectToStringValue(VM&, const JSCell* owner, JSString* value);

    JSPropertyNameIterator* enumerationCache();
    void setEnumerationCache(VM&, const Structure* owner, JSPropertyNameIterator* value);

    static JS_EXPORTDATA const ClassInfo s_info;

private:
    StructureRareData(VM&, Structure*);
    StructureRareData(VM&, const StructureRareData*);

    static const unsigned StructureFlags = OverridesVisitChildren | JSCell::StructureFlags;

    WriteBarrier<Structure> m_previous;
    WriteBarrier<JSString> m_objectToStringValue;
    WriteBarrier<JSPropertyNameIterator> m_enumerationCache;
};

} // namespace JSC

#endif // StructureRareData_h
