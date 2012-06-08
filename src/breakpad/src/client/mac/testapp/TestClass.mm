// Copyright (c) 2006, Google Inc.
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

#include <unistd.h>

#import "TestClass.h"

struct AStruct {
  int x;
  float y;
  double z;
};

class InternalTestClass {
 public:
  InternalTestClass(int a) : a_(a) {}
  ~InternalTestClass() {}

  void snooze(float a);
  void snooze(int a);
  int snooze(int a, float b);

 protected:
  int a_;
  AStruct s_;

  static void InternalFunction(AStruct &s);
  static float kStaticFloatValue;
};

void InternalTestClass::snooze(float a) {
  InternalFunction(s_);
  sleep(a_ * a);
}

void InternalTestClass::snooze(int a) {
  InternalFunction(s_);
  sleep(a_ * a);
}

int InternalTestClass::snooze(int a, float b) {
  InternalFunction(s_);
  sleep(a_ * a * b);

  return 33;
}

void InternalTestClass::InternalFunction(AStruct &s) {
  s.x = InternalTestClass::kStaticFloatValue;
}

float InternalTestClass::kStaticFloatValue = 42;

static float PlainOldFunction() {
  return 3.14145f;
}

@implementation TestClass

- (void)wait {
  InternalTestClass t(10);
  float z = PlainOldFunction();

  while (1) {
    t.snooze(z);
  }
}

@end
