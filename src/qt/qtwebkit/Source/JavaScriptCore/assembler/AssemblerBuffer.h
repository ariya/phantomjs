/*
 * Copyright (C) 2008, 2012 Apple Inc. All rights reserved.
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

#ifndef AssemblerBuffer_h
#define AssemblerBuffer_h

#if ENABLE(ASSEMBLER)

#include "ExecutableAllocator.h"
#include "JITCompilationEffort.h"
#include "VM.h"
#include "stdint.h"
#include <string.h>
#include <wtf/Assertions.h>
#include <wtf/FastMalloc.h>
#include <wtf/StdLibExtras.h>

namespace JSC {

    struct AssemblerLabel {
        AssemblerLabel()
            : m_offset(std::numeric_limits<uint32_t>::max())
        {
        }

        explicit AssemblerLabel(uint32_t offset)
            : m_offset(offset)
        {
        }

        bool isSet() const { return (m_offset != std::numeric_limits<uint32_t>::max()); }

        AssemblerLabel labelAtOffset(int offset) const
        {
            return AssemblerLabel(m_offset + offset);
        }

        uint32_t m_offset;
    };

    class AssemblerBuffer {
        static const int inlineCapacity = 128;
    public:
        AssemblerBuffer()
            : m_storage(inlineCapacity)
            , m_buffer(m_storage.begin())
            , m_capacity(inlineCapacity)
            , m_index(0)
        {
        }

        ~AssemblerBuffer()
        {
        }

        bool isAvailable(int space)
        {
            return m_index + space <= m_capacity;
        }

        void ensureSpace(int space)
        {
            if (!isAvailable(space))
                grow();
        }

        bool isAligned(int alignment) const
        {
            return !(m_index & (alignment - 1));
        }

        template<typename IntegralType>
        void putIntegral(IntegralType value)
        {
            ensureSpace(sizeof(IntegralType));
            putIntegralUnchecked(value);
        }

        template<typename IntegralType>
        void putIntegralUnchecked(IntegralType value)
        {
            ASSERT(isAvailable(sizeof(IntegralType)));
            *reinterpret_cast_ptr<IntegralType*>(m_buffer + m_index) = value;
            m_index += sizeof(IntegralType);
        }

        void putByteUnchecked(int8_t value) { putIntegralUnchecked(value); }
        void putByte(int8_t value) { putIntegral(value); }
        void putShortUnchecked(int16_t value) { putIntegralUnchecked(value); }
        void putShort(int16_t value) { putIntegral(value); }
        void putIntUnchecked(int32_t value) { putIntegralUnchecked(value); }
        void putInt(int32_t value) { putIntegral(value); }
        void putInt64Unchecked(int64_t value) { putIntegralUnchecked(value); }
        void putInt64(int64_t value) { putIntegral(value); }

        void* data() const
        {
            return m_buffer;
        }

        size_t codeSize() const
        {
            return m_index;
        }

        AssemblerLabel label() const
        {
            return AssemblerLabel(m_index);
        }

        PassRefPtr<ExecutableMemoryHandle> executableCopy(VM& vm, void* ownerUID, JITCompilationEffort effort)
        {
            if (!m_index)
                return 0;

            RefPtr<ExecutableMemoryHandle> result = vm.executableAllocator.allocate(vm, m_index, ownerUID, effort);

            if (!result)
                return 0;

            ExecutableAllocator::makeWritable(result->start(), result->sizeInBytes());

            memcpy(result->start(), m_buffer, m_index);
            
            return result.release();
        }

        unsigned debugOffset() { return m_index; }

    protected:
        void append(const char* data, int size)
        {
            if (!isAvailable(size))
                grow(size);

            memcpy(m_buffer + m_index, data, size);
            m_index += size;
        }

        void grow(int extraCapacity = 0)
        {
            m_capacity += m_capacity / 2 + extraCapacity;

            m_storage.grow(m_capacity);
            m_buffer = m_storage.begin();
        }

    private:
        Vector<char, inlineCapacity, UnsafeVectorOverflow> m_storage;
        char* m_buffer;
        int m_capacity;
        int m_index;
    };

} // namespace JSC

#endif // ENABLE(ASSEMBLER)

#endif // AssemblerBuffer_h
