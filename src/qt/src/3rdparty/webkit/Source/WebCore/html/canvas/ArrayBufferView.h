/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef ArrayBufferView_h
#define ArrayBufferView_h

#include "ArrayBuffer.h"
#include "ExceptionCode.h"

#include <algorithm>
#include <limits.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class ArrayBufferView : public RefCounted<ArrayBufferView> {
  public:
    virtual bool isByteArray() const { return false; }
    virtual bool isUnsignedByteArray() const { return false; }
    virtual bool isShortArray() const { return false; }
    virtual bool isUnsignedShortArray() const { return false; }
    virtual bool isIntArray() const { return false; }
    virtual bool isUnsignedIntArray() const { return false; }
    virtual bool isFloatArray() const { return false; }
    virtual bool isDataView() const { return false; }

    PassRefPtr<ArrayBuffer> buffer() const
    {
        return m_buffer;
    }

    void* baseAddress() const
    {
        return m_baseAddress;
    }

    unsigned byteOffset() const
    {
        return m_byteOffset;
    }

    virtual unsigned byteLength() const = 0;

    virtual ~ArrayBufferView();

  protected:
    ArrayBufferView(PassRefPtr<ArrayBuffer> buffer, unsigned byteOffset);

    void setImpl(ArrayBufferView* array, unsigned byteOffset, ExceptionCode& ec);

    void setRangeImpl(const char* data, size_t dataByteLength, unsigned byteOffset, ExceptionCode& ec);

    void zeroRangeImpl(unsigned byteOffset, size_t rangeByteLength, ExceptionCode& ec);

    static void calculateOffsetAndLength(int start, int end, unsigned arraySize,
                                         unsigned* offset, unsigned* length);

    // Helper to verify that a given sub-range of an ArrayBuffer is
    // within range.
    template <typename T>
    static bool verifySubRange(PassRefPtr<ArrayBuffer> buffer,
                               unsigned byteOffset,
                               unsigned numElements)
    {
        if (!buffer)
            return false;
        if (sizeof(T) > 1 && byteOffset % sizeof(T))
            return false;
        if (byteOffset > buffer->byteLength())
            return false;
        unsigned remainingElements = (buffer->byteLength() - byteOffset) / sizeof(T);
        if (numElements > remainingElements)
            return false;
        return true;
    }

    // Input offset is in number of elements from this array's view;
    // output offset is in number of bytes from the underlying buffer's view.
    template <typename T>
    static void clampOffsetAndNumElements(PassRefPtr<ArrayBuffer> buffer,
                                          unsigned arrayByteOffset,
                                          unsigned *offset,
                                          unsigned *numElements)
    {
        unsigned maxOffset = (UINT_MAX - arrayByteOffset) / sizeof(T);
        if (*offset > maxOffset) {
            *offset = buffer->byteLength();
            *numElements = 0;
            return;
        }
        *offset = arrayByteOffset + *offset * sizeof(T);
        *offset = std::min(buffer->byteLength(), *offset);
        unsigned remainingElements = (buffer->byteLength() - *offset) / sizeof(T);
        *numElements = std::min(remainingElements, *numElements);
    }

    // This is the address of the ArrayBuffer's storage, plus the byte offset.
    void* m_baseAddress;

    unsigned m_byteOffset;

  private:
    RefPtr<ArrayBuffer> m_buffer;
};

} // namespace WebCore

#endif // ArrayBufferView_h
