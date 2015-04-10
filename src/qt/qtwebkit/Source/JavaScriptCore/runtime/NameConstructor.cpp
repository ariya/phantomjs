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
#include "NameConstructor.h"

#include "JSGlobalObject.h"
#include "NamePrototype.h"
#include "Operations.h"

namespace JSC {

ASSERT_HAS_TRIVIAL_DESTRUCTOR(NameConstructor);

const ClassInfo NameConstructor::s_info = { "Function", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(NameConstructor) };

NameConstructor::NameConstructor(JSGlobalObject* globalObject, Structure* structure)
    : InternalFunction(globalObject, structure)
{
}

void NameConstructor::finishCreation(ExecState* exec, NamePrototype* prototype)
{
    Base::finishCreation(exec->vm(), prototype->classInfo()->className);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().prototype, prototype, DontEnum | DontDelete | ReadOnly);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().length, jsNumber(1), DontDelete | ReadOnly | DontEnum);
}

static EncodedJSValue JSC_HOST_CALL constructPrivateName(ExecState* exec)
{
    JSValue publicName = exec->argumentCount() ? exec->argument(0) : jsUndefined();
    return JSValue::encode(NameInstance::create(exec->vm(), exec->lexicalGlobalObject()->privateNameStructure(), publicName.toString(exec)));
}

ConstructType NameConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructPrivateName;
    return ConstructTypeHost;
}

CallType NameConstructor::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = constructPrivateName;
    return CallTypeHost;
}

} // namespace JSC
