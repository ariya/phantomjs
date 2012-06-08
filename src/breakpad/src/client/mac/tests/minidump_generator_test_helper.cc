// Copyright (c) 2010, Google Inc.
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

// minidump_generator_test_helper.cc: A helper program that
//   minidump_generator_test.cc can launch to test certain things
//   that require a separate executable.

#include <unistd.h>

#include "client/mac/handler/exception_handler.h"
#include "common/mac/MachIPC.h"

using google_breakpad::MachPortSender;
using google_breakpad::MachReceiveMessage;
using google_breakpad::MachSendMessage;
using google_breakpad::ReceivePort;

int main(int argc, char** argv) {
  if (argc < 2)
    return 1;

  if (strcmp(argv[1], "crash") != 0) {
    const int kTimeoutMs = 2000;
    // Send parent process the task and thread ports.
    MachSendMessage child_message(0);
    child_message.AddDescriptor(mach_task_self());
    child_message.AddDescriptor(mach_thread_self());

    MachPortSender child_sender(argv[1]);
    if (child_sender.SendMessage(child_message, kTimeoutMs) != KERN_SUCCESS) {
      fprintf(stderr, "Error sending message from child process!\n");
      exit(1);
    }

    // Loop forever.
    while (true) {
      sleep(100);
    }
  } else if (argc == 3 && strcmp(argv[1], "crash") == 0) {
    // Instantiate an OOP exception handler
    google_breakpad::ExceptionHandler eh("", NULL, NULL, NULL, true, argv[2]);
    // and crash.
    int *a = (int*)0x42;
    *a = 1;
  }

  return 0;
}
