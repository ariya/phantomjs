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

#include <stdio.h>

#include <cstring>
#include <iostream>
#include <string>

#include "common/linux/dump_symbols.h"

using google_breakpad::WriteSymbolFile;

int usage(const char* self) {
  fprintf(stderr, "Usage: %s [OPTION] <binary-with-debugging-info> "
          "[directory-for-debug-file]\n\n", self);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -c    Do not generate CFI section\n");
  return 1;
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 4)
    return usage(argv[0]);

  bool cfi = true;
  if (strcmp("-c", argv[1]) == 0)
    cfi = false;
  if (!cfi && argc == 2)
    return usage(argv[0]);

  const char *binary;
  std::string debug_dir;
  if (cfi) {
    binary = argv[1];
    if (argc == 3)
      debug_dir = argv[2];
  } else {
    binary = argv[2];
    if (argc == 4)
      debug_dir = argv[3];
  }

  if (!WriteSymbolFile(binary, debug_dir, cfi, std::cout)) {
    fprintf(stderr, "Failed to write symbol file.\n");
    return 1;
  }

  return 0;
}
