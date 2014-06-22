/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef JITStubCall_h
#define JITStubCall_h

#include "MacroAssemblerCodeRef.h"

#if ENABLE(JIT)

namespace JSC {

    class JITStubCall {
    public:
        JITStubCall(JIT* jit, JSObject* (JIT_STUB *stub)(STUB_ARGS_DECLARATION))
            : m_jit(jit)
            , m_stub(stub)
#if USE(JSVALUE32_64) || !ASSERT_DISABLED
            , m_returnType(Cell)
#endif
            , m_stackIndex(JITSTACKFRAME_ARGS_INDEX)
        {
        }

        JITStubCall(JIT* jit, JSPropertyNameIterator* (JIT_STUB *stub)(STUB_ARGS_DECLARATION))
            : m_jit(jit)
            , m_stub(stub)
#if USE(JSVALUE32_64) || !ASSERT_DISABLED
            , m_returnType(Cell)
#endif
            , m_stackIndex(JITSTACKFRAME_ARGS_INDEX)
        {
        }

        JITStubCall(JIT* jit, void* (JIT_STUB *stub)(STUB_ARGS_DECLARATION))
            : m_jit(jit)
            , m_stub(stub)
#if USE(JSVALUE32_64) || !ASSERT_DISABLED      
            , m_returnType(VoidPtr)
#endif
            , m_stackIndex(JITSTACKFRAME_ARGS_INDEX)
        {
        }

        JITStubCall(JIT* jit, int (JIT_STUB *stub)(STUB_ARGS_DECLARATION))
            : m_jit(jit)
            , m_stub(stub)
#if USE(JSVALUE32_64) || !ASSERT_DISABLED
            , m_returnType(Int)
#endif
            , m_stackIndex(JITSTACKFRAME_ARGS_INDEX)
        {
        }

        JITStubCall(JIT* jit, bool (JIT_STUB *stub)(STUB_ARGS_DECLARATION))
            : m_jit(jit)
            , m_stub(stub)
#if USE(JSVALUE32_64) || !ASSERT_DISABLED
            , m_returnType(Int)
#endif
            , m_stackIndex(JITSTACKFRAME_ARGS_INDEX)
        {
        }

        JITStubCall(JIT* jit, void (JIT_STUB *stub)(STUB_ARGS_DECLARATION))
            : m_jit(jit)
            , m_stub(stub)
#if USE(JSVALUE32_64) || !ASSERT_DISABLED
            , m_returnType(Void)
#endif
            , m_stackIndex(JITSTACKFRAME_ARGS_INDEX)
        {
        }

        JITStubCall(JIT* jit, EncodedJSValue (JIT_STUB *stub)(STUB_ARGS_DECLARATION))
            : m_jit(jit)
            , m_stub(stub)
#if USE(JSVALUE32_64) || !ASSERT_DISABLED
            , m_returnType(Value)
#endif
            , m_stackIndex(JITSTACKFRAME_ARGS_INDEX)
        {
        }

        // Arguments are added first to last.

        void skipArgument()
        {
            m_stackIndex += stackIndexStep;
        }

        void addArgument(JIT::TrustedImm32 argument)
        {
            m_jit->poke(argument, m_stackIndex);
            m_stackIndex += stackIndexStep;
        }
        
        void addArgument(JIT::Imm32 argument)
        {
            m_jit->poke(argument, m_stackIndex);
            m_stackIndex += stackIndexStep;
        }

        void addArgument(JIT::TrustedImmPtr argument)
        {
            m_jit->poke(argument, m_stackIndex);
            m_stackIndex += stackIndexStep;
        }
        
        void addArgument(JIT::ImmPtr argument)
        {
            m_jit->poke(argument, m_stackIndex);
            m_stackIndex += stackIndexStep;
        }

        void addArgument(JIT::RegisterID argument)
        {
#if USE(JSVALUE32_64)
            m_jit->poke(argument, m_stackIndex);
#else
            m_jit->poke64(argument, m_stackIndex);
#endif
            m_stackIndex += stackIndexStep;
        }
        
#if USE(JSVALUE32_64)
        void addArgument(const JSValue& value)
        {
            m_jit->poke(JIT::Imm32(value.payload()), m_stackIndex);
            m_jit->poke(JIT::Imm32(value.tag()), m_stackIndex + 1);
            m_stackIndex += stackIndexStep;
        }
#else
        void addArgument(JIT::TrustedImm64 argument)
        {
            m_jit->poke(argument, m_stackIndex);
            m_stackIndex += stackIndexStep;
        }

        void addArgument(JIT::Imm64 argument)
        {
            m_jit->poke(argument, m_stackIndex);
            m_stackIndex += stackIndexStep;
        }
#endif

        void addArgument(JIT::RegisterID tag, JIT::RegisterID payload)
        {
            m_jit->poke(payload, m_stackIndex);
            m_jit->poke(tag, m_stackIndex + 1);
            m_stackIndex += stackIndexStep;
        }

#if USE(JSVALUE32_64)
        void addArgument(unsigned srcVirtualRegister)
        {
            if (m_jit->m_codeBlock->isConstantRegisterIndex(srcVirtualRegister)) {
                addArgument(m_jit->getConstantOperand(srcVirtualRegister));
                return;
            }

            m_jit->emitLoad(srcVirtualRegister, JIT::regT1, JIT::regT0);
            addArgument(JIT::regT1, JIT::regT0);
        }

        void getArgument(size_t argumentNumber, JIT::RegisterID tag, JIT::RegisterID payload)
        {
            size_t stackIndex = JITSTACKFRAME_ARGS_INDEX + (argumentNumber * stackIndexStep);
            m_jit->peek(payload, stackIndex);
            m_jit->peek(tag, stackIndex + 1);
        }
#else
        void addArgument(unsigned src, JIT::RegisterID scratchRegister) // src is a virtual register.
        {
            if (m_jit->m_codeBlock->isConstantRegisterIndex(src))
                addArgument(JIT::Imm64(JSValue::encode(m_jit->m_codeBlock->getConstant(src))));
            else {
                m_jit->load64(JIT::Address(JIT::callFrameRegister, src * sizeof(Register)), scratchRegister);
                addArgument(scratchRegister);
            }
            m_jit->killLastResultRegister();
        }
#endif

        JIT::Call call()
        {
#if ENABLE(OPCODE_SAMPLING)
            if (m_jit->m_bytecodeOffset != (unsigned)-1)
                m_jit->sampleInstruction(m_jit->m_codeBlock->instructions().begin() + m_jit->m_bytecodeOffset, true);
#endif

            m_jit->restoreArgumentReference();
            m_jit->updateTopCallFrame();
            JIT::Call call = m_jit->call();
            m_jit->m_calls.append(CallRecord(call, m_jit->m_bytecodeOffset, m_stub.value()));

#if ENABLE(OPCODE_SAMPLING)
            if (m_jit->m_bytecodeOffset != (unsigned)-1)
                m_jit->sampleInstruction(m_jit->m_codeBlock->instructions().begin() + m_jit->m_bytecodeOffset, false);
#endif

#if USE(JSVALUE32_64)
            m_jit->unmap();
#else
            m_jit->killLastResultRegister();
#endif
            return call;
        }

#if USE(JSVALUE32_64)
        JIT::Call call(unsigned dst) // dst is a virtual register.
        {
            ASSERT(m_returnType == Value || m_returnType == Cell);
            JIT::Call call = this->call();
            if (m_returnType == Value)
                m_jit->emitStore(dst, JIT::regT1, JIT::regT0);
            else
                m_jit->emitStoreCell(dst, JIT::returnValueRegister);
            return call;
        }
        
        JIT::Call callWithValueProfiling(unsigned dst)
        {
            ASSERT(m_returnType == Value || m_returnType == Cell);
            JIT::Call call = this->call();
            ASSERT(JIT::returnValueRegister == JIT::regT0);
            if (m_returnType == Cell)
                m_jit->move(JIT::TrustedImm32(JSValue::CellTag), JIT::regT1);
            m_jit->emitValueProfilingSite();
            if (m_returnType == Value)
                m_jit->emitStore(dst, JIT::regT1, JIT::regT0);
            else
                m_jit->emitStoreCell(dst, JIT::returnValueRegister);
            return call;
        }
#else
        JIT::Call call(unsigned dst) // dst is a virtual register.
        {
            ASSERT(m_returnType == Value || m_returnType == Cell);
            JIT::Call call = this->call();
            m_jit->emitPutVirtualRegister(dst);
            return call;
        }
        
        JIT::Call callWithValueProfiling(unsigned dst)
        {
            ASSERT(m_returnType == Value || m_returnType == Cell);
            JIT::Call call = this->call();
            ASSERT(JIT::returnValueRegister == JIT::regT0);
            m_jit->emitValueProfilingSite();
            m_jit->emitPutVirtualRegister(dst);
            return call;
        }
#endif

        JIT::Call call(JIT::RegisterID dst) // dst is a machine register.
        {
#if USE(JSVALUE32_64) || !ASSERT_DISABLED
            ASSERT(m_returnType == Value || m_returnType == VoidPtr || m_returnType == Int || m_returnType == Cell);
#endif
            JIT::Call call = this->call();
            if (dst != JIT::returnValueRegister)
                m_jit->move(JIT::returnValueRegister, dst);
            return call;
        }

    private:
        static const size_t stackIndexStep = sizeof(EncodedJSValue) == 2 * sizeof(void*) ? 2 : 1;

        JIT* m_jit;
        FunctionPtr m_stub;
#if USE(JSVALUE32_64) || !ASSERT_DISABLED
        enum { Void, VoidPtr, Int, Value, Cell } m_returnType;
#endif
        size_t m_stackIndex;
    };
}

#endif // ENABLE(JIT)

#endif // JITStubCall_h
