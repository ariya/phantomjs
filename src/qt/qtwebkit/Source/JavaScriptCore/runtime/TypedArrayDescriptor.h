/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef TypedArrayDescriptor_h
#define TypedArrayDescriptor_h

namespace JSC {

struct ClassInfo;

enum TypedArrayType {
    TypedArrayNone,
    TypedArrayInt8,
    TypedArrayInt16,
    TypedArrayInt32,
    TypedArrayUint8,
    TypedArrayUint8Clamped,
    TypedArrayUint16,
    TypedArrayUint32,
    TypedArrayFloat32,
    TypedArrayFloat64
};

struct TypedArrayDescriptor {
    TypedArrayDescriptor()
        : m_classInfo(0)
        , m_storageOffset(0)
        , m_lengthOffset(0)
    {
    }
    TypedArrayDescriptor(const ClassInfo* classInfo, size_t storageOffset, size_t lengthOffset)
        : m_classInfo(classInfo)
        , m_storageOffset(storageOffset)
        , m_lengthOffset(lengthOffset)
    {
    }
    const ClassInfo* m_classInfo;
    size_t m_storageOffset;
    size_t m_lengthOffset;
};

enum TypedArraySignedness {
    SignedTypedArray,
    UnsignedTypedArray
};
enum TypedArrayRounding {
    TruncateRounding,
    ClampRounding
};

} // namespace JSC

#endif // TypedArrayDescriptor_h

