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
 */

#include "config.h"
#include <wtf/MD5.h>
#include <wtf/StringExtras.h>
#include <wtf/text/CString.h>

namespace TestWebKitAPI {

static void expectMD5(CString input, CString expected)
{
    MD5 md5;
    md5.addBytes(reinterpret_cast<const uint8_t*>(input.data()), input.length());
    Vector<uint8_t, 16> digest;
    md5.checksum(digest);
    char* buf = 0;
    CString actual = CString::newUninitialized(32, buf);
    for (size_t i = 0; i < 16; i++, buf += 2)
        snprintf(buf, 3, "%02x", digest.at(i));

    ASSERT_EQ(expected.length(), actual.length());
    ASSERT_STREQ(expected.data(), actual.data());
}

TEST(WTF_MD5, Computation)
{
    // MD5 Test suite from http://www.ietf.org/rfc/rfc1321.txt.
    expectMD5("", "d41d8cd98f00b204e9800998ecf8427e");
    expectMD5("a", "0cc175b9c0f1b6a831c399e269772661");
    expectMD5("abc", "900150983cd24fb0d6963f7d28e17f72");
    expectMD5("message digest", "f96b697d7cb7938d525a2f31aaf161d0");
    expectMD5("abcdefghijklmnopqrstuvwxyz", "c3fcd3d76192e4007dfb496cca67e13b");
    expectMD5("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "d174ab98d277d9f5a5611c2c9f419d9f");
    expectMD5("12345678901234567890123456789012345678901234567890123456789012345678901234567890", "57edf4a22be3c955ac49da2e2107b67a");
}

} // namespace TestWebKitAPI
