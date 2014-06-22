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

#include "client/windows/unittests/exception_handler_test.h"

#include <windows.h>
#include <dbghelp.h>
#include <strsafe.h>
#include <objbase.h>
#include <shellapi.h>

#include <string>

#include "breakpad_googletest_includes.h"
#include "client/windows/crash_generation/crash_generation_server.h"
#include "client/windows/handler/exception_handler.h"
#include "client/windows/unittests/dump_analysis.h"  // NOLINT
#include "common/windows/string_utils-inl.h"
#include "google_breakpad/processor/minidump.h"

namespace testing {

DisableExceptionHandlerInScope::DisableExceptionHandlerInScope() {
  catch_exceptions_ = GTEST_FLAG(catch_exceptions);
  GTEST_FLAG(catch_exceptions) = false;
}

DisableExceptionHandlerInScope::~DisableExceptionHandlerInScope() {
  GTEST_FLAG(catch_exceptions) = catch_exceptions_;
}

}  // namespace testing

namespace {

using std::wstring;
using namespace google_breakpad;

const wchar_t kPipeName[] = L"\\\\.\\pipe\\BreakpadCrashTest\\TestCaseServer";
const char kSuccessIndicator[] = "success";
const char kFailureIndicator[] = "failure";

const MINIDUMP_TYPE kFullDumpType = static_cast<MINIDUMP_TYPE>(
    MiniDumpWithFullMemory |  // Full memory from process.
    MiniDumpWithProcessThreadData |  // Get PEB and TEB.
    MiniDumpWithHandleData);  // Get all handle information.

class ExceptionHandlerTest : public ::testing::Test {
 protected:
  // Member variable for each test that they can use
  // for temporary storage.
  TCHAR temp_path_[MAX_PATH];

  // Actually constructs a temp path name.
  virtual void SetUp();

  // Deletes temporary files.
  virtual void TearDown();

  void DoCrashInvalidParameter();
  void DoCrashPureVirtualCall();

  // Utility function to test for a path's existence.
  static BOOL DoesPathExist(const TCHAR *path_name);

  // Client callback.
  static void ClientDumpCallback(
      void *dump_context,
      const google_breakpad::ClientInfo *client_info,
      const std::wstring *dump_path);

  static bool DumpCallback(const wchar_t* dump_path,
                           const wchar_t* minidump_id,
                           void* context,
                           EXCEPTION_POINTERS* exinfo,
                           MDRawAssertionInfo* assertion,
                           bool succeeded);

  static std::wstring dump_file;
  static std::wstring full_dump_file;
};

std::wstring ExceptionHandlerTest::dump_file;
std::wstring ExceptionHandlerTest::full_dump_file;

void ExceptionHandlerTest::SetUp() {
  const ::testing::TestInfo* const test_info =
    ::testing::UnitTest::GetInstance()->current_test_info();
  TCHAR temp_path[MAX_PATH] = { '\0' };
  TCHAR test_name_wide[MAX_PATH] = { '\0' };
  // We want the temporary directory to be what the OS returns
  // to us, + the test case name.
  GetTempPath(MAX_PATH, temp_path);
  // THe test case name is exposed to use as a c-style string,
  // But we might be working in UNICODE here on Windows.
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

void ExceptionHandlerTest::TearDown() {
  if (!dump_file.empty()) {
    ::DeleteFile(dump_file.c_str());
    dump_file = L"";
  }
  if (!full_dump_file.empty()) {
    ::DeleteFile(full_dump_file.c_str());
    full_dump_file = L"";
  }
}

BOOL ExceptionHandlerTest::DoesPathExist(const TCHAR *path_name) {
  DWORD flags = GetFileAttributes(path_name);
  if (flags == INVALID_FILE_ATTRIBUTES) {
    return FALSE;
  }
  return TRUE;
}

// static
void ExceptionHandlerTest::ClientDumpCallback(
    void *dump_context,
    const google_breakpad::ClientInfo *client_info,
    const wstring *dump_path) {
  dump_file = *dump_path;
  // Create the full dump file name from the dump path.
  full_dump_file = dump_file.substr(0, dump_file.length() - 4) + L"-full.dmp";
}

// static
bool ExceptionHandlerTest::DumpCallback(const wchar_t* dump_path,
                    const wchar_t* minidump_id,
                    void* context,
                    EXCEPTION_POINTERS* exinfo,
                    MDRawAssertionInfo* assertion,
                    bool succeeded) {
  dump_file = dump_path;
  dump_file += L"\\";
  dump_file += minidump_id;
  dump_file += L".dmp";
    return succeeded;
}

void ExceptionHandlerTest::DoCrashInvalidParameter() {
  google_breakpad::ExceptionHandler *exc =
      new google_breakpad::ExceptionHandler(
          temp_path_, NULL, NULL, NULL,
          google_breakpad::ExceptionHandler::HANDLER_INVALID_PARAMETER,
          kFullDumpType, kPipeName, NULL);

  // Disable the message box for assertions
  _CrtSetReportMode(_CRT_ASSERT, 0);

  // Although this is executing in the child process of the death test,
  // if it's not true we'll still get an error rather than the crash
  // being expected.
  ASSERT_TRUE(exc->IsOutOfProcess());
  printf(NULL);
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

void ExceptionHandlerTest::DoCrashPureVirtualCall() {
  google_breakpad::ExceptionHandler *exc =
      new google_breakpad::ExceptionHandler(
          temp_path_, NULL, NULL, NULL,
          google_breakpad::ExceptionHandler::HANDLER_PURECALL,
          kFullDumpType, kPipeName, NULL);

  // Disable the message box for assertions
  _CrtSetReportMode(_CRT_ASSERT, 0);

  // Although this is executing in the child process of the death test,
  // if it's not true we'll still get an error rather than the crash
  // being expected.
  ASSERT_TRUE(exc->IsOutOfProcess());

  // Create a new frame to ensure PureVirtualCall is not optimized to some
  // other line in this function.
  {
    PureVirtualCall instance;
  }
}

// This test validates that the minidump is written correctly.
TEST_F(ExceptionHandlerTest, InvalidParameterMiniDumpTest) {
  ASSERT_TRUE(DoesPathExist(temp_path_));

  // Call with a bad argument
  ASSERT_TRUE(DoesPathExist(temp_path_));
  wstring dump_path(temp_path_);
  google_breakpad::CrashGenerationServer server(
      kPipeName, NULL, NULL, NULL, ClientDumpCallback, NULL, NULL, NULL, NULL,
      NULL, true, &dump_path);

  ASSERT_TRUE(dump_file.empty() && full_dump_file.empty());

  // This HAS to be EXPECT_, because when this test case is executed in the
  // child process, the server registration will fail due to the named pipe
  // being the same.
  EXPECT_TRUE(server.Start());
  EXPECT_EXIT(DoCrashInvalidParameter(), ::testing::ExitedWithCode(0), "");
  ASSERT_TRUE(!dump_file.empty() && !full_dump_file.empty());
  ASSERT_TRUE(DoesPathExist(dump_file.c_str()));

  // Verify the dump for infos.
  DumpAnalysis mini(dump_file);
  DumpAnalysis full(full_dump_file);

  // The dump should have all of these streams.
  EXPECT_TRUE(mini.HasStream(ThreadListStream));
  EXPECT_TRUE(full.HasStream(ThreadListStream));
  EXPECT_TRUE(mini.HasStream(ModuleListStream));
  EXPECT_TRUE(full.HasStream(ModuleListStream));
  EXPECT_TRUE(mini.HasStream(ExceptionStream));
  EXPECT_TRUE(full.HasStream(ExceptionStream));
  EXPECT_TRUE(mini.HasStream(SystemInfoStream));
  EXPECT_TRUE(full.HasStream(SystemInfoStream));
  EXPECT_TRUE(mini.HasStream(MiscInfoStream));
  EXPECT_TRUE(full.HasStream(MiscInfoStream));
  EXPECT_TRUE(mini.HasStream(HandleDataStream));
  EXPECT_TRUE(full.HasStream(HandleDataStream));

  // We expect PEB and TEBs in this dump.
  EXPECT_TRUE(mini.HasTebs() || full.HasTebs());
  EXPECT_TRUE(mini.HasPeb() || full.HasPeb());

  // Minidump should have a memory listing, but no 64-bit memory.
  EXPECT_TRUE(mini.HasStream(MemoryListStream));
  EXPECT_FALSE(mini.HasStream(Memory64ListStream));

  EXPECT_FALSE(full.HasStream(MemoryListStream));
  EXPECT_TRUE(full.HasStream(Memory64ListStream));

  // This is the only place we don't use OR because we want both not
  // to have the streams.
  EXPECT_FALSE(mini.HasStream(ThreadExListStream));
  EXPECT_FALSE(full.HasStream(ThreadExListStream));
  EXPECT_FALSE(mini.HasStream(CommentStreamA));
  EXPECT_FALSE(full.HasStream(CommentStreamA));
  EXPECT_FALSE(mini.HasStream(CommentStreamW));
  EXPECT_FALSE(full.HasStream(CommentStreamW));
  EXPECT_FALSE(mini.HasStream(FunctionTableStream));
  EXPECT_FALSE(full.HasStream(FunctionTableStream));
  EXPECT_FALSE(mini.HasStream(MemoryInfoListStream));
  EXPECT_FALSE(full.HasStream(MemoryInfoListStream));
  EXPECT_FALSE(mini.HasStream(ThreadInfoListStream));
  EXPECT_FALSE(full.HasStream(ThreadInfoListStream));
  EXPECT_FALSE(mini.HasStream(HandleOperationListStream));
  EXPECT_FALSE(full.HasStream(HandleOperationListStream));
  EXPECT_FALSE(mini.HasStream(TokenStream));
  EXPECT_FALSE(full.HasStream(TokenStream));
}


// This test validates that the minidump is written correctly.
TEST_F(ExceptionHandlerTest, PureVirtualCallMiniDumpTest) {
  ASSERT_TRUE(DoesPathExist(temp_path_));

  // Call with a bad argument
  ASSERT_TRUE(DoesPathExist(temp_path_));
  wstring dump_path(temp_path_);
  google_breakpad::CrashGenerationServer server(
      kPipeName, NULL, NULL, NULL, ClientDumpCallback, NULL, NULL, NULL, NULL,
      NULL, true, &dump_path);

  ASSERT_TRUE(dump_file.empty() && full_dump_file.empty());

  // This HAS to be EXPECT_, because when this test case is executed in the
  // child process, the server registration will fail due to the named pipe
  // being the same.
  EXPECT_TRUE(server.Start());
  EXPECT_EXIT(DoCrashPureVirtualCall(), ::testing::ExitedWithCode(0), "");
  ASSERT_TRUE(!dump_file.empty() && !full_dump_file.empty());
  ASSERT_TRUE(DoesPathExist(dump_file.c_str()));

  // Verify the dump for infos.
  DumpAnalysis mini(dump_file);
  DumpAnalysis full(full_dump_file);

  // The dump should have all of these streams.
  EXPECT_TRUE(mini.HasStream(ThreadListStream));
  EXPECT_TRUE(full.HasStream(ThreadListStream));
  EXPECT_TRUE(mini.HasStream(ModuleListStream));
  EXPECT_TRUE(full.HasStream(ModuleListStream));
  EXPECT_TRUE(mini.HasStream(ExceptionStream));
  EXPECT_TRUE(full.HasStream(ExceptionStream));
  EXPECT_TRUE(mini.HasStream(SystemInfoStream));
  EXPECT_TRUE(full.HasStream(SystemInfoStream));
  EXPECT_TRUE(mini.HasStream(MiscInfoStream));
  EXPECT_TRUE(full.HasStream(MiscInfoStream));
  EXPECT_TRUE(mini.HasStream(HandleDataStream));
  EXPECT_TRUE(full.HasStream(HandleDataStream));

  // We expect PEB and TEBs in this dump.
  EXPECT_TRUE(mini.HasTebs() || full.HasTebs());
  EXPECT_TRUE(mini.HasPeb() || full.HasPeb());

  // Minidump should have a memory listing, but no 64-bit memory.
  EXPECT_TRUE(mini.HasStream(MemoryListStream));
  EXPECT_FALSE(mini.HasStream(Memory64ListStream));

  EXPECT_FALSE(full.HasStream(MemoryListStream));
  EXPECT_TRUE(full.HasStream(Memory64ListStream));

  // This is the only place we don't use OR because we want both not
  // to have the streams.
  EXPECT_FALSE(mini.HasStream(ThreadExListStream));
  EXPECT_FALSE(full.HasStream(ThreadExListStream));
  EXPECT_FALSE(mini.HasStream(CommentStreamA));
  EXPECT_FALSE(full.HasStream(CommentStreamA));
  EXPECT_FALSE(mini.HasStream(CommentStreamW));
  EXPECT_FALSE(full.HasStream(CommentStreamW));
  EXPECT_FALSE(mini.HasStream(FunctionTableStream));
  EXPECT_FALSE(full.HasStream(FunctionTableStream));
  EXPECT_FALSE(mini.HasStream(MemoryInfoListStream));
  EXPECT_FALSE(full.HasStream(MemoryInfoListStream));
  EXPECT_FALSE(mini.HasStream(ThreadInfoListStream));
  EXPECT_FALSE(full.HasStream(ThreadInfoListStream));
  EXPECT_FALSE(mini.HasStream(HandleOperationListStream));
  EXPECT_FALSE(full.HasStream(HandleOperationListStream));
  EXPECT_FALSE(mini.HasStream(TokenStream));
  EXPECT_FALSE(full.HasStream(TokenStream));
}

// Test that writing a minidump produces a valid minidump containing
// some expected structures.
TEST_F(ExceptionHandlerTest, WriteMinidumpTest) {
  ExceptionHandler handler(temp_path_,
                           NULL,
                           DumpCallback,
                           NULL,
                           ExceptionHandler::HANDLER_ALL);

  // Disable GTest SEH handler
  testing::DisableExceptionHandlerInScope disable_exception_handler;

  ASSERT_TRUE(handler.WriteMinidump());
  ASSERT_FALSE(dump_file.empty());

  string minidump_filename;
  ASSERT_TRUE(WindowsStringUtils::safe_wcstombs(dump_file,
                                                &minidump_filename));

  // Read the minidump and verify some info.
  Minidump minidump(minidump_filename);
  ASSERT_TRUE(minidump.Read());
  // TODO(ted): more comprehensive tests...
}

// Test that an additional memory region can be included in the minidump.
TEST_F(ExceptionHandlerTest, AdditionalMemory) {
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  const uint32_t kMemorySize = si.dwPageSize;

  // Get some heap memory.
  uint8_t* memory = new uint8_t[kMemorySize];
  const uintptr_t kMemoryAddress = reinterpret_cast<uintptr_t>(memory);
  ASSERT_TRUE(memory);

  // Stick some data into the memory so the contents can be verified.
  for (uint32_t i = 0; i < kMemorySize; ++i) {
    memory[i] = i % 255;
  }

  ExceptionHandler handler(temp_path_,
                           NULL,
                           DumpCallback,
                           NULL,
                           ExceptionHandler::HANDLER_ALL);

  // Disable GTest SEH handler
  testing::DisableExceptionHandlerInScope disable_exception_handler;

  // Add the memory region to the list of memory to be included.
  handler.RegisterAppMemory(memory, kMemorySize);
  ASSERT_TRUE(handler.WriteMinidump());
  ASSERT_FALSE(dump_file.empty());

  string minidump_filename;
  ASSERT_TRUE(WindowsStringUtils::safe_wcstombs(dump_file,
                                                &minidump_filename));

  // Read the minidump. Ensure that the memory region is present
  Minidump minidump(minidump_filename);
  ASSERT_TRUE(minidump.Read());

  MinidumpMemoryList* dump_memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(dump_memory_list);
  const MinidumpMemoryRegion* region =
    dump_memory_list->GetMemoryRegionForAddress(kMemoryAddress);
  ASSERT_TRUE(region);

  EXPECT_EQ(kMemoryAddress, region->GetBase());
  EXPECT_EQ(kMemorySize, region->GetSize());

  // Verify memory contents.
  EXPECT_EQ(0, memcmp(region->GetMemory(), memory, kMemorySize));

  delete[] memory;
}

// Test that a memory region that was previously registered
// can be unregistered.
TEST_F(ExceptionHandlerTest, AdditionalMemoryRemove) {
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  const uint32_t kMemorySize = si.dwPageSize;

  // Get some heap memory.
  uint8_t* memory = new uint8_t[kMemorySize];
  const uintptr_t kMemoryAddress = reinterpret_cast<uintptr_t>(memory);
  ASSERT_TRUE(memory);

  // Stick some data into the memory so the contents can be verified.
  for (uint32_t i = 0; i < kMemorySize; ++i) {
    memory[i] = i % 255;
  }

  ExceptionHandler handler(temp_path_,
                           NULL,
                           DumpCallback,
                           NULL,
                           ExceptionHandler::HANDLER_ALL);

  // Disable GTest SEH handler
  testing::DisableExceptionHandlerInScope disable_exception_handler;

  // Add the memory region to the list of memory to be included.
  handler.RegisterAppMemory(memory, kMemorySize);

  // ...and then remove it
  handler.UnregisterAppMemory(memory);

  ASSERT_TRUE(handler.WriteMinidump());
  ASSERT_FALSE(dump_file.empty());

  string minidump_filename;
  ASSERT_TRUE(WindowsStringUtils::safe_wcstombs(dump_file,
                                                &minidump_filename));

  // Read the minidump. Ensure that the memory region is not present.
  Minidump minidump(minidump_filename);
  ASSERT_TRUE(minidump.Read());

  MinidumpMemoryList* dump_memory_list = minidump.GetMemoryList();
  ASSERT_TRUE(dump_memory_list);
  const MinidumpMemoryRegion* region =
    dump_memory_list->GetMemoryRegionForAddress(kMemoryAddress);
  EXPECT_FALSE(region);

  delete[] memory;
}

}  // namespace
