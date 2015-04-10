/*
 *  Copyright (C) 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef WTF_Vector_h
#define WTF_Vector_h

#include <wtf/Alignment.h>
#include <wtf/CheckedArithmetic.h>
#include <wtf/FastAllocBase.h>
#include <wtf/Noncopyable.h>
#include <wtf/NotFound.h>
#include <wtf/StdLibExtras.h>
#include <wtf/ValueCheck.h>
#include <wtf/VectorTraits.h>
#include <limits>
#include <utility>

namespace WTF {

    template <bool needsDestruction, typename T>
    struct VectorDestructor;

    template<typename T>
    struct VectorDestructor<false, T>
    {
        static void destruct(T*, T*) {}
    };

    template<typename T>
    struct VectorDestructor<true, T>
    {
        static void destruct(T* begin, T* end) 
        {
            for (T* cur = begin; cur != end; ++cur)
                cur->~T();
        }
    };

    template <bool needsInitialization, bool canInitializeWithMemset, typename T>
    struct VectorInitializer;

    template<bool ignore, typename T>
    struct VectorInitializer<false, ignore, T>
    {
        static void initialize(T*, T*) {}
    };

    template<typename T>
    struct VectorInitializer<true, false, T>
    {
        static void initialize(T* begin, T* end) 
        {
            for (T* cur = begin; cur != end; ++cur)
                new (NotNull, cur) T;
        }
    };

    template<typename T>
    struct VectorInitializer<true, true, T>
    {
        static void initialize(T* begin, T* end) 
        {
            memset(begin, 0, reinterpret_cast<char*>(end) - reinterpret_cast<char*>(begin));
        }
    };

    template <bool canMoveWithMemcpy, typename T>
    struct VectorMover;

    template<typename T>
    struct VectorMover<false, T>
    {
        static void move(const T* src, const T* srcEnd, T* dst)
        {
            while (src != srcEnd) {
                new (NotNull, dst) T(*src);
#if COMPILER(SUNCC) && __SUNPRO_CC <= 0x590
                const_cast<T*>(src)->~T(); // Work around obscure SunCC 12 compiler bug.
#else
                src->~T();
#endif
                ++dst;
                ++src;
            }
        }
        static void moveOverlapping(const T* src, const T* srcEnd, T* dst)
        {
            if (src > dst)
                move(src, srcEnd, dst);
            else {
                T* dstEnd = dst + (srcEnd - src);
                while (src != srcEnd) {
                    --srcEnd;
                    --dstEnd;
                    new (NotNull, dstEnd) T(*srcEnd);
                    srcEnd->~T();
                }
            }
        }
    };

    template<typename T>
    struct VectorMover<true, T>
    {
        static void move(const T* src, const T* srcEnd, T* dst) 
        {
            memcpy(dst, src, reinterpret_cast<const char*>(srcEnd) - reinterpret_cast<const char*>(src));
        }
        static void moveOverlapping(const T* src, const T* srcEnd, T* dst) 
        {
            memmove(dst, src, reinterpret_cast<const char*>(srcEnd) - reinterpret_cast<const char*>(src));
        }
    };

    template <bool canCopyWithMemcpy, typename T>
    struct VectorCopier;

    template<typename T>
    struct VectorCopier<false, T>
    {
        static void uninitializedCopy(const T* src, const T* srcEnd, T* dst) 
        {
            while (src != srcEnd) {
                new (NotNull, dst) T(*src);
                ++dst;
                ++src;
            }
        }
    };

    template<typename T>
    struct VectorCopier<true, T>
    {
        static void uninitializedCopy(const T* src, const T* srcEnd, T* dst) 
        {
            memcpy(dst, src, reinterpret_cast<const char*>(srcEnd) - reinterpret_cast<const char*>(src));
        }
    };

    template <bool canFillWithMemset, typename T>
    struct VectorFiller;

    template<typename T>
    struct VectorFiller<false, T>
    {
        static void uninitializedFill(T* dst, T* dstEnd, const T& val) 
        {
            while (dst != dstEnd) {
                new (NotNull, dst) T(val);
                ++dst;
            }
        }
    };

    template<typename T>
    struct VectorFiller<true, T>
    {
        static void uninitializedFill(T* dst, T* dstEnd, const T& val) 
        {
            ASSERT(sizeof(T) == sizeof(char));
#if COMPILER(GCC) && defined(_FORTIFY_SOURCE)
            if (!__builtin_constant_p(dstEnd - dst) || (!(dstEnd - dst)))
#endif
                memset(dst, val, dstEnd - dst);
        }
    };
    
    template<bool canCompareWithMemcmp, typename T>
    struct VectorComparer;
    
    template<typename T>
    struct VectorComparer<false, T>
    {
        static bool compare(const T* a, const T* b, size_t size)
        {
            for (size_t i = 0; i < size; ++i)
                if (!(a[i] == b[i]))
                    return false;
            return true;
        }
    };

    template<typename T>
    struct VectorComparer<true, T>
    {
        static bool compare(const T* a, const T* b, size_t size)
        {
            return memcmp(a, b, sizeof(T) * size) == 0;
        }
    };
    
    template<typename T>
    struct VectorTypeOperations
    {
        static void destruct(T* begin, T* end)
        {
            VectorDestructor<VectorTraits<T>::needsDestruction, T>::destruct(begin, end);
        }

        static void initialize(T* begin, T* end)
        {
            VectorInitializer<VectorTraits<T>::needsInitialization, VectorTraits<T>::canInitializeWithMemset, T>::initialize(begin, end);
        }

        static void move(const T* src, const T* srcEnd, T* dst)
        {
            VectorMover<VectorTraits<T>::canMoveWithMemcpy, T>::move(src, srcEnd, dst);
        }

        static void moveOverlapping(const T* src, const T* srcEnd, T* dst)
        {
            VectorMover<VectorTraits<T>::canMoveWithMemcpy, T>::moveOverlapping(src, srcEnd, dst);
        }

        static void uninitializedCopy(const T* src, const T* srcEnd, T* dst)
        {
            VectorCopier<VectorTraits<T>::canCopyWithMemcpy, T>::uninitializedCopy(src, srcEnd, dst);
        }

        static void uninitializedFill(T* dst, T* dstEnd, const T& val)
        {
            VectorFiller<VectorTraits<T>::canFillWithMemset, T>::uninitializedFill(dst, dstEnd, val);
        }
        
        static bool compare(const T* a, const T* b, size_t size)
        {
            return VectorComparer<VectorTraits<T>::canCompareWithMemcmp, T>::compare(a, b, size);
        }
    };

    template<typename T>
    class VectorBufferBase {
        WTF_MAKE_NONCOPYABLE(VectorBufferBase);
    public:
        void allocateBuffer(size_t newCapacity)
        {
            ASSERT(newCapacity);
            if (newCapacity > std::numeric_limits<unsigned>::max() / sizeof(T))
                CRASH();
            size_t sizeToAllocate = fastMallocGoodSize(newCapacity * sizeof(T));
            m_capacity = sizeToAllocate / sizeof(T);
            m_buffer = static_cast<T*>(fastMalloc(sizeToAllocate));
        }

        bool tryAllocateBuffer(size_t newCapacity)
        {
            ASSERT(newCapacity);
            if (newCapacity > std::numeric_limits<unsigned>::max() / sizeof(T))
                return false;

            size_t sizeToAllocate = fastMallocGoodSize(newCapacity * sizeof(T));
            T* newBuffer;
            if (tryFastMalloc(sizeToAllocate).getValue(newBuffer)) {
                m_capacity = sizeToAllocate / sizeof(T);
                m_buffer = newBuffer;
                return true;
            }
            return false;
        }

        bool shouldReallocateBuffer(size_t newCapacity) const
        {
            return VectorTraits<T>::canMoveWithMemcpy && m_capacity && newCapacity;
        }

        void reallocateBuffer(size_t newCapacity)
        {
            ASSERT(shouldReallocateBuffer(newCapacity));
            if (newCapacity > std::numeric_limits<size_t>::max() / sizeof(T))
                CRASH();
            size_t sizeToAllocate = fastMallocGoodSize(newCapacity * sizeof(T));
            m_capacity = sizeToAllocate / sizeof(T);
            m_buffer = static_cast<T*>(fastRealloc(m_buffer, sizeToAllocate));
        }

        void deallocateBuffer(T* bufferToDeallocate)
        {
            if (!bufferToDeallocate)
                return;
            
            if (m_buffer == bufferToDeallocate) {
                m_buffer = 0;
                m_capacity = 0;
            }

            fastFree(bufferToDeallocate);
        }

        T* buffer() { return m_buffer; }
        const T* buffer() const { return m_buffer; }
        size_t capacity() const { return m_capacity; }

        T* releaseBuffer()
        {
            T* buffer = m_buffer;
            m_buffer = 0;
            m_capacity = 0;
            return buffer;
        }

    protected:
        VectorBufferBase()
            : m_buffer(0)
            , m_capacity(0)
        {
        }

        VectorBufferBase(T* buffer, size_t capacity)
            : m_buffer(buffer)
            , m_capacity(capacity)
        {
        }

        ~VectorBufferBase()
        {
            // FIXME: It would be nice to find a way to ASSERT that m_buffer hasn't leaked here.
        }

        T* m_buffer;
        unsigned m_capacity;
    };

    template<typename T, size_t inlineCapacity>
    class VectorBuffer;

    template<typename T>
    class VectorBuffer<T, 0> : private VectorBufferBase<T> {
    private:
        typedef VectorBufferBase<T> Base;
    public:
        VectorBuffer()
        {
        }

        VectorBuffer(size_t capacity)
        {
            // Calling malloc(0) might take a lock and may actually do an
            // allocation on some systems.
            if (capacity)
                allocateBuffer(capacity);
        }

        ~VectorBuffer()
        {
            deallocateBuffer(buffer());
        }
        
        void swap(VectorBuffer<T, 0>& other)
        {
            std::swap(m_buffer, other.m_buffer);
            std::swap(m_capacity, other.m_capacity);
        }
        
        void restoreInlineBufferIfNeeded() { }

        using Base::allocateBuffer;
        using Base::tryAllocateBuffer;
        using Base::shouldReallocateBuffer;
        using Base::reallocateBuffer;
        using Base::deallocateBuffer;

        using Base::buffer;
        using Base::capacity;

        using Base::releaseBuffer;
    private:
        using Base::m_buffer;
        using Base::m_capacity;
    };

    template<typename T, size_t inlineCapacity>
    class VectorBuffer : private VectorBufferBase<T> {
        WTF_MAKE_NONCOPYABLE(VectorBuffer);
    private:
        typedef VectorBufferBase<T> Base;
    public:
        VectorBuffer()
            : Base(inlineBuffer(), inlineCapacity)
        {
        }

        VectorBuffer(size_t capacity)
            : Base(inlineBuffer(), inlineCapacity)
        {
            if (capacity > inlineCapacity)
                Base::allocateBuffer(capacity);
        }

        ~VectorBuffer()
        {
            deallocateBuffer(buffer());
        }

        void allocateBuffer(size_t newCapacity)
        {
            // FIXME: This should ASSERT(!m_buffer) to catch misuse/leaks.
            if (newCapacity > inlineCapacity)
                Base::allocateBuffer(newCapacity);
            else {
                m_buffer = inlineBuffer();
                m_capacity = inlineCapacity;
            }
        }

        bool tryAllocateBuffer(size_t newCapacity)
        {
            if (newCapacity > inlineCapacity)
                return Base::tryAllocateBuffer(newCapacity);
            m_buffer = inlineBuffer();
            m_capacity = inlineCapacity;
            return true;
        }

        void deallocateBuffer(T* bufferToDeallocate)
        {
            if (bufferToDeallocate == inlineBuffer())
                return;
            Base::deallocateBuffer(bufferToDeallocate);
        }

        bool shouldReallocateBuffer(size_t newCapacity) const
        {
            // We cannot reallocate the inline buffer.
            return Base::shouldReallocateBuffer(newCapacity) && std::min(static_cast<size_t>(m_capacity), newCapacity) > inlineCapacity;
        }

        void reallocateBuffer(size_t newCapacity)
        {
            ASSERT(shouldReallocateBuffer(newCapacity));
            Base::reallocateBuffer(newCapacity);
        }

        void swap(VectorBuffer<T, inlineCapacity>& other)
        {
            if (buffer() == inlineBuffer() && other.buffer() == other.inlineBuffer()) {
                WTF::swap(m_inlineBuffer, other.m_inlineBuffer);
                std::swap(m_capacity, other.m_capacity);
            } else if (buffer() == inlineBuffer()) {
                m_buffer = other.m_buffer;
                other.m_buffer = other.inlineBuffer();
                WTF::swap(m_inlineBuffer, other.m_inlineBuffer);
                std::swap(m_capacity, other.m_capacity);
            } else if (other.buffer() == other.inlineBuffer()) {
                other.m_buffer = m_buffer;
                m_buffer = inlineBuffer();
                WTF::swap(m_inlineBuffer, other.m_inlineBuffer);
                std::swap(m_capacity, other.m_capacity);
            } else {
                std::swap(m_buffer, other.m_buffer);
                std::swap(m_capacity, other.m_capacity);
            }
        }

        void restoreInlineBufferIfNeeded()
        {
            if (m_buffer)
                return;
            m_buffer = inlineBuffer();
            m_capacity = inlineCapacity;
        }

        using Base::buffer;
        using Base::capacity;

        T* releaseBuffer()
        {
            if (buffer() == inlineBuffer())
                return 0;
            return Base::releaseBuffer();
        }

    private:
        using Base::m_buffer;
        using Base::m_capacity;

        static const size_t m_inlineBufferSize = inlineCapacity * sizeof(T);
        T* inlineBuffer() { return reinterpret_cast_ptr<T*>(m_inlineBuffer.buffer); }
        const T* inlineBuffer() const { return reinterpret_cast_ptr<const T*>(m_inlineBuffer.buffer); }

        AlignedBuffer<m_inlineBufferSize, WTF_ALIGN_OF(T)> m_inlineBuffer;
    };
    
    struct UnsafeVectorOverflow {
        static NO_RETURN_DUE_TO_ASSERT void overflowed()
        {
            ASSERT_NOT_REACHED();
        }
    };

    template<typename T, size_t inlineCapacity = 0, typename OverflowHandler = CrashOnOverflow>
    class Vector : private VectorBuffer<T, inlineCapacity> {
        WTF_MAKE_FAST_ALLOCATED;
    private:
        typedef VectorBuffer<T, inlineCapacity> Base;
        typedef VectorTypeOperations<T> TypeOperations;

    public:
        typedef T ValueType;

        typedef T* iterator;
        typedef const T* const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

        Vector()
            : m_size(0)
        {
        }
        
        explicit Vector(size_t size)
            : Base(size)
            , m_size(size)
        {
            if (begin())
                TypeOperations::initialize(begin(), end());
        }

        ~Vector()
        {
            if (m_size)
                shrink(0);
        }

        Vector(const Vector&);
        template<size_t otherCapacity, typename otherOverflowBehaviour>
        Vector(const Vector<T, otherCapacity, otherOverflowBehaviour>&);

        Vector& operator=(const Vector&);
        template<size_t otherCapacity, typename otherOverflowBehaviour>
        Vector& operator=(const Vector<T, otherCapacity, otherOverflowBehaviour>&);

#if COMPILER_SUPPORTS(CXX_RVALUE_REFERENCES)
        Vector(Vector&&);
        Vector& operator=(Vector&&);
#endif

        size_t size() const { return m_size; }
        size_t capacity() const { return Base::capacity(); }
        bool isEmpty() const { return !size(); }

        T& at(size_t i)
        {
            if (UNLIKELY(i >= size()))
                OverflowHandler::overflowed();
            return Base::buffer()[i];
        }
        const T& at(size_t i) const 
        {
            if (UNLIKELY(i >= size()))
                OverflowHandler::overflowed();
            return Base::buffer()[i];
        }
        T& at(Checked<size_t> i)
        {
            RELEASE_ASSERT(i < size());
            return Base::buffer()[i];
        }
        const T& at(Checked<size_t> i) const
        {
            RELEASE_ASSERT(i < size());
            return Base::buffer()[i];
        }

        T& operator[](size_t i) { return at(i); }
        const T& operator[](size_t i) const { return at(i); }
        T& operator[](Checked<size_t> i) { return at(i); }
        const T& operator[](Checked<size_t> i) const { return at(i); }

        T* data() { return Base::buffer(); }
        const T* data() const { return Base::buffer(); }

        iterator begin() { return data(); }
        iterator end() { return begin() + m_size; }
        const_iterator begin() const { return data(); }
        const_iterator end() const { return begin() + m_size; }

        reverse_iterator rbegin() { return reverse_iterator(end()); }
        reverse_iterator rend() { return reverse_iterator(begin()); }
        const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
        const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

        T& first() { return at(0); }
        const T& first() const { return at(0); }
        T& last() { return at(size() - 1); }
        const T& last() const { return at(size() - 1); }

        template<typename U> bool contains(const U&) const;
        template<typename U> size_t find(const U&) const;
        template<typename U> size_t reverseFind(const U&) const;

        void shrink(size_t size);
        void grow(size_t size);
        void resize(size_t size);
        void resizeToFit(size_t size);
        void reserveCapacity(size_t newCapacity);
        bool tryReserveCapacity(size_t newCapacity);
        void reserveInitialCapacity(size_t initialCapacity);
        void shrinkCapacity(size_t newCapacity);
        void shrinkToFit() { shrinkCapacity(size()); }

        void clear() { shrinkCapacity(0); }

        template<typename U> void append(const U*, size_t);
        template<typename U> void append(const U&);
        template<typename U> void uncheckedAppend(const U& val);
        template<typename U, size_t otherCapacity> void appendVector(const Vector<U, otherCapacity>&);
        template<typename U> bool tryAppend(const U*, size_t);

        template<typename U> void insert(size_t position, const U*, size_t);
        template<typename U> void insert(size_t position, const U&);
        template<typename U, size_t c> void insert(size_t position, const Vector<U, c>&);

        void remove(size_t position);
        void remove(size_t position, size_t length);

        void removeLast() 
        {
            if (UNLIKELY(isEmpty()))
                OverflowHandler::overflowed();
            shrink(size() - 1); 
        }

        Vector(size_t size, const T& val)
            : Base(size)
            , m_size(size)
        {
            if (begin())
                TypeOperations::uninitializedFill(begin(), end(), val);
        }

        void fill(const T&, size_t);
        void fill(const T& val) { fill(val, size()); }

        template<typename Iterator> void appendRange(Iterator start, Iterator end);

        T* releaseBuffer();

        void swap(Vector<T, inlineCapacity, OverflowHandler>& other)
        {
            std::swap(m_size, other.m_size);
            Base::swap(other);
        }

        void reverse();

        void checkConsistency();

    private:
        void expandCapacity(size_t newMinCapacity);
        const T* expandCapacity(size_t newMinCapacity, const T*);
        bool tryExpandCapacity(size_t newMinCapacity);
        const T* tryExpandCapacity(size_t newMinCapacity, const T*);
        template<typename U> U* expandCapacity(size_t newMinCapacity, U*); 
        template<typename U> void appendSlowCase(const U&);

        unsigned m_size;

        using Base::buffer;
        using Base::capacity;
        using Base::swap;
        using Base::allocateBuffer;
        using Base::deallocateBuffer;
        using Base::tryAllocateBuffer;
        using Base::shouldReallocateBuffer;
        using Base::reallocateBuffer;
        using Base::restoreInlineBufferIfNeeded;
        using Base::releaseBuffer;
    };

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    Vector<T, inlineCapacity, OverflowHandler>::Vector(const Vector& other)
        : Base(other.capacity())
        , m_size(other.size())
    {
        if (begin())
            TypeOperations::uninitializedCopy(other.begin(), other.end(), begin());
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    template<size_t otherCapacity, typename otherOverflowBehaviour>
    Vector<T, inlineCapacity, OverflowHandler>::Vector(const Vector<T, otherCapacity, otherOverflowBehaviour>& other)
        : Base(other.capacity())
        , m_size(other.size())
    {
        if (begin())
            TypeOperations::uninitializedCopy(other.begin(), other.end(), begin());
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    Vector<T, inlineCapacity, OverflowHandler>& Vector<T, inlineCapacity, OverflowHandler>::operator=(const Vector<T, inlineCapacity, OverflowHandler>& other)
    {
        if (&other == this)
            return *this;
        
        if (size() > other.size())
            shrink(other.size());
        else if (other.size() > capacity()) {
            clear();
            reserveCapacity(other.size());
            if (!begin())
                return *this;
        }
        
// Works around an assert in VS2010. See https://connect.microsoft.com/VisualStudio/feedback/details/558044/std-copy-should-not-check-dest-when-first-last
#if COMPILER(MSVC) && defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL
        if (!begin())
            return *this;
#endif

        std::copy(other.begin(), other.begin() + size(), begin());
        TypeOperations::uninitializedCopy(other.begin() + size(), other.end(), end());
        m_size = other.size();

        return *this;
    }

    inline bool typelessPointersAreEqual(const void* a, const void* b) { return a == b; }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    template<size_t otherCapacity, typename otherOverflowBehaviour>
    Vector<T, inlineCapacity, OverflowHandler>& Vector<T, inlineCapacity, OverflowHandler>::operator=(const Vector<T, otherCapacity, otherOverflowBehaviour>& other)
    {
        // If the inline capacities match, we should call the more specific
        // template.  If the inline capacities don't match, the two objects
        // shouldn't be allocated the same address.
        ASSERT(!typelessPointersAreEqual(&other, this));

        if (size() > other.size())
            shrink(other.size());
        else if (other.size() > capacity()) {
            clear();
            reserveCapacity(other.size());
            if (!begin())
                return *this;
        }
        
// Works around an assert in VS2010. See https://connect.microsoft.com/VisualStudio/feedback/details/558044/std-copy-should-not-check-dest-when-first-last
#if COMPILER(MSVC) && defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL
        if (!begin())
            return *this;
#endif

        std::copy(other.begin(), other.begin() + size(), begin());
        TypeOperations::uninitializedCopy(other.begin() + size(), other.end(), end());
        m_size = other.size();

        return *this;
    }

#if COMPILER_SUPPORTS(CXX_RVALUE_REFERENCES)
    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    Vector<T, inlineCapacity, OverflowHandler>::Vector(Vector<T, inlineCapacity, OverflowHandler>&& other)
        : m_size(0)
    {
        // It's a little weird to implement a move constructor using swap but this way we
        // don't have to add a move constructor to VectorBuffer.
        swap(other);
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    Vector<T, inlineCapacity, OverflowHandler>& Vector<T, inlineCapacity, OverflowHandler>::operator=(Vector<T, inlineCapacity, OverflowHandler>&& other)
    {
        swap(other);
        return *this;
    }
#endif

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    template<typename U>
    bool Vector<T, inlineCapacity, OverflowHandler>::contains(const U& value) const
    {
        return find(value) != notFound;
    }
 
    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    template<typename U>
    size_t Vector<T, inlineCapacity, OverflowHandler>::find(const U& value) const
    {
        for (size_t i = 0; i < size(); ++i) {
            if (at(i) == value)
                return i;
        }
        return notFound;
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    template<typename U>
    size_t Vector<T, inlineCapacity, OverflowHandler>::reverseFind(const U& value) const
    {
        for (size_t i = 1; i <= size(); ++i) {
            const size_t index = size() - i;
            if (at(index) == value)
                return index;
        }
        return notFound;
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    void Vector<T, inlineCapacity, OverflowHandler>::fill(const T& val, size_t newSize)
    {
        if (size() > newSize)
            shrink(newSize);
        else if (newSize > capacity()) {
            clear();
            reserveCapacity(newSize);
            if (!begin())
                return;
        }
        
        std::fill(begin(), end(), val);
        TypeOperations::uninitializedFill(end(), begin() + newSize, val);
        m_size = newSize;
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    template<typename Iterator>
    void Vector<T, inlineCapacity, OverflowHandler>::appendRange(Iterator start, Iterator end)
    {
        for (Iterator it = start; it != end; ++it)
            append(*it);
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    void Vector<T, inlineCapacity, OverflowHandler>::expandCapacity(size_t newMinCapacity)
    {
        reserveCapacity(std::max(newMinCapacity, std::max(static_cast<size_t>(16), capacity() + capacity() / 4 + 1)));
    }
    
    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    const T* Vector<T, inlineCapacity, OverflowHandler>::expandCapacity(size_t newMinCapacity, const T* ptr)
    {
        if (ptr < begin() || ptr >= end()) {
            expandCapacity(newMinCapacity);
            return ptr;
        }
        size_t index = ptr - begin();
        expandCapacity(newMinCapacity);
        return begin() + index;
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    bool Vector<T, inlineCapacity, OverflowHandler>::tryExpandCapacity(size_t newMinCapacity)
    {
        return tryReserveCapacity(std::max(newMinCapacity, std::max(static_cast<size_t>(16), capacity() + capacity() / 4 + 1)));
    }
    
    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    const T* Vector<T, inlineCapacity, OverflowHandler>::tryExpandCapacity(size_t newMinCapacity, const T* ptr)
    {
        if (ptr < begin() || ptr >= end()) {
            if (!tryExpandCapacity(newMinCapacity))
                return 0;
            return ptr;
        }
        size_t index = ptr - begin();
        if (!tryExpandCapacity(newMinCapacity))
            return 0;
        return begin() + index;
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler> template<typename U>
    inline U* Vector<T, inlineCapacity, OverflowHandler>::expandCapacity(size_t newMinCapacity, U* ptr)
    {
        expandCapacity(newMinCapacity);
        return ptr;
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    inline void Vector<T, inlineCapacity, OverflowHandler>::resize(size_t size)
    {
        if (size <= m_size)
            TypeOperations::destruct(begin() + size, end());
        else {
            if (size > capacity())
                expandCapacity(size);
            if (begin())
                TypeOperations::initialize(end(), begin() + size);
        }
        
        m_size = size;
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    void Vector<T, inlineCapacity, OverflowHandler>::resizeToFit(size_t size)
    {
        reserveCapacity(size);
        resize(size);
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    void Vector<T, inlineCapacity, OverflowHandler>::shrink(size_t size)
    {
        ASSERT(size <= m_size);
        TypeOperations::destruct(begin() + size, end());
        m_size = size;
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    void Vector<T, inlineCapacity, OverflowHandler>::grow(size_t size)
    {
        ASSERT(size >= m_size);
        if (size > capacity())
            expandCapacity(size);
        if (begin())
            TypeOperations::initialize(end(), begin() + size);
        m_size = size;
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    void Vector<T, inlineCapacity, OverflowHandler>::reserveCapacity(size_t newCapacity)
    {
        if (newCapacity <= capacity())
            return;
        T* oldBuffer = begin();
        T* oldEnd = end();
        Base::allocateBuffer(newCapacity);
        if (begin())
            TypeOperations::move(oldBuffer, oldEnd, begin());
        Base::deallocateBuffer(oldBuffer);
    }
    
    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    bool Vector<T, inlineCapacity, OverflowHandler>::tryReserveCapacity(size_t newCapacity)
    {
        if (newCapacity <= capacity())
            return true;
        T* oldBuffer = begin();
        T* oldEnd = end();
        if (!Base::tryAllocateBuffer(newCapacity))
            return false;
        ASSERT(begin());
        TypeOperations::move(oldBuffer, oldEnd, begin());
        Base::deallocateBuffer(oldBuffer);
        return true;
    }
    
    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    inline void Vector<T, inlineCapacity, OverflowHandler>::reserveInitialCapacity(size_t initialCapacity)
    {
        ASSERT(!m_size);
        ASSERT(capacity() == inlineCapacity);
        if (initialCapacity > inlineCapacity)
            Base::allocateBuffer(initialCapacity);
    }
    
    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    void Vector<T, inlineCapacity, OverflowHandler>::shrinkCapacity(size_t newCapacity)
    {
        if (newCapacity >= capacity())
            return;

        if (newCapacity < size()) 
            shrink(newCapacity);

        T* oldBuffer = begin();
        if (newCapacity > 0) {
            if (Base::shouldReallocateBuffer(newCapacity)) {
                Base::reallocateBuffer(newCapacity);
                return;
            }

            T* oldEnd = end();
            Base::allocateBuffer(newCapacity);
            if (begin() != oldBuffer)
                TypeOperations::move(oldBuffer, oldEnd, begin());
        }

        Base::deallocateBuffer(oldBuffer);
        Base::restoreInlineBufferIfNeeded();
    }

    // Templatizing these is better than just letting the conversion happen implicitly,
    // because for instance it allows a PassRefPtr to be appended to a RefPtr vector
    // without refcount thrash.

    template<typename T, size_t inlineCapacity, typename OverflowHandler> template<typename U>
    void Vector<T, inlineCapacity, OverflowHandler>::append(const U* data, size_t dataSize)
    {
        size_t newSize = m_size + dataSize;
        if (newSize > capacity()) {
            data = expandCapacity(newSize, data);
            if (!begin())
                return;
        }
        if (newSize < m_size)
            CRASH();
        T* dest = end();
        for (size_t i = 0; i < dataSize; ++i)
            new (NotNull, &dest[i]) T(data[i]);
        m_size = newSize;
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler> template<typename U>
    bool Vector<T, inlineCapacity, OverflowHandler>::tryAppend(const U* data, size_t dataSize)
    {
        size_t newSize = m_size + dataSize;
        if (newSize > capacity()) {
            data = tryExpandCapacity(newSize, data);
            if (!data)
                return false;
            ASSERT(begin());
        }
        if (newSize < m_size)
            return false;
        T* dest = end();
        for (size_t i = 0; i < dataSize; ++i)
            new (NotNull, &dest[i]) T(data[i]);
        m_size = newSize;
        return true;
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler> template<typename U>
    ALWAYS_INLINE void Vector<T, inlineCapacity, OverflowHandler>::append(const U& val)
    {
        if (size() != capacity()) {
            new (NotNull, end()) T(val);
            ++m_size;
            return;
        }

        appendSlowCase(val);
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler> template<typename U>
    void Vector<T, inlineCapacity, OverflowHandler>::appendSlowCase(const U& val)
    {
        ASSERT(size() == capacity());

        const U* ptr = &val;
        ptr = expandCapacity(size() + 1, ptr);
        if (!begin())
            return;

        new (NotNull, end()) T(*ptr);
        ++m_size;
    }

    // This version of append saves a branch in the case where you know that the
    // vector's capacity is large enough for the append to succeed.

    template<typename T, size_t inlineCapacity, typename OverflowHandler> template<typename U>
    inline void Vector<T, inlineCapacity, OverflowHandler>::uncheckedAppend(const U& val)
    {
        ASSERT(size() < capacity());
        const U* ptr = &val;
        new (NotNull, end()) T(*ptr);
        ++m_size;
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler> template<typename U, size_t otherCapacity>
    inline void Vector<T, inlineCapacity, OverflowHandler>::appendVector(const Vector<U, otherCapacity>& val)
    {
        append(val.begin(), val.size());
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler> template<typename U>
    void Vector<T, inlineCapacity, OverflowHandler>::insert(size_t position, const U* data, size_t dataSize)
    {
        ASSERT_WITH_SECURITY_IMPLICATION(position <= size());
        size_t newSize = m_size + dataSize;
        if (newSize > capacity()) {
            data = expandCapacity(newSize, data);
            if (!begin())
                return;
        }
        if (newSize < m_size)
            CRASH();
        T* spot = begin() + position;
        TypeOperations::moveOverlapping(spot, end(), spot + dataSize);
        for (size_t i = 0; i < dataSize; ++i)
            new (NotNull, &spot[i]) T(data[i]);
        m_size = newSize;
    }
     
    template<typename T, size_t inlineCapacity, typename OverflowHandler> template<typename U>
    inline void Vector<T, inlineCapacity, OverflowHandler>::insert(size_t position, const U& val)
    {
        ASSERT_WITH_SECURITY_IMPLICATION(position <= size());
        const U* data = &val;
        if (size() == capacity()) {
            data = expandCapacity(size() + 1, data);
            if (!begin())
                return;
        }
        T* spot = begin() + position;
        TypeOperations::moveOverlapping(spot, end(), spot + 1);
        new (NotNull, spot) T(*data);
        ++m_size;
    }
   
    template<typename T, size_t inlineCapacity, typename OverflowHandler> template<typename U, size_t c>
    inline void Vector<T, inlineCapacity, OverflowHandler>::insert(size_t position, const Vector<U, c>& val)
    {
        insert(position, val.begin(), val.size());
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    inline void Vector<T, inlineCapacity, OverflowHandler>::remove(size_t position)
    {
        ASSERT_WITH_SECURITY_IMPLICATION(position < size());
        T* spot = begin() + position;
        spot->~T();
        TypeOperations::moveOverlapping(spot + 1, end(), spot);
        --m_size;
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    inline void Vector<T, inlineCapacity, OverflowHandler>::remove(size_t position, size_t length)
    {
        ASSERT_WITH_SECURITY_IMPLICATION(position <= size());
        ASSERT_WITH_SECURITY_IMPLICATION(position + length <= size());
        T* beginSpot = begin() + position;
        T* endSpot = beginSpot + length;
        TypeOperations::destruct(beginSpot, endSpot); 
        TypeOperations::moveOverlapping(endSpot, end(), beginSpot);
        m_size -= length;
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    inline void Vector<T, inlineCapacity, OverflowHandler>::reverse()
    {
        for (size_t i = 0; i < m_size / 2; ++i)
            std::swap(at(i), at(m_size - 1 - i));
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    inline T* Vector<T, inlineCapacity, OverflowHandler>::releaseBuffer()
    {
        T* buffer = Base::releaseBuffer();
        if (inlineCapacity && !buffer && m_size) {
            // If the vector had some data, but no buffer to release,
            // that means it was using the inline buffer. In that case,
            // we create a brand new buffer so the caller always gets one.
            size_t bytes = m_size * sizeof(T);
            buffer = static_cast<T*>(fastMalloc(bytes));
            memcpy(buffer, data(), bytes);
        }
        m_size = 0;
        return buffer;
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    inline void Vector<T, inlineCapacity, OverflowHandler>::checkConsistency()
    {
#if !ASSERT_DISABLED
        for (size_t i = 0; i < size(); ++i)
            ValueCheck<T>::checkConsistency(at(i));
#endif
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    void deleteAllValues(const Vector<T, inlineCapacity, OverflowHandler>& collection)
    {
        typedef typename Vector<T, inlineCapacity, OverflowHandler>::const_iterator iterator;
        iterator end = collection.end();
        for (iterator it = collection.begin(); it != end; ++it)
            delete *it;
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    inline void swap(Vector<T, inlineCapacity, OverflowHandler>& a, Vector<T, inlineCapacity, OverflowHandler>& b)
    {
        a.swap(b);
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    bool operator==(const Vector<T, inlineCapacity, OverflowHandler>& a, const Vector<T, inlineCapacity, OverflowHandler>& b)
    {
        if (a.size() != b.size())
            return false;

        return VectorTypeOperations<T>::compare(a.data(), b.data(), a.size());
    }

    template<typename T, size_t inlineCapacity, typename OverflowHandler>
    inline bool operator!=(const Vector<T, inlineCapacity, OverflowHandler>& a, const Vector<T, inlineCapacity, OverflowHandler>& b)
    {
        return !(a == b);
    }

#if !ASSERT_DISABLED
    template<typename T> struct ValueCheck<Vector<T> > {
        typedef Vector<T> TraitType;
        static void checkConsistency(const Vector<T>& v)
        {
            v.checkConsistency();
        }
    };
#endif

} // namespace WTF

using WTF::Vector;
using WTF::UnsafeVectorOverflow;

#endif // WTF_Vector_h
