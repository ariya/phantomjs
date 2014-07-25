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

#include "dump_analysis.h"  // NOLINT
#include "gtest/gtest.h"

DumpAnalysis::~DumpAnalysis() {
  if (dump_file_view_ != NULL) {
    EXPECT_TRUE(::UnmapViewOfFile(dump_file_view_));
    ::CloseHandle(dump_file_mapping_);
    dump_file_mapping_ = NULL;
  }

  if (dump_file_handle_ != NULL) {
    ::CloseHandle(dump_file_handle_);
    dump_file_handle_ = NULL;
  }
}

void DumpAnalysis::EnsureDumpMapped() {
  if (dump_file_view_ == NULL) {
    dump_file_handle_ = ::CreateFile(dump_file_.c_str(),
      GENERIC_READ,
      0,
      NULL,
      OPEN_EXISTING,
      0,
      NULL);
    ASSERT_TRUE(dump_file_handle_ != NULL);
    ASSERT_TRUE(dump_file_mapping_ == NULL);

    dump_file_mapping_ = ::CreateFileMapping(dump_file_handle_,
      NULL,
      PAGE_READONLY,
      0,
      0,
      NULL);
    ASSERT_TRUE(dump_file_mapping_ != NULL);

    dump_file_view_ = ::MapViewOfFile(dump_file_mapping_,
      FILE_MAP_READ,
      0,
      0,
      0);
    ASSERT_TRUE(dump_file_view_ != NULL);
  }
}

bool DumpAnalysis::HasTebs() const {
  MINIDUMP_THREAD_LIST* thread_list = NULL;
  size_t thread_list_size = GetStream(ThreadListStream, &thread_list);

  if (thread_list_size > 0 && thread_list != NULL) {
    for (ULONG i = 0; i < thread_list->NumberOfThreads; ++i) {
      if (!HasMemory(thread_list->Threads[i].Teb))
        return false;
    }

    return true;
  }

  // No thread list, no TEB info.
  return false;
}

bool DumpAnalysis::HasPeb() const {
  MINIDUMP_THREAD_LIST* thread_list = NULL;
  size_t thread_list_size = GetStream(ThreadListStream, &thread_list);

  if (thread_list_size > 0 && thread_list != NULL &&
      thread_list->NumberOfThreads > 0) {
    FakeTEB* teb = NULL;
    if (!HasMemory(thread_list->Threads[0].Teb, &teb))
      return false;

    return HasMemory(teb->peb);
  }

  return false;
}

bool DumpAnalysis::HasStream(ULONG stream_number) const {
  void* stream = NULL;
  size_t stream_size = GetStreamImpl(stream_number, &stream);
  return stream_size > 0 && stream != NULL;
}

size_t DumpAnalysis::GetStreamImpl(ULONG stream_number, void** stream) const {
  MINIDUMP_DIRECTORY* directory = NULL;
  ULONG memory_list_size = 0;
  BOOL ret = ::MiniDumpReadDumpStream(dump_file_view_,
                                      stream_number,
                                      &directory,
                                      stream,
                                      &memory_list_size);

  return ret ? memory_list_size : 0;
}

bool DumpAnalysis::HasMemoryImpl(const void *addr_in, size_t structuresize,
                                 void **structure) const {
  uintptr_t address = reinterpret_cast<uintptr_t>(addr_in);
  MINIDUMP_MEMORY_LIST* memory_list = NULL;
  size_t memory_list_size = GetStream(MemoryListStream, &memory_list);
  if (memory_list_size > 0 && memory_list != NULL) {
    for (ULONG i = 0; i < memory_list->NumberOfMemoryRanges; ++i) {
      MINIDUMP_MEMORY_DESCRIPTOR& descr = memory_list->MemoryRanges[i];
      const uintptr_t range_start =
          static_cast<uintptr_t>(descr.StartOfMemoryRange);
      uintptr_t range_end = range_start + descr.Memory.DataSize;

      if (address >= range_start &&
          address + structuresize < range_end) {
        // The start address falls in the range, and the end address is
        // in bounds, return a pointer to the structure if requested.
        if (structure != NULL)
          *structure = RVA_TO_ADDR(dump_file_view_, descr.Memory.Rva);

        return true;
      }
    }
  }

  // We didn't find the range in a MINIDUMP_MEMORY_LIST, so maybe this
  // is a full dump using MINIDUMP_MEMORY64_LIST with all the memory at the
  // end of the dump file.
  MINIDUMP_MEMORY64_LIST* memory64_list = NULL;
  memory_list_size = GetStream(Memory64ListStream, &memory64_list);
  if (memory_list_size > 0 && memory64_list != NULL) {
    // Keep track of where the current descriptor maps to.
    RVA64 curr_rva = memory64_list->BaseRva;
    for (ULONG i = 0; i < memory64_list->NumberOfMemoryRanges; ++i) {
      MINIDUMP_MEMORY_DESCRIPTOR64& descr = memory64_list->MemoryRanges[i];
      uintptr_t range_start =
          static_cast<uintptr_t>(descr.StartOfMemoryRange);
      uintptr_t range_end = range_start + static_cast<size_t>(descr.DataSize);

      if (address >= range_start &&
          address + structuresize < range_end) {
        // The start address falls in the range, and the end address is
        // in bounds, return a pointer to the structure if requested.
        if (structure != NULL)
          *structure = RVA_TO_ADDR(dump_file_view_, curr_rva);

        return true;
      }

      // Advance the current RVA.
      curr_rva += descr.DataSize;
    }
  }

  return false;
}
