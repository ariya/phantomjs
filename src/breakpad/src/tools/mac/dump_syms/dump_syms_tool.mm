// -*- mode: c++ -*-

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

// dump_syms_tool.mm: Command line tool that uses the DumpSymbols class.
// TODO(waylonis): accept stdin

#include <mach-o/arch.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#include "common/mac/dump_syms.h"
#include "common/mac/macho_utilities.h"

using google_breakpad::DumpSymbols;
using std::vector;

struct Options {
  Options() : srcPath(), arch(), cfi(true) { }
  NSString *srcPath;
  const NXArchInfo *arch;
  bool cfi;
};

//=============================================================================
static bool Start(const Options &options) {
  DumpSymbols dump_symbols;

  if (!dump_symbols.Read(options.srcPath))
    return false;

  if (options.arch) {
    if (!dump_symbols.SetArchitecture(options.arch->cputype,
                                      options.arch->cpusubtype)) {
      fprintf(stderr, "%s: no architecture '%s' is present in file.\n",
              [options.srcPath fileSystemRepresentation], options.arch->name);
      size_t available_size;
      const struct fat_arch *available =
        dump_symbols.AvailableArchitectures(&available_size);
      if (available_size == 1)
        fprintf(stderr, "the file's architecture is: ");
      else
        fprintf(stderr, "architectures present in the file are:\n");
      for (size_t i = 0; i < available_size; i++) {
        const struct fat_arch *arch = &available[i];
        const NXArchInfo *arch_info =
          NXGetArchInfoFromCpuType(arch->cputype, arch->cpusubtype);
        if (arch_info)
          fprintf(stderr, "%s (%s)\n", arch_info->name, arch_info->description);
        else
          fprintf(stderr, "unrecognized cpu type 0x%x, subtype 0x%x\n",
                  arch->cputype, arch->cpusubtype);
      }
      return false;
    }
  }

  return dump_symbols.WriteSymbolFile(std::cout, options.cfi);
}

//=============================================================================
static void Usage(int argc, const char *argv[]) {
  fprintf(stderr, "Output a Breakpad symbol file from a Mach-o file.\n");
  fprintf(stderr, "Usage: %s [-a ARCHITECTURE] [-c] <Mach-o file>\n",
          argv[0]);
  fprintf(stderr, "\t-a: Architecture type [default: native, or whatever is\n");
  fprintf(stderr, "\t    in the file, if it contains only one architecture]\n");
  fprintf(stderr, "\t-c: Do not generate CFI section\n");
  fprintf(stderr, "\t-h: Usage\n");
  fprintf(stderr, "\t-?: Usage\n");
}

//=============================================================================
static void SetupOptions(int argc, const char *argv[], Options *options) {
  extern int optind;
  signed char ch;

  while ((ch = getopt(argc, (char * const *)argv, "a:ch?")) != -1) {
    switch (ch) {
      case 'a': {
        const NXArchInfo *arch_info = NXGetArchInfoFromName(optarg);
        if (!arch_info) {
          fprintf(stderr, "%s: Invalid architecture: %s\n", argv[0], optarg);
          Usage(argc, argv);
          exit(1);
        }
        options->arch = arch_info;
        break;
      }
      case 'c':
        options->cfi = false;
        break;
      case '?':
      case 'h':
        Usage(argc, argv);
        exit(0);
        break;
    }
  }

  if ((argc - optind) != 1) {
    fprintf(stderr, "Must specify Mach-o file\n");
    Usage(argc, argv);
    exit(1);
  }

  options->srcPath = [[NSFileManager defaultManager]
                       stringWithFileSystemRepresentation:argv[optind]
                       length:strlen(argv[optind])];
}

//=============================================================================
int main (int argc, const char * argv[]) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  Options options;
  bool result;

  SetupOptions(argc, argv, &options);
  result = Start(options);

  [pool release];

  return !result;
}
