/*
 * Copyright (C) 2009, 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef Weak_h
#define Weak_h

#include <wtf/Noncopyable.h>
#include <wtf/NullPtr.h>

namespace JSC {

template<typename T> class PassWeak;
class WeakImpl;
class WeakHandleOwner;

// This is a free function rather than a Weak<T> member function so we can put it in Weak.cpp.
JS_EXPORT_PRIVATE void weakClearSlowCase(WeakImpl*&);

template<typename T> class Weak {
    WTF_MAKE_NONCOPYABLE(Weak);
public:
    Weak()
        : m_impl(0)
    {
    }

    explicit Weak(std::nullptr_t)
        : m_impl(0)
    {
    }

    explicit Weak(T*, WeakHandleOwner* = 0, void* context = 0);

    enum HashTableDeletedValueTag { HashTableDeletedValue };
    bool isHashTableDeletedValue() const;
    Weak(HashTableDeletedValueTag);

    template<typename U> Weak(const PassWeak<U>&);

    ~Weak()
    {
        clear();
    }

    void swap(Weak&);
    Weak& operator=(const PassWeak<T>&);
    
    bool operator!() const;
    T* operator->() const;
    T& operator*() const;
    T* get() const;

    bool was(T*) const;

    // This conversion operator allows implicit conversion to bool but not to other integer types.
    typedef void* (Weak::*UnspecifiedBoolType);
    operator UnspecifiedBoolType*() const;

    PassWeak<T> release();
    void clear()
    {
        if (!m_impl)
            return;
        weakClearSlowCase(m_impl);
    }
    
private:
    static WeakImpl* hashTableDeletedValue();

    WeakImpl* m_impl;
};

} // namespace JSC

#endif // Weak_h
