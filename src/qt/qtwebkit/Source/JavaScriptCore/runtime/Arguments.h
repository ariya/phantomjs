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

#include "CodeOrigin.h"
#include "JSActivation.h"
#include "JSDestructibleObject.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "Interpreter.h"
#include "ObjectConstructor.h"

namespace JSC {

class Arguments : public JSDestructibleObject {
    friend class JIT;
    friend class DFG::SpeculativeJIT;
public:
    typedef JSDestructibleObject Base;

    static Arguments* create(VM& vm, CallFrame* callFrame)
    {
        Arguments* arguments = new (NotNull, allocateCell<Arguments>(vm.heap)) Arguments(callFrame);
        arguments->finishCreation(callFrame);
        return arguments;
    }
        
    static Arguments* create(VM& vm, CallFrame* callFrame, InlineCallFrame* inlineCallFrame)
    {
        Arguments* arguments = new (NotNull, allocateCell<Arguments>(vm.heap)) Arguments(callFrame);
        arguments->finishCreation(callFrame, inlineCallFrame);
        return arguments;
    }

    enum { MaxArguments = 0x10000 };

private:
    enum NoParametersType { NoParameters };
        
    Arguments(CallFrame*);
    Arguments(CallFrame*, NoParametersType);
        
    void tearOffForInlineCallFrame(VM& vm, Register*, InlineCallFrame*);

public:
    static const ClassInfo s_info;

    static void visitChildren(JSCell*, SlotVisitor&);

    void fillArgList(ExecState*, MarkedArgumentBuffer&);

    uint32_t length(ExecState* exec) const 
    {
        if (UNLIKELY(m_overrodeLength))
            return get(exec, exec->propertyNames().length).toUInt32(exec);
        return m_numArguments; 
    }
        
    void copyToArguments(ExecState*, CallFrame*, uint32_t length);
    void tearOff(CallFrame*);
    void tearOff(CallFrame*, InlineCallFrame*);
    bool isTornOff() const { return m_registerArray; }
    void didTearOffActivation(ExecState*, JSActivation*);

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype) 
    { 
        return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info); 
    }
        
protected:
    static const unsigned StructureFlags = OverridesGetOwnPropertySlot | InterceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero | OverridesVisitChildren | OverridesGetPropertyNames | JSObject::StructureFlags;

    void finishCreation(CallFrame*);
    void finishCreation(CallFrame*, InlineCallFrame*);

private:
    static void destroy(JSCell*);
    static bool getOwnPropertySlot(JSCell*, ExecState*, PropertyName, PropertySlot&);
    static bool getOwnPropertySlotByIndex(JSCell*, ExecState*, unsigned propertyName, PropertySlot&);
    static bool getOwnPropertyDescriptor(JSObject*, ExecState*, PropertyName, PropertyDescriptor&);
    static void getOwnPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
    static void put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&);
    static void putByIndex(JSCell*, ExecState*, unsigned propertyName, JSValue, bool shouldThrow);
    static bool deleteProperty(JSCell*, ExecState*, PropertyName);
    static bool deletePropertyByIndex(JSCell*, ExecState*, unsigned propertyName);
    static bool defineOwnProperty(JSObject*, ExecState*, PropertyName, PropertyDescriptor&, bool shouldThrow);
    void createStrictModeCallerIfNecessary(ExecState*);
    void createStrictModeCalleeIfNecessary(ExecState*);

    bool isArgument(size_t);
    bool trySetArgument(VM&, size_t argument, JSValue);
    JSValue tryGetArgument(size_t argument);
    bool isDeletedArgument(size_t);
    bool tryDeleteArgument(size_t);
    WriteBarrierBase<Unknown>& argument(size_t);
    void allocateSlowArguments();

    void init(CallFrame*);

    WriteBarrier<JSActivation> m_activation;

    unsigned m_numArguments;

    // We make these full byte booleans to make them easy to test from the JIT,
    // and because even if they were single-bit booleans we still wouldn't save
    // any space.
    bool m_overrodeLength; 
    bool m_overrodeCallee;
    bool m_overrodeCaller;
    bool m_isStrictMode;

    WriteBarrierBase<Unknown>* m_registers;
    OwnArrayPtr<WriteBarrier<Unknown> > m_registerArray;

    OwnArrayPtr<SlowArgument> m_slowArguments;

    WriteBarrier<JSFunction> m_callee;
};

Arguments* asArguments(JSValue);

inline Arguments* asArguments(JSValue value)
{
    ASSERT(asObject(value)->inherits(&Arguments::s_info));
    return static_cast<Arguments*>(asObject(value));
}

inline Arguments::Arguments(CallFrame* callFrame)
    : JSDestructibleObject(callFrame->vm(), callFrame->lexicalGlobalObject()->argumentsStructure())
{
}

inline Arguments::Arguments(CallFrame* callFrame, NoParametersType)
    : JSDestructibleObject(callFrame->vm(), callFrame->lexicalGlobalObject()->argumentsStructure())
{
}

inline void Arguments::allocateSlowArguments()
{
    if (m_slowArguments)
        return;
    m_slowArguments = adoptArrayPtr(new SlowArgument[m_numArguments]);
    for (size_t i = 0; i < m_numArguments; ++i) {
        ASSERT(m_slowArguments[i].status == SlowArgument::Normal);
        m_slowArguments[i].index = CallFrame::argumentOffset(i);
    }
}

inline bool Arguments::tryDeleteArgument(size_t argument)
{
    if (!isArgument(argument))
        return false;
    allocateSlowArguments();
    m_slowArguments[argument].status = SlowArgument::Deleted;
    return true;
}

inline bool Arguments::trySetArgument(VM& vm, size_t argument, JSValue value)
{
    if (!isArgument(argument))
        return false;
    this->argument(argument).set(vm, this, value);
    return true;
}

inline JSValue Arguments::tryGetArgument(size_t argument)
{
    if (!isArgument(argument))
        return JSValue();
    return this->argument(argument).get();
}

inline bool Arguments::isDeletedArgument(size_t argument)
{
    if (argument >= m_numArguments)
        return false;
    if (!m_slowArguments)
        return false;
    if (m_slowArguments[argument].status != SlowArgument::Deleted)
        return false;
    return true;
}

inline bool Arguments::isArgument(size_t argument)
{
    if (argument >= m_numArguments)
        return false;
    if (m_slowArguments && m_slowArguments[argument].status == SlowArgument::Deleted)
        return false;
    return true;
}

inline WriteBarrierBase<Unknown>& Arguments::argument(size_t argument)
{
    ASSERT(isArgument(argument));
    if (!m_slowArguments)
        return m_registers[CallFrame::argumentOffset(argument)];

    int index = m_slowArguments[argument].index;
    if (!m_activation || m_slowArguments[argument].status != SlowArgument::Captured)
        return m_registers[index];

    return m_activation->registerAt(index);
}

inline void Arguments::finishCreation(CallFrame* callFrame)
{
    Base::finishCreation(callFrame->vm());
    ASSERT(inherits(&s_info));

    JSFunction* callee = jsCast<JSFunction*>(callFrame->callee());
    m_numArguments = callFrame->argumentCount();
    m_registers = reinterpret_cast<WriteBarrierBase<Unknown>*>(callFrame->registers());
    m_callee.set(callFrame->vm(), this, callee);
    m_overrodeLength = false;
    m_overrodeCallee = false;
    m_overrodeCaller = false;
    m_isStrictMode = callFrame->codeBlock()->isStrictMode();

    SharedSymbolTable* symbolTable = callFrame->codeBlock()->symbolTable();
    const SlowArgument* slowArguments = symbolTable->slowArguments();
    if (slowArguments) {
        allocateSlowArguments();
        size_t count = std::min<unsigned>(m_numArguments, symbolTable->parameterCount());
        for (size_t i = 0; i < count; ++i)
            m_slowArguments[i] = slowArguments[i];
    }

    // The bytecode generator omits op_tear_off_activation in cases of no
    // declared parameters, so we need to tear off immediately.
    if (m_isStrictMode || !callee->jsExecutable()->parameterCount())
        tearOff(callFrame);
}

inline void Arguments::finishCreation(CallFrame* callFrame, InlineCallFrame* inlineCallFrame)
{
    Base::finishCreation(callFrame->vm());
    ASSERT(inherits(&s_info));

    JSFunction* callee = inlineCallFrame->calleeForCallFrame(callFrame);
    m_numArguments = inlineCallFrame->arguments.size() - 1;
    m_registers = reinterpret_cast<WriteBarrierBase<Unknown>*>(callFrame->registers()) + inlineCallFrame->stackOffset;
    m_callee.set(callFrame->vm(), this, callee);
    m_overrodeLength = false;
    m_overrodeCallee = false;
    m_overrodeCaller = false;
    m_isStrictMode = jsCast<FunctionExecutable*>(inlineCallFrame->executable.get())->isStrictMode();
    ASSERT(!jsCast<FunctionExecutable*>(inlineCallFrame->executable.get())->symbolTable(inlineCallFrame->isCall ? CodeForCall : CodeForConstruct)->slowArguments());

    // The bytecode generator omits op_tear_off_activation in cases of no
    // declared parameters, so we need to tear off immediately.
    if (m_isStrictMode || !callee->jsExecutable()->parameterCount())
        tearOff(callFrame, inlineCallFrame);
}

} // namespace JSC

#endif // Arguments_h
