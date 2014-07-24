/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef NeverDestroyed_h
#define NeverDestroyed_h

#include <wtf/Alignment.h>
#include <wtf/Noncopyable.h>
#include <wtf/StdLibExtras.h>
#include <wtf/TypeTraits.h>

// NeverDestroyed is a smart pointer like class who ensures that the destructor
// for the given object is never called, but doesn't use the heap to allocate it.
// It's useful for static local variables, and can be used like so:
//
// MySharedGlobal& mySharedGlobal()
// {
//   static NeverDestroyed<MySharedGlobal> myGlobal("Hello", 42);
//   return myGlobal;
// }
//

namespace WTF {

template<typename T> class NeverDestroyed {
    WTF_MAKE_NONCOPYABLE(NeverDestroyed);

public:
#if COMPILER_SUPPORTS(CXX_VARIADIC_TEMPLATES)
    template<typename... Args>
    NeverDestroyed(Args&&... args)
    {
        new (asPtr()) T(std::forward<Args>(args)...);
    }
#else
    NeverDestroyed()
    {
        new (NotNull, asPtr()) T;
    }

    template<typename P1>
    NeverDestroyed(const P1& p1)
    {
        new (NotNull, asPtr()) T(p1);
    }
#endif

    operator T&() { return *asPtr(); }

private:
#if COMPILER_SUPPORTS(CXX_RVALUE_REFERENCES)
    NeverDestroyed(NeverDestroyed&&) WTF_DELETED_FUNCTION;
    NeverDestroyed& operator=(NeverDestroyed&&) WTF_DELETED_FUNCTION;
#endif

    typedef typename WTF::RemoveConst<T>::Type *PointerType;

    PointerType asPtr() { return reinterpret_cast<PointerType>(&m_storage); }

    // FIXME: Investigate whether we should allocate a hunk of virtual memory
    // and hand out chunks of it to NeverDestroyed instead, to reduce fragmentation.
    AlignedBuffer<sizeof(T), WTF_ALIGN_OF(T)> m_storage;
};

} // namespace WTF;

using WTF::NeverDestroyed;

#endif // NeverDestroyed_h
