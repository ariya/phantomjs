/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef Operands_h
#define Operands_h

#include "CallFrame.h"
#include "JSObject.h"
#include <wtf/PrintStream.h>
#include <wtf/Vector.h>

namespace JSC {

// argument 0 is 'this'.
inline bool operandIsArgument(int operand) { return operand < 0; }
inline int operandToArgument(int operand) { return -operand + CallFrame::thisArgumentOffset(); }
inline int argumentToOperand(int argument) { return -argument + CallFrame::thisArgumentOffset(); }

template<typename T> struct OperandValueTraits;

template<typename T>
struct OperandValueTraits {
    static T defaultValue() { return T(); }
    static void dump(const T& value, PrintStream& out) { value.dump(out); }
};

enum OperandKind { ArgumentOperand, LocalOperand };

template<typename T, typename Traits = OperandValueTraits<T> >
class Operands {
public:
    Operands() { }
    
    explicit Operands(size_t numArguments, size_t numLocals)
    {
        m_arguments.fill(Traits::defaultValue(), numArguments);
        m_locals.fill(Traits::defaultValue(), numLocals);
    }
    
    size_t numberOfArguments() const { return m_arguments.size(); }
    size_t numberOfLocals() const { return m_locals.size(); }
    
    T& argument(size_t idx) { return m_arguments[idx]; }
    const T& argument(size_t idx) const { return m_arguments[idx]; }
    
    T& local(size_t idx) { return m_locals[idx]; }
    const T& local(size_t idx) const { return m_locals[idx]; }
    
    template<OperandKind operandKind>
    size_t sizeFor() const
    {
        if (operandKind == ArgumentOperand)
            return numberOfArguments();
        return numberOfLocals();
    }
    template<OperandKind operandKind>
    T& atFor(size_t idx)
    {
        if (operandKind == ArgumentOperand)
            return argument(idx);
        return local(idx);
    }
    template<OperandKind operandKind>
    const T& atFor(size_t idx) const
    {
        if (operandKind == ArgumentOperand)
            return argument(idx);
        return local(idx);
    }
    
    void ensureLocals(size_t size)
    {
        if (size <= m_locals.size())
            return;

        size_t oldSize = m_locals.size();
        m_locals.resize(size);
        for (size_t i = oldSize; i < m_locals.size(); ++i)
            m_locals[i] = Traits::defaultValue();
    }
    
    void setLocal(size_t idx, const T& value)
    {
        ensureLocals(idx + 1);
        
        m_locals[idx] = value;
    }
    
    T getLocal(size_t idx)
    {
        if (idx >= m_locals.size())
            return Traits::defaultValue();
        return m_locals[idx];
    }
    
    void setArgumentFirstTime(size_t idx, const T& value)
    {
        ASSERT(m_arguments[idx] == Traits::defaultValue());
        argument(idx) = value;
    }
    
    void setLocalFirstTime(size_t idx, const T& value)
    {
        ASSERT(idx >= m_locals.size() || m_locals[idx] == Traits::defaultValue());
        setLocal(idx, value);
    }
    
    T& operand(int operand)
    {
        if (operandIsArgument(operand)) {
            int argument = operandToArgument(operand);
            return m_arguments[argument];
        }
        
        return m_locals[operand];
    }
    
    const T& operand(int operand) const { return const_cast<const T&>(const_cast<Operands*>(this)->operand(operand)); }
    
    bool hasOperand(int operand) const
    {
        if (operandIsArgument(operand))
            return true;
        return static_cast<size_t>(operand) < numberOfLocals();
    }
    
    void setOperand(int operand, const T& value)
    {
        if (operandIsArgument(operand)) {
            int argument = operandToArgument(operand);
            m_arguments[argument] = value;
            return;
        }
        
        setLocal(operand, value);
    }
    
    size_t size() const { return numberOfArguments() + numberOfLocals(); }
    const T& at(size_t index) const
    {
        if (index < numberOfArguments())
            return m_arguments[index];
        return m_locals[index - numberOfArguments()];
    }
    T& at(size_t index)
    {
        if (index < numberOfArguments())
            return m_arguments[index];
        return m_locals[index - numberOfArguments()];
    }
    const T& operator[](size_t index) const { return at(index); }
    T& operator[](size_t index) { return at(index); }

    bool isArgument(size_t index) const { return index < numberOfArguments(); }
    bool isVariable(size_t index) const { return !isArgument(index); }
    int argumentForIndex(size_t index) const
    {
        return index;
    }
    int variableForIndex(size_t index) const
    {
        return index - m_arguments.size();
    }
    int operandForIndex(size_t index) const
    {
        if (index < numberOfArguments())
            return argumentToOperand(index);
        return index - numberOfArguments();
    }
    
    void setOperandFirstTime(int operand, const T& value)
    {
        if (operandIsArgument(operand)) {
            setArgumentFirstTime(operandToArgument(operand), value);
            return;
        }
        
        setLocalFirstTime(operand, value);
    }
    
    void clear()
    {
        for (size_t i = 0; i < m_arguments.size(); ++i)
            m_arguments[i] = Traits::defaultValue();
        for (size_t i = 0; i < m_locals.size(); ++i)
            m_locals[i] = Traits::defaultValue();
    }
    
private:
    Vector<T, 8> m_arguments;
    Vector<T, 16> m_locals;
};

template<typename T, typename Traits>
void dumpOperands(const Operands<T, Traits>& operands, PrintStream& out)
{
    for (size_t argument = operands.numberOfArguments(); argument--;) {
        if (argument != operands.numberOfArguments() - 1)
            out.printf(" ");
        out.print("arg", argument, ":");
        Traits::dump(operands.argument(argument), out);
    }
    out.printf(" : ");
    for (size_t local = 0; local < operands.numberOfLocals(); ++local) {
        if (local)
            out.printf(" ");
        out.print("r", local, ":");
        Traits::dump(operands.local(local), out);
    }
}

} // namespace JSC

#endif // Operands_h

