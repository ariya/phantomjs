/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef JSProxy_h
#define JSProxy_h

#include "JSDestructibleObject.h"

namespace JSC {

class JSProxy : public JSDestructibleObject {
public:
    typedef JSDestructibleObject Base;

    static JSProxy* create(VM& vm, Structure* structure, JSObject* target)
    {
        JSProxy* proxy = new (NotNull, allocateCell<JSProxy>(vm.heap)) JSProxy(vm, structure);
        proxy->finishCreation(vm, target);
        return proxy;
    }

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(ProxyType, StructureFlags), &s_info);
    }

    static JS_EXPORTDATA const ClassInfo s_info;

    JSObject* target() const { return m_target.get(); }

protected:
    JSProxy(VM& vm, Structure* structure)
        : JSDestructibleObject(vm, structure)
    {
    }

    void finishCreation(VM& vm)
    {
        Base::finishCreation(vm);
    }

    void finishCreation(VM& vm, JSObject* target)
    {
        Base::finishCreation(vm);
        m_target.set(vm, this, target);
    }

    static const unsigned StructureFlags = OverridesVisitChildren | OverridesGetOwnPropertySlot | OverridesGetPropertyNames | InterceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero | Base::StructureFlags;

    JS_EXPORT_PRIVATE static void visitChildren(JSCell*, SlotVisitor&);

    JS_EXPORT_PRIVATE void setTarget(VM&, JSGlobalObject*);

    JS_EXPORT_PRIVATE static String className(const JSObject*);
    JS_EXPORT_PRIVATE static bool getOwnPropertySlot(JSCell*, ExecState*, PropertyName, PropertySlot&);
    JS_EXPORT_PRIVATE static bool getOwnPropertySlotByIndex(JSCell*, ExecState*, unsigned, PropertySlot&);
    JS_EXPORT_PRIVATE static bool getOwnPropertyDescriptor(JSObject*, ExecState*, PropertyName, PropertyDescriptor&);
    JS_EXPORT_PRIVATE static void put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&);
    JS_EXPORT_PRIVATE static void putByIndex(JSCell*, ExecState*, unsigned, JSValue, bool shouldThrow);
    JS_EXPORT_PRIVATE static void putDirectVirtual(JSObject*, ExecState*, PropertyName, JSValue, unsigned attributes);
    JS_EXPORT_PRIVATE static bool deleteProperty(JSCell*, ExecState*, PropertyName);
    JS_EXPORT_PRIVATE static bool deletePropertyByIndex(JSCell*, ExecState*, unsigned);
    JS_EXPORT_PRIVATE static void getOwnPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
    JS_EXPORT_PRIVATE static void getPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
    JS_EXPORT_PRIVATE static bool defineOwnProperty(JSObject*, ExecState*, PropertyName, PropertyDescriptor&, bool shouldThrow);

private:
    WriteBarrier<JSObject> m_target;
};

} // namespace JSC

#endif // JSProxy_h
