/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef ArgList_h
#define ArgList_h

#include "CallFrame.h"
#include "Register.h"
#include <wtf/HashSet.h>
#include <wtf/Vector.h>

namespace JSC {

    class MarkStack;
    typedef MarkStack SlotVisitor;

    class MarkedArgumentBuffer {
        WTF_MAKE_NONCOPYABLE(MarkedArgumentBuffer);
    private:
        static const unsigned inlineCapacity = 8;
        typedef Vector<Register, inlineCapacity> VectorType;
        typedef HashSet<MarkedArgumentBuffer*> ListSet;

    public:
        typedef VectorType::iterator iterator;
        typedef VectorType::const_iterator const_iterator;

        // Constructor for a read-write list, to which you may append values.
        // FIXME: Remove all clients of this API, then remove this API.
        MarkedArgumentBuffer()
            : m_isUsingInlineBuffer(true)
            , m_markSet(0)
#ifndef NDEBUG
            , m_isReadOnly(false)
#endif
        {
            m_buffer = m_vector.data();
            m_size = 0;
        }

        // Constructor for a read-only list whose data has already been allocated elsewhere.
        MarkedArgumentBuffer(Register* buffer, size_t size)
            : m_buffer(buffer)
            , m_size(size)
            , m_isUsingInlineBuffer(true)
            , m_markSet(0)
#ifndef NDEBUG
            , m_isReadOnly(true)
#endif
        {
        }

        void initialize(WriteBarrier<Unknown>* buffer, size_t size)
        {
            ASSERT(!m_markSet);
            ASSERT(isEmpty());

            m_buffer = reinterpret_cast<Register*>(buffer);
            m_size = size;
#ifndef NDEBUG
            m_isReadOnly = true;
#endif
        }

        ~MarkedArgumentBuffer()
        {
            if (m_markSet)
                m_markSet->remove(this);
        }

        size_t size() const { return m_size; }
        bool isEmpty() const { return !m_size; }

        JSValue at(size_t i) const
        {
            if (i < m_size)
                return m_buffer[i].jsValue();
            return jsUndefined();
        }

        void clear()
        {
            m_vector.clear();
            m_buffer = 0;
            m_size = 0;
        }

        void append(JSValue v)
        {
            ASSERT(!m_isReadOnly);

#if ENABLE(JSC_ZOMBIES)
            ASSERT(!v.isZombie());
#endif

            if (m_isUsingInlineBuffer && m_size < inlineCapacity) {
                m_vector.uncheckedAppend(v);
                ++m_size;
            } else {
                // Putting this case all in one function measurably improves
                // the performance of the fast "just append to inline buffer" case.
                slowAppend(v);
                ++m_size;
                m_isUsingInlineBuffer = false;
            }
        }

        void removeLast()
        { 
            ASSERT(m_size);
            m_size--;
            m_vector.removeLast();
        }

        JSValue last() 
        {
            ASSERT(m_size);
            return m_buffer[m_size - 1].jsValue();
        }
        
        iterator begin() { return m_buffer; }
        iterator end() { return m_buffer + m_size; }

        const_iterator begin() const { return m_buffer; }
        const_iterator end() const { return m_buffer + m_size; }

        static void markLists(HeapRootVisitor&, ListSet&);

    private:
        void slowAppend(JSValue);
        
        Register* m_buffer;
        size_t m_size;
        bool m_isUsingInlineBuffer;

        VectorType m_vector;
        ListSet* m_markSet;
#ifndef NDEBUG
        bool m_isReadOnly;
#endif

    private:
        // Prohibits new / delete, which would break GC.
        friend class JSGlobalData;
        
        void* operator new(size_t size)
        {
            return fastMalloc(size);
        }
        void operator delete(void* p)
        {
            fastFree(p);
        }

        void* operator new[](size_t);
        void operator delete[](void*);

        void* operator new(size_t, void*);
        void operator delete(void*, size_t);
    };

    class ArgList {
        friend class JIT;
    public:
        typedef JSValue* iterator;
        typedef const JSValue* const_iterator;

        ArgList()
            : m_args(0)
            , m_argCount(0)
        {
        }
        
        ArgList(ExecState* exec)
            : m_args(reinterpret_cast<JSValue*>(&exec[exec->hostThisRegister() + 1]))
            , m_argCount(exec->argumentCount())
        {
        }
        
        ArgList(JSValue* args, unsigned argCount)
            : m_args(args)
            , m_argCount(argCount)
        {
#if ENABLE(JSC_ZOMBIES)
            for (size_t i = 0; i < argCount; i++)
                ASSERT(!m_args[i].isZombie());
#endif
        }
        
        ArgList(Register* args, int argCount)
            : m_args(reinterpret_cast<JSValue*>(args))
            , m_argCount(argCount)
        {
            ASSERT(argCount >= 0);
        }

        ArgList(const MarkedArgumentBuffer& args)
            : m_args(reinterpret_cast<JSValue*>(const_cast<Register*>(args.begin())))
            , m_argCount(args.size())
        {
        }

        JSValue at(size_t idx) const
        {
            if (idx < m_argCount)
                return m_args[idx];
            return jsUndefined();
        }

        bool isEmpty() const { return !m_argCount; }

        size_t size() const { return m_argCount; }
        
        iterator begin() { return m_args; }
        iterator end() { return m_args + m_argCount; }
        
        const_iterator begin() const { return m_args; }
        const_iterator end() const { return m_args + m_argCount; }

        void getSlice(int startIndex, ArgList& result) const;
    private:
        JSValue* m_args;
        size_t m_argCount;
    };

} // namespace JSC

#endif // ArgList_h
