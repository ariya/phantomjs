/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Cameron Zwarich (cwzwarich@uwaterloo.ca)
 *  Copyright (C) 2007 Maks Orlovich
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

#ifndef Arguments_h
#define Arguments_h

#include "JSActivation.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "Interpreter.h"
#include "ObjectConstructor.h"

namespace JSC {

    struct ArgumentsData {
        WTF_MAKE_NONCOPYABLE(ArgumentsData); WTF_MAKE_FAST_ALLOCATED;
    public:
        ArgumentsData() { }
        WriteBarrier<JSActivation> activation;

        unsigned numParameters;
        ptrdiff_t firstParameterIndex;
        unsigned numArguments;

        WriteBarrier<Unknown>* registers;
        OwnArrayPtr<WriteBarrier<Unknown> > registerArray;

        WriteBarrier<Unknown>* extraArguments;
        OwnArrayPtr<bool> deletedArguments;
        WriteBarrier<Unknown> extraArgumentsFixedBuffer[4];

        WriteBarrier<JSFunction> callee;
        bool overrodeLength : 1;
        bool overrodeCallee : 1;
        bool overrodeCaller : 1;
        bool isStrictMode : 1;
    };


    class Arguments : public JSNonFinalObject {
    public:
        // Use an enum because otherwise gcc insists on doing a memory
        // read.
        enum { MaxArguments = 0x10000 };

        enum NoParametersType { NoParameters };

        Arguments(CallFrame*);
        Arguments(CallFrame*, NoParametersType);
        virtual ~Arguments();

        static const ClassInfo s_info;

        virtual void visitChildren(SlotVisitor&);

        void fillArgList(ExecState*, MarkedArgumentBuffer&);

        uint32_t numProvidedArguments(ExecState* exec) const 
        {
            if (UNLIKELY(d->overrodeLength))
                return get(exec, exec->propertyNames().length).toUInt32(exec);
            return d->numArguments; 
        }
        
        void copyToRegisters(ExecState* exec, Register* buffer, uint32_t maxSize);
        void copyRegisters(JSGlobalData&);
        bool isTornOff() const { return d->registerArray; }
        void setActivation(JSGlobalData& globalData, JSActivation* activation)
        {
            ASSERT(!d->registerArray);
            d->activation.set(globalData, this, activation);
            d->registers = &activation->registerAt(0);
        }

        static Structure* createStructure(JSGlobalData& globalData, JSValue prototype) 
        { 
            return Structure::create(globalData, prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info); 
        }

    protected:
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesVisitChildren | OverridesGetPropertyNames | JSObject::StructureFlags;

    private:
        void getArgumentsData(CallFrame*, JSFunction*&, ptrdiff_t& firstParameterIndex, Register*& argv, int& argc);
        virtual bool getOwnPropertySlot(ExecState*, const Identifier& propertyName, PropertySlot&);
        virtual bool getOwnPropertySlot(ExecState*, unsigned propertyName, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(ExecState*, const Identifier&, PropertyDescriptor&);
        virtual void getOwnPropertyNames(ExecState*, PropertyNameArray&, EnumerationMode mode = ExcludeDontEnumProperties);
        virtual void put(ExecState*, const Identifier& propertyName, JSValue, PutPropertySlot&);
        virtual void put(ExecState*, unsigned propertyName, JSValue);
        virtual bool deleteProperty(ExecState*, const Identifier& propertyName);
        virtual bool deleteProperty(ExecState*, unsigned propertyName);
        void createStrictModeCallerIfNecessary(ExecState*);
        void createStrictModeCalleeIfNecessary(ExecState*);

        void init(CallFrame*);

        OwnPtr<ArgumentsData> d;
    };

    Arguments* asArguments(JSValue);

    inline Arguments* asArguments(JSValue value)
    {
        ASSERT(asObject(value)->inherits(&Arguments::s_info));
        return static_cast<Arguments*>(asObject(value));
    }

    ALWAYS_INLINE void Arguments::getArgumentsData(CallFrame* callFrame, JSFunction*& function, ptrdiff_t& firstParameterIndex, Register*& argv, int& argc)
    {
        function = asFunction(callFrame->callee());

        int numParameters = function->jsExecutable()->parameterCount();
        argc = callFrame->argumentCountIncludingThis();

        if (argc <= numParameters)
            argv = callFrame->registers() - RegisterFile::CallFrameHeaderSize - numParameters;
        else
            argv = callFrame->registers() - RegisterFile::CallFrameHeaderSize - numParameters - argc;

        argc -= 1; // - 1 to skip "this"
        firstParameterIndex = -RegisterFile::CallFrameHeaderSize - numParameters;
    }

    inline Arguments::Arguments(CallFrame* callFrame)
        : JSNonFinalObject(callFrame->globalData(), callFrame->lexicalGlobalObject()->argumentsStructure())
        , d(adoptPtr(new ArgumentsData))
    {
        ASSERT(inherits(&s_info));

        JSFunction* callee;
        ptrdiff_t firstParameterIndex;
        Register* argv;
        int numArguments;
        getArgumentsData(callFrame, callee, firstParameterIndex, argv, numArguments);

        d->numParameters = callee->jsExecutable()->parameterCount();
        d->firstParameterIndex = firstParameterIndex;
        d->numArguments = numArguments;

        d->registers = reinterpret_cast<WriteBarrier<Unknown>*>(callFrame->registers());

        WriteBarrier<Unknown>* extraArguments;
        if (d->numArguments <= d->numParameters)
            extraArguments = 0;
        else {
            unsigned numExtraArguments = d->numArguments - d->numParameters;
            if (numExtraArguments > sizeof(d->extraArgumentsFixedBuffer) / sizeof(WriteBarrier<Unknown>))
                extraArguments = new WriteBarrier<Unknown>[numExtraArguments];
            else
                extraArguments = d->extraArgumentsFixedBuffer;
            for (unsigned i = 0; i < numExtraArguments; ++i)
                extraArguments[i].set(callFrame->globalData(), this, argv[d->numParameters + i].jsValue());
        }

        d->extraArguments = extraArguments;

        d->callee.set(callFrame->globalData(), this, callee);
        d->overrodeLength = false;
        d->overrodeCallee = false;
        d->overrodeCaller = false;
        d->isStrictMode = callFrame->codeBlock()->isStrictMode();
        if (d->isStrictMode)
            copyRegisters(callFrame->globalData());
    }

    inline Arguments::Arguments(CallFrame* callFrame, NoParametersType)
        : JSNonFinalObject(callFrame->globalData(), callFrame->lexicalGlobalObject()->argumentsStructure())
        , d(adoptPtr(new ArgumentsData))
    {
        ASSERT(inherits(&s_info));
        ASSERT(!asFunction(callFrame->callee())->jsExecutable()->parameterCount());

        unsigned numArguments = callFrame->argumentCount();

        d->numParameters = 0;
        d->numArguments = numArguments;

        WriteBarrier<Unknown>* extraArguments;
        if (numArguments > sizeof(d->extraArgumentsFixedBuffer) / sizeof(Register))
            extraArguments = new WriteBarrier<Unknown>[numArguments];
        else
            extraArguments = d->extraArgumentsFixedBuffer;

        Register* argv = callFrame->registers() - RegisterFile::CallFrameHeaderSize - numArguments - 1;
        for (unsigned i = 0; i < numArguments; ++i)
            extraArguments[i].set(callFrame->globalData(), this, argv[i].jsValue());

        d->extraArguments = extraArguments;

        d->callee.set(callFrame->globalData(), this, asFunction(callFrame->callee()));
        d->overrodeLength = false;
        d->overrodeCallee = false;
        d->overrodeCaller = false;
        d->isStrictMode = callFrame->codeBlock()->isStrictMode();
        if (d->isStrictMode)
            copyRegisters(callFrame->globalData());
    }

    inline void Arguments::copyRegisters(JSGlobalData& globalData)
    {
        ASSERT(!isTornOff());

        if (!d->numParameters)
            return;

        int registerOffset = d->numParameters + RegisterFile::CallFrameHeaderSize;
        size_t registerArraySize = d->numParameters;

        OwnArrayPtr<WriteBarrier<Unknown> > registerArray = adoptArrayPtr(new WriteBarrier<Unknown>[registerArraySize]);
        for (size_t i = 0; i < registerArraySize; i++)
            registerArray[i].set(globalData, this, d->registers[i - registerOffset].get());
        d->registers = registerArray.get() + registerOffset;
        d->registerArray = registerArray.release();
    }

    // This JSActivation function is defined here so it can get at Arguments::setRegisters.
    inline void JSActivation::copyRegisters(JSGlobalData& globalData)
    {
        ASSERT(!m_registerArray);

        size_t numLocals = m_numCapturedVars + m_numParametersMinusThis;

        if (!numLocals)
            return;

        int registerOffset = m_numParametersMinusThis + RegisterFile::CallFrameHeaderSize;
        size_t registerArraySize = numLocals + RegisterFile::CallFrameHeaderSize;

        OwnArrayPtr<WriteBarrier<Unknown> > registerArray = copyRegisterArray(globalData, m_registers - registerOffset, registerArraySize, m_numParametersMinusThis + 1);
        WriteBarrier<Unknown>* registers = registerArray.get() + registerOffset;
        setRegisters(registers, registerArray.release());
    }

} // namespace JSC

#endif // Arguments_h
