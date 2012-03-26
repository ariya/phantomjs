/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef JSZombie_h
#define JSZombie_h

#include "JSCell.h"
#include "Structure.h"

#if ENABLE(JSC_ZOMBIES)
namespace JSC {

class JSZombie : public JSCell {
public:
    JSZombie(JSGlobalData& globalData, const ClassInfo* oldInfo, Structure* structure)
        : JSCell(globalData, structure)
        , m_oldInfo(oldInfo)
    {
        ASSERT(inherits(&s_info));
    }

    virtual bool isZombie() const { return true; }

    virtual bool isGetterSetter() const { ASSERT_NOT_REACHED(); return false; }
    virtual bool isAPIValueWrapper() const { ASSERT_NOT_REACHED(); return false; }
    virtual bool isPropertyNameIterator() const { ASSERT_NOT_REACHED(); return false; }
    virtual CallType getCallData(CallData&) { ASSERT_NOT_REACHED(); return CallTypeNone; }
    virtual ConstructType getConstructData(ConstructData&) { ASSERT_NOT_REACHED(); return ConstructTypeNone; }
    virtual bool getUInt32(uint32_t&) const { ASSERT_NOT_REACHED(); return false; }
    virtual JSValue toPrimitive(ExecState*, PreferredPrimitiveType) const { ASSERT_NOT_REACHED(); return jsNull(); }
    virtual bool getPrimitiveNumber(ExecState*, double&, JSValue&) { ASSERT_NOT_REACHED(); return false; }
    virtual bool toBoolean(ExecState*) const { ASSERT_NOT_REACHED(); return false; }
    virtual double toNumber(ExecState*) const { ASSERT_NOT_REACHED(); return 0.0; }
    virtual UString toString(ExecState*) const { ASSERT_NOT_REACHED(); return ""; }
    virtual JSObject* toObject(ExecState*) const { ASSERT_NOT_REACHED(); return 0; }
    virtual void visitChildren(SlotVisitor&) { ASSERT_NOT_REACHED(); }
    virtual void put(ExecState*, const Identifier&, JSValue, PutPropertySlot&) { ASSERT_NOT_REACHED(); }
    virtual void put(ExecState*, unsigned, JSValue) { ASSERT_NOT_REACHED(); }
    virtual bool deleteProperty(ExecState*, const Identifier&) { ASSERT_NOT_REACHED(); return false; }
    virtual bool deleteProperty(ExecState*, unsigned) { ASSERT_NOT_REACHED(); return false; }
    virtual JSObject* toThisObject(ExecState*) const { ASSERT_NOT_REACHED(); return 0; }
    virtual JSValue toStrictThisObject(ExecState*) const { ASSERT_NOT_REACHED(); return JSValue(); }
    virtual JSValue getJSNumber() { ASSERT_NOT_REACHED(); return jsNull(); }
    virtual bool getOwnPropertySlot(ExecState*, const Identifier&, PropertySlot&) { ASSERT_NOT_REACHED(); return false; }
    virtual bool getOwnPropertySlot(ExecState*, unsigned, PropertySlot&) { ASSERT_NOT_REACHED(); return false; }
    
    static Structure* createStructure(JSGlobalData& globalData, JSValue prototype)
    {
        return Structure::create(globalData, prototype, TypeInfo(LeafType, 0), AnonymousSlotCount, &s_info);
    }

    static const ClassInfo s_info;

private:
    const ClassInfo* m_oldInfo;
};

}

#endif // ENABLE(JSC_ZOMBIES)

#endif // JSZombie_h
