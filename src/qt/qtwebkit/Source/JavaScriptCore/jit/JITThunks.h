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

#ifndef JITThunks_h
#define JITThunks_h

#include <wtf/Platform.h>

#if ENABLE(JIT)

#include "CallData.h"
#include "Intrinsic.h"
#include "LowLevelInterpreter.h"
#include "MacroAssemblerCodeRef.h"
#include "ThunkGenerator.h"
#include "Weak.h"
#include "WeakInlines.h"
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefPtr.h>

namespace JSC {

class VM;
class NativeExecutable;

class JITThunks {
public:
    JITThunks();
    ~JITThunks();

    MacroAssemblerCodePtr ctiNativeCall(VM*);
    MacroAssemblerCodePtr ctiNativeConstruct(VM*);

    MacroAssemblerCodeRef ctiStub(VM*, ThunkGenerator);

    NativeExecutable* hostFunctionStub(VM*, NativeFunction, NativeFunction constructor);
    NativeExecutable* hostFunctionStub(VM*, NativeFunction, ThunkGenerator, Intrinsic);

    void clearHostFunctionStubs();

private:
    typedef HashMap<ThunkGenerator, MacroAssemblerCodeRef> CTIStubMap;
    CTIStubMap m_ctiStubMap;
    typedef HashMap<std::pair<NativeFunction, NativeFunction>, Weak<NativeExecutable> > HostFunctionStubMap;
    OwnPtr<HostFunctionStubMap> m_hostFunctionStubMap;
};

} // namespace JSC

#endif // ENABLE(JIT)

#endif // JITThunks_h

