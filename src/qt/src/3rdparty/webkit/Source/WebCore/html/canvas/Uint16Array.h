/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef Uint16Array_h
#define Uint16Array_h

#include "IntegralTypedArrayBase.h"

namespace WebCore {

class ArrayBuffer;

class Uint16Array : public IntegralTypedArrayBase<unsigned short> {
public:
    static PassRefPtr<Uint16Array> create(unsigned length);
    static PassRefPtr<Uint16Array> create(unsigned short* array, unsigned length);
    static PassRefPtr<Uint16Array> create(PassRefPtr<ArrayBuffer> buffer, unsigned byteOffset, unsigned length);

    // Can’t use "using" here due to a bug in the RVCT compiler.
    void set(TypedArrayBase<unsigned short>* array, unsigned offset, ExceptionCode& ec) { TypedArrayBase<unsigned short>::set(array, offset, ec); }
    void set(unsigned index, double value) { IntegralTypedArrayBase<unsigned short>::set(index, value); }

    PassRefPtr<Uint16Array> subarray(int start) const;
    PassRefPtr<Uint16Array> subarray(int start, int end) const;

private:
    Uint16Array(PassRefPtr<ArrayBuffer> buffer,
                            unsigned byteOffset,
                            unsigned length);
    // Make constructor visible to superclass.
    friend class TypedArrayBase<unsigned short>;

    // Overridden from ArrayBufferView.
    virtual bool isUnsignedShortArray() const { return true; }
};

} // namespace WebCore

#endif // Uint16Array_h
