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

// minidump_generator_test.cc: Unit tests for google_breakpad::MinidumpGenerator

#include <AvailabilityMacros.h>
#ifndef MAC_OS_X_VERSION_10_6
#define MAC_OS_X_VERSION_10_6 1060
#endif
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "breakpad_googletest_includes.h"
#include "client/mac/handler/minidump_generator.h"
#include "client/mac/tests/spawn_child_process.h"
#include "common/mac/MachIPC.h"
#include "common/tests/auto_tempdir.h"
#include "google_breakpad/processor/minidump.h"

namespace google_breakpad {
// This acts as the log sink for INFO logging from the processor
// logging code. The logging output confuses XCode and makes it think
// there are unit test failures. testlogging.h handles the overriding.
std::ostringstream info_log;
}

namespace {
using std::string;
using std::vector;
using google_breakpad::AutoTempDir;
using google_breakpad::MinidumpGenerator;
using google_breakpad::MachPortSender;
using google_breakpad::MachReceiveMessage;
using google_breakpad::MachSendMessage;
using google_breakpad::Minidump;
using google_breakpad::MinidumpContext;
using google_breakpad::MinidumpException;
using google_breakpad::MinidumpModule;
using google_breakpad::MinidumpModuleList;
using google_breakpad::MinidumpSystemInfo;
using google_breakpad::MinidumpThread;
using google_breakpad::MinidumpThreadList;
using google_breakpad::ReceivePort;
using testing::Test;
using namespace google_breakpad_test;

class MinidumpGeneratorTest : public Test {
 public:
  AutoTempDir tempDir;
};

static void *Junk(void* data) {
  bool* wait = reinterpret_cast<bool*>(data);
  while (!*wait) {
    usleep(10000);
  }
  return NULL;
}

TEST_F(MinidumpGeneratorTest, InProcess) {
  MinidumpGenerator generator;
  string dump_filename =
      MinidumpGenerator::UniqueNameInDirectory(tempDir.path(), NULL);

  // Run an extra thread since MinidumpGenerator assumes there
  // are 2 or more threads.
  pthread_t junk_thread;
  bool quit = false;
  ASSERT_EQ(0, pthread_create(&junk_thread, NULL, Junk, &quit));

  ASSERT_TRUE(generator.Write(dump_filename.c_str()));
  // Ensure that minidump file exists and is > 0 bytes.
  struct stat st;
  ASSERT_EQ(0, stat(dump_filename.c_str(), &st));
  ASSERT_LT(0, st.st_size);

  // join the background thread
  quit = true;
  pthread_join(junk_thread, NULL);

  // Read the minidump, sanity check some data.
  Minidump minidump(dump_filename.c_str());
  ASSERT_TRUE(minidump.Read());

  MinidumpSystemInfo* system_info = minidump.GetSystemInfo();
  ASSERT_TRUE(system_info);
  const MDRawSystemInfo* raw_info = system_info->system_info();
  ASSERT_TRUE(raw_info);
  EXPECT_EQ(kNativeArchitecture, raw_info->processor_architecture);

  MinidumpThreadList* thread_list = minidump.GetThreadList();
  ASSERT_TRUE(thread_list);
  ASSERT_EQ((unsigned int)1, thread_list->thread_count());

  MinidumpThread* main_thread = thread_list->GetThreadAtIndex(0);
  ASSERT_TRUE(main_thread);
  MinidumpContext* context = main_thread->GetContext();
  ASSERT_TRUE(context);
  EXPECT_EQ(kNativeContext, context->GetContextCPU());

  MinidumpModuleList* module_list = minidump.GetModuleList();
  ASSERT_TRUE(module_list);
  const MinidumpModule* main_module = module_list->GetMainModule();
  ASSERT_TRUE(main_module);
  EXPECT_EQ(GetExecutablePath(), main_module->code_file());
}

TEST_F(MinidumpGeneratorTest, OutOfProcess) {
  const int kTimeoutMs = 2000;
  // Create a mach port to receive the child task on.
  char machPortName[128];
  sprintf(machPortName, "MinidumpGeneratorTest.OutOfProcess.%d", getpid());
  ReceivePort parent_recv_port(machPortName);

  // Give the child process a pipe to block on.
  int fds[2];
  ASSERT_EQ(0, pipe(fds));

  // Fork off a child process to dump.
  pid_t pid = fork();
  if (pid == 0) {
    // In the child process
    close(fds[1]);

    // Send parent process the task port.
    MachSendMessage child_message(0);
    child_message.AddDescriptor(mach_task_self());

    MachPortSender child_sender(machPortName);
    if (child_sender.SendMessage(child_message, kTimeoutMs) != KERN_SUCCESS) {
      fprintf(stderr, "Error sending message from child process!\n");
      exit(1);
    }

    // Wait for the parent process.
    uint8_t data;
    read(fds[0], &data, 1);
    exit(0);
  }
  // In the parent process.
  ASSERT_NE(-1, pid);
  close(fds[0]);

  // Read the child's task port.
  MachReceiveMessage child_message;
  ASSERT_EQ(KERN_SUCCESS,
	    parent_recv_port.WaitForMessage(&child_message, kTimeoutMs));
  mach_port_t child_task = child_message.GetTranslatedPort(0);
  ASSERT_NE((mach_port_t)MACH_PORT_NULL, child_task);

  // Write a minidump of the child process.
  MinidumpGenerator generator(child_task, MACH_PORT_NULL);
  string dump_filename =
      MinidumpGenerator::UniqueNameInDirectory(tempDir.path(), NULL);
  ASSERT_TRUE(generator.Write(dump_filename.c_str()));

  // Ensure that minidump file exists and is > 0 bytes.
  struct stat st;
  ASSERT_EQ(0, stat(dump_filename.c_str(), &st));
  ASSERT_LT(0, st.st_size);

  // Unblock child process
  uint8_t data = 1;
  (void)write(fds[1], &data, 1);

  // Child process should have exited with a zero status.
  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_NE(0, WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));

  // Read the minidump, sanity check some data.
  Minidump minidump(dump_filename.c_str());
  ASSERT_TRUE(minidump.Read());

  MinidumpSystemInfo* system_info = minidump.GetSystemInfo();
  ASSERT_TRUE(system_info);
  const MDRawSystemInfo* raw_info = system_info->system_info();
  ASSERT_TRUE(raw_info);
  EXPECT_EQ(kNativeArchitecture, raw_info->processor_architecture);

  MinidumpThreadList* thread_list = minidump.GetThreadList();
  ASSERT_TRUE(thread_list);
  ASSERT_EQ((unsigned int)1, thread_list->thread_count());

  MinidumpThread* main_thread = thread_list->GetThreadAtIndex(0);
  ASSERT_TRUE(main_thread);
  MinidumpContext* context = main_thread->GetContext();
  ASSERT_TRUE(context);
  EXPECT_EQ(kNativeContext, context->GetContextCPU());

  MinidumpModuleList* module_list = minidump.GetModuleList();
  ASSERT_TRUE(module_list);
  const MinidumpModule* main_module = module_list->GetMainModule();
  ASSERT_TRUE(main_module);
  EXPECT_EQ(GetExecutablePath(), main_module->code_file());
}

// This test fails on 10.5, but I don't have easy access to a 10.5 machine,
// so it's simpler to just limit it to 10.6 for now.
#if (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6) && \
  (defined(__x86_64__) || defined(__i386__))

TEST_F(MinidumpGeneratorTest, CrossArchitectureDump) {
  const int kTimeoutMs = 5000;
  // Create a mach port to receive the child task on.
  char machPortName[128];
  sprintf(machPortName,
          "MinidumpGeneratorTest.CrossArchitectureDump.%d", getpid());

  ReceivePort parent_recv_port(machPortName);

  // Spawn a child process to dump.
  string helper_path = GetHelperPath();
  const char* argv[] = {
    helper_path.c_str(),
    machPortName,
    NULL
  };
  pid_t pid = spawn_child_process(argv);
  ASSERT_NE(-1, pid);

  // Read the child's task port.
  MachReceiveMessage child_message;
  ASSERT_EQ(KERN_SUCCESS,
	    parent_recv_port.WaitForMessage(&child_message, kTimeoutMs));
  mach_port_t child_task = child_message.GetTranslatedPort(0);
  ASSERT_NE((mach_port_t)MACH_PORT_NULL, child_task);

  // Write a minidump of the child process.
  MinidumpGenerator generator(child_task, MACH_PORT_NULL);
  string dump_filename =
      MinidumpGenerator::UniqueNameInDirectory(tempDir.path(), NULL);
  ASSERT_TRUE(generator.Write(dump_filename.c_str()));

  // Ensure that minidump file exists and is > 0 bytes.
  struct stat st;
  ASSERT_EQ(0, stat(dump_filename.c_str(), &st));
  ASSERT_LT(0, st.st_size);

  // Kill child process.
  kill(pid, SIGKILL);

  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));

const MDCPUArchitecture kExpectedArchitecture =
#if defined(__x86_64__)
  MD_CPU_ARCHITECTURE_X86
#elif defined(__i386__)
  MD_CPU_ARCHITECTURE_AMD64
#endif
  ;
const uint32_t kExpectedContext =
#if defined(__i386__)
  MD_CONTEXT_AMD64
#elif defined(__x86_64__)
  MD_CONTEXT_X86
#endif
  ;

  // Read the minidump, sanity check some data.
  Minidump minidump(dump_filename.c_str());
  ASSERT_TRUE(minidump.Read());

  MinidumpSystemInfo* system_info = minidump.GetSystemInfo();
  ASSERT_TRUE(system_info);
  const MDRawSystemInfo* raw_info = system_info->system_info();
  ASSERT_TRUE(raw_info);
  EXPECT_EQ(kExpectedArchitecture, raw_info->processor_architecture);

  MinidumpThreadList* thread_list = minidump.GetThreadList();
  ASSERT_TRUE(thread_list);
  ASSERT_EQ((unsigned int)1, thread_list->thread_count());

  MinidumpThread* main_thread = thread_list->GetThreadAtIndex(0);
  ASSERT_TRUE(main_thread);
  MinidumpContext* context = main_thread->GetContext();
  ASSERT_TRUE(context);
  EXPECT_EQ(kExpectedContext, context->GetContextCPU());

  MinidumpModuleList* module_list = minidump.GetModuleList();
  ASSERT_TRUE(module_list);
  const MinidumpModule* main_module = module_list->GetMainModule();
  ASSERT_TRUE(main_module);
  EXPECT_EQ(helper_path, main_module->code_file());
}
#endif  // 10.6 && (x86-64 || i386)

}
