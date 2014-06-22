// Copyright (c) 2012 Google Inc.
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

#ifndef CLIENT_LINUX_HANDLER_MINIDUMP_DESCRIPTOR_H_
#define CLIENT_LINUX_HANDLER_MINIDUMP_DESCRIPTOR_H_

#include <assert.h>
#include <sys/types.h>

#include <string>

#include "common/using_std_string.h"

// The MinidumpDescriptor describes how to access a minidump: it can contain
// either a file descriptor or a path.
// Note that when using files, it is created with the path to a directory.
// The actual path where the minidump is generated is created by this class.
namespace google_breakpad {

class MinidumpDescriptor {
 public:
  MinidumpDescriptor() : fd_(-1), size_limit_(-1) {}

  explicit MinidumpDescriptor(const string& directory)
      : fd_(-1),
        directory_(directory),
        c_path_(NULL),
        size_limit_(-1) {
    assert(!directory.empty());
  }

  explicit MinidumpDescriptor(int fd)
      : fd_(fd),
        c_path_(NULL),
        size_limit_(-1) {
    assert(fd != -1);
  }

  explicit MinidumpDescriptor(const MinidumpDescriptor& descriptor);
  MinidumpDescriptor& operator=(const MinidumpDescriptor& descriptor);

  bool IsFD() const { return fd_ != -1; }

  int fd() const { return fd_; }

  string directory() const { return directory_; }

  const char* path() const { return c_path_; }

  // Updates the path so it is unique.
  // Should be called from a normal context: this methods uses the heap.
  void UpdatePath();

  off_t size_limit() const { return size_limit_; }
  void set_size_limit(off_t limit) { size_limit_ = limit; }

 private:
  // The file descriptor where the minidump is generated.
  int fd_;

  // The directory where the minidump should be generated.
  string directory_;
  // The full path to the generated minidump.
  string path_;
  // The C string of |path_|. Precomputed so it can be access from a compromised
  // context.
  const char* c_path_;

  off_t size_limit_;
};

}  // namespace google_breakpad

#endif  // CLIENT_LINUX_HANDLER_MINIDUMP_DESCRIPTOR_H_
