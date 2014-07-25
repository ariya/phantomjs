// Copyright (c) 2011, Google Inc.
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

// file_utils.cc: Implement utility functions for file manipulation.
// See file_utils.h for details.

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common/linux/eintr_wrapper.h"
#include "common/tests/file_utils.h"

namespace google_breakpad {

bool CopyFile(const char* from_path, const char* to_path) {
  int infile = HANDLE_EINTR(open(from_path, O_RDONLY));
  if (infile < 0) {
    perror("open");
    return false;
  }

  int outfile = HANDLE_EINTR(creat(to_path, 0666));
  if (outfile < 0) {
    perror("creat");
    if (HANDLE_EINTR(close(infile)) < 0) {
      perror("close");
    }
    return false;
  }

  char buffer[1024];
  bool result = true;

  while (result) {
    ssize_t bytes_read = HANDLE_EINTR(read(infile, buffer, sizeof(buffer)));
    if (bytes_read < 0) {
      perror("read");
      result = false;
      break;
    }
    if (bytes_read == 0)
      break;
    ssize_t bytes_written_per_read = 0;
    do {
      ssize_t bytes_written_partial = HANDLE_EINTR(write(
          outfile,
          &buffer[bytes_written_per_read],
          bytes_read - bytes_written_per_read));
      if (bytes_written_partial < 0) {
        perror("write");
        result = false;
        break;
      }
      bytes_written_per_read += bytes_written_partial;
    } while (bytes_written_per_read < bytes_read);
  }

  if (HANDLE_EINTR(close(infile)) == -1) {
    perror("close");
    result = false;
  }
  if (HANDLE_EINTR(close(outfile)) == -1) {
    perror("close");
    result = false;
  }

  return result;
}

bool ReadFile(const char* path, void* buffer, ssize_t* buffer_size) {
  int fd = HANDLE_EINTR(open(path, O_RDONLY));
  if (fd == -1) {
    perror("open");
    return false;
  }

  bool ok = true;
  if (buffer && buffer_size && *buffer_size > 0) {
    memset(buffer, 0, sizeof(*buffer_size));
    *buffer_size = HANDLE_EINTR(read(fd, buffer, *buffer_size));
    if (*buffer_size == -1) {
      perror("read");
      ok = false;
    }
  }
  if (HANDLE_EINTR(close(fd)) == -1) {
    perror("close");
    ok = false;
  }
  return ok;
}

bool WriteFile(const char* path, const void* buffer, size_t buffer_size) {
  int fd = HANDLE_EINTR(open(path, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU));
  if (fd == -1) {
    perror("open");
    return false;
  }

  bool ok = true;
  if (buffer) {
    size_t bytes_written_total = 0;
    ssize_t bytes_written_partial = 0;
    const char* data = reinterpret_cast<const char*>(buffer);
    while (bytes_written_total < buffer_size) {
      bytes_written_partial =
          HANDLE_EINTR(write(fd, data + bytes_written_total,
                             buffer_size - bytes_written_total));
      if (bytes_written_partial < 0) {
        perror("write");
        ok = false;
        break;
      }
      bytes_written_total += bytes_written_partial;
    }
  }
  if (HANDLE_EINTR(close(fd)) == -1) {
    perror("close");
    ok = false;
  }
  return ok;
}

}  // namespace google_breakpad
