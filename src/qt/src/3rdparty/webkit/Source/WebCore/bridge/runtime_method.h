/*
 * Copyright (C) 2003, 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef RUNTIME_FUNCTION_H_
#define RUNTIME_FUNCTION_H_

#include "BridgeJSC.h"
#include <runtime/InternalFunction.h>
#include <runtime/JSGlobalObject.h>
#include <wtf/OwnPtr.h>

namespace JSC {

class RuntimeMethod : public InternalFunction {
public:
    RuntimeMethod(ExecState*, JSGlobalObject*, Structure*, const Identifier& name, Bindings::MethodList&);
    Bindings::MethodList* methods() const { return _methodList.get(); }

    static const ClassInfo s_info;

    static FunctionPrototype* createPrototype(ExecState*, JSGlobalObject* globalObject)
    {
        return globalObject->functionPrototype();
    }

    static Structure* createStructure(JSGlobalData& globalData, JSValue prototype)
    {
        return Structure::create(globalData, prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
    }

protected:
    static const unsigned StructureFlags = OverridesGetOwnPropertySlot | InternalFunction::StructureFlags;

private:
    static JSValue lengthGetter(ExecState*, JSValue, const Identifier&);
    virtual bool getOwnPropertySlot(ExecState*, const Identifier&, PropertySlot&);
    virtual bool getOwnPropertyDescriptor(ExecState*, const Identifier&, PropertyDescriptor&);
    virtual CallType getCallData(CallData&);

    OwnPtr<Bindings::MethodList> _methodList;
};

} // namespace JSC

#endif
