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
#include "JITThunks.h"

#if ENABLE(JIT)

#include "Executable.h"
#include "JIT.h"
#include "VM.h"
#include "Operations.h"

namespace JSC {

JITThunks::JITThunks()
    : m_hostFunctionStubMap(adoptPtr(new HostFunctionStubMap))
{
}

JITThunks::~JITThunks()
{
}

MacroAssemblerCodePtr JITThunks::ctiNativeCall(VM* vm)
{
#if ENABLE(LLINT)
    if (!vm->canUseJIT())
        return MacroAssemblerCodePtr::createLLIntCodePtr(llint_native_call_trampoline);
#endif
    return ctiStub(vm, nativeCallGenerator).code();
}
MacroAssemblerCodePtr JITThunks::ctiNativeConstruct(VM* vm)
{
#if ENABLE(LLINT)
    if (!vm->canUseJIT())
        return MacroAssemblerCodePtr::createLLIntCodePtr(llint_native_construct_trampoline);
#endif
    return ctiStub(vm, nativeConstructGenerator).code();
}

MacroAssemblerCodeRef JITThunks::ctiStub(VM* vm, ThunkGenerator generator)
{
    CTIStubMap::AddResult entry = m_ctiStubMap.add(generator, MacroAssemblerCodeRef());
    if (entry.isNewEntry)
        entry.iterator->value = generator(vm);
    return entry.iterator->value;
}

NativeExecutable* JITThunks::hostFunctionStub(VM* vm, NativeFunction function, NativeFunction constructor)
{
    if (NativeExecutable* nativeExecutable = m_hostFunctionStubMap->get(std::make_pair(function, constructor)))
        return nativeExecutable;

    NativeExecutable* nativeExecutable = NativeExecutable::create(*vm, JIT::compileCTINativeCall(vm, function), function, MacroAssemblerCodeRef::createSelfManagedCodeRef(ctiNativeConstruct(vm)), constructor, NoIntrinsic);
    weakAdd(*m_hostFunctionStubMap, std::make_pair(function, constructor), PassWeak<NativeExecutable>(nativeExecutable));
    return nativeExecutable;
}

NativeExecutable* JITThunks::hostFunctionStub(VM* vm, NativeFunction function, ThunkGenerator generator, Intrinsic intrinsic)
{
    if (NativeExecutable* nativeExecutable = m_hostFunctionStubMap->get(std::make_pair(function, &callHostFunctionAsConstructor)))
        return nativeExecutable;

    MacroAssemblerCodeRef code;
    if (generator) {
        if (vm->canUseJIT())
            code = generator(vm);
        else
            code = MacroAssemblerCodeRef();
    } else
        code = JIT::compileCTINativeCall(vm, function);

    NativeExecutable* nativeExecutable = NativeExecutable::create(*vm, code, function, MacroAssemblerCodeRef::createSelfManagedCodeRef(ctiNativeConstruct(vm)), callHostFunctionAsConstructor, intrinsic);
    weakAdd(*m_hostFunctionStubMap, std::make_pair(function, &callHostFunctionAsConstructor), PassWeak<NativeExecutable>(nativeExecutable));
    return nativeExecutable;
}

void JITThunks::clearHostFunctionStubs()
{
    m_hostFunctionStubMap.clear();
}

} // namespace JSC

#endif // ENABLE(JIT)
