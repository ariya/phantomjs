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
#include "SessionState.h"

namespace CoreIPC {

// This assumes that when we encode a RefPtr we want to encode the object it points to and it is never null.
template<typename T> struct ArgumentCoder<RefPtr<T> > {
    static void encode(ArgumentEncoder& encoder, const RefPtr<T>& item)
    {
        item->encode(encoder);
    }

    static bool decode(ArgumentDecoder& decoder, RefPtr<T>& item)
    {
        item = T::decode(decoder);
        return item;
    }
};

} // namespace CoreIPC

namespace WebKit {

SessionState::SessionState()
    : m_currentIndex(0)
{
}

SessionState::SessionState(const BackForwardListItemVector& list, uint32_t currentIndex)
    : m_list(list)
    , m_currentIndex(currentIndex)
{
}

bool SessionState::isEmpty() const
{
    // Because this might change later, callers should use this instead of
    // calling list().isEmpty() directly themselves.
    return m_list.isEmpty();
}
    
void SessionState::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << m_list;
    encoder << m_currentIndex;
}

bool SessionState::decode(CoreIPC::ArgumentDecoder& decoder, SessionState& state)
{
    if (!decoder.decode(state.m_list))
        return false;
    if (!decoder.decode(state.m_currentIndex))
        return false;
    return true;
}
    
} // namespace WebKit
