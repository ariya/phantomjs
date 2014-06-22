// Copyright 2009, Google Inc.
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

#include <windows.h>
#include <dbghelp.h>
#include <strsafe.h>
#include <objbase.h>
#include <shellapi.h>

#include <string>

#include "breakpad_googletest_includes.h"
#include "client/windows/crash_generation/crash_generation_server.h"
#include "client/windows/handler/exception_handler.h"
#include "client/windows/unittests/exception_handler_test.h"
#include "common/windows/string_utils-inl.h"
#include "google_breakpad/processor/minidump.h"

namespace {

using std::wstring;
using namespace google_breakpad;

const wchar_t kPipeName[] = L"\\\\.\\pipe\\BreakpadCrashTest\\TestCaseServer";
const char kSuccessIndicator[] = "success";
const char kFailureIndicator[] = "failure";

// Utility function to test for a path's existence.
BOOL DoesPathExist(const TCHAR *path_name);

enum OutOfProcGuarantee {
  OUT_OF_PROC_GUARANTEED,
  OUT_OF_PROC_BEST_EFFORT,
};

class ExceptionHandlerDeathTest : public ::testing::Test {
 protected:
  // Member variable for each test that they can use
  // for temporary storage.
  TCHAR temp_path_[MAX_PATH];
  // Actually constructs a temp path name.
  virtual void SetUp();
  // A helper method that tests can use to crash.
  void DoCrashAccessViolation(const OutOfProcGuarantee out_of_proc_guarantee);
  void DoCrashPureVirtualCall();
};

void ExceptionHandlerDeathTest::SetUp() {
  const ::testing::TestInfo* const test_info =
    ::testing::UnitTest::GetInstance()->current_test_info();
  TCHAR temp_path[MAX_PATH] = { '\0' };
  TCHAR test_name_wide[MAX_PATH] = { '\0' };
  // We want the temporary directory to be what the OS returns
  // to us, + the test case name.
  GetTempPath(MAX_PATH, temp_path);
  // The test case name is exposed as a c-style string,
  // convert it to a wchar_t string.
  int dwRet = MultiByteToWideChar(CP_ACP, 0, test_info->name(),
                                  strlen(test_info->name()),
                                  test_name_wide,
                                  MAX_PATH);
  if (!dwRet) {
    assert(false);
  }
  StringCchPrintfW(temp_path_, MAX_PATH, L"%s%s", temp_path, test_name_wide);
  CreateDirectory(temp_path_, NULL);
}

BOOL DoesPathExist(const TCHAR *path_name) {
  DWORD flags = GetFileAttributes(path_name);
  if (flags == INVALID_FILE_ATTRIBUTES) {
    return FALSE;
  }
  return TRUE;
}

bool MinidumpWrittenCallback(const wchar_t* dump_path,
                             const wchar_t* minidump_id,
                             void* context,
                             EXCEPTION_POINTERS* exinfo,
                             MDRawAssertionInfo* assertion,
                             bool succeeded) {
  if (succeeded && DoesPathExist(dump_path)) {
    fprintf(stderr, kSuccessIndicator);
  } else {
    fprintf(stderr, kFailureIndicator);
  }
  // If we don't flush, the output doesn't get sent before
  // this process dies.
  fflush(stderr);
  return succeeded;
}

TEST_F(ExceptionHandlerDeathTest, InProcTest) {
  // For the in-proc test, we just need to instantiate an exception
  // handler in in-proc mode, and crash.   Since the entire test is
  // reexecuted in the child process, we don't have to worry about
  // the semantics of the exception handler being inherited/not
  // inherited across CreateProcess().
  ASSERT_TRUE(DoesPathExist(temp_path_));
  scoped_ptr<google_breakpad::ExceptionHandler> exc(
      new google_breakpad::ExceptionHandler(
          temp_path_,
          NULL,
          &MinidumpWrittenCallback,
          NULL,
          google_breakpad::ExceptionHandler::HANDLER_ALL));

  // Disable GTest SEH handler
  testing::DisableExceptionHandlerInScope disable_exception_handler;

  int *i = NULL;
  ASSERT_DEATH((*i)++, kSuccessIndicator);
}

static bool gDumpCallbackCalled = false;

void clientDumpCallback(void *dump_context,
                        const google_breakpad::ClientInfo *client_info,
                        const std::wstring *dump_path) {
  gDumpCallbackCalled = true;
}

void ExceptionHandlerDeathTest::DoCrashAccessViolation(
    const OutOfProcGuarantee out_of_proc_guarantee) {
  scoped_ptr<google_breakpad::ExceptionHandler> exc;

  if (out_of_proc_guarantee == OUT_OF_PROC_GUARANTEED) {
    google_breakpad::CrashGenerationClient *client =
        new google_breakpad::CrashGenerationClient(kPipeName,
                                                   MiniDumpNormal,
                                                   NULL);  // custom_info
    ASSERT_TRUE(client->Register());
    exc.reset(new google_breakpad::ExceptionHandler(
        temp_path_,
        NULL,   // filter
        NULL,   // callback
        NULL,   // callback_context
        google_breakpad::ExceptionHandler::HANDLER_ALL,
        client));
  } else {
    ASSERT_TRUE(out_of_proc_guarantee == OUT_OF_PROC_BEST_EFFORT);
    exc.reset(new google_breakpad::ExceptionHandler(
        temp_path_,
        NULL,   // filter
        NULL,   // callback
        NULL,   // callback_context
        google_breakpad::ExceptionHandler::HANDLER_ALL,
        MiniDumpNormal,
        kPipeName,
        NULL));  // custom_info
  }

  // Disable GTest SEH handler
  testing::DisableExceptionHandlerInScope disable_exception_handler;

  // Although this is executing in the child process of the death test,
  // if it's not true we'll still get an error rather than the crash
  // being expected.
  ASSERT_TRUE(exc->IsOutOfProcess());
  int *i = NULL;
  printf("%d\n", (*i)++);
}

TEST_F(ExceptionHandlerDeathTest, OutOfProcTest) {
  // We can take advantage of a detail of google test here to save some
  // complexity in testing: when you do a death test, it actually forks.
  // So we can make the main test harness the crash generation server,
  // and call ASSERT_DEATH on a NULL dereference, it to expecting test
  // the out of process scenario, since it's happening in a different
  // process!  This is different from the above because, above, we pass
  // a NULL pipe name, and we also don't start a crash generation server.

  ASSERT_TRUE(DoesPathExist(temp_path_));
  std::wstring dump_path(temp_path_);
  google_breakpad::CrashGenerationServer server(
      kPipeName, NULL, NULL, NULL, &clientDumpCallback, NULL, NULL, NULL, NULL,
      NULL, true, &dump_path);

  // This HAS to be EXPECT_, because when this test case is executed in the
  // child process, the server registration will fail due to the named pipe
  // being the same.
  EXPECT_TRUE(server.Start());
  gDumpCallbackCalled = false;
  ASSERT_DEATH(this->DoCrashAccessViolation(OUT_OF_PROC_BEST_EFFORT), "");
  EXPECT_TRUE(gDumpCallbackCalled);
}

TEST_F(ExceptionHandlerDeathTest, OutOfProcGuaranteedTest) {
  // This is similar to the previous test (OutOfProcTest).  The only difference
  // is that in this test, the crash generation client is created and registered
  // with the crash generation server outside of the ExceptionHandler
  // constructor which allows breakpad users to opt out of the default
  // in-process dump generation when the registration with the crash generation
  // server fails.

  ASSERT_TRUE(DoesPathExist(temp_path_));
  std::wstring dump_path(temp_path_);
  google_breakpad::CrashGenerationServer server(
      kPipeName, NULL, NULL, NULL, &clientDumpCallback, NULL, NULL, NULL, NULL,
      NULL, true, &dump_path);

  // This HAS to be EXPECT_, because when this test case is executed in the
  // child process, the server registration will fail due to the named pipe
  // being the same.
  EXPECT_TRUE(server.Start());
  gDumpCallbackCalled = false;
  ASSERT_DEATH(this->DoCrashAccessViolation(OUT_OF_PROC_GUARANTEED), "");
  EXPECT_TRUE(gDumpCallbackCalled);
}

TEST_F(ExceptionHandlerDeathTest, InvalidParameterTest) {
  using google_breakpad::ExceptionHandler;

  ASSERT_TRUE(DoesPathExist(temp_path_));
  ExceptionHandler handler(temp_path_, NULL, NULL, NULL,
                           ExceptionHandler::HANDLER_INVALID_PARAMETER);

  // Disable the message box for assertions
  _CrtSetReportMode(_CRT_ASSERT, 0);

  // Call with a bad argument. The invalid parameter will be swallowed
  // and a dump will be generated, the process will exit(0).
  ASSERT_EXIT(printf(NULL), ::testing::ExitedWithCode(0), "");
}


struct PureVirtualCallBase {
  PureVirtualCallBase() {
    // We have to reinterpret so the linker doesn't get confused because the
    // method isn't defined.
    reinterpret_cast<PureVirtualCallBase*>(this)->PureFunction();
  }
  virtual ~PureVirtualCallBase() {}
  virtual void PureFunction() const = 0;
};
struct PureVirtualCall : public PureVirtualCallBase {
  PureVirtualCall() { PureFunction(); }
  virtual void PureFunction() const {}
};

void ExceptionHandlerDeathTest::DoCrashPureVirtualCall() {
  PureVirtualCall instance;
}

TEST_F(ExceptionHandlerDeathTest, PureVirtualCallTest) {
  using google_breakpad::ExceptionHandler;

  ASSERT_TRUE(DoesPathExist(temp_path_));
  ExceptionHandler handler(temp_path_, NULL, NULL, NULL,
                           ExceptionHandler::HANDLER_PURECALL);

  // Disable the message box for assertions
  _CrtSetReportMode(_CRT_ASSERT, 0);

  // Calls a pure virtual function.
  EXPECT_EXIT(DoCrashPureVirtualCall(), ::testing::ExitedWithCode(0), "");
}

wstring find_minidump_in_directory(const wstring &directory) {
  wstring search_path = directory + L"\\*";
  WIN32_FIND_DATA find_data;
  HANDLE find_handle = FindFirstFileW(search_path.c_str(), &find_data);
  if (find_handle == INVALID_HANDLE_VALUE)
    return wstring();

  wstring filename;
  do {
    const wchar_t extension[] = L".dmp";
    const int extension_length = sizeof(extension) / sizeof(extension[0]) - 1;
    const int filename_length = wcslen(find_data.cFileName);
    if (filename_length > extension_length &&
    wcsncmp(extension,
            find_data.cFileName + filename_length - extension_length,
            extension_length) == 0) {
      filename = directory + L"\\" + find_data.cFileName;
      break;
    }
  } while (FindNextFile(find_handle, &find_data));
  FindClose(find_handle);
  return filename;
}

#ifndef ADDRESS_SANITIZER

TEST_F(ExceptionHandlerDeathTest, InstructionPointerMemory) {
  ASSERT_TRUE(DoesPathExist(temp_path_));
  scoped_ptr<google_breakpad::ExceptionHandler> exc(
      new google_breakpad::ExceptionHandler(
          temp_path_,
          NULL,
          NULL,
          NULL,
          google_breakpad::ExceptionHandler::HANDLER_ALL));

  // Disable GTest SEH handler
  testing::DisableExceptionHandlerInScope disable_exception_handler;

  // Get some executable memory.
  const uint32_t kMemorySize = 256;  // bytes
  const int kOffset = kMemorySize / 2;
  // This crashes with SIGILL on x86/x86-64/arm.
  const unsigned char instructions[] = { 0xff, 0xff, 0xff, 0xff };
  char* memory = reinterpret_cast<char*>(VirtualAlloc(NULL,
                                                      kMemorySize,
                                                      MEM_COMMIT | MEM_RESERVE,
                                                      PAGE_EXECUTE_READWRITE));
  ASSERT_TRUE(memory);

  // Write some instructions that will crash. Put them
  // in the middle of the block of memory, because the
  // minidump should contain 128 bytes on either side of the
  // instruction pointer.
  memcpy(memory + kOffset, instructions, sizeof(instructions));

  // Now execute the instructions, which should crash.
  typedef void (*void_function)(void);
  void_function memory_function =
      reinterpret_cast<void_function>(memory + kOffset);
  ASSERT_DEATH(memory_function(), "");

  // free the memory.
  VirtualFree(memory, 0, MEM_RELEASE);

  // Verify that the resulting minidump contains the memory around the IP
  wstring minidump_filename_wide = find_minidump_in_directory(temp_path_);
  ASSERT_FALSE(minidump_filename_wide.empty());
  string minidump_filename;
  ASSERT_TRUE(WindowsStringUtils::safe_wcstombs(minidump_filename_wide,
                                                &minidump_filename));

  // Read the minidump. Locate the exception record and the
  // memory list, and then ensure that there is a memory region
  // in the memory list that covers the instruction pointer from
  // the exception record.
  {
    Minidump minidump(minidump_filename);
    ASSERT_TRUE(minidump.Read());

    MinidumpException* exception = minidump.GetException();
    MinidumpMemoryList* memory_list = minidump.GetMemoryList();
    ASSERT_TRUE(exception);
    ASSERT_TRUE(memory_list);
    ASSERT_LT((unsigned)0, memory_list->region_count());

    MinidumpContext* context = exception->GetContext();
    ASSERT_TRUE(context);

    uint64_t instruction_pointer;
    ASSERT_TRUE(context->GetInstructionPointer(&instruction_pointer));

    MinidumpMemoryRegion* region =
        memory_list->GetMemoryRegionForAddress(instruction_pointer);
    ASSERT_TRUE(region);

    EXPECT_EQ(kMemorySize, region->GetSize());
    const uint8_t* bytes = region->GetMemory();
    ASSERT_TRUE(bytes);

    uint8_t prefix_bytes[kOffset];
    uint8_t suffix_bytes[kMemorySize - kOffset - sizeof(instructions)];
    memset(prefix_bytes, 0, sizeof(prefix_bytes));
    memset(suffix_bytes, 0, sizeof(suffix_bytes));
    EXPECT_EQ(0, memcmp(bytes, prefix_bytes, sizeof(prefix_bytes)));
    EXPECT_EQ(0, memcmp(bytes + kOffset, instructions, sizeof(instructions)));
    EXPECT_EQ(0, memcmp(bytes + kOffset + sizeof(instructions),
                        suffix_bytes, sizeof(suffix_bytes)));
  }

  DeleteFileW(minidump_filename_wide.c_str());
}

TEST_F(ExceptionHandlerDeathTest, InstructionPointerMemoryMinBound) {
  ASSERT_TRUE(DoesPathExist(temp_path_));
  scoped_ptr<google_breakpad::ExceptionHandler> exc(
      new google_breakpad::ExceptionHandler(
          temp_path_,
          NULL,
          NULL,
          NULL,
          google_breakpad::ExceptionHandler::HANDLER_ALL));

  // Disable GTest SEH handler
  testing::DisableExceptionHandlerInScope disable_exception_handler;

  SYSTEM_INFO sSysInfo;         // Useful information about the system
  GetSystemInfo(&sSysInfo);     // Initialize the structure.

  const uint32_t kMemorySize = 256;  // bytes
  const DWORD kPageSize = sSysInfo.dwPageSize;
  const int kOffset = 0;
  // This crashes with SIGILL on x86/x86-64/arm.
  const unsigned char instructions[] = { 0xff, 0xff, 0xff, 0xff };
  // Get some executable memory. Specifically, reserve two pages,
  // but only commit the second.
  char* all_memory = reinterpret_cast<char*>(VirtualAlloc(NULL,
                                                          kPageSize * 2,
                                                          MEM_RESERVE,
                                                          PAGE_NOACCESS));
  ASSERT_TRUE(all_memory);
  char* memory = all_memory + kPageSize;
  ASSERT_TRUE(VirtualAlloc(memory, kPageSize,
                           MEM_COMMIT, PAGE_EXECUTE_READWRITE));

  // Write some instructions that will crash. Put them
  // in the middle of the block of memory, because the
  // minidump should contain 128 bytes on either side of the
  // instruction pointer.
  memcpy(memory + kOffset, instructions, sizeof(instructions));

  // Now execute the instructions, which should crash.
  typedef void (*void_function)(void);
  void_function memory_function =
      reinterpret_cast<void_function>(memory + kOffset);
  ASSERT_DEATH(memory_function(), "");

  // free the memory.
  VirtualFree(memory, 0, MEM_RELEASE);

  // Verify that the resulting minidump contains the memory around the IP
  wstring minidump_filename_wide = find_minidump_in_directory(temp_path_);
  ASSERT_FALSE(minidump_filename_wide.empty());
  string minidump_filename;
  ASSERT_TRUE(WindowsStringUtils::safe_wcstombs(minidump_filename_wide,
                                                &minidump_filename));

  // Read the minidump. Locate the exception record and the
  // memory list, and then ensure that there is a memory region
  // in the memory list that covers the instruction pointer from
  // the exception record.
  {
    Minidump minidump(minidump_filename);
    ASSERT_TRUE(minidump.Read());

    MinidumpException* exception = minidump.GetException();
    MinidumpMemoryList* memory_list = minidump.GetMemoryList();
    ASSERT_TRUE(exception);
    ASSERT_TRUE(memory_list);
    ASSERT_LT((unsigned)0, memory_list->region_count());

    MinidumpContext* context = exception->GetContext();
    ASSERT_TRUE(context);

    uint64_t instruction_pointer;
    ASSERT_TRUE(context->GetInstructionPointer(&instruction_pointer));

    MinidumpMemoryRegion* region =
        memory_list->GetMemoryRegionForAddress(instruction_pointer);
    ASSERT_TRUE(region);

    EXPECT_EQ(kMemorySize / 2, region->GetSize());
    const uint8_t* bytes = region->GetMemory();
    ASSERT_TRUE(bytes);

    uint8_t suffix_bytes[kMemorySize / 2 - sizeof(instructions)];
    memset(suffix_bytes, 0, sizeof(suffix_bytes));
    EXPECT_TRUE(memcmp(bytes + kOffset,
                       instructions, sizeof(instructions)) == 0);
    EXPECT_TRUE(memcmp(bytes + kOffset + sizeof(instructions),
                       suffix_bytes, sizeof(suffix_bytes)) == 0);
  }

  DeleteFileW(minidump_filename_wide.c_str());
}

TEST_F(ExceptionHandlerDeathTest, InstructionPointerMemoryMaxBound) {
  ASSERT_TRUE(DoesPathExist(temp_path_));
  scoped_ptr<google_breakpad::ExceptionHandler> exc(
      new google_breakpad::ExceptionHandler(
          temp_path_,
          NULL,
          NULL,
          NULL,
          google_breakpad::ExceptionHandler::HANDLER_ALL));

  // Disable GTest SEH handler
  testing::DisableExceptionHandlerInScope disable_exception_handler;

  SYSTEM_INFO sSysInfo;         // Useful information about the system
  GetSystemInfo(&sSysInfo);     // Initialize the structure.

  const DWORD kPageSize = sSysInfo.dwPageSize;
  // This crashes with SIGILL on x86/x86-64/arm.
  const unsigned char instructions[] = { 0xff, 0xff, 0xff, 0xff };
  const int kOffset = kPageSize - sizeof(instructions);
  // Get some executable memory. Specifically, reserve two pages,
  // but only commit the first.
  char* memory = reinterpret_cast<char*>(VirtualAlloc(NULL,
                                                      kPageSize * 2,
                                                      MEM_RESERVE,
                                                      PAGE_NOACCESS));
  ASSERT_TRUE(memory);
  ASSERT_TRUE(VirtualAlloc(memory, kPageSize,
                           MEM_COMMIT, PAGE_EXECUTE_READWRITE));

  // Write some instructions that will crash.
  memcpy(memory + kOffset, instructions, sizeof(instructions));

  // Now execute the instructions, which should crash.
  typedef void (*void_function)(void);
  void_function memory_function =
      reinterpret_cast<void_function>(memory + kOffset);
  ASSERT_DEATH(memory_function(), "");

  // free the memory.
  VirtualFree(memory, 0, MEM_RELEASE);

  // Verify that the resulting minidump contains the memory around the IP
  wstring minidump_filename_wide = find_minidump_in_directory(temp_path_);
  ASSERT_FALSE(minidump_filename_wide.empty());
  string minidump_filename;
  ASSERT_TRUE(WindowsStringUtils::safe_wcstombs(minidump_filename_wide,
                                                &minidump_filename));

  // Read the minidump. Locate the exception record and the
  // memory list, and then ensure that there is a memory region
  // in the memory list that covers the instruction pointer from
  // the exception record.
  {
    Minidump minidump(minidump_filename);
    ASSERT_TRUE(minidump.Read());

    MinidumpException* exception = minidump.GetException();
    MinidumpMemoryList* memory_list = minidump.GetMemoryList();
    ASSERT_TRUE(exception);
    ASSERT_TRUE(memory_list);
    ASSERT_LT((unsigned)0, memory_list->region_count());

    MinidumpContext* context = exception->GetContext();
    ASSERT_TRUE(context);

    uint64_t instruction_pointer;
    ASSERT_TRUE(context->GetInstructionPointer(&instruction_pointer));

    MinidumpMemoryRegion* region =
        memory_list->GetMemoryRegionForAddress(instruction_pointer);
    ASSERT_TRUE(region);

    const size_t kPrefixSize = 128;  // bytes
    EXPECT_EQ(kPrefixSize + sizeof(instructions), region->GetSize());
    const uint8_t* bytes = region->GetMemory();
    ASSERT_TRUE(bytes);

    uint8_t prefix_bytes[kPrefixSize];
    memset(prefix_bytes, 0, sizeof(prefix_bytes));
    EXPECT_EQ(0, memcmp(bytes, prefix_bytes, sizeof(prefix_bytes)));
    EXPECT_EQ(0, memcmp(bytes + kPrefixSize,
                        instructions, sizeof(instructions)));
  }

  DeleteFileW(minidump_filename_wide.c_str());
}

#endif  // !ADDRESS_SANITIZER

}  // namespace
