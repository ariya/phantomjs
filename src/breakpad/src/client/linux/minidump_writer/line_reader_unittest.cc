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

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "client/linux/minidump_writer/line_reader.h"
#include "breakpad_googletest_includes.h"
#include "common/linux/eintr_wrapper.h"

using namespace google_breakpad;

#if !defined(__ANDROID__)
#define TEMPDIR "/tmp"
#else
#define TEMPDIR "/data/local/tmp"
#endif

static int TemporaryFile() {
  static const char templ[] = TEMPDIR "/line-reader-unittest-XXXXXX";
  char templ_copy[sizeof(templ)];
  memcpy(templ_copy, templ, sizeof(templ));
  const int fd = mkstemp(templ_copy);
  if (fd >= 0)
    unlink(templ_copy);

  return fd;
}

namespace {
typedef testing::Test LineReaderTest;
}

TEST(LineReaderTest, EmptyFile) {
  const int fd = TemporaryFile();
  LineReader reader(fd);

  const char *line;
  unsigned len;
  ASSERT_FALSE(reader.GetNextLine(&line, &len));

  close(fd);
}

TEST(LineReaderTest, OneLineTerminated) {
  const int fd = TemporaryFile();
  const int r = HANDLE_EINTR(write(fd, "a\n", 2));
  ASSERT_EQ(2, r);
  lseek(fd, 0, SEEK_SET);
  LineReader reader(fd);

  const char *line;
  unsigned int len;
  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ((unsigned int)1, len);
  ASSERT_EQ('a', line[0]);
  ASSERT_EQ('\0', line[1]);
  reader.PopLine(len);

  ASSERT_FALSE(reader.GetNextLine(&line, &len));

  close(fd);
}

TEST(LineReaderTest, OneLine) {
  const int fd = TemporaryFile();
  const int r = HANDLE_EINTR(write(fd, "a", 1));
  ASSERT_EQ(1, r);
  lseek(fd, 0, SEEK_SET);
  LineReader reader(fd);

  const char *line;
  unsigned len;
  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ((unsigned)1, len);
  ASSERT_EQ('a', line[0]);
  ASSERT_EQ('\0', line[1]);
  reader.PopLine(len);

  ASSERT_FALSE(reader.GetNextLine(&line, &len));

  close(fd);
}

TEST(LineReaderTest, TwoLinesTerminated) {
  const int fd = TemporaryFile();
  const int r = HANDLE_EINTR(write(fd, "a\nb\n", 4));
  ASSERT_EQ(4, r);
  lseek(fd, 0, SEEK_SET);
  LineReader reader(fd);

  const char *line;
  unsigned len;
  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ((unsigned)1, len);
  ASSERT_EQ('a', line[0]);
  ASSERT_EQ('\0', line[1]);
  reader.PopLine(len);

  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ((unsigned)1, len);
  ASSERT_EQ('b', line[0]);
  ASSERT_EQ('\0', line[1]);
  reader.PopLine(len);

  ASSERT_FALSE(reader.GetNextLine(&line, &len));

  close(fd);
}

TEST(LineReaderTest, TwoLines) {
  const int fd = TemporaryFile();
  const int r = HANDLE_EINTR(write(fd, "a\nb", 3));
  ASSERT_EQ(3, r);
  lseek(fd, 0, SEEK_SET);
  LineReader reader(fd);

  const char *line;
  unsigned len;
  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ((unsigned)1, len);
  ASSERT_EQ('a', line[0]);
  ASSERT_EQ('\0', line[1]);
  reader.PopLine(len);

  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ((unsigned)1, len);
  ASSERT_EQ('b', line[0]);
  ASSERT_EQ('\0', line[1]);
  reader.PopLine(len);

  ASSERT_FALSE(reader.GetNextLine(&line, &len));

  close(fd);
}

TEST(LineReaderTest, MaxLength) {
  const int fd = TemporaryFile();
  char l[LineReader::kMaxLineLen - 1];
  memset(l, 'a', sizeof(l));
  const int r = HANDLE_EINTR(write(fd, l, sizeof(l)));
  ASSERT_EQ(sizeof(l), r);
  lseek(fd, 0, SEEK_SET);
  LineReader reader(fd);

  const char *line;
  unsigned len;
  ASSERT_TRUE(reader.GetNextLine(&line, &len));
  ASSERT_EQ(sizeof(l), len);
  ASSERT_TRUE(memcmp(l, line, sizeof(l)) == 0);
  ASSERT_EQ('\0', line[len]);

  close(fd);
}

TEST(LineReaderTest, TooLong) {
  const int fd = TemporaryFile();
  char l[LineReader::kMaxLineLen];
  memset(l, 'a', sizeof(l));
  const int r = HANDLE_EINTR(write(fd, l, sizeof(l)));
  ASSERT_EQ(sizeof(l), r);
  lseek(fd, 0, SEEK_SET);
  LineReader reader(fd);

  const char *line;
  unsigned len;
  ASSERT_FALSE(reader.GetNextLine(&line, &len));

  close(fd);
}
