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
#include "DecoderAdapter.h"

#include "DataReference.h"
#include "WebCoreArgumentCoders.h"
#include <wtf/text/WTFString.h>

namespace WebKit {

DecoderAdapter::DecoderAdapter(const uint8_t* buffer, size_t bufferSize)
    : m_decoder(CoreIPC::ArgumentDecoder::create(buffer, bufferSize))
{
    // Keep format compatibility by decoding an unused uint64_t value
    // that used to be encoded by the argument encoder.
    uint64_t value;
    m_decoder->decode(value);
    ASSERT(!value);
}

bool DecoderAdapter::decodeBytes(Vector<uint8_t>& bytes)
{
    CoreIPC::DataReference dataReference;
    if (!m_decoder->decode(dataReference))
        return false;

    bytes = dataReference.vector();
    return true;
}

bool DecoderAdapter::decodeBool(bool& value)
{
    return m_decoder->decode(value);
}

bool DecoderAdapter::decodeUInt16(uint16_t& value)
{
    return m_decoder->decode(value);
}

bool DecoderAdapter::decodeUInt32(uint32_t& value)
{
    return m_decoder->decode(value);
}

bool DecoderAdapter::decodeUInt64(uint64_t& value)
{
    return m_decoder->decode(value);
}

bool DecoderAdapter::decodeInt32(int32_t& value)
{
    return m_decoder->decode(value);
}

bool DecoderAdapter::decodeInt64(int64_t& value)
{
    return m_decoder->decode(value);
}

bool DecoderAdapter::decodeFloat(float& value)
{
    return m_decoder->decode(value);
}

bool DecoderAdapter::decodeDouble(double& value)
{
    return m_decoder->decode(value);
}

bool DecoderAdapter::decodeString(String& value)
{
    // This mimics the CoreIPC binary encoding of Strings prior to r88886.
    // Whenever the CoreIPC binary encoding changes, we'll have to "undo" the changes here.
    // FIXME: We shouldn't use the CoreIPC binary encoding format for history,
    // and we should come up with a migration strategy so we can actually bump the version number
    // without breaking encoding/decoding of the history tree.

    uint32_t length;
    if (!m_decoder->decode(length))
        return false;

    if (length == std::numeric_limits<uint32_t>::max()) {
        // This is the null string.
        value = String();
        return true;
    }

    uint64_t lengthInBytes;
    if (!m_decoder->decode(lengthInBytes))
        return false;

    if (lengthInBytes % sizeof(UChar) || lengthInBytes / sizeof(UChar) != length) {
        m_decoder->markInvalid();
        return false;
    }

    if (!m_decoder->bufferIsLargeEnoughToContain<UChar>(length)) {
        m_decoder->markInvalid();
        return false;
    }

    UChar* buffer;
    String string = String::createUninitialized(length, buffer);
    if (!m_decoder->decodeFixedLengthData(reinterpret_cast<uint8_t*>(buffer), length * sizeof(UChar), __alignof(UChar)))
        return false;

    value = string;
    return true;
}

}
