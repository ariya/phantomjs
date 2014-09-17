// Copyright (c) 2010 Google Inc.
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

#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include <string>

#include "breakpad_googletest_includes.h"
#include "client/linux/handler/exception_handler.h"
#include "client/linux/minidump_writer/minidump_writer.h"
#include "common/linux/eintr_wrapper.h"
#include "common/linux/file_id.h"
#include "common/linux/linux_libc_support.h"
#include "common/tests/auto_tempdir.h"
#include "third_party/lss/linux_syscall_support.h"
#include "google_breakpad/processor/minidump.h"

using namespace google_breakpad;

// Length of a formatted GUID string =
// sizeof(MDGUID) * 2 + 4 (for dashes) + 1 (null terminator)
const int kGUIDStringSize = 37;

static void sigchld_handler(int signo) { }

class ExceptionHandlerTest : public ::testing::Test {
 protected:
  void SetUp() {
    // We need to be able to wait for children, so SIGCHLD cannot be SIG_IGN.
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    ASSERT_NE(sigaction(SIGCHLD, &sa, &old_action), -1);
  }

  void TearDown() {
    sigaction(SIGCHLD, &old_action, NULL);
  }

  struct sigaction old_action;
};

TEST(ExceptionHandlerTest, Simple) {
  AutoTempDir temp_dir;
  ExceptionHandler handler(temp_dir.path(), NULL, NULL, NULL, true);
}

static bool DoneCallback(const char* dump_path,
                         const char* minidump_id,
                         void* context,
                         bool succeeded) {
  if (!succeeded)
    return succeeded;

  int fd = (intptr_t) context;
  uint32_t len = my_strlen(minidump_id);
  HANDLE_EINTR(sys_write(fd, &len, sizeof(len)));
  HANDLE_EINTR(sys_write(fd, minidump_id, len));
  sys_close(fd);

  return true;
}

TEST(ExceptionHandlerTest, ChildCrash) {
  AutoTempDir temp_dir;
  int fds[2];
  ASSERT_NE(pipe(fds), -1);

  const pid_t child = fork();
  if (child == 0) {
    close(fds[0]);
    ExceptionHandler handler(temp_dir.path(), NULL, DoneCallback, (void*) fds[1],
                             true);
    *reinterpret_cast<volatile int*>(NULL) = 0;
  }
  close(fds[1]);

  int status;
  ASSERT_NE(HANDLE_EINTR(waitpid(child, &status, 0)), -1);
  ASSERT_TRUE(WIFSIGNALED(status));
  ASSERT_EQ(WTERMSIG(status), SIGSEGV);

  struct pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = fds[0];
  pfd.events = POLLIN | POLLERR;

  const int r = HANDLE_EINTR(poll(&pfd, 1, 0));
  ASSERT_EQ(r, 1);
  ASSERT_TRUE(pfd.revents & POLLIN);

  uint32_t len;
  ASSERT_EQ(read(fds[0], &len, sizeof(len)), (ssize_t)sizeof(len));
  ASSERT_LT(len, (uint32_t)2048);
  char* filename = reinterpret_cast<char*>(malloc(len + 1));
  ASSERT_EQ(read(fds[0], filename, len), len);
  filename[len] = 0;
  close(fds[0]);

  const std::string minidump_filename = temp_dir.path() + "/" + filename +
                                        ".dmp";

  struct stat st;
  ASSERT_EQ(stat(minidump_filename.c_str(), &st), 0);
  ASSERT_GT(st.st_size, 0u);
  unlink(minidump_filename.c_str());
}

// Test that memory around the instruction pointer is written
// to the dump as a MinidumpMemoryRegion.
TEST(ExceptionHandlerTest, InstructionPointerMemory) {
  AutoTempDir temp_dir;
  int fds[2];
  ASSERT_NE(pipe(fds), -1);

  // These are defined here so the parent can use them to check the
  // data from the minidump afterwards.
  const u_int32_t kMemorySize = 256;  // bytes
  const int kOffset = kMemorySize / 2;
  // This crashes with SIGILL on x86/x86-64/arm.
  const unsigned char instructions[] = { 0xff, 0xff, 0xff, 0xff };

  const pid_t child = fork();
  if (child == 0) {
    close(fds[0]);
    ExceptionHandler handler(temp_dir.path(), NULL, DoneCallback,
                             (void*) fds[1], true);
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
  }
  close(fds[1]);

  int status;
  ASSERT_NE(HANDLE_EINTR(waitpid(child, &status, 0)), -1);
  ASSERT_TRUE(WIFSIGNALED(status));
  ASSERT_EQ(WTERMSIG(status), SIGILL);

  struct pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = fds[0];
  pfd.events = POLLIN | POLLERR;

  const int r = HANDLE_EINTR(poll(&pfd, 1, 0));
  ASSERT_EQ(r, 1);
  ASSERT_TRUE(pfd.revents & POLLIN);

  uint32_t len;
  ASSERT_EQ(read(fds[0], &len, sizeof(len)), (ssize_t)sizeof(len));
  ASSERT_LT(len, (uint32_t)2048);
  char* filename = reinterpret_cast<char*>(malloc(len + 1));
  ASSERT_EQ(read(fds[0], filename, len), len);
  filename[len] = 0;
  close(fds[0]);

  const std::string minidump_filename = temp_dir.path() + "/" + filename +
                                        ".dmp";

  struct stat st;
  ASSERT_EQ(stat(minidump_filename.c_str(), &st), 0);
  ASSERT_GT(st.st_size, 0u);

  // Read the minidump. Locate the exception record and the
  // memory list, and then ensure that there is a memory region
  // in the memory list that covers the instruction pointer from
  // the exception record.
  Minidump minidump(minidump_filename);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(exception);
  ASSERT_TRUE(memory_list);
  ASSERT_LT(0, memory_list->region_count());

  MinidumpContext* context = exception->GetContext();
  ASSERT_TRUE(context);

  u_int64_t instruction_pointer;
  switch (context->GetContextCPU()) {
  case MD_CONTEXT_X86:
    instruction_pointer = context->GetContextX86()->eip;
    break;
  case MD_CONTEXT_AMD64:
    instruction_pointer = context->GetContextAMD64()->rip;
    break;
  case MD_CONTEXT_ARM:
    instruction_pointer = context->GetContextARM()->iregs[15];
    break;
  default:
    FAIL() << "Unknown context CPU: " << context->GetContextCPU();
    break;
  }

  MinidumpMemoryRegion* region =
    memory_list->GetMemoryRegionForAddress(instruction_pointer);
  ASSERT_TRUE(region);

  EXPECT_EQ(kMemorySize, region->GetSize());
  const u_int8_t* bytes = region->GetMemory();
  ASSERT_TRUE(bytes);

  u_int8_t prefix_bytes[kOffset];
  u_int8_t suffix_bytes[kMemorySize - kOffset - sizeof(instructions)];
  memset(prefix_bytes, 0, sizeof(prefix_bytes));
  memset(suffix_bytes, 0, sizeof(suffix_bytes));
  EXPECT_TRUE(memcmp(bytes, prefix_bytes, sizeof(prefix_bytes)) == 0);
  EXPECT_TRUE(memcmp(bytes + kOffset, instructions, sizeof(instructions)) == 0);
  EXPECT_TRUE(memcmp(bytes + kOffset + sizeof(instructions),
                     suffix_bytes, sizeof(suffix_bytes)) == 0);

  unlink(minidump_filename.c_str());
  free(filename);
}

// Test that the memory region around the instruction pointer is
// bounded correctly on the low end.
TEST(ExceptionHandlerTest, InstructionPointerMemoryMinBound) {
  AutoTempDir temp_dir;
  int fds[2];
  ASSERT_NE(pipe(fds), -1);

  // These are defined here so the parent can use them to check the
  // data from the minidump afterwards.
  const u_int32_t kMemorySize = 256;  // bytes
  const int kOffset = 0;
  // This crashes with SIGILL on x86/x86-64/arm.
  const unsigned char instructions[] = { 0xff, 0xff, 0xff, 0xff };

  const pid_t child = fork();
  if (child == 0) {
    close(fds[0]);
    ExceptionHandler handler(temp_dir.path(), NULL, DoneCallback,
                             (void*) fds[1], true);
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
  }
  close(fds[1]);

  int status;
  ASSERT_NE(HANDLE_EINTR(waitpid(child, &status, 0)), -1);
  ASSERT_TRUE(WIFSIGNALED(status));
  ASSERT_EQ(WTERMSIG(status), SIGILL);

  struct pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = fds[0];
  pfd.events = POLLIN | POLLERR;

  const int r = HANDLE_EINTR(poll(&pfd, 1, 0));
  ASSERT_EQ(r, 1);
  ASSERT_TRUE(pfd.revents & POLLIN);

  uint32_t len;
  ASSERT_EQ(read(fds[0], &len, sizeof(len)), (ssize_t)sizeof(len));
  ASSERT_LT(len, (uint32_t)2048);
  char* filename = reinterpret_cast<char*>(malloc(len + 1));
  ASSERT_EQ(read(fds[0], filename, len), len);
  filename[len] = 0;
  close(fds[0]);

  const std::string minidump_filename = temp_dir.path() + "/" + filename +
                                        ".dmp";

  struct stat st;
  ASSERT_EQ(stat(minidump_filename.c_str(), &st), 0);
  ASSERT_GT(st.st_size, 0u);

  // Read the minidump. Locate the exception record and the
  // memory list, and then ensure that there is a memory region
  // in the memory list that covers the instruction pointer from
  // the exception record.
  Minidump minidump(minidump_filename);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(exception);
  ASSERT_TRUE(memory_list);
  ASSERT_LT(0, memory_list->region_count());

  MinidumpContext* context = exception->GetContext();
  ASSERT_TRUE(context);

  u_int64_t instruction_pointer;
  switch (context->GetContextCPU()) {
  case MD_CONTEXT_X86:
    instruction_pointer = context->GetContextX86()->eip;
    break;
  case MD_CONTEXT_AMD64:
    instruction_pointer = context->GetContextAMD64()->rip;
    break;
  case MD_CONTEXT_ARM:
    instruction_pointer = context->GetContextARM()->iregs[15];
    break;
  default:
    FAIL() << "Unknown context CPU: " << context->GetContextCPU();
    break;
  }

  MinidumpMemoryRegion* region =
    memory_list->GetMemoryRegionForAddress(instruction_pointer);
  ASSERT_TRUE(region);

  EXPECT_EQ(kMemorySize / 2, region->GetSize());
  const u_int8_t* bytes = region->GetMemory();
  ASSERT_TRUE(bytes);

  u_int8_t suffix_bytes[kMemorySize / 2 - sizeof(instructions)];
  memset(suffix_bytes, 0, sizeof(suffix_bytes));
  EXPECT_TRUE(memcmp(bytes + kOffset, instructions, sizeof(instructions)) == 0);
  EXPECT_TRUE(memcmp(bytes + kOffset + sizeof(instructions),
                     suffix_bytes, sizeof(suffix_bytes)) == 0);

  unlink(minidump_filename.c_str());
  free(filename);
}

// Test that the memory region around the instruction pointer is
// bounded correctly on the high end.
TEST(ExceptionHandlerTest, InstructionPointerMemoryMaxBound) {
  AutoTempDir temp_dir;
  int fds[2];
  ASSERT_NE(pipe(fds), -1);

  // These are defined here so the parent can use them to check the
  // data from the minidump afterwards.
  // Use 4k here because the OS will hand out a single page even
  // if a smaller size is requested, and this test wants to
  // test the upper bound of the memory range.
  const u_int32_t kMemorySize = 4096;  // bytes
  // This crashes with SIGILL on x86/x86-64/arm.
  const unsigned char instructions[] = { 0xff, 0xff, 0xff, 0xff };
  const int kOffset = kMemorySize - sizeof(instructions);

  const pid_t child = fork();
  if (child == 0) {
    close(fds[0]);
    ExceptionHandler handler(temp_dir.path(), NULL, DoneCallback,
                             (void*) fds[1], true);
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
  }
  close(fds[1]);

  int status;
  ASSERT_NE(HANDLE_EINTR(waitpid(child, &status, 0)), -1);
  ASSERT_TRUE(WIFSIGNALED(status));
  ASSERT_EQ(WTERMSIG(status), SIGILL);

  struct pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = fds[0];
  pfd.events = POLLIN | POLLERR;

  const int r = HANDLE_EINTR(poll(&pfd, 1, 0));
  ASSERT_EQ(r, 1);
  ASSERT_TRUE(pfd.revents & POLLIN);

  uint32_t len;
  ASSERT_EQ(read(fds[0], &len, sizeof(len)), (ssize_t)sizeof(len));
  ASSERT_LT(len, (uint32_t)2048);
  char* filename = reinterpret_cast<char*>(malloc(len + 1));
  ASSERT_EQ(read(fds[0], filename, len), len);
  filename[len] = 0;
  close(fds[0]);

  const std::string minidump_filename = temp_dir.path() + "/" + filename +
                                        ".dmp";

  struct stat st;
  ASSERT_EQ(stat(minidump_filename.c_str(), &st), 0);
  ASSERT_GT(st.st_size, 0u);

  // Read the minidump. Locate the exception record and the
  // memory list, and then ensure that there is a memory region
  // in the memory list that covers the instruction pointer from
  // the exception record.
  Minidump minidump(minidump_filename);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(exception);
  ASSERT_TRUE(memory_list);
  ASSERT_LT(0, memory_list->region_count());

  MinidumpContext* context = exception->GetContext();
  ASSERT_TRUE(context);

  u_int64_t instruction_pointer;
  switch (context->GetContextCPU()) {
  case MD_CONTEXT_X86:
    instruction_pointer = context->GetContextX86()->eip;
    break;
  case MD_CONTEXT_AMD64:
    instruction_pointer = context->GetContextAMD64()->rip;
    break;
  case MD_CONTEXT_ARM:
    instruction_pointer = context->GetContextARM()->iregs[15];
    break;
  default:
    FAIL() << "Unknown context CPU: " << context->GetContextCPU();
    break;
  }

  MinidumpMemoryRegion* region =
    memory_list->GetMemoryRegionForAddress(instruction_pointer);
  ASSERT_TRUE(region);

  const size_t kPrefixSize = 128;  // bytes
  EXPECT_EQ(kPrefixSize + sizeof(instructions), region->GetSize());
  const u_int8_t* bytes = region->GetMemory();
  ASSERT_TRUE(bytes);

  u_int8_t prefix_bytes[kPrefixSize];
  memset(prefix_bytes, 0, sizeof(prefix_bytes));
  EXPECT_TRUE(memcmp(bytes, prefix_bytes, sizeof(prefix_bytes)) == 0);
  EXPECT_TRUE(memcmp(bytes + kPrefixSize,
                     instructions, sizeof(instructions)) == 0);

  unlink(minidump_filename.c_str());
  free(filename);
}

// Ensure that an extra memory block doesn't get added when the
// instruction pointer is not in mapped memory.
TEST(ExceptionHandlerTest, InstructionPointerMemoryNullPointer) {
  AutoTempDir temp_dir;
  int fds[2];
  ASSERT_NE(pipe(fds), -1);


  const pid_t child = fork();
  if (child == 0) {
    close(fds[0]);
    ExceptionHandler handler(temp_dir.path(), NULL, DoneCallback,
                             (void*) fds[1], true);
    // Try calling a NULL pointer.
    typedef void (*void_function)(void);
    void_function memory_function =
      reinterpret_cast<void_function>(NULL);
    memory_function();
  }
  close(fds[1]);

  int status;
  ASSERT_NE(HANDLE_EINTR(waitpid(child, &status, 0)), -1);
  ASSERT_TRUE(WIFSIGNALED(status));
  ASSERT_EQ(WTERMSIG(status), SIGSEGV);

  struct pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = fds[0];
  pfd.events = POLLIN | POLLERR;

  const int r = HANDLE_EINTR(poll(&pfd, 1, 0));
  ASSERT_EQ(r, 1);
  ASSERT_TRUE(pfd.revents & POLLIN);

  uint32_t len;
  ASSERT_EQ(read(fds[0], &len, sizeof(len)), (ssize_t)sizeof(len));
  ASSERT_LT(len, (uint32_t)2048);
  char* filename = reinterpret_cast<char*>(malloc(len + 1));
  ASSERT_EQ(read(fds[0], filename, len), len);
  filename[len] = 0;
  close(fds[0]);

  const std::string minidump_filename = temp_dir.path() + "/" + filename +
                                        ".dmp";

  struct stat st;
  ASSERT_EQ(stat(minidump_filename.c_str(), &st), 0);
  ASSERT_GT(st.st_size, 0u);

  // Read the minidump. Locate the exception record and the
  // memory list, and then ensure that there is a memory region
  // in the memory list that covers the instruction pointer from
  // the exception record.
  Minidump minidump(minidump_filename);
  ASSERT_TRUE(minidump.Read());

  MinidumpException* exception = minidump.GetException();
  MinidumpMemoryList* memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(exception);
  ASSERT_TRUE(memory_list);
  ASSERT_EQ((unsigned int)1, memory_list->region_count());

  unlink(minidump_filename.c_str());
  free(filename);
}

static bool SimpleCallback(const char* dump_path,
                           const char* minidump_id,
                           void* context,
                           bool succeeded) {
  if (!succeeded)
    return succeeded;

  string* minidump_file = reinterpret_cast<string*>(context);
  minidump_file->append(dump_path);
  minidump_file->append("/");
  minidump_file->append(minidump_id);
  minidump_file->append(".dmp");
  return true;
}

// Test that anonymous memory maps can be annotated with names and IDs.
TEST(ExceptionHandlerTest, ModuleInfo) {
  // These are defined here so the parent can use them to check the
  // data from the minidump afterwards.
  const u_int32_t kMemorySize = sysconf(_SC_PAGESIZE);
  const char* kMemoryName = "a fake module";
  const u_int8_t kModuleGUID[sizeof(MDGUID)] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
  };
  char module_identifier_buffer[kGUIDStringSize];
  FileID::ConvertIdentifierToString(kModuleGUID,
                                    module_identifier_buffer,
                                    sizeof(module_identifier_buffer));
  string module_identifier(module_identifier_buffer);
  // Strip out dashes
  size_t pos;
  while ((pos = module_identifier.find('-')) != string::npos) {
    module_identifier.erase(pos, 1);
  }
  // And append a zero, because module IDs include an "age" field
  // which is always zero on Linux.
  module_identifier += "0";

  // Get some memory.
  char* memory =
    reinterpret_cast<char*>(mmap(NULL,
                                 kMemorySize,
                                 PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANON,
                                 -1,
                                 0));
  const uintptr_t kMemoryAddress = reinterpret_cast<uintptr_t>(memory);
  ASSERT_TRUE(memory);

  string minidump_filename;
  AutoTempDir temp_dir;
  ExceptionHandler handler(temp_dir.path(), NULL, SimpleCallback,
                           (void*)&minidump_filename, true);
  // Add info about the anonymous memory mapping.
  handler.AddMappingInfo(kMemoryName,
                         kModuleGUID,
                         kMemoryAddress,
                         kMemorySize,
                         0);
  handler.WriteMinidump();

  // Read the minidump. Load the module list, and ensure that
  // the mmap'ed |memory| is listed with the given module name
  // and debug ID.
  Minidump minidump(minidump_filename);
  ASSERT_TRUE(minidump.Read());

  MinidumpModuleList* module_list = minidump.GetModuleList();
  ASSERT_TRUE(module_list);
  const MinidumpModule* module =
    module_list->GetModuleForAddress(kMemoryAddress);
  ASSERT_TRUE(module);

  EXPECT_EQ(kMemoryAddress, module->base_address());
  EXPECT_EQ(kMemorySize, module->size());
  EXPECT_EQ(kMemoryName, module->code_file());
  EXPECT_EQ(module_identifier, module->debug_identifier());

  unlink(minidump_filename.c_str());
}

static const unsigned kControlMsgSize =
    CMSG_SPACE(sizeof(int)) + CMSG_SPACE(sizeof(struct ucred));

static bool
CrashHandler(const void* crash_context, size_t crash_context_size,
             void* context) {
  const int fd = (intptr_t) context;
  int fds[2];
  if (pipe(fds) == -1) {
    // There doesn't seem to be any way to reliably handle
    // this failure without the parent process hanging
    // At least make sure that this process doesn't access
    // unexpected file descriptors
    fds[0] = -1;
    fds[1] = -1;
  }
  struct kernel_msghdr msg = {0};
  struct kernel_iovec iov;
  iov.iov_base = const_cast<void*>(crash_context);
  iov.iov_len = crash_context_size;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  char cmsg[kControlMsgSize];
  memset(cmsg, 0, kControlMsgSize);
  msg.msg_control = cmsg;
  msg.msg_controllen = sizeof(cmsg);

  struct cmsghdr *hdr = CMSG_FIRSTHDR(&msg);
  hdr->cmsg_level = SOL_SOCKET;
  hdr->cmsg_type = SCM_RIGHTS;
  hdr->cmsg_len = CMSG_LEN(sizeof(int));
  *((int*) CMSG_DATA(hdr)) = fds[1];
  hdr = CMSG_NXTHDR((struct msghdr*) &msg, hdr);
  hdr->cmsg_level = SOL_SOCKET;
  hdr->cmsg_type = SCM_CREDENTIALS;
  hdr->cmsg_len = CMSG_LEN(sizeof(struct ucred));
  struct ucred *cred = reinterpret_cast<struct ucred*>(CMSG_DATA(hdr));
  cred->uid = getuid();
  cred->gid = getgid();
  cred->pid = getpid();

  HANDLE_EINTR(sys_sendmsg(fd, &msg, 0));
  sys_close(fds[1]);

  char b;
  HANDLE_EINTR(sys_read(fds[0], &b, 1));

  return true;
}

TEST(ExceptionHandlerTest, ExternalDumper) {
  int fds[2];
  ASSERT_NE(socketpair(AF_UNIX, SOCK_DGRAM, 0, fds), -1);
  static const int on = 1;
  setsockopt(fds[0], SOL_SOCKET, SO_PASSCRED, &on, sizeof(on));
  setsockopt(fds[1], SOL_SOCKET, SO_PASSCRED, &on, sizeof(on));

  const pid_t child = fork();
  if (child == 0) {
    close(fds[0]);
    ExceptionHandler handler("/tmp1", NULL, NULL, (void*) fds[1], true);
    handler.set_crash_handler(CrashHandler);
    *reinterpret_cast<volatile int*>(NULL) = 0;
  }
  close(fds[1]);
  struct msghdr msg = {0};
  struct iovec iov;
  static const unsigned kCrashContextSize =
      sizeof(ExceptionHandler::CrashContext);
  char context[kCrashContextSize];
  char control[kControlMsgSize];
  iov.iov_base = context;
  iov.iov_len = kCrashContextSize;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = control;
  msg.msg_controllen = kControlMsgSize;

  const ssize_t n = HANDLE_EINTR(recvmsg(fds[0], &msg, 0));
  ASSERT_EQ(n, kCrashContextSize);
  ASSERT_EQ(msg.msg_controllen, kControlMsgSize);
  ASSERT_EQ(msg.msg_flags, 0);
  ASSERT_EQ(close(fds[0]), 0);

  pid_t crashing_pid = -1;
  int signal_fd = -1;
  for (struct cmsghdr *hdr = CMSG_FIRSTHDR(&msg); hdr;
       hdr = CMSG_NXTHDR(&msg, hdr)) {
    if (hdr->cmsg_level != SOL_SOCKET)
      continue;
    if (hdr->cmsg_type == SCM_RIGHTS) {
      const unsigned len = hdr->cmsg_len -
          (((uint8_t*)CMSG_DATA(hdr)) - (uint8_t*)hdr);
      ASSERT_EQ(len, sizeof(int));
      signal_fd = *((int *) CMSG_DATA(hdr));
    } else if (hdr->cmsg_type == SCM_CREDENTIALS) {
      const struct ucred *cred =
          reinterpret_cast<struct ucred*>(CMSG_DATA(hdr));
      crashing_pid = cred->pid;
    }
  }

  ASSERT_NE(crashing_pid, -1);
  ASSERT_NE(signal_fd, -1);

  AutoTempDir temp_dir;
  std::string templ = temp_dir.path() + "/exception-handler-unittest";
  ASSERT_TRUE(WriteMinidump(templ.c_str(), crashing_pid, context,
                            kCrashContextSize));
  static const char b = 0;
  HANDLE_EINTR(write(signal_fd, &b, 1));
  ASSERT_EQ(close(signal_fd), 0);

  int status;
  ASSERT_NE(HANDLE_EINTR(waitpid(child, &status, 0)), -1);
  ASSERT_TRUE(WIFSIGNALED(status));
  ASSERT_EQ(WTERMSIG(status), SIGSEGV);

  struct stat st;
  ASSERT_EQ(stat(templ.c_str(), &st), 0);
  ASSERT_GT(st.st_size, 0u);
  unlink(templ.c_str());
}
