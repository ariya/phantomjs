// Copyright (c) 2012, Google Inc.
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

// core2md.cc: A utility to convert an ELF core file to a minidump file.

#include <stdio.h>

#include "client/linux/minidump_writer/minidump_writer.h"
#include "client/linux/minidump_writer/linux_core_dumper.h"

using google_breakpad::AppMemoryList;
using google_breakpad::MappingList;
using google_breakpad::LinuxCoreDumper;

static int ShowUsage(const char* argv0) {
  fprintf(stderr, "Usage: %s <core file> <procfs dir> <output>\n", argv0);
  return 1;
}

bool WriteMinidumpFromCore(const char* filename,
                           const char* core_path,
                           const char* procfs_override) {
  MappingList mappings;
  AppMemoryList memory_list;
  LinuxCoreDumper dumper(0, core_path, procfs_override);
  return google_breakpad::WriteMinidump(filename, mappings, memory_list,
                                        &dumper);
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    return ShowUsage(argv[0]);
  }

  const char* core_file = argv[1];
  const char* procfs_dir = argv[2];
  const char* minidump_file = argv[3];
  if (!WriteMinidumpFromCore(minidump_file,
                             core_file,
                             procfs_dir)) {
    fprintf(stderr, "Unable to generate minidump.\n");
    return 1;
  }

  return 0;
}
