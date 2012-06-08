// Copyright (c) 2010 Google Inc.
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

#ifndef CLIENT_LINUX_CRASH_GENERATION_CRASH_GENERATION_CLIENT_H_
#define CLIENT_LINUX_CRASH_GENERATION_CRASH_GENERATION_CLIENT_H_

#include <stddef.h>

namespace google_breakpad {

class CrashGenerationClient {
public:
  ~CrashGenerationClient()
  {
  }

  // Request the crash server to generate a dump.  |blob| is a hack,
  // see exception_handler.h and minidump_writer.h
  //
  // Return true if the dump was successful; false otherwise.
  bool RequestDump(const void* blob, size_t blob_size);

  // Return a new CrashGenerationClient if |server_fd| is valid and
  // connects to a CrashGenerationServer.  Otherwise, return NULL.
  // The returned CrashGenerationClient* is owned by the caller of
  // this function.
  static CrashGenerationClient* TryCreate(int server_fd);

private:
  CrashGenerationClient(int server_fd) : server_fd_(server_fd)
  {
  }

  int server_fd_;

  // prevent copy construction and assignment
  CrashGenerationClient(const CrashGenerationClient&);
  CrashGenerationClient& operator=(const CrashGenerationClient&);
};

} // namespace google_breakpad

#endif // CLIENT_LINUX_CRASH_GENERATION_CRASH_GENERATION_CLIENT_H_
