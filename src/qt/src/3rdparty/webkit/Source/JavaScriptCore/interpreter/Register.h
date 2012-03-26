/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Register_h
#define Register_h

#include "JSValue.h"
#include <wtf/Assertions.h>
#include <wtf/VectorTraits.h>

namespace JSC {

    class CodeBlock;
    class ExecState;
    class JSActivation;
    class JSObject;
    class JSPropertyNameIterator;
    class ScopeChainNode;

    struct Instruction;

    typedef ExecState CallFrame;

    class Register {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        Register();

        Register(const JSValue&);
        Register& operator=(const JSValue&);
        JSValue jsValue() const;
        EncodedJSValue encodedJSValue() const;
        
        Register& operator=(CallFrame*);
        Register& operator=(CodeBlock*);
        Register& operator=(ScopeChainNode*);
        Register& operator=(Instruction*);

        int32_t i() const;
        JSActivation* activation() const;
        CallFrame* callFrame() const;
        CodeBlock* codeBlock() const;
        JSObject* function() const;
        JSPropertyNameIterator* propertyNameIterator() const;
        ScopeChainNode* scopeChain() const;
        Instruction* vPC() const;

        static Register withInt(int32_t i)
        {
            Register r = jsNumber(i);
            return r;
        }

        static inline Register withCallee(JSObject* callee);

    private:
        union {
            EncodedJSValue value;
            CallFrame* callFrame;
            CodeBlock* codeBlock;
            Instruction* vPC;
        } u;
    };

    ALWAYS_INLINE Register::Register()
    {
#ifndef NDEBUG
        *this = JSValue();
#endif
    }

    ALWAYS_INLINE Register::Register(const JSValue& v)
    {
#if ENABLE(JSC_ZOMBIES)
        ASSERT(!v.isZombie());
#endif
        u.value = JSValue::encode(v);
    }

    ALWAYS_INLINE Register& Register::operator=(const JSValue& v)
    {
#if ENABLE(JSC_ZOMBIES)
        ASSERT(!v.isZombie());
#endif
        u.value = JSValue::encode(v);
        return *this;
    }

    ALWAYS_INLINE JSValue Register::jsValue() const
    {
        return JSValue::decode(u.value);
    }

    ALWAYS_INLINE EncodedJSValue Register::encodedJSValue() const
    {
        return u.value;
    }

    // Interpreter functions

    ALWAYS_INLINE Register& Register::operator=(CallFrame* callFrame)
    {
        u.callFrame = callFrame;
        return *this;
    }

    ALWAYS_INLINE Register& Register::operator=(CodeBlock* codeBlock)
    {
        u.codeBlock = codeBlock;
        return *this;
    }

    ALWAYS_INLINE Register& Register::operator=(Instruction* vPC)
    {
        u.vPC = vPC;
        return *this;
    }

    ALWAYS_INLINE int32_t Register::i() const
    {
        return jsValue().asInt32();
    }

    ALWAYS_INLINE CallFrame* Register::callFrame() const
    {
        return u.callFrame;
    }
    
    ALWAYS_INLINE CodeBlock* Register::codeBlock() const
    {
        return u.codeBlock;
    }

    ALWAYS_INLINE Instruction* Register::vPC() const
    {
        return u.vPC;
    }

} // namespace JSC

namespace WTF {

    template<> struct VectorTraits<JSC::Register> : VectorTraitsBase<true, JSC::Register> { };

} // namespace WTF

#endif // Register_h
