// Copyright (c) 2008, Google Inc.
// All rights reserved
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

//
//  breakpad_nlist_test.cc
//  minidump_test
//
//  Created by Neal Sidhwaney on 4/13/08.
//  Copyright 2008 Google Inc. All rights reserved.
//

#include "client/mac/handler/testcases/breakpad_nlist_test.h"
#include <mach-o/nlist.h>
#include "client/mac/handler/breakpad_nlist_64.h"

BreakpadNlistTest test1(TEST_INVOCATION(BreakpadNlistTest, CompareToNM));

BreakpadNlistTest::BreakpadNlistTest(TestInvocation *invocation)
    : TestCase(invocation) {
}


BreakpadNlistTest::~BreakpadNlistTest() {
}

void BreakpadNlistTest::CompareToNM() {
#if TARGET_CPU_X86_64
  system("/usr/bin/nm -arch x86_64 /usr/lib/dyld > /tmp/dyld-namelist.txt");
#elif TARGET_CPU_PPC64
  system("/usr/bin/nm -arch ppc64 /usr/lib/dyld > /tmp/dyld-namelist.txt");
#endif

  FILE *fd = fopen("/tmp/dyld-namelist.txt", "rt");

  char oneNMAddr[30];
  char symbolType;
  char symbolName[500];
  while (!feof(fd)) {
    fscanf(fd, "%s %c %s", oneNMAddr, &symbolType, symbolName);
    breakpad_nlist symbolList[2];
    breakpad_nlist &list = symbolList[0];

    memset(symbolList, 0, sizeof(breakpad_nlist)*2);
    const char *symbolNames[2];
    symbolNames[0] = (const char*)symbolName;
    symbolNames[1] = "\0";
    breakpad_nlist_64("/usr/lib/dyld", &list, symbolNames);
    uint64_t nmAddr = strtol(oneNMAddr, NULL, 16);
    if (!IsSymbolMoreThanOnceInDyld(symbolName)) {
      CPTAssert(nmAddr == symbolList[0].n_value);
    }
  }

  fclose(fd);
}

bool BreakpadNlistTest::IsSymbolMoreThanOnceInDyld(const char *symbolName) {
  // These are the symbols that occur more than once when nm dumps
  // the symbol table of /usr/lib/dyld.  Our nlist program returns
  // the first address because it's doing a search so we need to exclude
  // these from causing the test to fail
  const char *multipleSymbols[] = {
    "__Z41__static_initialization_and_destruction_0ii",
    "___tcf_0",
    "___tcf_1",
    "_read_encoded_value_with_base",
    "_read_sleb128",
    "_read_uleb128",
    "\0"};

  bool found = false;

  for (int i = 0; multipleSymbols[i][0]; i++) {
    if (!strcmp(multipleSymbols[i], symbolName)) {
      found = true;
      break;
    }
  }

  return found;
}
