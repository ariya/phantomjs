/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// A straightforward SHA-1 implementation based on RFC 3174.
// http://www.ietf.org/rfc/rfc3174.txt
// The names of functions and variables (such as "a", "b", and "f") follow notations in RFC 3174.

#include "config.h"
#include "SHA1.h"

#include "Assertions.h"

#include "StringExtras.h"
#include "text/CString.h"

namespace WTF {

static inline uint32_t f(int t, uint32_t b, uint32_t c, uint32_t d)
{
    ASSERT(t >= 0 && t < 80);
    if (t < 20)
        return (b & c) | ((~b) & d);
    if (t < 40)
        return b ^ c ^ d;
    if (t < 60)
        return (b & c) | (b & d) | (c & d);
    return b ^ c ^ d;
}

static inline uint32_t k(int t)
{
    ASSERT(t >= 0 && t < 80);
    if (t < 20)
        return 0x5a827999;
    if (t < 40)
        return 0x6ed9eba1;
    if (t < 60)
        return 0x8f1bbcdc;
    return 0xca62c1d6;
}

static inline uint32_t rotateLeft(int n, uint32_t x)
{
    ASSERT(n >= 0 && n < 32);
    return (x << n) | (x >> (32 - n));
}

SHA1::SHA1()
{
    reset();
}

void SHA1::addBytes(const uint8_t* input, size_t length)
{
    while (length--) {
        ASSERT(m_cursor < 64);
        m_buffer[m_cursor++] = *input++;
        ++m_totalBytes;
        if (m_cursor == 64)
            processBlock();
    }
}

void SHA1::computeHash(Vector<uint8_t, 20>& digest)
{
    finalize();

    digest.clear();
    digest.resize(20);
    for (size_t i = 0; i < 5; ++i) {
        // Treat hashValue as a big-endian value.
        uint32_t hashValue = m_hash[i];
        for (int j = 0; j < 4; ++j) {
            digest[4 * i + (3 - j)] = hashValue & 0xFF;
            hashValue >>= 8;
        }
    }

    reset();
}

CString SHA1::hexDigest(const Vector<uint8_t, 20>& digest)
{
    char* start = 0;
    CString result = CString::newUninitialized(40, start);
    char* buffer = start;
    for (size_t i = 0; i < 20; ++i) {
        snprintf(buffer, 3, "%02X", digest.at(i));
        buffer += 2;
    }
    return result;
}

CString SHA1::computeHexDigest()
{
    Vector<uint8_t, 20> digest;
    computeHash(digest);
    return hexDigest(digest);
}

void SHA1::finalize()
{
    ASSERT(m_cursor < 64);
    m_buffer[m_cursor++] = 0x80;
    if (m_cursor > 56) {
        // Pad out to next block.
        while (m_cursor < 64)
            m_buffer[m_cursor++] = 0x00;
        processBlock();
    }

    for (size_t i = m_cursor; i < 56; ++i)
        m_buffer[i] = 0x00;

    // Write the length as a big-endian 64-bit value.
    uint64_t bits = m_totalBytes * 8;
    for (int i = 0; i < 8; ++i) {
        m_buffer[56 + (7 - i)] = bits & 0xFF;
        bits >>= 8;
    }
    m_cursor = 64;
    processBlock();
}

void SHA1::processBlock()
{
    ASSERT(m_cursor == 64);

    uint32_t w[80] = { 0 };
    for (int t = 0; t < 16; ++t)
        w[t] = (m_buffer[t * 4] << 24) | (m_buffer[t * 4 + 1] << 16) | (m_buffer[t * 4 + 2] << 8) | m_buffer[t * 4 + 3];
    for (int t = 16; t < 80; ++t)
        w[t] = rotateLeft(1, w[t - 3] ^ w[t - 8] ^ w[t - 14] ^ w[t - 16]);

    uint32_t a = m_hash[0];
    uint32_t b = m_hash[1];
    uint32_t c = m_hash[2];
    uint32_t d = m_hash[3];
    uint32_t e = m_hash[4];

    for (int t = 0; t < 80; ++t) {
        uint32_t temp = rotateLeft(5, a) + f(t, b, c, d) + e + w[t] + k(t);
        e = d;
        d = c;
        c = rotateLeft(30, b);
        b = a;
        a = temp;
    }

    m_hash[0] += a;
    m_hash[1] += b;
    m_hash[2] += c;
    m_hash[3] += d;
    m_hash[4] += e;

    m_cursor = 0;
}

void SHA1::reset()
{
    m_cursor = 0;
    m_totalBytes = 0;
    m_hash[0] = 0x67452301;
    m_hash[1] = 0xefcdab89;
    m_hash[2] = 0x98badcfe;
    m_hash[3] = 0x10325476;
    m_hash[4] = 0xc3d2e1f0;

    // Clear the buffer after use in case it's sensitive.
    memset(m_buffer, 0, sizeof(m_buffer));
}

} // namespace WTF
