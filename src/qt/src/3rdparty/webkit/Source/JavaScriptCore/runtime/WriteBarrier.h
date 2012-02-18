/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef WriteBarrier_h
#define WriteBarrier_h

#include "JSValue.h"

namespace JSC {
class JSCell;
class JSGlobalData;

inline void writeBarrier(JSGlobalData&, const JSCell*, JSValue)
{
}

inline void writeBarrier(JSGlobalData&, const JSCell*, JSCell*)
{
}

typedef enum { } Unknown;
typedef JSValue* HandleSlot;

template <typename T> struct JSValueChecker {
    static const bool IsJSValue = false;
};

template <> struct JSValueChecker<JSValue> {
    static const bool IsJSValue = true;
};

// We have a separate base class with no constructors for use in Unions.
template <typename T> class WriteBarrierBase {
public:
    COMPILE_ASSERT(!JSValueChecker<T>::IsJSValue, WriteBarrier_JSValue_is_invalid__use_unknown);
    void set(JSGlobalData& globalData, const JSCell* owner, T* value)
    {
        this->m_cell = reinterpret_cast<JSCell*>(value);
        writeBarrier(globalData, owner, this->m_cell);
#if ENABLE(JSC_ZOMBIES)
        ASSERT(!isZombie(owner));
        ASSERT(!isZombie(m_cell));
#endif
    }
    
    T* get() const
    {
        return reinterpret_cast<T*>(m_cell);
    }

    T* operator*() const
    {
        ASSERT(m_cell);
#if ENABLE(JSC_ZOMBIES)
        ASSERT(!isZombie(m_cell));
#endif
        return static_cast<T*>(m_cell);
    }

    T* operator->() const
    {
        ASSERT(m_cell);
        return static_cast<T*>(m_cell);
    }

    void clear() { m_cell = 0; }
    
    JSCell** slot() { return &m_cell; }
    
    typedef T* (WriteBarrierBase::*UnspecifiedBoolType);
    operator UnspecifiedBoolType*() const { return m_cell ? reinterpret_cast<UnspecifiedBoolType*>(1) : 0; }
    
    bool operator!() const { return !m_cell; }

    void setWithoutWriteBarrier(T* value)
    {
        this->m_cell = reinterpret_cast<JSCell*>(value);
#if ENABLE(JSC_ZOMBIES)
        ASSERT(!m_cell || !isZombie(m_cell));
#endif
    }

private:
    JSCell* m_cell;
};

template <> class WriteBarrierBase<Unknown> {
public:
    void set(JSGlobalData& globalData, const JSCell* owner, JSValue value)
    {
#if ENABLE(JSC_ZOMBIES)
        ASSERT(!isZombie(owner));
        ASSERT(!value.isZombie());
#endif
        m_value = JSValue::encode(value);
        writeBarrier(globalData, owner, value);
    }
    void setWithoutWriteBarrier(JSValue value)
    {
#if ENABLE(JSC_ZOMBIES)
        ASSERT(!value.isZombie());
#endif
        m_value = JSValue::encode(value);
    }

    JSValue get() const
    {
        return JSValue::decode(m_value);
    }
    void clear() { m_value = JSValue::encode(JSValue()); }
    void setUndefined() { m_value = JSValue::encode(jsUndefined()); }
    bool isNumber() const { return get().isNumber(); }
    bool isObject() const { return get().isObject(); }
    bool isNull() const { return get().isNull(); }
    bool isGetterSetter() const { return get().isGetterSetter(); }
    
    JSValue* slot()
    { 
        union {
            EncodedJSValue* v;
            JSValue* slot;
        } u;
        u.v = &m_value;
        return u.slot;
    }
    
    typedef JSValue (WriteBarrierBase::*UnspecifiedBoolType);
    operator UnspecifiedBoolType*() const { return get() ? reinterpret_cast<UnspecifiedBoolType*>(1) : 0; }
    bool operator!() const { return !get(); } 
    
private:
    EncodedJSValue m_value;
};

template <typename T> class WriteBarrier : public WriteBarrierBase<T> {
public:
    WriteBarrier()
    {
        this->setWithoutWriteBarrier(0);
    }

    WriteBarrier(JSGlobalData& globalData, const JSCell* owner, T* value)
    {
        this->set(globalData, owner, value);
    }
};

template <> class WriteBarrier<Unknown> : public WriteBarrierBase<Unknown> {
public:
    WriteBarrier()
    {
        this->setWithoutWriteBarrier(JSValue());
    }

    WriteBarrier(JSGlobalData& globalData, const JSCell* owner, JSValue value)
    {
        this->set(globalData, owner, value);
    }
};

template <typename U, typename V> inline bool operator==(const WriteBarrierBase<U>& lhs, const WriteBarrierBase<V>& rhs)
{
    return lhs.get() == rhs.get();
}

} // namespace JSC

#endif // WriteBarrier_h
