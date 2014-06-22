/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef DataView_h
#define DataView_h

#include <wtf/ArrayBufferView.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

typedef int ExceptionCode;

class DataView : public ArrayBufferView {
public:
    static PassRefPtr<DataView> create(unsigned length);
    static PassRefPtr<DataView> create(PassRefPtr<ArrayBuffer>, unsigned byteOffset, unsigned byteLength);

    virtual unsigned length() const { return m_byteLength; }
    virtual unsigned byteLength() const { return m_byteLength; }
    virtual PassRefPtr<ArrayBufferView> slice(int, int) const { return 0; }

    int8_t getInt8(unsigned byteOffset, ExceptionCode&);
    uint8_t getUint8(unsigned byteOffset, ExceptionCode&);
    int16_t getInt16(unsigned byteOffset, ExceptionCode& ec) { return getInt16(byteOffset, false, ec); }
    int16_t getInt16(unsigned byteOffset, bool littleEndian, ExceptionCode&);
    uint16_t getUint16(unsigned byteOffset, ExceptionCode& ec) { return getUint16(byteOffset, false, ec); }
    uint16_t getUint16(unsigned byteOffset, bool littleEndian, ExceptionCode&);
    int32_t getInt32(unsigned byteOffset, ExceptionCode& ec) { return getInt32(byteOffset, false, ec); }
    int32_t getInt32(unsigned byteOffset, bool littleEndian, ExceptionCode&);
    uint32_t getUint32(unsigned byteOffset, ExceptionCode& ec) { return getUint32(byteOffset, false, ec); }
    uint32_t getUint32(unsigned byteOffset, bool littleEndian, ExceptionCode&);
    float getFloat32(unsigned byteOffset, ExceptionCode& ec) { return getFloat32(byteOffset, false, ec); }
    float getFloat32(unsigned byteOffset, bool littleEndian, ExceptionCode&);
    double getFloat64(unsigned byteOffset, ExceptionCode& ec) { return getFloat64(byteOffset, false, ec); }
    double getFloat64(unsigned byteOffset, bool littleEndian, ExceptionCode&);

    void setInt8(unsigned byteOffset, int8_t value, ExceptionCode&);
    void setUint8(unsigned byteOffset, uint8_t value, ExceptionCode&);
    void setInt16(unsigned byteOffset, int16_t value, ExceptionCode& ec) { setInt16(byteOffset, value, false, ec); }
    void setInt16(unsigned byteOffset, int16_t value, bool littleEndian, ExceptionCode&);
    void setUint16(unsigned byteOffset, uint16_t value, ExceptionCode& ec) { setUint16(byteOffset, value, false, ec); }
    void setUint16(unsigned byteOffset, uint16_t value, bool littleEndian, ExceptionCode&);
    void setInt32(unsigned byteOffset, int32_t value, ExceptionCode& ec) { setInt32(byteOffset, value, false, ec); } 
    void setInt32(unsigned byteOffset, int32_t value, bool littleEndian, ExceptionCode&);
    void setUint32(unsigned byteOffset, uint32_t value, ExceptionCode& ec) { setUint32(byteOffset, value, false, ec); }
    void setUint32(unsigned byteOffset, uint32_t value, bool littleEndian, ExceptionCode&);
    void setFloat32(unsigned byteOffset, float value, ExceptionCode& ec) { setFloat32(byteOffset, value, false, ec); }
    void setFloat32(unsigned byteOffset, float value, bool littleEndian, ExceptionCode&);
    void setFloat64(unsigned byteOffset, double value, ExceptionCode& ec) { setFloat64(byteOffset, value, false, ec); }
    void setFloat64(unsigned byteOffset, double value, bool littleEndian, ExceptionCode&);

    virtual ViewType getType() const
    {
        return TypeDataView;
    }

protected:
    virtual void neuter();

private:
    DataView(PassRefPtr<ArrayBuffer>, unsigned byteOffset, unsigned byteLength);

    template<typename T>
    inline bool beyondRange(unsigned byteOffset) const { return byteOffset >= m_byteLength || byteOffset + sizeof(T) > m_byteLength; }

    template<typename T>
    T getData(unsigned byteOffset, bool littleEndian, ExceptionCode&) const;

    template<typename T>
    void setData(unsigned byteOffset, T value, bool littleEndian, ExceptionCode&);

    unsigned m_byteLength;
};


} // namespace WebCore

#endif // DataView_h
