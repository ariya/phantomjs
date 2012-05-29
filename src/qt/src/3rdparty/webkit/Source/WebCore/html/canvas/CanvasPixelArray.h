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

#ifndef CanvasPixelArray_h
#define CanvasPixelArray_h

#include <wtf/ByteArray.h>
#include <wtf/MathExtras.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {
    
class CanvasPixelArray : public RefCounted<CanvasPixelArray> {
public:
    static PassRefPtr<CanvasPixelArray> create(unsigned length);
    static PassRefPtr<CanvasPixelArray> create(PassRefPtr<ByteArray>);

    ByteArray* data() { return m_data.get(); }
    const ByteArray* data() const { return m_data.get(); }
    unsigned length() const { return m_data->length(); }
    
    void set(unsigned index, double value)
    {
        m_data->set(index, value);
    }

    void set(unsigned index, unsigned char value)
    {
        m_data->set(index, value);
    }
    
    bool get(unsigned index, unsigned char& result) const
    {
        return m_data->get(index, result);
    }

    unsigned char get(unsigned index) const
    {
        return m_data->get(index);
    }

private:
    CanvasPixelArray(unsigned length);
    CanvasPixelArray(PassRefPtr<ByteArray>);

    RefPtr<ByteArray> m_data;
};
    
} // namespace WebCore

#endif // CanvasPixelArray_h
