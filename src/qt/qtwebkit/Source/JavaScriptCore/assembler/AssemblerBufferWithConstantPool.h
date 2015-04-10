/*
 * Copyright (C) 2009 University of Szeged
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY UNIVERSITY OF SZEGED ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL UNIVERSITY OF SZEGED OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AssemblerBufferWithConstantPool_h
#define AssemblerBufferWithConstantPool_h

#if ENABLE(ASSEMBLER)

#include "AssemblerBuffer.h"
#include <wtf/SegmentedVector.h>

#define ASSEMBLER_HAS_CONSTANT_POOL 1

namespace JSC {

/*
    On a constant pool 4 or 8 bytes data can be stored. The values can be
    constants or addresses. The addresses should be 32 or 64 bits. The constants
    should be double-precisions float or integer numbers which are hard to be
    encoded as few machine instructions.

    TODO: The pool is desinged to handle both 32 and 64 bits values, but
    currently only the 4 bytes constants are implemented and tested.

    The AssemblerBuffer can contain multiple constant pools. Each pool is inserted
    into the instruction stream - protected by a jump instruction from the
    execution flow.

    The flush mechanism is called when no space remain to insert the next instruction
    into the pool. Three values are used to determine when the constant pool itself
    have to be inserted into the instruction stream (Assembler Buffer):

    - maxPoolSize: size of the constant pool in bytes, this value cannot be
        larger than the maximum offset of a PC relative memory load

    - barrierSize: size of jump instruction in bytes which protects the
        constant pool from execution

    - maxInstructionSize: maximum length of a machine instruction in bytes

    There are some callbacks which solve the target architecture specific
    address handling:

    - TYPE patchConstantPoolLoad(TYPE load, int value):
        patch the 'load' instruction with the index of the constant in the
        constant pool and return the patched instruction.

    - void patchConstantPoolLoad(void* loadAddr, void* constPoolAddr):
        patch the a PC relative load instruction at 'loadAddr' address with the
        final relative offset. The offset can be computed with help of
        'constPoolAddr' (the address of the constant pool) and index of the
        constant (which is stored previously in the load instruction itself).

    - TYPE placeConstantPoolBarrier(int size):
        return with a constant pool barrier instruction which jumps over the
        constant pool.

    The 'put*WithConstant*' functions should be used to place a data into the
    constant pool.
*/

template <int maxPoolSize, int barrierSize, int maxInstructionSize, class AssemblerType>
class AssemblerBufferWithConstantPool : public AssemblerBuffer {
    typedef SegmentedVector<uint32_t, 512> LoadOffsets;
    using AssemblerBuffer::putIntegral;
    using AssemblerBuffer::putIntegralUnchecked;
public:
    typedef struct {
        short high;
        short low;
    } TwoShorts;

    enum {
        UniqueConst,
        ReusableConst,
        UnusedEntry,
    };

    AssemblerBufferWithConstantPool()
        : AssemblerBuffer()
        , m_numConsts(0)
        , m_maxDistance(maxPoolSize)
        , m_lastConstDelta(0)
    {
        m_pool = static_cast<uint32_t*>(fastMalloc(maxPoolSize));
        m_mask = static_cast<char*>(fastMalloc(maxPoolSize / sizeof(uint32_t)));
    }

    ~AssemblerBufferWithConstantPool()
    {
        fastFree(m_mask);
        fastFree(m_pool);
    }

    void ensureSpace(int space)
    {
        flushIfNoSpaceFor(space);
        AssemblerBuffer::ensureSpace(space);
    }

    void ensureSpace(int insnSpace, int constSpace)
    {
        flushIfNoSpaceFor(insnSpace, constSpace);
        AssemblerBuffer::ensureSpace(insnSpace);
    }

    void ensureSpaceForAnyInstruction(int amount = 1)
    {
        flushIfNoSpaceFor(amount * maxInstructionSize, amount * sizeof(uint64_t));
    }

    bool isAligned(int alignment)
    {
        flushIfNoSpaceFor(alignment);
        return AssemblerBuffer::isAligned(alignment);
    }

    void putByteUnchecked(int value)
    {
        AssemblerBuffer::putByteUnchecked(value);
        correctDeltas(1);
    }

    void putByte(int value)
    {
        flushIfNoSpaceFor(1);
        AssemblerBuffer::putByte(value);
        correctDeltas(1);
    }

    void putShortUnchecked(int value)
    {
        AssemblerBuffer::putShortUnchecked(value);
        correctDeltas(2);
    }

    void putShort(int value)
    {
        flushIfNoSpaceFor(2);
        AssemblerBuffer::putShort(value);
        correctDeltas(2);
    }

    void putIntUnchecked(int value)
    {
        AssemblerBuffer::putIntUnchecked(value);
        correctDeltas(4);
    }

    void putInt(int value)
    {
        flushIfNoSpaceFor(4);
        AssemblerBuffer::putInt(value);
        correctDeltas(4);
    }

    void putInt64Unchecked(int64_t value)
    {
        AssemblerBuffer::putInt64Unchecked(value);
        correctDeltas(8);
    }

    void putIntegral(TwoShorts value)
    {
        putIntegral(value.high);
        putIntegral(value.low);
    }

    void putIntegralUnchecked(TwoShorts value)
    {
        putIntegralUnchecked(value.high);
        putIntegralUnchecked(value.low);
    }

    PassRefPtr<ExecutableMemoryHandle> executableCopy(VM& vm, void* ownerUID, JITCompilationEffort effort)
    {
        flushConstantPool(false);
        return AssemblerBuffer::executableCopy(vm, ownerUID, effort);
    }

    void putShortWithConstantInt(uint16_t insn, uint32_t constant, bool isReusable = false)
    {
        putIntegralWithConstantInt(insn, constant, isReusable);
    }

    void putIntWithConstantInt(uint32_t insn, uint32_t constant, bool isReusable = false)
    {
        putIntegralWithConstantInt(insn, constant, isReusable);
    }

    // This flushing mechanism can be called after any unconditional jumps.
    void flushWithoutBarrier(bool isForced = false)
    {
        // Flush if constant pool is more than 60% full to avoid overuse of this function.
        if (isForced || 5 * static_cast<uint32_t>(m_numConsts) > 3 * maxPoolSize / sizeof(uint32_t))
            flushConstantPool(false);
    }

    uint32_t* poolAddress()
    {
        return m_pool;
    }

    int sizeOfConstantPool()
    {
        return m_numConsts;
    }

private:
    void correctDeltas(int insnSize)
    {
        m_maxDistance -= insnSize;
        m_lastConstDelta -= insnSize;
        if (m_lastConstDelta < 0)
            m_lastConstDelta = 0;
    }

    void correctDeltas(int insnSize, int constSize)
    {
        correctDeltas(insnSize);

        m_maxDistance -= m_lastConstDelta;
        m_lastConstDelta = constSize;
    }

    template<typename IntegralType>
    void putIntegralWithConstantInt(IntegralType insn, uint32_t constant, bool isReusable)
    {
        if (!m_numConsts)
            m_maxDistance = maxPoolSize;
        flushIfNoSpaceFor(sizeof(IntegralType), 4);

        m_loadOffsets.append(codeSize());
        if (isReusable) {
            for (int i = 0; i < m_numConsts; ++i) {
                if (m_mask[i] == ReusableConst && m_pool[i] == constant) {
                    putIntegral(static_cast<IntegralType>(AssemblerType::patchConstantPoolLoad(insn, i)));
                    correctDeltas(sizeof(IntegralType));
                    return;
                }
            }
        }

        m_pool[m_numConsts] = constant;
        m_mask[m_numConsts] = static_cast<char>(isReusable ? ReusableConst : UniqueConst);

        putIntegral(static_cast<IntegralType>(AssemblerType::patchConstantPoolLoad(insn, m_numConsts)));
        ++m_numConsts;

        correctDeltas(sizeof(IntegralType), 4);
    }

    void flushConstantPool(bool useBarrier = true)
    {
        if (m_numConsts == 0)
            return;
        int alignPool = (codeSize() + (useBarrier ? barrierSize : 0)) & (sizeof(uint64_t) - 1);

        if (alignPool)
            alignPool = sizeof(uint64_t) - alignPool;

        // Callback to protect the constant pool from execution
        if (useBarrier)
            putIntegral(AssemblerType::placeConstantPoolBarrier(m_numConsts * sizeof(uint32_t) + alignPool));

        if (alignPool) {
            if (alignPool & 1)
                AssemblerBuffer::putByte(AssemblerType::padForAlign8);
            if (alignPool & 2)
                AssemblerBuffer::putShort(AssemblerType::padForAlign16);
            if (alignPool & 4)
                AssemblerBuffer::putInt(AssemblerType::padForAlign32);
        }

        int constPoolOffset = codeSize();
        append(reinterpret_cast<char*>(m_pool), m_numConsts * sizeof(uint32_t));

        // Patch each PC relative load
        for (LoadOffsets::Iterator iter = m_loadOffsets.begin(); iter != m_loadOffsets.end(); ++iter) {
            void* loadAddr = reinterpret_cast<char*>(data()) + *iter;
            AssemblerType::patchConstantPoolLoad(loadAddr, reinterpret_cast<char*>(data()) + constPoolOffset);
        }

        m_loadOffsets.clear();
        m_numConsts = 0;
    }

    void flushIfNoSpaceFor(int nextInsnSize)
    {
        if (m_numConsts == 0)
            return;
        int lastConstDelta = m_lastConstDelta > nextInsnSize ? m_lastConstDelta - nextInsnSize : 0;
        if ((m_maxDistance < nextInsnSize + lastConstDelta + barrierSize + (int)sizeof(uint32_t)))
            flushConstantPool();
    }

    void flushIfNoSpaceFor(int nextInsnSize, int nextConstSize)
    {
        if (m_numConsts == 0)
            return;
        if ((m_maxDistance < nextInsnSize + m_lastConstDelta + nextConstSize + barrierSize + (int)sizeof(uint32_t)) ||
            (m_numConsts * sizeof(uint32_t) + nextConstSize >= maxPoolSize))
            flushConstantPool();
    }

    uint32_t* m_pool;
    char* m_mask;
    LoadOffsets m_loadOffsets;

    int m_numConsts;
    int m_maxDistance;
    int m_lastConstDelta;
};

} // namespace JSC

#endif // ENABLE(ASSEMBLER)

#endif // AssemblerBufferWithConstantPool_h
