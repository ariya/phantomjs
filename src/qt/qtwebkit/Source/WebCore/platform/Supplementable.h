/*
 * Copyright (C) 2012 Google, Inc. All Rights Reserved.
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

#ifndef Supplementable_h
#define Supplementable_h

#include <wtf/Assertions.h>
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

#if !ASSERT_DISABLED
#include <wtf/Threading.h>
#endif

namespace WebCore {

// What you should know about Supplementable and Supplement
// ========================================================
// Supplementable and Supplement instances are meant to be thread local. They
// should only be accessed from within the thread that created them. The
// 2 classes are not designed for safe access from another thread. Violating
// this design assumption can result in memory corruption and unpredictable
// behavior.
//
// What you should know about the Supplement keys
// ==============================================
// The Supplement is expected to use the same const char* string instance
// as its key. The Supplementable's SupplementMap will use the address of the
// string as the key and not the characters themselves. Hence, 2 strings with
// the same characters will be treated as 2 different keys.
//
// In practice, it is recommended that Supplements implements a static method
// for returning its key to use. For example:
//
//     class MyClass : public Supplement<MySupplementable> {
//         ...
//         static const char* supplementName();
//     }
//
//     const char* MyClass::supplementName()
//     {
//         return "MyClass";
//     }
//
// An example of the using the key:
//
//     MyClass* MyClass::from(MySupplementable* host)
//     {
//         return reinterpret_cast<MyClass*>(Supplement<MySupplementable>::from(host, supplementName()));
//     }

template<typename T>
class Supplementable;

template<typename T>
class Supplement {
public:
    virtual ~Supplement() { }
#if !ASSERT_DISABLED || defined(ADDRESS_SANITIZER)
    virtual bool isRefCountedWrapper() const { return false; }
#endif

    static void provideTo(Supplementable<T>* host, const char* key, PassOwnPtr<Supplement<T> > supplement)
    {
        host->provideSupplement(key, supplement);
    }

    static Supplement<T>* from(Supplementable<T>* host, const char* key)
    {
        return host ? host->requireSupplement(key) : 0;
    }
};

template<typename T>
class Supplementable {
public:
    void provideSupplement(const char* key, PassOwnPtr<Supplement<T> > supplement)
    {
        ASSERT(m_threadId == currentThread());
        ASSERT(!m_supplements.get(key));
        m_supplements.set(key, supplement);
    }

    void removeSupplement(const char* key)
    {
        ASSERT(m_threadId == currentThread());
        m_supplements.remove(key);
    }

    Supplement<T>* requireSupplement(const char* key)
    {
        ASSERT(m_threadId == currentThread());
        return m_supplements.get(key);
    }

#if !ASSERT_DISABLED
protected:
    Supplementable() : m_threadId(currentThread()) { }
#endif

private:
    typedef HashMap<const char*, OwnPtr<Supplement<T> >, PtrHash<const char*> > SupplementMap;
    SupplementMap m_supplements;
#if !ASSERT_DISABLED
    ThreadIdentifier m_threadId;
#endif
};

} // namespace WebCore

#endif // Supplementable_h

