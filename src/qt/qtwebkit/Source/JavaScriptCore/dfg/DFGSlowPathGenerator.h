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

#ifndef DFGSlowPathGenerator_h
#define DFGSlowPathGenerator_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGCommon.h"
#include "DFGSilentRegisterSavePlan.h"
#include "DFGSpeculativeJIT.h"
#include <wtf/FastAllocBase.h>
#include <wtf/PassOwnPtr.h>

namespace JSC { namespace DFG {

class SlowPathGenerator {
    WTF_MAKE_FAST_ALLOCATED;
public:
    SlowPathGenerator(SpeculativeJIT* jit)
        : m_currentNode(jit->m_currentNode)
    {
    }
    virtual ~SlowPathGenerator() { }
    void generate(SpeculativeJIT* jit)
    {
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLogF("Generating slow path %p at offset 0x%x\n", this, jit->m_jit.debugOffset());
#endif
        m_label = jit->m_jit.label();
        jit->m_currentNode = m_currentNode;
        generateInternal(jit);
#if !ASSERT_DISABLED
        jit->m_jit.breakpoint(); // make sure that the generator jumps back to somewhere
#endif
    }
    MacroAssembler::Label label() const { return m_label; }
    virtual MacroAssembler::Call call() const
    {
        RELEASE_ASSERT_NOT_REACHED(); // By default slow path generators don't have a call.
        return MacroAssembler::Call();
    }
protected:
    virtual void generateInternal(SpeculativeJIT*) = 0;
    MacroAssembler::Label m_label;
    Node* m_currentNode;
};

template<typename JumpType>
class JumpingSlowPathGenerator : public SlowPathGenerator {
public:
    JumpingSlowPathGenerator(JumpType from, SpeculativeJIT* jit)
        : SlowPathGenerator(jit)
        , m_from(from)
        , m_to(jit->m_jit.label())
    {
    }
    
protected:
    void linkFrom(SpeculativeJIT* jit)
    {
        m_from.link(&jit->m_jit);
    }
    
    void jumpTo(SpeculativeJIT* jit)
    {
        jit->m_jit.jump().linkTo(m_to, &jit->m_jit);
    }

    JumpType m_from;
    MacroAssembler::Label m_to;
};

template<typename JumpType, typename FunctionType, typename ResultType>
class CallSlowPathGenerator : public JumpingSlowPathGenerator<JumpType> {
public:
    CallSlowPathGenerator(
        JumpType from, SpeculativeJIT* jit, FunctionType function,
        SpillRegistersMode spillMode, ResultType result)
        : JumpingSlowPathGenerator<JumpType>(from, jit)
        , m_function(function)
        , m_spillMode(spillMode)
        , m_result(result)
    {
        if (m_spillMode == NeedToSpill)
            jit->silentSpillAllRegistersImpl(false, m_plans, result);
    }
    
    virtual MacroAssembler::Call call() const
    {
        return m_call;
    }
    
protected:
    void setUp(SpeculativeJIT* jit)
    {
        this->linkFrom(jit);
        if (m_spillMode == NeedToSpill) {
            for (unsigned i = 0; i < m_plans.size(); ++i)
                jit->silentSpill(m_plans[i]);
        }
    }
    
    void recordCall(MacroAssembler::Call call)
    {
        m_call = call;
    }
    
    void tearDown(SpeculativeJIT* jit)
    {
        if (m_spillMode == NeedToSpill) {
            GPRReg canTrample = SpeculativeJIT::pickCanTrample(m_result);
            for (unsigned i = m_plans.size(); i--;)
                jit->silentFill(m_plans[i], canTrample);
        }
        this->jumpTo(jit);
    }

    FunctionType m_function;
    SpillRegistersMode m_spillMode;
    ResultType m_result;
    MacroAssembler::Call m_call;
    Vector<SilentRegisterSavePlan, 2> m_plans;
};

template<typename JumpType, typename FunctionType, typename ResultType>
class CallResultAndNoArgumentsSlowPathGenerator
    : public CallSlowPathGenerator<JumpType, FunctionType, ResultType> {
public:
    CallResultAndNoArgumentsSlowPathGenerator(
        JumpType from, SpeculativeJIT* jit, FunctionType function,
        SpillRegistersMode spillMode, ResultType result)
        : CallSlowPathGenerator<JumpType, FunctionType, ResultType>(
            from, jit, function, spillMode, result)
    {
    }
    
protected:
    void generateInternal(SpeculativeJIT* jit)
    {
        this->setUp(jit);
        this->recordCall(jit->callOperation(this->m_function, this->m_result));
        this->tearDown(jit);
    }
};

template<
    typename JumpType, typename FunctionType, typename ResultType,
    typename ArgumentType1>
class CallResultAndOneArgumentSlowPathGenerator
    : public CallSlowPathGenerator<JumpType, FunctionType, ResultType> {
public:
    CallResultAndOneArgumentSlowPathGenerator(
        JumpType from, SpeculativeJIT* jit, FunctionType function,
        SpillRegistersMode spillMode, ResultType result, ArgumentType1 argument1)
        : CallSlowPathGenerator<JumpType, FunctionType, ResultType>(
            from, jit, function, spillMode, result)
        , m_argument1(argument1)
    {
    }
    
protected:
    void generateInternal(SpeculativeJIT* jit)
    {
        this->setUp(jit);
        this->recordCall(jit->callOperation(this->m_function, this->m_result, m_argument1));
        this->tearDown(jit);
    }

    ArgumentType1 m_argument1;
};

template<
    typename JumpType, typename FunctionType, typename ResultType,
    typename ArgumentType1, typename ArgumentType2>
class CallResultAndTwoArgumentsSlowPathGenerator
    : public CallSlowPathGenerator<JumpType, FunctionType, ResultType> {
public:
    CallResultAndTwoArgumentsSlowPathGenerator(
        JumpType from, SpeculativeJIT* jit, FunctionType function,
        SpillRegistersMode spillMode, ResultType result, ArgumentType1 argument1,
        ArgumentType2 argument2)
        : CallSlowPathGenerator<JumpType, FunctionType, ResultType>(
            from, jit, function, spillMode, result)
        , m_argument1(argument1)
        , m_argument2(argument2)
    {
    }
    
protected:
    void generateInternal(SpeculativeJIT* jit)
    {
        this->setUp(jit);
        this->recordCall(jit->callOperation(this->m_function, this->m_result, m_argument1, m_argument2));
        this->tearDown(jit);
    }

    ArgumentType1 m_argument1;
    ArgumentType2 m_argument2;
};

template<
    typename JumpType, typename FunctionType, typename ResultType,
    typename ArgumentType1, typename ArgumentType2, typename ArgumentType3>
class CallResultAndThreeArgumentsSlowPathGenerator
    : public CallSlowPathGenerator<JumpType, FunctionType, ResultType> {
public:
    CallResultAndThreeArgumentsSlowPathGenerator(
        JumpType from, SpeculativeJIT* jit, FunctionType function,
        SpillRegistersMode spillMode, ResultType result, ArgumentType1 argument1,
        ArgumentType2 argument2, ArgumentType3 argument3)
        : CallSlowPathGenerator<JumpType, FunctionType, ResultType>(
            from, jit, function, spillMode, result)
        , m_argument1(argument1)
        , m_argument2(argument2)
        , m_argument3(argument3)
    {
    }

protected:    
    void generateInternal(SpeculativeJIT* jit)
    {
        this->setUp(jit);
        this->recordCall(
            jit->callOperation(
                this->m_function, this->m_result, m_argument1, m_argument2,
                m_argument3));
        this->tearDown(jit);
    }

    ArgumentType1 m_argument1;
    ArgumentType2 m_argument2;
    ArgumentType3 m_argument3;
};

template<
    typename JumpType, typename FunctionType, typename ResultType,
    typename ArgumentType1, typename ArgumentType2, typename ArgumentType3,
    typename ArgumentType4>
class CallResultAndFourArgumentsSlowPathGenerator
    : public CallSlowPathGenerator<JumpType, FunctionType, ResultType> {
public:
    CallResultAndFourArgumentsSlowPathGenerator(
        JumpType from, SpeculativeJIT* jit, FunctionType function,
        SpillRegistersMode spillMode, ResultType result, ArgumentType1 argument1,
        ArgumentType2 argument2, ArgumentType3 argument3, ArgumentType4 argument4)
        : CallSlowPathGenerator<JumpType, FunctionType, ResultType>(
            from, jit, function, spillMode, result)
        , m_argument1(argument1)
        , m_argument2(argument2)
        , m_argument3(argument3)
        , m_argument4(argument4)
    {
    }
    
protected:
    void generateInternal(SpeculativeJIT* jit)
    {
        this->setUp(jit);
        this->recordCall(
            jit->callOperation(
                this->m_function, this->m_result, m_argument1, m_argument2,
                m_argument3, m_argument4));
        this->tearDown(jit);
    }

    ArgumentType1 m_argument1;
    ArgumentType2 m_argument2;
    ArgumentType3 m_argument3;
    ArgumentType4 m_argument4;
};

template<
    typename JumpType, typename FunctionType, typename ResultType,
    typename ArgumentType1, typename ArgumentType2, typename ArgumentType3,
    typename ArgumentType4, typename ArgumentType5>
class CallResultAndFiveArgumentsSlowPathGenerator
    : public CallSlowPathGenerator<JumpType, FunctionType, ResultType> {
public:
    CallResultAndFiveArgumentsSlowPathGenerator(
        JumpType from, SpeculativeJIT* jit, FunctionType function,
        SpillRegistersMode spillMode, ResultType result, ArgumentType1 argument1,
        ArgumentType2 argument2, ArgumentType3 argument3, ArgumentType4 argument4,
        ArgumentType5 argument5)
        : CallSlowPathGenerator<JumpType, FunctionType, ResultType>(
            from, jit, function, spillMode, result)
        , m_argument1(argument1)
        , m_argument2(argument2)
        , m_argument3(argument3)
        , m_argument4(argument4)
        , m_argument5(argument5)
    {
    }

protected:    
    void generateInternal(SpeculativeJIT* jit)
    {
        this->setUp(jit);
        this->recordCall(
            jit->callOperation(
                this->m_function, this->m_result, m_argument1, m_argument2,
                m_argument3, m_argument4, m_argument5));
        this->tearDown(jit);
    }

    ArgumentType1 m_argument1;
    ArgumentType2 m_argument2;
    ArgumentType3 m_argument3;
    ArgumentType4 m_argument4;
    ArgumentType5 m_argument5;
};

template<typename JumpType, typename FunctionType, typename ResultType>
inline PassOwnPtr<SlowPathGenerator> slowPathCall(
    JumpType from, SpeculativeJIT* jit, FunctionType function,
    ResultType result, SpillRegistersMode spillMode = NeedToSpill)
{
    return adoptPtr(
        new CallResultAndNoArgumentsSlowPathGenerator<
            JumpType, FunctionType, ResultType>(
                from, jit, function, spillMode, result));
}

template<
    typename JumpType, typename FunctionType, typename ResultType,
    typename ArgumentType1>
inline PassOwnPtr<SlowPathGenerator> slowPathCall(
    JumpType from, SpeculativeJIT* jit, FunctionType function,
    ResultType result, ArgumentType1 argument1,
    SpillRegistersMode spillMode = NeedToSpill)
{
    return adoptPtr(
        new CallResultAndOneArgumentSlowPathGenerator<
            JumpType, FunctionType, ResultType, ArgumentType1>(
                from, jit, function, spillMode, result, argument1));
}

template<
    typename JumpType, typename FunctionType, typename ResultType,
    typename ArgumentType1, typename ArgumentType2>
inline PassOwnPtr<SlowPathGenerator> slowPathCall(
    JumpType from, SpeculativeJIT* jit, FunctionType function,
    ResultType result, ArgumentType1 argument1, ArgumentType2 argument2,
    SpillRegistersMode spillMode = NeedToSpill)
{
    return adoptPtr(
        new CallResultAndTwoArgumentsSlowPathGenerator<
            JumpType, FunctionType, ResultType, ArgumentType1, ArgumentType2>(
                from, jit, function, spillMode, result, argument1, argument2));
}

template<
    typename JumpType, typename FunctionType, typename ResultType,
    typename ArgumentType1, typename ArgumentType2, typename ArgumentType3>
inline PassOwnPtr<SlowPathGenerator> slowPathCall(
    JumpType from, SpeculativeJIT* jit, FunctionType function,
    ResultType result, ArgumentType1 argument1, ArgumentType2 argument2,
    ArgumentType3 argument3, SpillRegistersMode spillMode = NeedToSpill)
{
    return adoptPtr(
        new CallResultAndThreeArgumentsSlowPathGenerator<
            JumpType, FunctionType, ResultType, ArgumentType1, ArgumentType2,
            ArgumentType3>(
                from, jit, function, spillMode, result, argument1, argument2,
                argument3));
}

template<
    typename JumpType, typename FunctionType, typename ResultType,
    typename ArgumentType1, typename ArgumentType2, typename ArgumentType3,
    typename ArgumentType4>
inline PassOwnPtr<SlowPathGenerator> slowPathCall(
    JumpType from, SpeculativeJIT* jit, FunctionType function,
    ResultType result, ArgumentType1 argument1, ArgumentType2 argument2,
    ArgumentType3 argument3, ArgumentType4 argument4,
    SpillRegistersMode spillMode = NeedToSpill)
{
    return adoptPtr(
        new CallResultAndFourArgumentsSlowPathGenerator<
            JumpType, FunctionType, ResultType, ArgumentType1, ArgumentType2,
            ArgumentType3, ArgumentType4>(
                from, jit, function, spillMode, result, argument1, argument2,
                argument3, argument4));
}

template<
    typename JumpType, typename FunctionType, typename ResultType,
    typename ArgumentType1, typename ArgumentType2, typename ArgumentType3,
    typename ArgumentType4, typename ArgumentType5>
inline PassOwnPtr<SlowPathGenerator> slowPathCall(
    JumpType from, SpeculativeJIT* jit, FunctionType function,
    ResultType result, ArgumentType1 argument1, ArgumentType2 argument2,
    ArgumentType3 argument3, ArgumentType4 argument4, ArgumentType5 argument5,
    SpillRegistersMode spillMode = NeedToSpill)
{
    return adoptPtr(
        new CallResultAndFiveArgumentsSlowPathGenerator<
            JumpType, FunctionType, ResultType, ArgumentType1, ArgumentType2,
            ArgumentType3, ArgumentType4, ArgumentType5>(
                from, jit, function, spillMode, result, argument1, argument2,
                argument3, argument4, argument5));
}

template<typename JumpType, typename DestinationType, typename SourceType, unsigned numberOfAssignments>
class AssigningSlowPathGenerator : public JumpingSlowPathGenerator<JumpType> {
public:
    AssigningSlowPathGenerator(
        JumpType from, SpeculativeJIT* jit,
        DestinationType destination[numberOfAssignments],
        SourceType source[numberOfAssignments])
        : JumpingSlowPathGenerator<JumpType>(from, jit)
    {
        for (unsigned i = numberOfAssignments; i--;) {
            m_destination[i] = destination[i];
            m_source[i] = source[i];
        }
    }

protected:
    virtual void generateInternal(SpeculativeJIT* jit)
    {
        this->linkFrom(jit);
        for (unsigned i = numberOfAssignments; i--;)
            jit->m_jit.move(m_source[i], m_destination[i]);
        this->jumpTo(jit);
    }

private:
    DestinationType m_destination[numberOfAssignments];
    SourceType m_source[numberOfAssignments];
};

template<typename JumpType, typename DestinationType, typename SourceType, unsigned numberOfAssignments>
inline PassOwnPtr<SlowPathGenerator> slowPathMove(
    JumpType from, SpeculativeJIT* jit, SourceType source[numberOfAssignments], DestinationType destination[numberOfAssignments])
{
    return adoptPtr(
        new AssigningSlowPathGenerator<
            JumpType, DestinationType, SourceType, numberOfAssignments>(
                from, jit, destination, source));
}

template<typename JumpType, typename DestinationType, typename SourceType>
inline PassOwnPtr<SlowPathGenerator> slowPathMove(
    JumpType from, SpeculativeJIT* jit, SourceType source, DestinationType destination)
{
    SourceType sourceArray[1] = { source };
    DestinationType destinationArray[1] = { destination };
    return adoptPtr(
        new AssigningSlowPathGenerator<
            JumpType, DestinationType, SourceType, 1>(
                from, jit, destinationArray, sourceArray));
}

template<typename JumpType, typename DestinationType, typename SourceType>
inline PassOwnPtr<SlowPathGenerator> slowPathMove(
    JumpType from, SpeculativeJIT* jit, SourceType source1, DestinationType destination1, SourceType source2, DestinationType destination2)
{
    SourceType sourceArray[2] = { source1, source2 };
    DestinationType destinationArray[2] = { destination1, destination2 };
    return adoptPtr(
        new AssigningSlowPathGenerator<
            JumpType, DestinationType, SourceType, 2>(
                from, jit, destinationArray, sourceArray));
}

} } // namespace JSC::DFG

#endif // ENABLD(DFG_JIT)

#endif // DFGSlowPathGenerator_h

