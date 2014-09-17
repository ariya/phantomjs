// Copyright (c) 2009, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "common/linux/linux_libc_support.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
typedef testing::Test LinuxLibcSupportTest;
}

TEST(LinuxLibcSupportTest, strlen) {
  static const char* test_data[] = { "", "a", "aa", "aaa", "aabc", NULL };
  for (unsigned i = 0; ; ++i) {
    if (!test_data[i])
      break;
    ASSERT_EQ(strlen(test_data[i]), my_strlen(test_data[i]));
  }
}

TEST(LinuxLibcSupportTest, strcmp) {
  static const char* test_data[] = {
    "", "",
    "a", "",
    "", "a",
    "a", "b",
    "a", "a",
    "ab", "aa",
    "abc", "ab",
    "abc", "abc",
    NULL,
  };

  for (unsigned i = 0; ; ++i) {
    if (!test_data[i*2])
      break;
    int libc_result = strcmp(test_data[i*2], test_data[i*2 + 1]);
    if (libc_result > 1)
      libc_result = 1;
    else if (libc_result < -1)
      libc_result = -1;
    ASSERT_EQ(my_strcmp(test_data[i*2], test_data[i*2 + 1]), libc_result);
  }
}

TEST(LinuxLibcSupportTest, strtoui) {
  int result;

  ASSERT_FALSE(my_strtoui(&result, ""));
  ASSERT_FALSE(my_strtoui(&result, "-1"));
  ASSERT_FALSE(my_strtoui(&result, "-"));
  ASSERT_FALSE(my_strtoui(&result, "a"));
  ASSERT_FALSE(my_strtoui(&result, "23472893472938472987987398472398"));

  ASSERT_TRUE(my_strtoui(&result, "0"));
  ASSERT_EQ(result, 0);
  ASSERT_TRUE(my_strtoui(&result, "1"));
  ASSERT_EQ(result, 1);
  ASSERT_TRUE(my_strtoui(&result, "12"));
  ASSERT_EQ(result, 12);
  ASSERT_TRUE(my_strtoui(&result, "123"));
  ASSERT_EQ(result, 123);
  ASSERT_TRUE(my_strtoui(&result, "0123"));
  ASSERT_EQ(result, 123);
}

TEST(LinuxLibcSupportTest, int_len) {
  ASSERT_EQ(my_int_len(0), 1);
  ASSERT_EQ(my_int_len(2), 1);
  ASSERT_EQ(my_int_len(5), 1);
  ASSERT_EQ(my_int_len(9), 1);
  ASSERT_EQ(my_int_len(10), 2);
  ASSERT_EQ(my_int_len(99), 2);
  ASSERT_EQ(my_int_len(100), 3);
  ASSERT_EQ(my_int_len(101), 3);
  ASSERT_EQ(my_int_len(1000), 4);
}

TEST(LinuxLibcSupportTest, itos) {
  char buf[10];

  my_itos(buf, 0, 1);
  ASSERT_EQ(0, memcmp(buf, "0", 1));

  my_itos(buf, 1, 1);
  ASSERT_EQ(0, memcmp(buf, "1", 1));

  my_itos(buf, 10, 2);
  ASSERT_EQ(0, memcmp(buf, "10", 2));

  my_itos(buf, 63, 2);
  ASSERT_EQ(0, memcmp(buf, "63", 2));

  my_itos(buf, 101, 3);
  ASSERT_EQ(0, memcmp(buf, "101", 2));
}

TEST(LinuxLibcSupportTest, strchr) {
  ASSERT_EQ(NULL, my_strchr("abc", 'd'));
  ASSERT_EQ(NULL, my_strchr("", 'd'));
  ASSERT_EQ(NULL, my_strchr("efghi", 'd'));

  ASSERT_TRUE(my_strchr("a", 'a'));
  ASSERT_TRUE(my_strchr("abc", 'a'));
  ASSERT_TRUE(my_strchr("bcda", 'a'));
  ASSERT_TRUE(my_strchr("sdfasdf", 'a'));
}

TEST(LinuxLibcSupportTest, read_hex_ptr) {
  uintptr_t result;
  const char* last;

  last = my_read_hex_ptr(&result, "");
  ASSERT_EQ(result, 0);
  ASSERT_EQ(*last, 0);

  last = my_read_hex_ptr(&result, "0");
  ASSERT_EQ(result, 0);
  ASSERT_EQ(*last, 0);

  last = my_read_hex_ptr(&result, "0123");
  ASSERT_EQ(result, 0x123);
  ASSERT_EQ(*last, 0);

  last = my_read_hex_ptr(&result, "0123a");
  ASSERT_EQ(result, 0x123a);
  ASSERT_EQ(*last, 0);

  last = my_read_hex_ptr(&result, "0123a-");
  ASSERT_EQ(result, 0x123a);
  ASSERT_EQ(*last, '-');
}
