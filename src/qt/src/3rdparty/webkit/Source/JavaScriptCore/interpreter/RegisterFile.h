/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RegisterFile_h
#define RegisterFile_h

#include "Heap.h"
#include "ExecutableAllocator.h"
#include "Register.h"
#include "Weak.h"
#include <stdio.h>
#include <wtf/Noncopyable.h>
#include <wtf/PageReservation.h>
#include <wtf/VMTags.h>

namespace JSC {

/*
    A register file is a stack of register frames. We represent a register
    frame by its offset from "base", the logical first entry in the register
    file. The bottom-most register frame's offset from base is 0.

    In a program where function "a" calls function "b" (global code -> a -> b),
    the register file might look like this:

    |       global frame     |        call frame      |        call frame      |     spare capacity     |
    -----------------------------------------------------------------------------------------------------
    |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 |    |    |    |    |    | <-- index in buffer
    -----------------------------------------------------------------------------------------------------
    | -3 | -2 | -1 |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 |    |    |    |    |    | <-- index relative to base
    -----------------------------------------------------------------------------------------------------
    |    <-globals | temps-> |  <-vars | temps->      |                 <-vars |
       ^              ^                   ^                                       ^
       |              |                   |                                       |
     buffer    base (frame 0)          frame 1                                 frame 2

    Since all variables, including globals, are accessed by negative offsets
    from their register frame pointers, to keep old global offsets correct, new
    globals must appear at the beginning of the register file, shifting base
    to the right.

    If we added one global variable to the register file depicted above, it
    would look like this:

    |         global frame        |<                                                                    >
    ------------------------------->                                                                    <
    |  0 |  1 |  2 |  3 |  4 |  5 |<                             >snip<                                 > <-- index in buffer
    ------------------------------->                                                                    <
    | -4 | -3 | -2 | -1 |  0 |  1 |<                                                                    > <-- index relative to base
    ------------------------------->                                                                    <
    |         <-globals | temps-> |
       ^                   ^
       |                   |
     buffer         base (frame 0)

    As you can see, global offsets relative to base have stayed constant,
    but base itself has moved. To keep up with possible changes to base,
    clients keep an indirect pointer, so their calculations update
    automatically when base changes.

    For client simplicity, the RegisterFile measures size and capacity from
    "base", not "buffer".
*/

    class JSGlobalObject;

    class RegisterFile {
        WTF_MAKE_NONCOPYABLE(RegisterFile);
    public:
        enum CallFrameHeaderEntry {
            CallFrameHeaderSize = 6,

            ArgumentCount = -6,
            CallerFrame = -5,
            Callee = -4,
            ScopeChain = -3,
            ReturnPC = -2, // This is either an Instruction* or a pointer into JIT generated code stored as an Instruction*.
            CodeBlock = -1,
        };

        enum { ProgramCodeThisRegister = -CallFrameHeaderSize - 1 };

        static const size_t defaultCapacity = 512 * 1024;
        static const size_t defaultMaxGlobals = 8 * 1024;
        static const size_t commitSize = 16 * 1024;
        // Allow 8k of excess registers before we start trying to reap the registerfile
        static const ptrdiff_t maxExcessCapacity = 8 * 1024;

        RegisterFile(JSGlobalData&, size_t capacity = defaultCapacity, size_t maxGlobals = defaultMaxGlobals);
        ~RegisterFile();
        
        void gatherConservativeRoots(ConservativeRoots&);

        Register* start() const { return m_start; }
        Register* end() const { return m_end; }
        size_t size() const { return m_end - m_start; }

        void setGlobalObject(JSGlobalObject*);
        JSGlobalObject* globalObject();

        bool grow(Register* newEnd);
        void shrink(Register* newEnd);
        
        void setNumGlobals(size_t numGlobals) { m_numGlobals = numGlobals; }
        int numGlobals() const { return m_numGlobals; }
        size_t maxGlobals() const { return m_maxGlobals; }

        Register* lastGlobal() const { return m_start - m_numGlobals; }
        
        static size_t committedByteCount();
        static void initializeThreading();

        Register* const * addressOfEnd() const
        {
            return &m_end;
        }

    private:
        void releaseExcessCapacity();
        void addToCommittedByteCount(long);
        size_t m_numGlobals;
        const size_t m_maxGlobals;
        Register* m_start;
        Register* m_end;
        Register* m_max;
        Register* m_maxUsed;
        Register* m_commitEnd;
        PageReservation m_reservation;

        Weak<JSGlobalObject> m_globalObject; // The global object whose vars are currently stored in the register file.
        class GlobalObjectOwner : public WeakHandleOwner {
            virtual void finalize(Handle<Unknown>, void* context)
            {
                static_cast<RegisterFile*>(context)->setNumGlobals(0);
            }
        } m_globalObjectOwner;
    };

    inline RegisterFile::RegisterFile(JSGlobalData& globalData, size_t capacity, size_t maxGlobals)
        : m_numGlobals(0)
        , m_maxGlobals(maxGlobals)
        , m_start(0)
        , m_end(0)
        , m_max(0)
        , m_globalObject(globalData, 0, &m_globalObjectOwner, this)
    {
        ASSERT(maxGlobals && isPageAligned(maxGlobals));
        ASSERT(capacity && isPageAligned(capacity));
        size_t bufferLength = (capacity + maxGlobals) * sizeof(Register);
        m_reservation = PageReservation::reserve(roundUpAllocationSize(bufferLength, commitSize), OSAllocator::JSVMStackPages);
        void* base = m_reservation.base();
        size_t committedSize = roundUpAllocationSize(maxGlobals * sizeof(Register), commitSize);
        m_reservation.commit(base, committedSize);
        addToCommittedByteCount(static_cast<long>(committedSize));
        m_commitEnd = reinterpret_cast_ptr<Register*>(reinterpret_cast<char*>(base) + committedSize);
        m_start = static_cast<Register*>(base) + maxGlobals;
        m_end = m_start;
        m_maxUsed = m_end;
        m_max = m_start + capacity;
    }

    inline void RegisterFile::shrink(Register* newEnd)
    {
        if (newEnd >= m_end)
            return;
        m_end = newEnd;
        if (m_end == m_start && (m_maxUsed - m_start) > maxExcessCapacity)
            releaseExcessCapacity();
    }

    inline bool RegisterFile::grow(Register* newEnd)
    {
        if (newEnd < m_end)
            return true;

        if (newEnd > m_max)
            return false;

        if (newEnd > m_commitEnd) {
            size_t size = roundUpAllocationSize(reinterpret_cast<char*>(newEnd) - reinterpret_cast<char*>(m_commitEnd), commitSize);
            m_reservation.commit(m_commitEnd, size);
            addToCommittedByteCount(static_cast<long>(size));
            m_commitEnd = reinterpret_cast_ptr<Register*>(reinterpret_cast<char*>(m_commitEnd) + size);
        }

        if (newEnd > m_maxUsed)
            m_maxUsed = newEnd;

        m_end = newEnd;
        return true;
    }

} // namespace JSC

#endif // RegisterFile_h
