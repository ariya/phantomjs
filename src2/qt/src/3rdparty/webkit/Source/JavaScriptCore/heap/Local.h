/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Local_h
#define Local_h

#include "Handle.h"
#include "JSGlobalData.h"

/*
    A strongly referenced handle whose lifetime is temporary, limited to a given
    LocalScope. Use Locals for local values on the stack. It is an error to
    create a Local outside of any LocalScope.
*/

namespace JSC {

template <typename T> class Local : public Handle<T> {
    friend class LocalScope;
    using Handle<T>::slot;

public:
    typedef typename Handle<T>::ExternalType ExternalType;

    Local(JSGlobalData&, ExternalType = ExternalType());
    Local(JSGlobalData&, Handle<T>);
    Local(const Local<T>&); // Adopting constructor. Used to return a Local to a calling function.

    Local& operator=(ExternalType);
    Local& operator=(Handle<T>);

private:
    Local(HandleSlot, ExternalType); // Used by LocalScope::release() to move a Local to a containing scope.
    void set(ExternalType);
};

template <typename T> inline Local<T>::Local(JSGlobalData& globalData, ExternalType value)
    : Handle<T>(globalData.allocateLocalHandle())
{
    set(value);
}

template <typename T> inline Local<T>::Local(JSGlobalData& globalData, Handle<T> other)
    : Handle<T>(globalData.allocateLocalHandle())
{
    set(other.get());
}

template <typename T> inline Local<T>::Local(const Local<T>& other)
    : Handle<T>(other.slot())
{
    const_cast<Local<T>&>(other).setSlot(0); // Prevent accidental sharing.
}

template <typename T> inline Local<T>::Local(HandleSlot slot, ExternalType value)
    : Handle<T>(slot, value)
{
}

template <typename T> inline Local<T>& Local<T>::operator=(ExternalType value)
{
    set(value);
    return *this;
}

template <typename T> inline Local<T>& Local<T>::operator=(Handle<T> other)
{
    set(other.get());
    return *this;
}

template <typename T> inline void Local<T>::set(ExternalType externalType)
{
    ASSERT(slot());
    ASSERT(!HandleTypes<T>::toJSValue(externalType) || !HandleTypes<T>::toJSValue(externalType).isCell() || Heap::isMarked(HandleTypes<T>::toJSValue(externalType).asCell()));
    *slot() = externalType;
}


template <typename T, unsigned inlineCapacity = 0> class LocalStack {
    typedef typename Handle<T>::ExternalType ExternalType;
public:
    LocalStack(JSGlobalData& globalData)
        : m_globalData(&globalData)
        , m_count(0)
    {
    }

    ExternalType peek() const
    {
        ASSERT(m_count > 0);
        return m_stack[m_count - 1].get();
    }

    ExternalType pop()
    {
        ASSERT(m_count > 0);
        return m_stack[--m_count].get();
    }

    void push(ExternalType value)
    {
        if (m_count == m_stack.size())
            m_stack.append(Local<T>(*m_globalData, value));
        else
            m_stack[m_count] = value;
        m_count++;
    }

    bool isEmpty() const { return !m_count; }
    unsigned size() const { return m_count; }

private:
    RefPtr<JSGlobalData> m_globalData;
    Vector<Local<T>, inlineCapacity> m_stack;
    unsigned m_count;
};

}

namespace WTF {

template<typename T> struct VectorTraits<JSC::Local<T> > : SimpleClassVectorTraits {
    static const bool needsDestruction = false;
    static const bool canInitializeWithMemset = false;
    static const bool canCompareWithMemcmp = false;
};

}

#endif
