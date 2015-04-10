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

#include "config.h"
#include "EncoderAdapter.h"

#include "DataReference.h"
#include "WebCoreArgumentCoders.h"
#include <wtf/text/WTFString.h>

namespace WebKit {

EncoderAdapter::EncoderAdapter()
    : m_encoder(CoreIPC::ArgumentEncoder::create())
{
    // Keep format compatibility by decoding an unused uint64_t value
    // that used to be encoded by the argument encoder.
    *m_encoder << static_cast<uint64_t>(0);
}

CoreIPC::DataReference EncoderAdapter::dataReference() const
{
    return CoreIPC::DataReference(m_encoder->buffer(), m_encoder->bufferSize());
}

void EncoderAdapter::encodeBytes(const uint8_t* bytes, size_t size)
{
    *m_encoder << CoreIPC::DataReference(bytes, size);
}

void EncoderAdapter::encodeBool(bool value)
{
    *m_encoder << value;
}

void EncoderAdapter::encodeUInt16(uint16_t value)
{
    *m_encoder << value;
}

void EncoderAdapter::encodeUInt32(uint32_t value)
{
    *m_encoder << value;
}

void EncoderAdapter::encodeUInt64(uint64_t value)
{
    *m_encoder << value;
}

void EncoderAdapter::encodeInt32(int32_t value)
{
    *m_encoder << value;
}

void EncoderAdapter::encodeInt64(int64_t value)
{
    *m_encoder << value;
}

void EncoderAdapter::encodeFloat(float value)
{
    *m_encoder << value;
}

void EncoderAdapter::encodeDouble(double value)
{
    *m_encoder << value;
}

void EncoderAdapter::encodeString(const String& value)
{
    // This mimics the CoreIPC binary encoding of Strings prior to r88886.
    // Whenever the CoreIPC binary encoding changes, we'll have to "undo" the changes here.
    // FIXME: We shouldn't use the CoreIPC binary encoding format for history,
    // and we should come up with a migration strategy so we can actually bump the version number
    // without breaking encoding/decoding of the history tree.

    // Special case the null string.
    if (value.isNull()) {
        *m_encoder << std::numeric_limits<uint32_t>::max();
        return;
    }

    uint32_t length = value.length();
    *m_encoder << length;

    uint64_t lengthInBytes = length * sizeof(UChar);
    *m_encoder << lengthInBytes;
    m_encoder->encodeFixedLengthData(reinterpret_cast<const uint8_t*>(value.characters()), length * sizeof(UChar), __alignof(UChar)); 
}

}
