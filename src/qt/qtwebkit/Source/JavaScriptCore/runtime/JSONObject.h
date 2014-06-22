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

#ifndef JSONObject_h
#define JSONObject_h

#include "JSObject.h"

namespace JSC {

    class Stringifier;

    class JSONObject : public JSNonFinalObject {
    public:
        typedef JSNonFinalObject Base;

        static JSONObject* create(ExecState* exec, JSGlobalObject* globalObject, Structure* structure)
        {
            JSONObject* object = new (NotNull, allocateCell<JSONObject>(*exec->heap())) JSONObject(globalObject, structure);
            object->finishCreation(globalObject);
            return object;
        }
        
        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
        {
            return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
        }
        
        static const ClassInfo s_info;

    protected:
        void finishCreation(JSGlobalObject*);
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | JSObject::StructureFlags;

    private:
        JSONObject(JSGlobalObject*, Structure*);
        static bool getOwnPropertySlot(JSCell*, ExecState*, PropertyName, PropertySlot&);
        static bool getOwnPropertyDescriptor(JSObject*, ExecState*, PropertyName, PropertyDescriptor&);

    };

    String JSONStringify(ExecState*, JSValue, unsigned indent);

} // namespace JSC

#endif // JSONObject_h
