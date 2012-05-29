/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DOMSettableTokenList.h"

namespace WebCore {

DOMSettableTokenList::DOMSettableTokenList()
    : m_value()
    , m_tokens()
{
}

DOMSettableTokenList::~DOMSettableTokenList()
{
}

const AtomicString DOMSettableTokenList::item(unsigned index) const
{
    if (index >= length())
        return AtomicString();
    return m_tokens[index];
}

bool DOMSettableTokenList::contains(const AtomicString& token, ExceptionCode& ec) const
{
    if (!validateToken(token, ec))
        return false;
    return m_tokens.contains(token);
}

void DOMSettableTokenList::add(const AtomicString& token, ExceptionCode& ec)
{
    if (!validateToken(token, ec) || m_tokens.contains(token))
        return;
    addInternal(token);
}

void DOMSettableTokenList::addInternal(const AtomicString& token)
{
    m_value = addToken(m_value, token);
    if (m_tokens.isNull())
        m_tokens.set(token, false);
    else
        m_tokens.add(token);
}

void DOMSettableTokenList::remove(const AtomicString& token, ExceptionCode& ec)
{
    if (!validateToken(token, ec) || !m_tokens.contains(token))
        return;
    removeInternal(token);
}

void DOMSettableTokenList::removeInternal(const AtomicString& token)
{
    m_value = removeToken(m_value, token);
    m_tokens.remove(token);
}

bool DOMSettableTokenList::toggle(const AtomicString& token, ExceptionCode& ec)
{
    if (!validateToken(token, ec))
        return false;
    if (m_tokens.contains(token)) {
        removeInternal(token);
        return false;
    }
    addInternal(token);
    return true;
}

void DOMSettableTokenList::setValue(const String& value)
{
    m_value = value;
    m_tokens.set(value, false);
}

} // namespace WebCore
