// Copyright (c) 2011, Google Inc.
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

// memory_mapped_file.cc: Implement google_breakpad::MemoryMappedFile.
// See memory_mapped_file.h for details.

#include "common/linux/memory_mapped_file.h"

#include <fcntl.h>
#include <sys/mman.h>
#if defined(__ANDROID__)
#include <sys/stat.h>
#endif
#include <unistd.h>

#include "common/memory_range.h"
#include "third_party/lss/linux_syscall_support.h"

namespace google_breakpad {

MemoryMappedFile::MemoryMappedFile() {}

MemoryMappedFile::MemoryMappedFile(const char* path) {
  Map(path);
}

MemoryMappedFile::~MemoryMappedFile() {
  Unmap();
}

bool MemoryMappedFile::Map(const char* path) {
  Unmap();

  int fd = sys_open(path, O_RDONLY, 0);
  if (fd == -1) {
    return false;
  }

#if defined(__ANDROID__)
  struct stat st;
  if (fstat(fd, &st) != 0) {
#elif defined(__x86_64__)
  struct kernel_stat st;
  if (sys_fstat(fd, &st) == -1 || st.st_size < 0) {
#else
  struct kernel_stat64 st;
  if (sys_fstat64(fd, &st) == -1 || st.st_size < 0) {
#endif
    sys_close(fd);
    return false;
  }

  // If the file size is zero, simply use an empty MemoryRange and return
  // true. Don't bother to call mmap() even though mmap() can handle an
  // empty file on some platforms.
  if (st.st_size == 0) {
    sys_close(fd);
    return true;
  }

#if defined(__x86_64__)
  void* data = sys_mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
#else
  void* data = sys_mmap2(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
#endif
  sys_close(fd);
  if (data == MAP_FAILED) {
    return false;
  }

  content_.Set(data, st.st_size);
  return true;
}

void MemoryMappedFile::Unmap() {
  if (content_.data()) {
    sys_munmap(const_cast<u_int8_t*>(content_.data()), content_.length());
    content_.Set(NULL, 0);
  }
}

}  // namespace google_breakpad
