/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "WebEvent.h"

#include "ArgumentDecoder.h"
#include "ArgumentEncoder.h"
#include "Arguments.h"

namespace WebKit {

WebEvent::WebEvent()
    : m_type(static_cast<uint32_t>(NoType))
    , m_modifiers(0)
    , m_timestamp(0)
{
}

WebEvent::WebEvent(Type type, Modifiers modifiers, double timestamp)
    : m_type(type)
    , m_modifiers(modifiers)
    , m_timestamp(timestamp)
{
}

void WebEvent::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << m_type;
    encoder << m_modifiers;
    encoder << m_timestamp;
}

bool WebEvent::decode(CoreIPC::ArgumentDecoder& decoder, WebEvent& result)
{
    if (!decoder.decode(result.m_type))
        return false;
    if (!decoder.decode(result.m_modifiers))
        return false;
    if (!decoder.decode(result.m_timestamp))
        return false;
    return true;
}
    
} // namespace WebKit
