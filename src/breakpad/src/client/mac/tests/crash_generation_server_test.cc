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
//
// crash_generation_server_test.cc
// Unit tests for CrashGenerationServer

#include <dirent.h>
#include <glob.h>
#include <stdint.h>
#include <sys/wait.h>
#include <unistd.h>

#include <string>

#include "breakpad_googletest_includes.h"
#include "client/mac/crash_generation/client_info.h"
#include "client/mac/crash_generation/crash_generation_client.h"
#include "client/mac/crash_generation/crash_generation_server.h"
#include "client/mac/handler/exception_handler.h"
#include "client/mac/tests/spawn_child_process.h"
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
using google_breakpad::AutoTempDir;
using google_breakpad::ClientInfo;
using google_breakpad::CrashGenerationClient;
using google_breakpad::CrashGenerationServer;
using google_breakpad::ExceptionHandler;
using google_breakpad::Minidump;
using google_breakpad::MinidumpContext;
using google_breakpad::MinidumpException;
using google_breakpad::MinidumpModule;
using google_breakpad::MinidumpModuleList;
using google_breakpad::MinidumpSystemInfo;
using google_breakpad::MinidumpThread;
using google_breakpad::MinidumpThreadList;
using testing::Test;
using namespace google_breakpad_test;

class CrashGenerationServerTest : public Test {
public:
  // The port name to receive messages on
  char mach_port_name[128];
  // Filename of the last dump that was generated
  string last_dump_name;
  // PID of the child process
  pid_t child_pid;
  // A temp dir
  AutoTempDir temp_dir;
  // Counter just to ensure that we don't hit the same port again
  static int i;

  void SetUp() {
    sprintf(mach_port_name,
	    "com.google.breakpad.ServerTest.%d.%d", getpid(),
	    CrashGenerationServerTest::i++);
    child_pid = (pid_t)-1;
  }
};
int CrashGenerationServerTest::i = 0;

// Test that starting and stopping a server works
TEST_F(CrashGenerationServerTest, testStartStopServer) {
  CrashGenerationServer server(mach_port_name,
			       NULL,  // dump callback
			       NULL,  // dump context
			       NULL,  // exit callback
			       NULL,  // exit context
			       false, // generate dumps
			       ""); // dump path
  ASSERT_TRUE(server.Start());
  ASSERT_TRUE(server.Stop());
}

// Test that requesting a dump via CrashGenerationClient works
// Test without actually dumping
TEST_F(CrashGenerationServerTest, testRequestDumpNoDump) {
  CrashGenerationServer server(mach_port_name,
                               NULL,  // dump callback
                               NULL,  // dump context
                               NULL,  // exit callback
                               NULL,  // exit context
                               false, // don't generate dumps
                               temp_dir.path()); // dump path
  ASSERT_TRUE(server.Start());

  pid_t pid = fork();
  ASSERT_NE(-1, pid);
  if (pid == 0) {
    CrashGenerationClient client(mach_port_name);
    bool result = client.RequestDump();
    exit(result ? 0 : 1);
  }

  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_TRUE(WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));
  EXPECT_TRUE(server.Stop());
  // check that no minidump was written
  string pattern = temp_dir.path() + "/*";
  glob_t dirContents;
  ret = glob(pattern.c_str(), GLOB_NOSORT, NULL, &dirContents);
  EXPECT_EQ(GLOB_NOMATCH, ret);
  if (ret != GLOB_NOMATCH)
    globfree(&dirContents);
}

void dumpCallback(void *context, const ClientInfo &client_info,
		  const std::string &file_path) {
  if (context) {
    CrashGenerationServerTest* self =
        reinterpret_cast<CrashGenerationServerTest*>(context);
    if (!file_path.empty())
      self->last_dump_name = file_path;
    self->child_pid = client_info.pid();
  }
}

void *RequestDump(void *context) {
  CrashGenerationClient client((const char*)context);
  bool result = client.RequestDump();
  return (void*)(result ? 0 : 1);
}

// Test that actually writing a minidump works
TEST_F(CrashGenerationServerTest, testRequestDump) {
  CrashGenerationServer server(mach_port_name,
                               dumpCallback,  // dump callback
                               this,  // dump context
                               NULL,  // exit callback
                               NULL,  // exit context
                               true, //  generate dumps
                               temp_dir.path()); // dump path
  ASSERT_TRUE(server.Start());

  pid_t pid = fork();
  ASSERT_NE(-1, pid);
  if (pid == 0) {
    // Have to spawn off a separate thread to request the dump,
    // because MinidumpGenerator assumes the handler thread is not
    // the only thread
    pthread_t thread;
    if (pthread_create(&thread, NULL, RequestDump, (void*)mach_port_name) != 0)
      exit(1);
    void* result;
    pthread_join(thread, &result);
    exit(reinterpret_cast<intptr_t>(result));
  }

  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_TRUE(WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));
  EXPECT_TRUE(server.Stop());
  // check that minidump was written
  ASSERT_FALSE(last_dump_name.empty());
  struct stat st;
  EXPECT_EQ(0, stat(last_dump_name.c_str(), &st));
  EXPECT_LT(0, st.st_size);
  // check client's PID
  ASSERT_EQ(pid, child_pid);
}

static void Crasher() {
  int *a = (int*)0x42;

  fprintf(stdout, "Going to crash...\n");
  fprintf(stdout, "A = %d", *a);
}

// Test that crashing a child process with an OOP ExceptionHandler installed
// results in a minidump being written by the CrashGenerationServer in
// the parent.
TEST_F(CrashGenerationServerTest, testChildProcessCrash) {
  CrashGenerationServer server(mach_port_name,
                               dumpCallback,  // dump callback
                               this,  // dump context
                               NULL,  // exit callback
                               NULL,  // exit context
                               true, //  generate dumps
                               temp_dir.path()); // dump path
  ASSERT_TRUE(server.Start());

  pid_t pid = fork();
  ASSERT_NE(-1, pid);
  if (pid == 0) {
    // Instantiate an OOP exception handler.
    ExceptionHandler eh("", NULL, NULL, NULL, true, mach_port_name);
    Crasher();
    // not reached
    exit(0);
  }

  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_FALSE(WIFEXITED(ret));
  EXPECT_TRUE(server.Stop());
  // check that minidump was written
  ASSERT_FALSE(last_dump_name.empty());
  struct stat st;
  EXPECT_EQ(0, stat(last_dump_name.c_str(), &st));
  EXPECT_LT(0, st.st_size);

  // Read the minidump, sanity check some data.
  Minidump minidump(last_dump_name.c_str());
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

#if (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6) && \
  (defined(__x86_64__) || defined(__i386__))
// Test that crashing a child process of a different architecture
// produces a valid minidump.
TEST_F(CrashGenerationServerTest, testChildProcessCrashCrossArchitecture) {
  CrashGenerationServer server(mach_port_name,
                               dumpCallback,  // dump callback
                               this,  // dump context
                               NULL,  // exit callback
                               NULL,  // exit context
                               true, //  generate dumps
                               temp_dir.path()); // dump path
  ASSERT_TRUE(server.Start());

  // Spawn a child process
  string helper_path = GetHelperPath();
  const char* argv[] = {
    helper_path.c_str(),
    "crash",
    mach_port_name,
    NULL
  };
  pid_t pid = spawn_child_process(argv);
  ASSERT_NE(-1, pid);

  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_FALSE(WIFEXITED(ret));
  EXPECT_TRUE(server.Stop());
  // check that minidump was written
  ASSERT_FALSE(last_dump_name.empty());
  struct stat st;
  EXPECT_EQ(0, stat(last_dump_name.c_str(), &st));
  EXPECT_LT(0, st.st_size);

const MDCPUArchitecture kExpectedArchitecture =
#if defined(__x86_64__)
  MD_CPU_ARCHITECTURE_X86
#elif defined(__i386__)
  MD_CPU_ARCHITECTURE_AMD64
#endif
  ;
const u_int32_t kExpectedContext =
#if defined(__i386__)
  MD_CONTEXT_AMD64
#elif defined(__x86_64__)
  MD_CONTEXT_X86
#endif
  ;

  // Read the minidump, sanity check some data.
  Minidump minidump(last_dump_name.c_str());
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
#endif

}  // namespace
