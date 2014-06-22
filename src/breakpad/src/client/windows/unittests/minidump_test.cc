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

#include <windows.h>
#include <objbase.h>
#include <dbghelp.h>

#include "client/windows/crash_generation/minidump_generator.h"
#include "client/windows/unittests/dump_analysis.h"  // NOLINT

#include "gtest/gtest.h"

namespace {

// Minidump with stacks, PEB, TEB, and unloaded module list.
const MINIDUMP_TYPE kSmallDumpType = static_cast<MINIDUMP_TYPE>(
    MiniDumpWithProcessThreadData |  // Get PEB and TEB.
    MiniDumpWithUnloadedModules);  // Get unloaded modules when available.

// Minidump with all of the above, plus memory referenced from stack.
const MINIDUMP_TYPE kLargerDumpType = static_cast<MINIDUMP_TYPE>(
    MiniDumpWithProcessThreadData |  // Get PEB and TEB.
    MiniDumpWithUnloadedModules |  // Get unloaded modules when available.
    MiniDumpWithIndirectlyReferencedMemory);  // Get memory referenced by stack.

// Large dump with all process memory.
const MINIDUMP_TYPE kFullDumpType = static_cast<MINIDUMP_TYPE>(
    MiniDumpWithFullMemory |  // Full memory from process.
    MiniDumpWithProcessThreadData |  // Get PEB and TEB.
    MiniDumpWithHandleData |  // Get all handle information.
    MiniDumpWithUnloadedModules);  // Get unloaded modules when available.

class MinidumpTest: public testing::Test {
 public:
  MinidumpTest() {
    wchar_t temp_dir_path[ MAX_PATH ] = {0};
    ::GetTempPath(MAX_PATH, temp_dir_path);
    dump_path_ = temp_dir_path;
  }

  virtual void SetUp() {
    // Make sure URLMon isn't loaded into our process.
    ASSERT_EQ(NULL, ::GetModuleHandle(L"urlmon.dll"));

    // Then load and unload it to ensure we have something to
    // stock the unloaded module list with.
    HMODULE urlmon = ::LoadLibrary(L"urlmon.dll");
    ASSERT_TRUE(urlmon != NULL);
    ASSERT_TRUE(::FreeLibrary(urlmon));
  }

  virtual void TearDown() {
    if (!dump_file_.empty()) {
      ::DeleteFile(dump_file_.c_str());
      dump_file_ = L"";
    }
    if (!full_dump_file_.empty()) {
      ::DeleteFile(full_dump_file_.c_str());
      full_dump_file_ = L"";
    }
  }

  bool WriteDump(ULONG flags) {
    using google_breakpad::MinidumpGenerator;

    // Fake exception is access violation on write to this.
    EXCEPTION_RECORD ex_record = {
        STATUS_ACCESS_VIOLATION,  // ExceptionCode
        0,  // ExceptionFlags
        NULL,  // ExceptionRecord;
        reinterpret_cast<void*>(0xCAFEBABE),  // ExceptionAddress;
        2,  // NumberParameters;
        { EXCEPTION_WRITE_FAULT, reinterpret_cast<ULONG_PTR>(this) }
    };
    CONTEXT ctx_record = {};
    EXCEPTION_POINTERS ex_ptrs = {
      &ex_record,
      &ctx_record,
    };

    MinidumpGenerator generator(dump_path_,
                                ::GetCurrentProcess(),
                                ::GetCurrentProcessId(),
                                ::GetCurrentThreadId(),
                                ::GetCurrentThreadId(),
                                &ex_ptrs,
                                NULL,
                                static_cast<MINIDUMP_TYPE>(flags),
                                TRUE);
    generator.GenerateDumpFile(&dump_file_);
    generator.GenerateFullDumpFile(&full_dump_file_);
    // And write a dump
    bool result = generator.WriteMinidump();
    return result == TRUE;
  }

 protected:
  std::wstring dump_file_;
  std::wstring full_dump_file_;

  std::wstring dump_path_;
};

// We need to be able to get file information from Windows
bool HasFileInfo(const std::wstring& file_path) {
  DWORD dummy;
  const wchar_t* path = file_path.c_str();
  DWORD length = ::GetFileVersionInfoSize(path, &dummy);
  if (length == 0)
    return NULL;

  void* data = calloc(length, 1);
  if (!data)
    return false;

  if (!::GetFileVersionInfo(path, dummy, length, data)) {
    free(data);
    return false;
  }

  void* translate = NULL;
  UINT page_count;
  BOOL query_result = VerQueryValue(
      data,
      L"\\VarFileInfo\\Translation",
      static_cast<void**>(&translate),
      &page_count);

  free(data);
  if (query_result && translate) {
    return true;
  } else {
    return false;
  }
}

TEST_F(MinidumpTest, Version) {
  // Loads DbgHelp.dll in process
  ImagehlpApiVersion();

  HMODULE dbg_help = ::GetModuleHandle(L"dbghelp.dll");
  ASSERT_TRUE(dbg_help != NULL);

  wchar_t dbg_help_file[1024] = {};
  ASSERT_TRUE(::GetModuleFileName(dbg_help,
                                  dbg_help_file,
                                  sizeof(dbg_help_file) /
                                      sizeof(*dbg_help_file)));
  ASSERT_TRUE(HasFileInfo(std::wstring(dbg_help_file)) != NULL);

//  LOG(INFO) << "DbgHelp.dll version: " << file_info->file_version();
}

TEST_F(MinidumpTest, Normal) {
  EXPECT_TRUE(WriteDump(MiniDumpNormal));
  DumpAnalysis mini(dump_file_);

  // We expect threads, modules and some memory.
  EXPECT_TRUE(mini.HasStream(ThreadListStream));
  EXPECT_TRUE(mini.HasStream(ModuleListStream));
  EXPECT_TRUE(mini.HasStream(MemoryListStream));
  EXPECT_TRUE(mini.HasStream(ExceptionStream));
  EXPECT_TRUE(mini.HasStream(SystemInfoStream));
  EXPECT_TRUE(mini.HasStream(MiscInfoStream));

  EXPECT_FALSE(mini.HasStream(ThreadExListStream));
  EXPECT_FALSE(mini.HasStream(Memory64ListStream));
  EXPECT_FALSE(mini.HasStream(CommentStreamA));
  EXPECT_FALSE(mini.HasStream(CommentStreamW));
  EXPECT_FALSE(mini.HasStream(HandleDataStream));
  EXPECT_FALSE(mini.HasStream(FunctionTableStream));
  EXPECT_FALSE(mini.HasStream(UnloadedModuleListStream));
  EXPECT_FALSE(mini.HasStream(MemoryInfoListStream));
  EXPECT_FALSE(mini.HasStream(ThreadInfoListStream));
  EXPECT_FALSE(mini.HasStream(HandleOperationListStream));
  EXPECT_FALSE(mini.HasStream(TokenStream));

  // We expect no PEB nor TEBs in this dump.
  EXPECT_FALSE(mini.HasTebs());
  EXPECT_FALSE(mini.HasPeb());

  // We expect no off-stack memory in this dump.
  EXPECT_FALSE(mini.HasMemory(this));
}

TEST_F(MinidumpTest, SmallDump) {
  ASSERT_TRUE(WriteDump(kSmallDumpType));
  DumpAnalysis mini(dump_file_);

  EXPECT_TRUE(mini.HasStream(ThreadListStream));
  EXPECT_TRUE(mini.HasStream(ModuleListStream));
  EXPECT_TRUE(mini.HasStream(MemoryListStream));
  EXPECT_TRUE(mini.HasStream(ExceptionStream));
  EXPECT_TRUE(mini.HasStream(SystemInfoStream));
  EXPECT_TRUE(mini.HasStream(UnloadedModuleListStream));
  EXPECT_TRUE(mini.HasStream(MiscInfoStream));

  // We expect PEB and TEBs in this dump.
  EXPECT_TRUE(mini.HasTebs());
  EXPECT_TRUE(mini.HasPeb());

  EXPECT_FALSE(mini.HasStream(ThreadExListStream));
  EXPECT_FALSE(mini.HasStream(Memory64ListStream));
  EXPECT_FALSE(mini.HasStream(CommentStreamA));
  EXPECT_FALSE(mini.HasStream(CommentStreamW));
  EXPECT_FALSE(mini.HasStream(HandleDataStream));
  EXPECT_FALSE(mini.HasStream(FunctionTableStream));
  EXPECT_FALSE(mini.HasStream(MemoryInfoListStream));
  EXPECT_FALSE(mini.HasStream(ThreadInfoListStream));
  EXPECT_FALSE(mini.HasStream(HandleOperationListStream));
  EXPECT_FALSE(mini.HasStream(TokenStream));

  // We expect no off-stack memory in this dump.
  EXPECT_FALSE(mini.HasMemory(this));
}

TEST_F(MinidumpTest, LargerDump) {
  ASSERT_TRUE(WriteDump(kLargerDumpType));
  DumpAnalysis mini(dump_file_);

  // The dump should have all of these streams.
  EXPECT_TRUE(mini.HasStream(ThreadListStream));
  EXPECT_TRUE(mini.HasStream(ModuleListStream));
  EXPECT_TRUE(mini.HasStream(MemoryListStream));
  EXPECT_TRUE(mini.HasStream(ExceptionStream));
  EXPECT_TRUE(mini.HasStream(SystemInfoStream));
  EXPECT_TRUE(mini.HasStream(UnloadedModuleListStream));
  EXPECT_TRUE(mini.HasStream(MiscInfoStream));

  // We expect memory referenced by stack in this dump.
  EXPECT_TRUE(mini.HasMemory(this));

  // We expect PEB and TEBs in this dump.
  EXPECT_TRUE(mini.HasTebs());
  EXPECT_TRUE(mini.HasPeb());

  EXPECT_FALSE(mini.HasStream(ThreadExListStream));
  EXPECT_FALSE(mini.HasStream(Memory64ListStream));
  EXPECT_FALSE(mini.HasStream(CommentStreamA));
  EXPECT_FALSE(mini.HasStream(CommentStreamW));
  EXPECT_FALSE(mini.HasStream(HandleDataStream));
  EXPECT_FALSE(mini.HasStream(FunctionTableStream));
  EXPECT_FALSE(mini.HasStream(MemoryInfoListStream));
  EXPECT_FALSE(mini.HasStream(ThreadInfoListStream));
  EXPECT_FALSE(mini.HasStream(HandleOperationListStream));
  EXPECT_FALSE(mini.HasStream(TokenStream));
}

TEST_F(MinidumpTest, FullDump) {
  ASSERT_TRUE(WriteDump(kFullDumpType));
  ASSERT_TRUE(dump_file_ != L"");
  ASSERT_TRUE(full_dump_file_ != L"");
  DumpAnalysis mini(dump_file_);
  DumpAnalysis full(full_dump_file_);

  // Either dumps can contain part of the information.

  // The dump should have all of these streams.
  EXPECT_TRUE(mini.HasStream(ThreadListStream));
  EXPECT_TRUE(full.HasStream(ThreadListStream));
  EXPECT_TRUE(mini.HasStream(ModuleListStream));
  EXPECT_TRUE(full.HasStream(ModuleListStream));
  EXPECT_TRUE(mini.HasStream(ExceptionStream));
  EXPECT_TRUE(full.HasStream(ExceptionStream));
  EXPECT_TRUE(mini.HasStream(SystemInfoStream));
  EXPECT_TRUE(full.HasStream(SystemInfoStream));
  EXPECT_TRUE(mini.HasStream(UnloadedModuleListStream));
  EXPECT_TRUE(full.HasStream(UnloadedModuleListStream));
  EXPECT_TRUE(mini.HasStream(MiscInfoStream));
  EXPECT_TRUE(full.HasStream(MiscInfoStream));
  EXPECT_TRUE(mini.HasStream(HandleDataStream));
  EXPECT_TRUE(full.HasStream(HandleDataStream));

  // We expect memory referenced by stack in this dump.
  EXPECT_FALSE(mini.HasMemory(this));
  EXPECT_TRUE(full.HasMemory(this));

  // We expect PEB and TEBs in this dump.
  EXPECT_TRUE(mini.HasTebs() || full.HasTebs());
  EXPECT_TRUE(mini.HasPeb() || full.HasPeb());

  EXPECT_TRUE(mini.HasStream(MemoryListStream));
  EXPECT_TRUE(full.HasStream(Memory64ListStream));
  EXPECT_FALSE(mini.HasStream(Memory64ListStream));
  EXPECT_FALSE(full.HasStream(MemoryListStream));

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

}  // namespace
