/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2007, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef CallFrame_h
#define CallFrame_h

#include "JSGlobalData.h"
#include "MacroAssemblerCodeRef.h"
#include "RegisterFile.h"

namespace JSC  {

    class Arguments;
    class JSActivation;
    class Interpreter;
    class ScopeChainNode;

    // Represents the current state of script execution.
    // Passed as the first argument to most functions.
    class ExecState : private Register {
    public:
        JSObject* callee() const { return this[RegisterFile::Callee].function(); }
        CodeBlock* codeBlock() const { return this[RegisterFile::CodeBlock].Register::codeBlock(); }
        ScopeChainNode* scopeChain() const
        {
            ASSERT(this[RegisterFile::ScopeChain].Register::scopeChain());
            return this[RegisterFile::ScopeChain].Register::scopeChain();
        }

        // Global object in which execution began.
        JSGlobalObject* dynamicGlobalObject();

        // Global object in which the currently executing code was defined.
        // Differs from dynamicGlobalObject() during function calls across web browser frames.
        inline JSGlobalObject* lexicalGlobalObject() const;

        // Differs from lexicalGlobalObject because this will have DOM window shell rather than
        // the actual DOM window, which can't be "this" for security reasons.
        inline JSObject* globalThisValue() const;

        inline JSGlobalData& globalData() const;

        // Convenience functions for access to global data.
        // It takes a few memory references to get from a call frame to the global data
        // pointer, so these are inefficient, and should be used sparingly in new code.
        // But they're used in many places in legacy code, so they're not going away any time soon.

        void clearException() { globalData().exception = JSValue(); }
        JSValue exception() const { return globalData().exception; }
        bool hadException() const { return globalData().exception; }

        const CommonIdentifiers& propertyNames() const { return *globalData().propertyNames; }
        const MarkedArgumentBuffer& emptyList() const { return *globalData().emptyList; }
        Interpreter* interpreter() { return globalData().interpreter; }
        Heap* heap() { return &globalData().heap; }
#ifndef NDEBUG
        void dumpCaller();
#endif
        static const HashTable* arrayTable(CallFrame* callFrame) { return callFrame->globalData().arrayTable; }
        static const HashTable* dateTable(CallFrame* callFrame) { return callFrame->globalData().dateTable; }
        static const HashTable* jsonTable(CallFrame* callFrame) { return callFrame->globalData().jsonTable; }
        static const HashTable* mathTable(CallFrame* callFrame) { return callFrame->globalData().mathTable; }
        static const HashTable* numberTable(CallFrame* callFrame) { return callFrame->globalData().numberTable; }
        static const HashTable* objectConstructorTable(CallFrame* callFrame) { return callFrame->globalData().objectConstructorTable; }
        static const HashTable* regExpTable(CallFrame* callFrame) { return callFrame->globalData().regExpTable; }
        static const HashTable* regExpConstructorTable(CallFrame* callFrame) { return callFrame->globalData().regExpConstructorTable; }
        static const HashTable* stringTable(CallFrame* callFrame) { return callFrame->globalData().stringTable; }

        static CallFrame* create(Register* callFrameBase) { return static_cast<CallFrame*>(callFrameBase); }
        Register* registers() { return this; }

        CallFrame& operator=(const Register& r) { *static_cast<Register*>(this) = r; return *this; }

        CallFrame* callerFrame() const { return this[RegisterFile::CallerFrame].callFrame(); }
#if ENABLE(JIT)
        ReturnAddressPtr returnPC() const { return ReturnAddressPtr(this[RegisterFile::ReturnPC].vPC()); }
#endif
#if ENABLE(INTERPRETER)
        Instruction* returnVPC() const { return this[RegisterFile::ReturnPC].vPC(); }
#endif

        void setCallerFrame(CallFrame* callerFrame) { static_cast<Register*>(this)[RegisterFile::CallerFrame] = callerFrame; }
        void setScopeChain(ScopeChainNode* scopeChain) { static_cast<Register*>(this)[RegisterFile::ScopeChain] = scopeChain; }

        ALWAYS_INLINE void init(CodeBlock* codeBlock, Instruction* vPC, ScopeChainNode* scopeChain,
            CallFrame* callerFrame, int argc, JSObject* callee)
        {
            ASSERT(callerFrame); // Use noCaller() rather than 0 for the outer host call frame caller.
            ASSERT(callerFrame == noCaller() || callerFrame->removeHostCallFrameFlag()->registerFile()->end() >= this);

            setCodeBlock(codeBlock);
            setScopeChain(scopeChain);
            setCallerFrame(callerFrame);
            setReturnPC(vPC); // This is either an Instruction* or a pointer into JIT generated code stored as an Instruction*.
            setArgumentCountIncludingThis(argc); // original argument count (for the sake of the "arguments" object)
            setCallee(callee);
        }

        // Read a register from the codeframe (or constant from the CodeBlock).
        inline Register& r(int);
        // Read a register for a non-constant 
        inline Register& uncheckedR(int);

        // Access to arguments.
        int hostThisRegister() { return -RegisterFile::CallFrameHeaderSize - argumentCountIncludingThis(); }
        JSValue hostThisValue() { return this[hostThisRegister()].jsValue(); }
        size_t argumentCount() const { return argumentCountIncludingThis() - 1; }
        size_t argumentCountIncludingThis() const { return this[RegisterFile::ArgumentCount].i(); }
        JSValue argument(int argumentNumber)
        {
            int argumentIndex = -RegisterFile::CallFrameHeaderSize - this[RegisterFile::ArgumentCount].i() + argumentNumber + 1;
            if (argumentIndex >= -RegisterFile::CallFrameHeaderSize)
                return jsUndefined();
            return this[argumentIndex].jsValue();
        }

        static CallFrame* noCaller() { return reinterpret_cast<CallFrame*>(HostCallFrameFlag); }

        bool hasHostCallFrameFlag() const { return reinterpret_cast<intptr_t>(this) & HostCallFrameFlag; }
        CallFrame* addHostCallFrameFlag() const { return reinterpret_cast<CallFrame*>(reinterpret_cast<intptr_t>(this) | HostCallFrameFlag); }
        CallFrame* removeHostCallFrameFlag() { return reinterpret_cast<CallFrame*>(reinterpret_cast<intptr_t>(this) & ~HostCallFrameFlag); }

        void setArgumentCountIncludingThis(int count) { static_cast<Register*>(this)[RegisterFile::ArgumentCount] = Register::withInt(count); }
        void setCallee(JSObject* callee) { static_cast<Register*>(this)[RegisterFile::Callee] = Register::withCallee(callee); }
        void setCodeBlock(CodeBlock* codeBlock) { static_cast<Register*>(this)[RegisterFile::CodeBlock] = codeBlock; }
        void setReturnPC(void* value) { static_cast<Register*>(this)[RegisterFile::ReturnPC] = (Instruction*)value; }

    private:
        static const intptr_t HostCallFrameFlag = 1;
#ifndef NDEBUG
        RegisterFile* registerFile();
#endif
        ExecState();
        ~ExecState();
    };

} // namespace JSC

#endif // CallFrame_h
