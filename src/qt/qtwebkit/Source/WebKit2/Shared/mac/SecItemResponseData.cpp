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
#include "SecItemResponseData.h"

#include "ArgumentCoders.h"
#include "ArgumentCodersCF.h"

namespace WebKit {

SecItemResponseData::SecItemResponseData()
{
}

SecItemResponseData::SecItemResponseData(OSStatus resultCode, CFTypeRef resultObject)
    : m_resultObject(resultObject)
    , m_resultCode(resultCode)
{
}

void SecItemResponseData::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << static_cast<int64_t>(m_resultCode);
    encoder << static_cast<bool>(m_resultObject.get());
    if (m_resultObject)
        CoreIPC::encode(encoder, m_resultObject.get());
}

bool SecItemResponseData::decode(CoreIPC::ArgumentDecoder& decoder, SecItemResponseData& secItemResponseData)
{
    int64_t resultCode;
    if (!decoder.decode(resultCode))
        return false;
    secItemResponseData.m_resultCode = (OSStatus)resultCode;
    secItemResponseData.m_resultObject = 0;

    bool expectResultObject;
    if (!decoder.decode(expectResultObject))
        return false;

    if (expectResultObject && !CoreIPC::decode(decoder, secItemResponseData.m_resultObject))
        return false;

    return true;
}

} // namespace WebKit
