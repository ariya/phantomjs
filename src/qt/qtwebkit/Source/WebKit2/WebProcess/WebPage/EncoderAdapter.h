/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef EncoderAdapter_h
#define EncoderAdapter_h

#include <wtf/Encoder.h>
#include <wtf/Forward.h>
#include <wtf/OwnPtr.h>

namespace CoreIPC {
    class ArgumentEncoder;
    class DataReference;
}

namespace WebKit {

class EncoderAdapter : public Encoder {
public:
    EncoderAdapter();

    CoreIPC::DataReference dataReference() const;

private:
    virtual void encodeBytes(const uint8_t*, size_t);
    virtual void encodeBool(bool);
    virtual void encodeUInt16(uint16_t value);
    virtual void encodeUInt32(uint32_t);
    virtual void encodeUInt64(uint64_t);
    virtual void encodeInt32(int32_t);
    virtual void encodeInt64(int64_t);
    virtual void encodeFloat(float);
    virtual void encodeDouble(double);
    virtual void encodeString(const String&);

    OwnPtr<CoreIPC::ArgumentEncoder> m_encoder;
};

} // namespace WebKit

#endif // EncoderAdapter_h
