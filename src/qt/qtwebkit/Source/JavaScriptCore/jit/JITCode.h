/*
 * Copyright (C) 2008, 2012 Apple Inc. All rights reserved.
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

#ifndef JITCode_h
#define JITCode_h

#if ENABLE(JIT) || ENABLE(LLINT)
#include "CallFrame.h"
#include "Disassembler.h"
#include "JITStubs.h"
#include "JSCJSValue.h"
#include "LegacyProfiler.h"
#include "MacroAssemblerCodeRef.h"
#endif

namespace JSC {

#if ENABLE(JIT)
    class VM;
    class JSStack;
#endif
    
    class JITCode {
#if ENABLE(JIT) || ENABLE(LLINT)
        typedef MacroAssemblerCodeRef CodeRef;
        typedef MacroAssemblerCodePtr CodePtr;
#else
        JITCode() { }
#endif
    public:
        enum JITType { None, HostCallThunk, InterpreterThunk, BaselineJIT, DFGJIT };
        
        static JITType bottomTierJIT()
        {
            return BaselineJIT;
        }
        
        static JITType topTierJIT()
        {
            return DFGJIT;
        }
        
        static JITType nextTierJIT(JITType jitType)
        {
            ASSERT_UNUSED(jitType, jitType == BaselineJIT || jitType == DFGJIT);
            return DFGJIT;
        }
        
        static bool isOptimizingJIT(JITType jitType)
        {
            return jitType == DFGJIT;
        }
        
        static bool isBaselineCode(JITType jitType)
        {
            return jitType == InterpreterThunk || jitType == BaselineJIT;
        }
        
#if ENABLE(JIT) || ENABLE(LLINT)
        JITCode()
            : m_jitType(None)
        {
        }

        JITCode(const CodeRef ref, JITType jitType)
            : m_ref(ref)
            , m_jitType(jitType)
        {
            ASSERT(jitType != None);
        }
        
        bool operator !() const
        {
            return !m_ref;
        }

        CodePtr addressForCall()
        {
            return m_ref.code();
        }

        void* executableAddressAtOffset(size_t offset) const
        {
            ASSERT(offset < size());
            return reinterpret_cast<char*>(m_ref.code().executableAddress()) + offset;
        }
        
        void* executableAddress() const
        {
            return executableAddressAtOffset(0);
        }
        
        void* dataAddressAtOffset(size_t offset) const
        {
            ASSERT(offset <= size()); // use <= instead of < because it is valid to ask for an address at the exclusive end of the code.
            return reinterpret_cast<char*>(m_ref.code().dataLocation()) + offset;
        }

        // This function returns the offset in bytes of 'pointerIntoCode' into
        // this block of code.  The pointer provided must be a pointer into this
        // block of code.  It is ASSERTed that no codeblock >4gb in size.
        unsigned offsetOf(void* pointerIntoCode)
        {
            intptr_t result = reinterpret_cast<intptr_t>(pointerIntoCode) - reinterpret_cast<intptr_t>(m_ref.code().executableAddress());
            ASSERT(static_cast<intptr_t>(static_cast<unsigned>(result)) == result);
            return static_cast<unsigned>(result);
        }

#if ENABLE(JIT)
        // Execute the code!
        inline JSValue execute(JSStack* stack, CallFrame* callFrame, VM* vm)
        {
            JSValue result = JSValue::decode(ctiTrampoline(m_ref.code().executableAddress(), stack, callFrame, 0, 0, vm));
            return vm->exception ? jsNull() : result;
        }
#endif

        void* start() const
        {
            return m_ref.code().dataLocation();
        }

        size_t size() const
        {
            ASSERT(m_ref.code().executableAddress());
            return m_ref.size();
        }
        
        bool tryToDisassemble(const char* prefix) const
        {
            return m_ref.tryToDisassemble(prefix);
        }

        ExecutableMemoryHandle* getExecutableMemory()
        {
            return m_ref.executableMemory();
        }
        
        JITType jitType() const
        {
            return m_jitType;
        }

        // Host functions are a bit special; they have a m_code pointer but they
        // do not individully ref the executable pool containing the trampoline.
        static JITCode HostFunction(CodeRef code)
        {
            return JITCode(code, HostCallThunk);
        }

        void clear()
        {
            m_ref.~CodeRef();
            new (NotNull, &m_ref) CodeRef();
        }

    private:
        JITCode(PassRefPtr<ExecutableMemoryHandle> executableMemory, JITType jitType)
            : m_ref(executableMemory)
            , m_jitType(jitType)
        {
        }

        CodeRef m_ref;
        JITType m_jitType;
#endif // ENABLE(JIT) || ENABLE(LLINT)
    };

} // namespace JSC

namespace WTF {

class PrintStream;
void printInternal(PrintStream&, JSC::JITCode::JITType);

} // namespace WTF

#endif
