// The original file was copied from sqlite, and was in the public domain.
// Modifications Copyright 2006 Google Inc. All Rights Reserved
/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, construct an
 * MD5 instance, call addBytes as needed on buffers full of bytes,
 * and then call checksum, which will fill a supplied 16-byte array
 * with the digest.
 */

#include "config.h"
#include "MD5.h"

#include "Assertions.h"
#ifndef NDEBUG
#include "StringExtras.h"
#include "text/CString.h"
#endif
#include <wtf/StdLibExtras.h>

namespace WTF {

#ifdef NDEBUG
static inline void testMD5() { }
#else
// MD5 test case.
static bool isTestMD5Done;

static void expectMD5(CString input, CString expected)
{
    MD5 md5;
    md5.addBytes(reinterpret_cast<const uint8_t*>(input.data()), input.length());
    Vector<uint8_t, 16> digest;
    md5.checksum(digest);
    char* buf = 0;
    CString actual = CString::newUninitialized(32, buf);
    for (size_t i = 0; i < 16; i++) {
        snprintf(buf, 3, "%02x", digest.at(i));
        buf += 2;
    }
    ASSERT_WITH_MESSAGE(actual == expected, "input:%s[%lu] actual:%s expected:%s", input.data(), static_cast<unsigned long>(input.length()), actual.data(), expected.data());
}

static void testMD5()
{
    if (isTestMD5Done)
        return;
    isTestMD5Done = true;

    // MD5 Test suite from http://www.ietf.org/rfc/rfc1321.txt
    expectMD5("", "d41d8cd98f00b204e9800998ecf8427e");
    expectMD5("a", "0cc175b9c0f1b6a831c399e269772661");
    expectMD5("abc", "900150983cd24fb0d6963f7d28e17f72");
    expectMD5("message digest", "f96b697d7cb7938d525a2f31aaf161d0");
    expectMD5("abcdefghijklmnopqrstuvwxyz", "c3fcd3d76192e4007dfb496cca67e13b");
    expectMD5("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "d174ab98d277d9f5a5611c2c9f419d9f");
    expectMD5("12345678901234567890123456789012345678901234567890123456789012345678901234567890", "57edf4a22be3c955ac49da2e2107b67a");
}
#endif

// Note: this code is harmless on little-endian machines.

static void reverseBytes(uint8_t* buf, unsigned longs)
{
    ASSERT(longs > 0);
    do {
        uint32_t t = static_cast<uint32_t>(buf[3] << 8 | buf[2]) << 16 | buf[1] << 8 | buf[0];
        ASSERT_WITH_MESSAGE(!(reinterpret_cast<uintptr_t>(buf) % sizeof(t)), "alignment error of buf");
        *reinterpret_cast_ptr<uint32_t *>(buf) = t;
        buf += 4;
    } while (--longs);
}

// The four core functions.
// F1 is originally defined as (x & y | ~x & z), but optimized somewhat: 4 bit ops -> 3 bit ops.
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

// This is the central step in the MD5 algorithm.
#define MD5STEP(f, w, x, y, z, data, s) \
    (w += f(x, y, z) + data, w = w << s | w >> (32 - s), w += x)

static void MD5Transform(uint32_t buf[4], const uint32_t in[16])
{
    uint32_t a = buf[0];
    uint32_t b = buf[1];
    uint32_t c = buf[2];
    uint32_t d = buf[3];

    MD5STEP(F1, a, b, c, d, in[ 0]+0xd76aa478,  7);
    MD5STEP(F1, d, a, b, c, in[ 1]+0xe8c7b756, 12);
    MD5STEP(F1, c, d, a, b, in[ 2]+0x242070db, 17);
    MD5STEP(F1, b, c, d, a, in[ 3]+0xc1bdceee, 22);
    MD5STEP(F1, a, b, c, d, in[ 4]+0xf57c0faf,  7);
    MD5STEP(F1, d, a, b, c, in[ 5]+0x4787c62a, 12);
    MD5STEP(F1, c, d, a, b, in[ 6]+0xa8304613, 17);
    MD5STEP(F1, b, c, d, a, in[ 7]+0xfd469501, 22);
    MD5STEP(F1, a, b, c, d, in[ 8]+0x698098d8,  7);
    MD5STEP(F1, d, a, b, c, in[ 9]+0x8b44f7af, 12);
    MD5STEP(F1, c, d, a, b, in[10]+0xffff5bb1, 17);
    MD5STEP(F1, b, c, d, a, in[11]+0x895cd7be, 22);
    MD5STEP(F1, a, b, c, d, in[12]+0x6b901122,  7);
    MD5STEP(F1, d, a, b, c, in[13]+0xfd987193, 12);
    MD5STEP(F1, c, d, a, b, in[14]+0xa679438e, 17);
    MD5STEP(F1, b, c, d, a, in[15]+0x49b40821, 22);

    MD5STEP(F2, a, b, c, d, in[ 1]+0xf61e2562,  5);
    MD5STEP(F2, d, a, b, c, in[ 6]+0xc040b340,  9);
    MD5STEP(F2, c, d, a, b, in[11]+0x265e5a51, 14);
    MD5STEP(F2, b, c, d, a, in[ 0]+0xe9b6c7aa, 20);
    MD5STEP(F2, a, b, c, d, in[ 5]+0xd62f105d,  5);
    MD5STEP(F2, d, a, b, c, in[10]+0x02441453,  9);
    MD5STEP(F2, c, d, a, b, in[15]+0xd8a1e681, 14);
    MD5STEP(F2, b, c, d, a, in[ 4]+0xe7d3fbc8, 20);
    MD5STEP(F2, a, b, c, d, in[ 9]+0x21e1cde6,  5);
    MD5STEP(F2, d, a, b, c, in[14]+0xc33707d6,  9);
    MD5STEP(F2, c, d, a, b, in[ 3]+0xf4d50d87, 14);
    MD5STEP(F2, b, c, d, a, in[ 8]+0x455a14ed, 20);
    MD5STEP(F2, a, b, c, d, in[13]+0xa9e3e905,  5);
    MD5STEP(F2, d, a, b, c, in[ 2]+0xfcefa3f8,  9);
    MD5STEP(F2, c, d, a, b, in[ 7]+0x676f02d9, 14);
    MD5STEP(F2, b, c, d, a, in[12]+0x8d2a4c8a, 20);

    MD5STEP(F3, a, b, c, d, in[ 5]+0xfffa3942,  4);
    MD5STEP(F3, d, a, b, c, in[ 8]+0x8771f681, 11);
    MD5STEP(F3, c, d, a, b, in[11]+0x6d9d6122, 16);
    MD5STEP(F3, b, c, d, a, in[14]+0xfde5380c, 23);
    MD5STEP(F3, a, b, c, d, in[ 1]+0xa4beea44,  4);
    MD5STEP(F3, d, a, b, c, in[ 4]+0x4bdecfa9, 11);
    MD5STEP(F3, c, d, a, b, in[ 7]+0xf6bb4b60, 16);
    MD5STEP(F3, b, c, d, a, in[10]+0xbebfbc70, 23);
    MD5STEP(F3, a, b, c, d, in[13]+0x289b7ec6,  4);
    MD5STEP(F3, d, a, b, c, in[ 0]+0xeaa127fa, 11);
    MD5STEP(F3, c, d, a, b, in[ 3]+0xd4ef3085, 16);
    MD5STEP(F3, b, c, d, a, in[ 6]+0x04881d05, 23);
    MD5STEP(F3, a, b, c, d, in[ 9]+0xd9d4d039,  4);
    MD5STEP(F3, d, a, b, c, in[12]+0xe6db99e5, 11);
    MD5STEP(F3, c, d, a, b, in[15]+0x1fa27cf8, 16);
    MD5STEP(F3, b, c, d, a, in[ 2]+0xc4ac5665, 23);

    MD5STEP(F4, a, b, c, d, in[ 0]+0xf4292244,  6);
    MD5STEP(F4, d, a, b, c, in[ 7]+0x432aff97, 10);
    MD5STEP(F4, c, d, a, b, in[14]+0xab9423a7, 15);
    MD5STEP(F4, b, c, d, a, in[ 5]+0xfc93a039, 21);
    MD5STEP(F4, a, b, c, d, in[12]+0x655b59c3,  6);
    MD5STEP(F4, d, a, b, c, in[ 3]+0x8f0ccc92, 10);
    MD5STEP(F4, c, d, a, b, in[10]+0xffeff47d, 15);
    MD5STEP(F4, b, c, d, a, in[ 1]+0x85845dd1, 21);
    MD5STEP(F4, a, b, c, d, in[ 8]+0x6fa87e4f,  6);
    MD5STEP(F4, d, a, b, c, in[15]+0xfe2ce6e0, 10);
    MD5STEP(F4, c, d, a, b, in[ 6]+0xa3014314, 15);
    MD5STEP(F4, b, c, d, a, in[13]+0x4e0811a1, 21);
    MD5STEP(F4, a, b, c, d, in[ 4]+0xf7537e82,  6);
    MD5STEP(F4, d, a, b, c, in[11]+0xbd3af235, 10);
    MD5STEP(F4, c, d, a, b, in[ 2]+0x2ad7d2bb, 15);
    MD5STEP(F4, b, c, d, a, in[ 9]+0xeb86d391, 21);

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}

MD5::MD5()
{
    // FIXME: Move unit tests somewhere outside the constructor. See bug 55853.
    testMD5();
    m_buf[0] = 0x67452301;
    m_buf[1] = 0xefcdab89;
    m_buf[2] = 0x98badcfe;
    m_buf[3] = 0x10325476;
    m_bits[0] = 0;
    m_bits[1] = 0;
    memset(m_in, 0, sizeof(m_in));
    ASSERT_WITH_MESSAGE(!(reinterpret_cast<uintptr_t>(m_in) % sizeof(uint32_t)), "alignment error of m_in");
}

void MD5::addBytes(const uint8_t* input, size_t length)
{
    const uint8_t* buf = input;

    // Update bitcount
    uint32_t t = m_bits[0];
    m_bits[0] = t + (length << 3);
    if (m_bits[0] < t)
        m_bits[1]++; // Carry from low to high
    m_bits[1] += length >> 29;

    t = (t >> 3) & 0x3f; // Bytes already in shsInfo->data

    // Handle any leading odd-sized chunks

    if (t) {
        uint8_t* p = m_in + t;

        t = 64 - t;
        if (length < t) {
            memcpy(p, buf, length);
            return;
        }
        memcpy(p, buf, t);
        reverseBytes(m_in, 16);
        MD5Transform(m_buf, reinterpret_cast_ptr<uint32_t*>(m_in)); // m_in is 4-byte aligned.
        buf += t;
        length -= t;
    }

    // Process data in 64-byte chunks

    while (length >= 64) {
        memcpy(m_in, buf, 64);
        reverseBytes(m_in, 16);
        MD5Transform(m_buf, reinterpret_cast_ptr<uint32_t*>(m_in)); // m_in is 4-byte aligned.
        buf += 64;
        length -= 64;
    }

    // Handle any remaining bytes of data.
    memcpy(m_in, buf, length);
}

void MD5::checksum(Vector<uint8_t, 16>& digest)
{
    // Compute number of bytes mod 64
    unsigned count = (m_bits[0] >> 3) & 0x3F;

    // Set the first char of padding to 0x80.  This is safe since there is
    // always at least one byte free
    uint8_t* p = m_in + count;
    *p++ = 0x80;

    // Bytes of padding needed to make 64 bytes
    count = 64 - 1 - count;

    // Pad out to 56 mod 64
    if (count < 8) {
        // Two lots of padding:  Pad the first block to 64 bytes
        memset(p, 0, count);
        reverseBytes(m_in, 16);
        MD5Transform(m_buf, reinterpret_cast_ptr<uint32_t *>(m_in)); // m_in is 4-byte aligned.

        // Now fill the next block with 56 bytes
        memset(m_in, 0, 56);
    } else {
        // Pad block to 56 bytes
        memset(p, 0, count - 8);
    }
    reverseBytes(m_in, 14);

    // Append length in bits and transform
    // m_in is 4-byte aligned.
    (reinterpret_cast_ptr<uint32_t*>(m_in))[14] = m_bits[0];
    (reinterpret_cast_ptr<uint32_t*>(m_in))[15] = m_bits[1];

    MD5Transform(m_buf, reinterpret_cast_ptr<uint32_t*>(m_in));
    reverseBytes(reinterpret_cast<uint8_t*>(m_buf), 4);

    // Now, m_buf contains checksum result.
    if (!digest.isEmpty())
        digest.clear();
    digest.append(reinterpret_cast<uint8_t*>(m_buf), 16);

    // In case it's sensitive
    memset(m_buf, 0, sizeof(m_buf));
    memset(m_bits, 0, sizeof(m_bits));
    memset(m_in, 0, sizeof(m_in));
}

} // namespace WTF
