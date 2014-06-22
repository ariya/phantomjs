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

// exception_handler_test.cc: Unit tests for google_breakpad::ExceptionHandler

#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "breakpad_googletest_includes.h"
#include "client/mac/handler/exception_handler.h"
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
using google_breakpad::AutoTempDir;
using google_breakpad::ExceptionHandler;
using google_breakpad::MachPortSender;
using google_breakpad::MachReceiveMessage;
using google_breakpad::MachSendMessage;
using google_breakpad::Minidump;
using google_breakpad::MinidumpContext;
using google_breakpad::MinidumpException;
using google_breakpad::MinidumpMemoryList;
using google_breakpad::MinidumpMemoryRegion;
using google_breakpad::ReceivePort;
using testing::Test;

class ExceptionHandlerTest : public Test {
 public:
  void InProcessCrash(bool aborting);
  AutoTempDir tempDir;
  string lastDumpName;
};

static void Crasher() {
  int *a = (int*)0x42;

  fprintf(stdout, "Going to crash...\n");
  fprintf(stdout, "A = %d", *a);
}

static void AbortCrasher() {
  fprintf(stdout, "Going to crash...\n");
  abort();
}

static void SoonToCrash(void(*crasher)()) {
  crasher();
}

static bool MDCallback(const char *dump_dir, const char *file_name,
                       void *context, bool success) {
  string path(dump_dir);
  path.append("/");
  path.append(file_name);
  path.append(".dmp");

  int fd = *reinterpret_cast<int*>(context);
  (void)write(fd, path.c_str(), path.length() + 1);
  close(fd);
  exit(0);
  // not reached
  return true;
}

void ExceptionHandlerTest::InProcessCrash(bool aborting) {
  // Give the child process a pipe to report back on.
  int fds[2];
  ASSERT_EQ(0, pipe(fds));
  // Fork off a child process so it can crash.
  pid_t pid = fork();
  if (pid == 0) {
    // In the child process.
    close(fds[0]);
    ExceptionHandler eh(tempDir.path(), NULL, MDCallback, &fds[1], true, NULL);
    // crash
    SoonToCrash(aborting ? &AbortCrasher : &Crasher);
    // not reached
    exit(1);
  }
  // In the parent process.
  ASSERT_NE(-1, pid);
  // Wait for the background process to return the minidump file.
  close(fds[1]);
  char minidump_file[PATH_MAX];
  ssize_t nbytes = read(fds[0], minidump_file, sizeof(minidump_file));
  ASSERT_NE(0, nbytes);

  Minidump minidump(minidump_file);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  ASSERT_TRUE(exception);

  const MDRawExceptionStream* raw_exception = exception->exception();
  ASSERT_TRUE(raw_exception);

  if (aborting) {
    EXPECT_EQ(MD_EXCEPTION_MAC_SOFTWARE,
              raw_exception->exception_record.exception_code);
    EXPECT_EQ(MD_EXCEPTION_CODE_MAC_ABORT,
              raw_exception->exception_record.exception_flags);
  } else {
    EXPECT_EQ(MD_EXCEPTION_MAC_BAD_ACCESS,
              raw_exception->exception_record.exception_code);
#if defined(__x86_64__)
    EXPECT_EQ(MD_EXCEPTION_CODE_MAC_INVALID_ADDRESS,
              raw_exception->exception_record.exception_flags);
#elif defined(__i386__)
    EXPECT_EQ(MD_EXCEPTION_CODE_MAC_PROTECTION_FAILURE,
              raw_exception->exception_record.exception_flags);
#endif
  }

  const MinidumpContext* context = exception->GetContext();
  ASSERT_TRUE(context);

  uint64_t instruction_pointer;
  ASSERT_TRUE(context->GetInstructionPointer(&instruction_pointer));

  // Ideally would like to sanity check that abort() is on the stack
  // but that's hard.
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(memory_list);
  MinidumpMemoryRegion* region =
      memory_list->GetMemoryRegionForAddress(instruction_pointer);
  EXPECT_TRUE(region);

  // Child process should have exited with a zero status.
  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_NE(0, WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));
}

TEST_F(ExceptionHandlerTest, InProcess) {
  InProcessCrash(false);
}

TEST_F(ExceptionHandlerTest, InProcessAbort) {
  InProcessCrash(true);
}

static bool DumpNameMDCallback(const char *dump_dir, const char *file_name,
                               void *context, bool success) {
  ExceptionHandlerTest *self = reinterpret_cast<ExceptionHandlerTest*>(context);
  if (dump_dir && file_name) {
    self->lastDumpName = dump_dir;
    self->lastDumpName += "/";
    self->lastDumpName += file_name;
    self->lastDumpName += ".dmp";
  }
  return true;
}

TEST_F(ExceptionHandlerTest, WriteMinidump) {
  ExceptionHandler eh(tempDir.path(), NULL, DumpNameMDCallback, this, true,
                      NULL);
  ASSERT_TRUE(eh.WriteMinidump());

  // Ensure that minidump file exists and is > 0 bytes.
  ASSERT_FALSE(lastDumpName.empty());
  struct stat st;
  ASSERT_EQ(0, stat(lastDumpName.c_str(), &st));
  ASSERT_LT(0, st.st_size);

  // The minidump should not contain an exception stream.
  Minidump minidump(lastDumpName);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  EXPECT_FALSE(exception);
}

TEST_F(ExceptionHandlerTest, WriteMinidumpWithException) {
  ExceptionHandler eh(tempDir.path(), NULL, DumpNameMDCallback, this, true,
                      NULL);
  ASSERT_TRUE(eh.WriteMinidump(true));

  // Ensure that minidump file exists and is > 0 bytes.
  ASSERT_FALSE(lastDumpName.empty());
  struct stat st;
  ASSERT_EQ(0, stat(lastDumpName.c_str(), &st));
  ASSERT_LT(0, st.st_size);

  // The minidump should contain an exception stream.
  Minidump minidump(lastDumpName);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  ASSERT_TRUE(exception);
  const MDRawExceptionStream* raw_exception = exception->exception();
  ASSERT_TRUE(raw_exception);

  EXPECT_EQ(MD_EXCEPTION_MAC_BREAKPOINT,
            raw_exception->exception_record.exception_code);
}

TEST_F(ExceptionHandlerTest, DumpChildProcess) {
  const int kTimeoutMs = 2000;
  // Create a mach port to receive the child task on.
  char machPortName[128];
  sprintf(machPortName, "ExceptionHandlerTest.%d", getpid());
  ReceivePort parent_recv_port(machPortName);

  // Give the child process a pipe to block on.
  int fds[2];
  ASSERT_EQ(0, pipe(fds));

  // Fork off a child process to dump.
  pid_t pid = fork();
  if (pid == 0) {
    // In the child process
    close(fds[1]);

    // Send parent process the task and thread ports.
    MachSendMessage child_message(0);
    child_message.AddDescriptor(mach_task_self());
    child_message.AddDescriptor(mach_thread_self());

    MachPortSender child_sender(machPortName);
    if (child_sender.SendMessage(child_message, kTimeoutMs) != KERN_SUCCESS)
      exit(1);

    // Wait for the parent process.
    uint8_t data;
    read(fds[0], &data, 1);
    exit(0);
  }
  // In the parent process.
  ASSERT_NE(-1, pid);
  close(fds[0]);

  // Read the child's task and thread ports.
  MachReceiveMessage child_message;
  ASSERT_EQ(KERN_SUCCESS,
	    parent_recv_port.WaitForMessage(&child_message, kTimeoutMs));
  mach_port_t child_task = child_message.GetTranslatedPort(0);
  mach_port_t child_thread = child_message.GetTranslatedPort(1);
  ASSERT_NE((mach_port_t)MACH_PORT_NULL, child_task);
  ASSERT_NE((mach_port_t)MACH_PORT_NULL, child_thread);

  // Write a minidump of the child process.
  bool result = ExceptionHandler::WriteMinidumpForChild(child_task,
                                                        child_thread,
                                                        tempDir.path(),
                                                        DumpNameMDCallback,
                                                        this);
  ASSERT_EQ(true, result);

  // Ensure that minidump file exists and is > 0 bytes.
  ASSERT_FALSE(lastDumpName.empty());
  struct stat st;
  ASSERT_EQ(0, stat(lastDumpName.c_str(), &st));
  ASSERT_LT(0, st.st_size);

  // Unblock child process
  uint8_t data = 1;
  (void)write(fds[1], &data, 1);

  // Child process should have exited with a zero status.
  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_NE(0, WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));
}

// Test that memory around the instruction pointer is written
// to the dump as a MinidumpMemoryRegion.
TEST_F(ExceptionHandlerTest, InstructionPointerMemory) {
  // Give the child process a pipe to report back on.
  int fds[2];
  ASSERT_EQ(0, pipe(fds));

  // These are defined here so the parent can use them to check the
  // data from the minidump afterwards.
  const uint32_t kMemorySize = 256;  // bytes
  const int kOffset = kMemorySize / 2;
  // This crashes with SIGILL on x86/x86-64/arm.
  const unsigned char instructions[] = { 0xff, 0xff, 0xff, 0xff };

  pid_t pid = fork();
  if (pid == 0) {
    close(fds[0]);
    ExceptionHandler eh(tempDir.path(), NULL, MDCallback, &fds[1], true, NULL);
    // Get some executable memory.
    char* memory =
      reinterpret_cast<char*>(mmap(NULL,
                                   kMemorySize,
                                   PROT_READ | PROT_WRITE | PROT_EXEC,
                                   MAP_PRIVATE | MAP_ANON,
                                   -1,
                                   0));
    if (!memory)
      exit(0);

    // Write some instructions that will crash. Put them in the middle
    // of the block of memory, because the minidump should contain 128
    // bytes on either side of the instruction pointer.
    memcpy(memory + kOffset, instructions, sizeof(instructions));

    // Now execute the instructions, which should crash.
    typedef void (*void_function)(void);
    void_function memory_function =
      reinterpret_cast<void_function>(memory + kOffset);
    memory_function();
    // not reached
    exit(1);
  }
  // In the parent process.
  ASSERT_NE(-1, pid);
  close(fds[1]);

  // Wait for the background process to return the minidump file.
  close(fds[1]);
  char minidump_file[PATH_MAX];
  ssize_t nbytes = read(fds[0], minidump_file, sizeof(minidump_file));
  ASSERT_NE(0, nbytes);
  // Ensure that minidump file exists and is > 0 bytes.
  struct stat st;
  ASSERT_EQ(0, stat(minidump_file, &st));
  ASSERT_LT(0, st.st_size);

  // Child process should have exited with a zero status.
  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_NE(0, WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));

  // Read the minidump. Locate the exception record and the
  // memory list, and then ensure that there is a memory region
  // in the memory list that covers the instruction pointer from
  // the exception record.
  Minidump minidump(minidump_file);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(exception);
  ASSERT_TRUE(memory_list);
  ASSERT_NE((unsigned int)0, memory_list->region_count());

  MinidumpContext* context = exception->GetContext();
  ASSERT_TRUE(context);

  uint64_t instruction_pointer;
  ASSERT_TRUE(context->GetInstructionPointer(&instruction_pointer));

  MinidumpMemoryRegion* region =
    memory_list->GetMemoryRegionForAddress(instruction_pointer);
  EXPECT_TRUE(region);

  EXPECT_EQ(kMemorySize, region->GetSize());
  const uint8_t* bytes = region->GetMemory();
  ASSERT_TRUE(bytes);

  uint8_t prefix_bytes[kOffset];
  uint8_t suffix_bytes[kMemorySize - kOffset - sizeof(instructions)];
  memset(prefix_bytes, 0, sizeof(prefix_bytes));
  memset(suffix_bytes, 0, sizeof(suffix_bytes));
  EXPECT_TRUE(memcmp(bytes, prefix_bytes, sizeof(prefix_bytes)) == 0);
  EXPECT_TRUE(memcmp(bytes + kOffset, instructions, sizeof(instructions)) == 0);
  EXPECT_TRUE(memcmp(bytes + kOffset + sizeof(instructions),
                     suffix_bytes, sizeof(suffix_bytes)) == 0);
}

// Test that the memory region around the instruction pointer is
// bounded correctly on the low end.
TEST_F(ExceptionHandlerTest, InstructionPointerMemoryMinBound) {
  // Give the child process a pipe to report back on.
  int fds[2];
  ASSERT_EQ(0, pipe(fds));

  // These are defined here so the parent can use them to check the
  // data from the minidump afterwards.
  const uint32_t kMemorySize = 256;  // bytes
  const int kOffset = 0;
  // This crashes with SIGILL on x86/x86-64/arm.
  const unsigned char instructions[] = { 0xff, 0xff, 0xff, 0xff };

  pid_t pid = fork();
  if (pid == 0) {
    close(fds[0]);
    ExceptionHandler eh(tempDir.path(), NULL, MDCallback, &fds[1], true, NULL);
    // Get some executable memory.
    char* memory =
      reinterpret_cast<char*>(mmap(NULL,
                                   kMemorySize,
                                   PROT_READ | PROT_WRITE | PROT_EXEC,
                                   MAP_PRIVATE | MAP_ANON,
                                   -1,
                                   0));
    if (!memory)
      exit(0);

    // Write some instructions that will crash. Put them at the start
    // of the block of memory, to ensure that the memory bounding
    // works properly.
    memcpy(memory + kOffset, instructions, sizeof(instructions));
    
    // Now execute the instructions, which should crash.
    typedef void (*void_function)(void);
    void_function memory_function =
      reinterpret_cast<void_function>(memory + kOffset);
    memory_function();
    // not reached
    exit(1);
  }
  // In the parent process.
  ASSERT_NE(-1, pid);
  close(fds[1]);

  // Wait for the background process to return the minidump file.
  close(fds[1]);
  char minidump_file[PATH_MAX];
  ssize_t nbytes = read(fds[0], minidump_file, sizeof(minidump_file));
  ASSERT_NE(0, nbytes);
  // Ensure that minidump file exists and is > 0 bytes.
  struct stat st;
  ASSERT_EQ(0, stat(minidump_file, &st));
  ASSERT_LT(0, st.st_size);

  // Child process should have exited with a zero status.
  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_NE(0, WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));

  // Read the minidump. Locate the exception record and the
  // memory list, and then ensure that there is a memory region
  // in the memory list that covers the instruction pointer from
  // the exception record.
  Minidump minidump(minidump_file);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(exception);
  ASSERT_TRUE(memory_list);
  ASSERT_NE((unsigned int)0, memory_list->region_count());

  MinidumpContext* context = exception->GetContext();
  ASSERT_TRUE(context);

  uint64_t instruction_pointer;
  ASSERT_TRUE(context->GetInstructionPointer(&instruction_pointer));

  MinidumpMemoryRegion* region =
    memory_list->GetMemoryRegionForAddress(instruction_pointer);
  EXPECT_TRUE(region);

  EXPECT_EQ(kMemorySize / 2, region->GetSize());
  const uint8_t* bytes = region->GetMemory();
  ASSERT_TRUE(bytes);

  uint8_t suffix_bytes[kMemorySize / 2 - sizeof(instructions)];
  memset(suffix_bytes, 0, sizeof(suffix_bytes));
  EXPECT_TRUE(memcmp(bytes + kOffset, instructions, sizeof(instructions)) == 0);
  EXPECT_TRUE(memcmp(bytes + kOffset + sizeof(instructions),
                     suffix_bytes, sizeof(suffix_bytes)) == 0);
}

// Test that the memory region around the instruction pointer is
// bounded correctly on the high end.
TEST_F(ExceptionHandlerTest, InstructionPointerMemoryMaxBound) {
  // Give the child process a pipe to report back on.
  int fds[2];
  ASSERT_EQ(0, pipe(fds));

  // These are defined here so the parent can use them to check the
  // data from the minidump afterwards.
  // Use 4k here because the OS will hand out a single page even
  // if a smaller size is requested, and this test wants to
  // test the upper bound of the memory range.
  const uint32_t kMemorySize = 4096;  // bytes
  // This crashes with SIGILL on x86/x86-64/arm.
  const unsigned char instructions[] = { 0xff, 0xff, 0xff, 0xff };
  const int kOffset = kMemorySize - sizeof(instructions);

  pid_t pid = fork();
  if (pid == 0) {
    close(fds[0]);
    ExceptionHandler eh(tempDir.path(), NULL, MDCallback, &fds[1], true, NULL);
    // Get some executable memory.
    char* memory =
      reinterpret_cast<char*>(mmap(NULL,
                                   kMemorySize,
                                   PROT_READ | PROT_WRITE | PROT_EXEC,
                                   MAP_PRIVATE | MAP_ANON,
                                   -1,
                                   0));
    if (!memory)
      exit(0);

    // Write some instructions that will crash. Put them at the start
    // of the block of memory, to ensure that the memory bounding
    // works properly.
    memcpy(memory + kOffset, instructions, sizeof(instructions));
    
    // Now execute the instructions, which should crash.
    typedef void (*void_function)(void);
    void_function memory_function =
      reinterpret_cast<void_function>(memory + kOffset);
    memory_function();
    // not reached
    exit(1);
  }
  // In the parent process.
  ASSERT_NE(-1, pid);
  close(fds[1]);

  // Wait for the background process to return the minidump file.
  close(fds[1]);
  char minidump_file[PATH_MAX];
  ssize_t nbytes = read(fds[0], minidump_file, sizeof(minidump_file));
  ASSERT_NE(0, nbytes);
  // Ensure that minidump file exists and is > 0 bytes.
  struct stat st;
  ASSERT_EQ(0, stat(minidump_file, &st));
  ASSERT_LT(0, st.st_size);

  // Child process should have exited with a zero status.
  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_NE(0, WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));

  // Read the minidump. Locate the exception record and the
  // memory list, and then ensure that there is a memory region
  // in the memory list that covers the instruction pointer from
  // the exception record.
  Minidump minidump(minidump_file);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(exception);
  ASSERT_TRUE(memory_list);
  ASSERT_NE((unsigned int)0, memory_list->region_count());

  MinidumpContext* context = exception->GetContext();
  ASSERT_TRUE(context);

  uint64_t instruction_pointer;
  ASSERT_TRUE(context->GetInstructionPointer(&instruction_pointer));

  MinidumpMemoryRegion* region =
    memory_list->GetMemoryRegionForAddress(instruction_pointer);
  EXPECT_TRUE(region);

  const size_t kPrefixSize = 128;  // bytes
  EXPECT_EQ(kPrefixSize + sizeof(instructions), region->GetSize());
  const uint8_t* bytes = region->GetMemory();
  ASSERT_TRUE(bytes);

  uint8_t prefix_bytes[kPrefixSize];
  memset(prefix_bytes, 0, sizeof(prefix_bytes));
  EXPECT_TRUE(memcmp(bytes, prefix_bytes, sizeof(prefix_bytes)) == 0);
  EXPECT_TRUE(memcmp(bytes + kPrefixSize,
                     instructions, sizeof(instructions)) == 0);
}

// Ensure that an extra memory block doesn't get added when the
// instruction pointer is not in mapped memory.
TEST_F(ExceptionHandlerTest, InstructionPointerMemoryNullPointer) {
  // Give the child process a pipe to report back on.
  int fds[2];
  ASSERT_EQ(0, pipe(fds));

  pid_t pid = fork();
  if (pid == 0) {
    close(fds[0]);
    ExceptionHandler eh(tempDir.path(), NULL, MDCallback, &fds[1], true, NULL);
    // Try calling a NULL pointer.
    typedef void (*void_function)(void);
    void_function memory_function =
      reinterpret_cast<void_function>(NULL);
    memory_function();
    // not reached
    exit(1);
  }
  // In the parent process.
  ASSERT_NE(-1, pid);
  close(fds[1]);

  // Wait for the background process to return the minidump file.
  close(fds[1]);
  char minidump_file[PATH_MAX];
  ssize_t nbytes = read(fds[0], minidump_file, sizeof(minidump_file));
  ASSERT_NE(0, nbytes);
  // Ensure that minidump file exists and is > 0 bytes.
  struct stat st;
  ASSERT_EQ(0, stat(minidump_file, &st));
  ASSERT_LT(0, st.st_size);

  // Child process should have exited with a zero status.
  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_NE(0, WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));

  // Read the minidump. Locate the exception record and the
  // memory list, and then ensure that there is only one memory region
  // in the memory list (the thread memory from the single thread).
  Minidump minidump(minidump_file);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(exception);
  ASSERT_TRUE(memory_list);
  ASSERT_EQ((unsigned int)1, memory_list->region_count());
}

static void *Junk(void *) {
  sleep(1000000);
  return NULL;
}

// Test that the memory list gets written correctly when multiple
// threads are running.
TEST_F(ExceptionHandlerTest, MemoryListMultipleThreads) {
  // Give the child process a pipe to report back on.
  int fds[2];
  ASSERT_EQ(0, pipe(fds));

  pid_t pid = fork();
  if (pid == 0) {
    close(fds[0]);
    ExceptionHandler eh(tempDir.path(), NULL, MDCallback, &fds[1], true, NULL);

    // Run an extra thread so >2 memory regions will be written.
    pthread_t junk_thread;
    if (pthread_create(&junk_thread, NULL, Junk, NULL) == 0)
      pthread_detach(junk_thread);

    // Just crash.
    Crasher();

    // not reached
    exit(1);
  }
  // In the parent process.
  ASSERT_NE(-1, pid);
  close(fds[1]);

  // Wait for the background process to return the minidump file.
  close(fds[1]);
  char minidump_file[PATH_MAX];
  ssize_t nbytes = read(fds[0], minidump_file, sizeof(minidump_file));
  ASSERT_NE(0, nbytes);
  // Ensure that minidump file exists and is > 0 bytes.
  struct stat st;
  ASSERT_EQ(0, stat(minidump_file, &st));
  ASSERT_LT(0, st.st_size);

  // Child process should have exited with a zero status.
  int ret;
  ASSERT_EQ(pid, waitpid(pid, &ret, 0));
  EXPECT_NE(0, WIFEXITED(ret));
  EXPECT_EQ(0, WEXITSTATUS(ret));

  // Read the minidump, and verify that the memory list can be read.
  Minidump minidump(minidump_file);
  ASSERT_TRUE(minidump.Read());

  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(memory_list);
  // Verify that there are three memory regions:
  // one per thread, and one for the instruction pointer memory.
  ASSERT_EQ((unsigned int)3, memory_list->region_count());
}

}
