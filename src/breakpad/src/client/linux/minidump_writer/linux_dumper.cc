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

// linux_dumper.cc: Implement google_breakpad::LinuxDumper.
// See linux_dumper.h for details.

// This code deals with the mechanics of getting information about a crashed
// process. Since this code may run in a compromised address space, the same
// rules apply as detailed at the top of minidump_writer.h: no libc calls and
// use the alternative allocator.

#include "client/linux/minidump_writer/linux_dumper.h"

#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

#include "client/linux/minidump_writer/line_reader.h"
#include "common/linux/file_id.h"
#include "common/linux/linux_libc_support.h"
#include "common/linux/memory_mapped_file.h"
#include "common/linux/safe_readlink.h"
#include "third_party/lss/linux_syscall_support.h"

static const char kMappedFileUnsafePrefix[] = "/dev/";
static const char kDeletedSuffix[] = " (deleted)";

inline static bool IsMappedFileOpenUnsafe(
    const google_breakpad::MappingInfo& mapping) {
  // It is unsafe to attempt to open a mapped file that lives under /dev,
  // because the semantics of the open may be driver-specific so we'd risk
  // hanging the crash dumper. And a file in /dev/ almost certainly has no
  // ELF file identifier anyways.
  return my_strncmp(mapping.name,
                    kMappedFileUnsafePrefix,
                    sizeof(kMappedFileUnsafePrefix) - 1) == 0;
}

namespace google_breakpad {

LinuxDumper::LinuxDumper(pid_t pid)
    : pid_(pid),
      crash_address_(0),
      crash_signal_(0),
      crash_thread_(0),
      threads_(&allocator_, 8),
      mappings_(&allocator_) {
}

LinuxDumper::~LinuxDumper() {
}

bool LinuxDumper::Init() {
  return EnumerateThreads() && EnumerateMappings();
}

bool
LinuxDumper::ElfFileIdentifierForMapping(const MappingInfo& mapping,
                                         bool member,
                                         unsigned int mapping_id,
                                         uint8_t identifier[sizeof(MDGUID)])
{
  assert(!member || mapping_id < mappings_.size());
  my_memset(identifier, 0, sizeof(MDGUID));
  if (IsMappedFileOpenUnsafe(mapping))
    return false;

  // Special-case linux-gate because it's not a real file.
  if (my_strcmp(mapping.name, kLinuxGateLibraryName) == 0) {
    const uintptr_t kPageSize = getpagesize();
    void* linux_gate = NULL;
    if (pid_ == sys_getpid()) {
      linux_gate = reinterpret_cast<void*>(mapping.start_addr);
    } else {
      linux_gate = allocator_.Alloc(kPageSize);
      CopyFromProcess(linux_gate, pid_,
                      reinterpret_cast<const void*>(mapping.start_addr),
                      kPageSize);
    }
    return FileID::ElfFileIdentifierFromMappedFile(linux_gate, identifier);
  }

  char filename[NAME_MAX];
  size_t filename_len = my_strlen(mapping.name);
  assert(filename_len < NAME_MAX);
  if (filename_len >= NAME_MAX)
    return false;
  memcpy(filename, mapping.name, filename_len);
  filename[filename_len] = '\0';
  bool filename_modified = HandleDeletedFileInMapping(filename);

  MemoryMappedFile mapped_file(filename);
  if (!mapped_file.data())  // Should probably check if size >= ElfW(Ehdr)?
    return false;

  bool success =
      FileID::ElfFileIdentifierFromMappedFile(mapped_file.data(), identifier);
  if (success && member && filename_modified) {
    mappings_[mapping_id]->name[filename_len -
                                sizeof(kDeletedSuffix) + 1] = '\0';
  }

  return success;
}

void*
LinuxDumper::FindBeginningOfLinuxGateSharedLibrary(pid_t pid) const {
  char auxv_path[NAME_MAX];
  if (!BuildProcPath(auxv_path, pid, "auxv"))
    return NULL;

  // Find the AT_SYSINFO_EHDR entry for linux-gate.so
  // See http://www.trilithium.com/johan/2005/08/linux-gate/ for more
  // information.
  int fd = sys_open(auxv_path, O_RDONLY, 0);
  if (fd < 0) {
    return NULL;
  }

  elf_aux_entry one_aux_entry;
  while (sys_read(fd,
                  &one_aux_entry,
                  sizeof(elf_aux_entry)) == sizeof(elf_aux_entry) &&
         one_aux_entry.a_type != AT_NULL) {
    if (one_aux_entry.a_type == AT_SYSINFO_EHDR) {
      close(fd);
      return reinterpret_cast<void*>(one_aux_entry.a_un.a_val);
    }
  }
  close(fd);
  return NULL;
}

void*
LinuxDumper::FindEntryPoint(pid_t pid) const {
  char auxv_path[NAME_MAX];
  if (!BuildProcPath(auxv_path, pid, "auxv"))
    return NULL;

  int fd = sys_open(auxv_path, O_RDONLY, 0);
  if (fd < 0) {
    return NULL;
  }

  // Find the AT_ENTRY entry
  elf_aux_entry one_aux_entry;
  while (sys_read(fd,
                  &one_aux_entry,
                  sizeof(elf_aux_entry)) == sizeof(elf_aux_entry) &&
         one_aux_entry.a_type != AT_NULL) {
    if (one_aux_entry.a_type == AT_ENTRY) {
      close(fd);
      return reinterpret_cast<void*>(one_aux_entry.a_un.a_val);
    }
  }
  close(fd);
  return NULL;
}

bool LinuxDumper::EnumerateMappings() {
  char maps_path[NAME_MAX];
  if (!BuildProcPath(maps_path, pid_, "maps"))
    return false;

  // linux_gate_loc is the beginning of the kernel's mapping of
  // linux-gate.so in the process.  It doesn't actually show up in the
  // maps list as a filename, so we use the aux vector to find it's
  // load location and special case it's entry when creating the list
  // of mappings.
  const void* linux_gate_loc;
  linux_gate_loc = FindBeginningOfLinuxGateSharedLibrary(pid_);
  // Although the initial executable is usually the first mapping, it's not
  // guaranteed (see http://crosbug.com/25355); therefore, try to use the
  // actual entry point to find the mapping.
  const void* entry_point_loc = FindEntryPoint(pid_);

  const int fd = sys_open(maps_path, O_RDONLY, 0);
  if (fd < 0)
    return false;
  LineReader* const line_reader = new(allocator_) LineReader(fd);

  const char* line;
  unsigned line_len;
  while (line_reader->GetNextLine(&line, &line_len)) {
    uintptr_t start_addr, end_addr, offset;

    const char* i1 = my_read_hex_ptr(&start_addr, line);
    if (*i1 == '-') {
      const char* i2 = my_read_hex_ptr(&end_addr, i1 + 1);
      if (*i2 == ' ') {
        const char* i3 = my_read_hex_ptr(&offset, i2 + 6 /* skip ' rwxp ' */);
        if (*i3 == ' ') {
          const char* name = NULL;
          // Only copy name if the name is a valid path name, or if
          // it's the VDSO image.
          if (((name = my_strchr(line, '/')) == NULL) &&
              linux_gate_loc &&
              reinterpret_cast<void*>(start_addr) == linux_gate_loc) {
            name = kLinuxGateLibraryName;
            offset = 0;
          }
          // Merge adjacent mappings with the same name into one module,
          // assuming they're a single library mapped by the dynamic linker
          if (name && !mappings_.empty()) {
            MappingInfo* module = mappings_.back();
            if ((start_addr == module->start_addr + module->size) &&
                (my_strlen(name) == my_strlen(module->name)) &&
                (my_strncmp(name, module->name, my_strlen(name)) == 0)) {
              module->size = end_addr - module->start_addr;
              line_reader->PopLine(line_len);
              continue;
            }
          }
          MappingInfo* const module = new(allocator_) MappingInfo;
          memset(module, 0, sizeof(MappingInfo));
          module->start_addr = start_addr;
          module->size = end_addr - start_addr;
          module->offset = offset;
          if (name != NULL) {
            const unsigned l = my_strlen(name);
            if (l < sizeof(module->name))
              memcpy(module->name, name, l);
          }
          // If this is the entry-point mapping, and it's not already the
          // first one, then we need to make it be first.  This is because
          // the minidump format assumes the first module is the one that
          // corresponds to the main executable (as codified in
          // processor/minidump.cc:MinidumpModuleList::GetMainModule()).
          if (entry_point_loc &&
              (entry_point_loc >=
                  reinterpret_cast<void*>(module->start_addr)) &&
              (entry_point_loc <
                  reinterpret_cast<void*>(module->start_addr+module->size)) &&
              !mappings_.empty()) {
            // push the module onto the front of the list.
            mappings_.resize(mappings_.size() + 1);
            for (size_t idx = mappings_.size() - 1; idx > 0; idx--)
              mappings_[idx] = mappings_[idx - 1];
            mappings_[0] = module;
          } else {
            mappings_.push_back(module);
          }
        }
      }
    }
    line_reader->PopLine(line_len);
  }

  sys_close(fd);

  return !mappings_.empty();
}

// Get information about the stack, given the stack pointer. We don't try to
// walk the stack since we might not have all the information needed to do
// unwind. So we just grab, up to, 32k of stack.
bool LinuxDumper::GetStackInfo(const void** stack, size_t* stack_len,
                               uintptr_t int_stack_pointer) {
  // Move the stack pointer to the bottom of the page that it's in.
  const uintptr_t page_size = getpagesize();

  uint8_t* const stack_pointer =
      reinterpret_cast<uint8_t*>(int_stack_pointer & ~(page_size - 1));

  // The number of bytes of stack which we try to capture.
  static const ptrdiff_t kStackToCapture = 32 * 1024;

  const MappingInfo* mapping = FindMapping(stack_pointer);
  if (!mapping)
    return false;
  const ptrdiff_t offset = stack_pointer - (uint8_t*) mapping->start_addr;
  const ptrdiff_t distance_to_end =
      static_cast<ptrdiff_t>(mapping->size) - offset;
  *stack_len = distance_to_end > kStackToCapture ?
      kStackToCapture : distance_to_end;
  *stack = stack_pointer;
  return true;
}

// Find the mapping which the given memory address falls in.
const MappingInfo* LinuxDumper::FindMapping(const void* address) const {
  const uintptr_t addr = (uintptr_t) address;

  for (size_t i = 0; i < mappings_.size(); ++i) {
    const uintptr_t start = static_cast<uintptr_t>(mappings_[i]->start_addr);
    if (addr >= start && addr - start < mappings_[i]->size)
      return mappings_[i];
  }

  return NULL;
}

bool LinuxDumper::HandleDeletedFileInMapping(char* path) const {
  static const size_t kDeletedSuffixLen = sizeof(kDeletedSuffix) - 1;

  // Check for ' (deleted)' in |path|.
  // |path| has to be at least as long as "/x (deleted)".
  const size_t path_len = my_strlen(path);
  if (path_len < kDeletedSuffixLen + 2)
    return false;
  if (my_strncmp(path + path_len - kDeletedSuffixLen, kDeletedSuffix,
                 kDeletedSuffixLen) != 0) {
    return false;
  }

  // Check |path| against the /proc/pid/exe 'symlink'.
  char exe_link[NAME_MAX];
  char new_path[NAME_MAX];
  if (!BuildProcPath(exe_link, pid_, "exe"))
    return false;
  if (!SafeReadLink(exe_link, new_path))
    return false;
  if (my_strcmp(path, new_path) != 0)
    return false;

  // Check to see if someone actually named their executable 'foo (deleted)'.
  struct kernel_stat exe_stat;
  struct kernel_stat new_path_stat;
  if (sys_stat(exe_link, &exe_stat) == 0 &&
      sys_stat(new_path, &new_path_stat) == 0 &&
      exe_stat.st_dev == new_path_stat.st_dev &&
      exe_stat.st_ino == new_path_stat.st_ino) {
    return false;
  }

  memcpy(path, exe_link, NAME_MAX);
  return true;
}

}  // namespace google_breakpad
