/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef DebuggerActivation_h
#define DebuggerActivation_h

#include "JSObject.h"

namespace JSC {

    class DebuggerActivation : public JSNonFinalObject {
    public:
        typedef JSNonFinalObject Base;

        static DebuggerActivation* create(VM& vm, JSObject* object)
        {
            DebuggerActivation* activation = new (NotNull, allocateCell<DebuggerActivation>(vm.heap)) DebuggerActivation(vm);
            activation->finishCreation(vm, object);
            return activation;
        }

        static void visitChildren(JSCell*, SlotVisitor&);
        static String className(const JSObject*);
        static bool getOwnPropertySlot(JSCell*, ExecState*, PropertyName, PropertySlot&);
        static void put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&);
        static void putDirectVirtual(JSObject*, ExecState*, PropertyName, JSValue, unsigned attributes);
        static bool deleteProperty(JSCell*, ExecState*, PropertyName);
        static void getOwnPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
        static bool getOwnPropertyDescriptor(JSObject*, ExecState*, PropertyName, PropertyDescriptor&);
        static bool defineOwnProperty(JSObject*, ExecState*, PropertyName, PropertyDescriptor&, bool shouldThrow);

        JS_EXPORTDATA static const ClassInfo s_info;

        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype) 
        {
            return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info); 
        }

    protected:
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesVisitChildren | JSObject::StructureFlags;

        JS_EXPORT_PRIVATE void finishCreation(VM&, JSObject* activation);

    private:
        JS_EXPORT_PRIVATE DebuggerActivation(VM&);
        WriteBarrier<JSActivation> m_activation;
    };

} // namespace JSC

#endif // DebuggerActivation_h
