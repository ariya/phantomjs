/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#include "config.h"
#include "NamePrototype.h"

#include "Error.h"
#include "Operations.h"

namespace JSC {

static EncodedJSValue JSC_HOST_CALL privateNameProtoFuncToString(ExecState*);

}

#include "NamePrototype.lut.h"

namespace JSC {

const ClassInfo NamePrototype::s_info = { "Name", &Base::s_info, 0, ExecState::privateNamePrototypeTable, CREATE_METHOD_TABLE(NamePrototype) };

/* Source for NamePrototype.lut.h
@begin privateNamePrototypeTable
  toString          privateNameProtoFuncToString         DontEnum|Function 0
@end
*/

NamePrototype::NamePrototype(ExecState* exec, Structure* structure)
    : Base(exec->vm(), structure, jsEmptyString(exec))
{
}

void NamePrototype::finishCreation(ExecState* exec)
{
    Base::finishCreation(exec->vm());
    ASSERT(inherits(&s_info));
}

bool NamePrototype::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<Base>(exec, ExecState::privateNamePrototypeTable(exec), jsCast<NamePrototype*>(cell), propertyName, slot);
}

bool NamePrototype::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<Base>(exec, ExecState::privateNamePrototypeTable(exec), jsCast<NamePrototype*>(object), propertyName, descriptor);
}

// ------------------------------ Functions ---------------------------

EncodedJSValue JSC_HOST_CALL privateNameProtoFuncToString(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.isObject())
        return throwVMTypeError(exec);

    JSObject* thisObject = asObject(thisValue);
    if (!thisObject->inherits(&NameInstance::s_info))
        return throwVMTypeError(exec);

    return JSValue::encode(jsCast<NameInstance*>(thisObject)->nameString());
}

} // namespace JSC
