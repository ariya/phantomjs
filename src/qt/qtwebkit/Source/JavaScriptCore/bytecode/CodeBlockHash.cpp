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

#include "config.h"
#include "CodeBlockHash.h"

#include "SourceCode.h"
#include <wtf/SHA1.h>

namespace JSC {

#define TABLE ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789")

CodeBlockHash::CodeBlockHash(const char* string)
    : m_hash(0)
{
    RELEASE_ASSERT(strlen(string) == 6);
    
    for (unsigned i = 0; i < 6; ++i) {
        m_hash *= 62;
        unsigned c = string[i];
        if (c >= 'A' && c <= 'Z') {
            m_hash += c - 'A';
            continue;
        }
        if (c >= 'a' && c <= 'z') {
            m_hash += c - 'a' + 26;
            continue;
        }
        ASSERT(c >= '0' && c <= '9');
        m_hash += c - '0' + 26 * 2;
    }
}

CodeBlockHash::CodeBlockHash(const SourceCode& sourceCode, CodeSpecializationKind kind)
    : m_hash(0)
{
    SHA1 sha1;
    sha1.addBytes(sourceCode.toString().utf8());
    Vector<uint8_t, 20> digest;
    sha1.computeHash(digest);
    m_hash += digest[0] | (digest[1] << 8) | (digest[2] << 16) | (digest[3] << 24);
    m_hash ^= static_cast<unsigned>(kind);
}

void CodeBlockHash::dump(PrintStream& out) const
{
    ASSERT(strlen(TABLE) == 62);
    
    char buffer[7];
    unsigned accumulator = m_hash;
    for (unsigned i = 6; i--;) {
        buffer[i] = TABLE[accumulator % 62];
        accumulator /= 62;
    }
    buffer[6] = 0;
    
#if !ASSERT_DISABLED
    CodeBlockHash recompute(buffer);
    ASSERT(recompute == *this);
#endif // !ASSERT_DISABLED
    
    out.print(buffer);
}

} // namespace JSC

