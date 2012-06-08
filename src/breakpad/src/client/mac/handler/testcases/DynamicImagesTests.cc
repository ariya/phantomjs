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
//  DynamicImagesTests.cpp
//  minidump_test
//
//  Created by Neal Sidhwaney on 4/17/08.
//  Copyright 2008 Google Inc. All rights reserved.
//

#include "client/mac/handler/testcases/DynamicImagesTests.h"
#include "client/mac/handler/dynamic_images.h"

DynamicImagesTests test2(TEST_INVOCATION(DynamicImagesTests,
                                         ReadTaskMemoryTest));
DynamicImagesTests test3(TEST_INVOCATION(DynamicImagesTests,
                                         ReadLibrariesFromLocalTaskTest));

DynamicImagesTests::DynamicImagesTests(TestInvocation *invocation)
    : TestCase(invocation) {
}

DynamicImagesTests::~DynamicImagesTests() {
}

void DynamicImagesTests::ReadTaskMemoryTest() {
  kern_return_t kr;

  // pick test2 as a symbol we know to be valid to read
  // anything will work, really
  void *addr = reinterpret_cast<void*>(&test2);
  std::vector<uint8_t> buf(getpagesize());

  fprintf(stderr, "reading 0x%p\n", addr);
  kr = google_breakpad::ReadTaskMemory(mach_task_self(),
                                       (uint64_t)addr,
                                       getpagesize(),
                                       buf);

  CPTAssert(kr == KERN_SUCCESS);

  CPTAssert(0 == memcmp(&buf[0], (const void*)addr, getpagesize()));
}

void DynamicImagesTests::ReadLibrariesFromLocalTaskTest() {

  mach_port_t me = mach_task_self();
  google_breakpad::DynamicImages *d = new google_breakpad::DynamicImages(me);

  fprintf(stderr,"Local task image count: %d\n", d->GetImageCount());

  CPTAssert(d->GetImageCount() > 0);
}
