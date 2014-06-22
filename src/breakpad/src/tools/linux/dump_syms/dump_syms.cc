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
#include <vector>

#include "common/linux/dump_symbols.h"

using google_breakpad::WriteSymbolFile;

int usage(const char* self) {
  fprintf(stderr, "Usage: %s [OPTION] <binary-with-debugging-info> "
          "[directories-for-debug-file]\n\n", self);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -c    Do not generate CFI section\n");
  fprintf(stderr, "  -r    Do not handle inter-compilation unit references\n");
  return 1;
}

int main(int argc, char **argv) {
  if (argc < 2)
    return usage(argv[0]);

  bool cfi = true;
  bool handle_inter_cu_refs = true;
  int arg_index = 1;
  while (arg_index < argc && strlen(argv[arg_index]) > 0 &&
         argv[arg_index][0] == '-') {
    if (strcmp("-c", argv[arg_index]) == 0) {
      cfi = false;
    } else if (strcmp("-r", argv[arg_index]) == 0) {
      handle_inter_cu_refs = false;
    } else {
      return usage(argv[0]);
    }
    ++arg_index;
  }
  if (arg_index == argc)
    return usage(argv[0]);

  const char* binary;
  std::vector<string> debug_dirs;
  binary = argv[arg_index];
  for (int debug_dir_index = arg_index + 1;
       debug_dir_index < argc;
       ++debug_dir_index) {
    debug_dirs.push_back(argv[debug_dir_index]);
  }

  SymbolData symbol_data = cfi ? ALL_SYMBOL_DATA : NO_CFI;
  google_breakpad::DumpOptions options(symbol_data, handle_inter_cu_refs);
  if (!WriteSymbolFile(binary, debug_dirs, options, std::cout)) {
    fprintf(stderr, "Failed to write symbol file.\n");
    return 1;
  }

  return 0;
}
