// Copyright (c) 2006, Google Inc.
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
// file_id.cc: Return a unique identifier for a file
//
// See file_id.h for documentation
//

#include "common/linux/file_id.h"

#include <arpa/inet.h>
#include <assert.h>
#if defined(__ANDROID__)
#include <linux/elf.h>
#include "client/linux/android_link.h"
#else
#include <elf.h>
#include <link.h>
#endif
#include <string.h>

#include <algorithm>

#include "common/linux/linux_libc_support.h"
#include "common/linux/memory_mapped_file.h"
#include "third_party/lss/linux_syscall_support.h"

namespace google_breakpad {

#ifndef NT_GNU_BUILD_ID
#define NT_GNU_BUILD_ID 3
#endif

FileID::FileID(const char* path) {
  strncpy(path_, path, sizeof(path_));
}

struct ElfClass32 {
  typedef Elf32_Ehdr Ehdr;
  typedef Elf32_Nhdr Nhdr;
  typedef Elf32_Shdr Shdr;
  static const int kClass = ELFCLASS32;
};

struct ElfClass64 {
  typedef Elf64_Ehdr Ehdr;
  typedef Elf64_Nhdr Nhdr;
  typedef Elf64_Shdr Shdr;
  static const int kClass = ELFCLASS64;
};

// These six functions are also used inside the crashed process, so be safe
// and use the syscall/libc wrappers instead of direct syscalls or libc.
template<typename ElfClass>
static void FindElfClassSection(const char *elf_base,
                                const char *section_name,
                                uint32_t section_type,
                                const void **section_start,
                                int *section_size) {
  typedef typename ElfClass::Ehdr Ehdr;
  typedef typename ElfClass::Shdr Shdr;

  assert(elf_base);
  assert(section_start);
  assert(section_size);

  assert(my_strncmp(elf_base, ELFMAG, SELFMAG) == 0);

  int name_len = my_strlen(section_name);

  const Ehdr* elf_header = reinterpret_cast<const Ehdr*>(elf_base);
  assert(elf_header->e_ident[EI_CLASS] == ElfClass::kClass);

  const Shdr* sections =
      reinterpret_cast<const Shdr*>(elf_base + elf_header->e_shoff);
  const Shdr* string_section = sections + elf_header->e_shstrndx;

  const Shdr* section = NULL;
  for (int i = 0; i < elf_header->e_shnum; ++i) {
    if (sections[i].sh_type == section_type) {
      const char* current_section_name = (char*)(elf_base +
                                                 string_section->sh_offset +
                                                 sections[i].sh_name);
      if (!my_strncmp(current_section_name, section_name, name_len)) {
        section = &sections[i];
        break;
      }
    }
  }
  if (section != NULL && section->sh_size > 0) {
    *section_start = elf_base + section->sh_offset;
    *section_size = section->sh_size;
  }
}

// Attempt to find a section named |section_name| of type |section_type|
// in the ELF binary data at |elf_mapped_base|. On success, returns true
// and sets |*section_start| to point to the start of the section data,
// and |*section_size| to the size of the section's data. If |elfclass|
// is not NULL, set |*elfclass| to the ELF file class.
static bool FindElfSection(const void *elf_mapped_base,
                           const char *section_name,
                           uint32_t section_type,
                           const void **section_start,
                           int *section_size,
                           int *elfclass) {
  assert(elf_mapped_base);
  assert(section_start);
  assert(section_size);

  *section_start = NULL;
  *section_size = 0;

  const char* elf_base =
    static_cast<const char*>(elf_mapped_base);
  const ElfW(Ehdr)* elf_header =
    reinterpret_cast<const ElfW(Ehdr)*>(elf_base);
  if (my_strncmp(elf_base, ELFMAG, SELFMAG) != 0)
    return false;

  if (elfclass) {
    *elfclass = elf_header->e_ident[EI_CLASS];
  }

  if (elf_header->e_ident[EI_CLASS] == ELFCLASS32) {
    FindElfClassSection<ElfClass32>(elf_base, section_name, section_type,
                                    section_start, section_size);
    return *section_start != NULL;
  } else if (elf_header->e_ident[EI_CLASS] == ELFCLASS64) {
    FindElfClassSection<ElfClass64>(elf_base, section_name, section_type,
                                    section_start, section_size);
    return *section_start != NULL;
  }

  return false;
}

template<typename ElfClass>
static bool ElfClassBuildIDNoteIdentifier(const void *section,
                                          uint8_t identifier[kMDGUIDSize]) {
  typedef typename ElfClass::Nhdr Nhdr;

  const Nhdr* note_header = reinterpret_cast<const Nhdr*>(section);
  if (note_header->n_type != NT_GNU_BUILD_ID ||
      note_header->n_descsz == 0) {
    return false;
  }

  const char* build_id = reinterpret_cast<const char*>(section) +
    sizeof(Nhdr) + note_header->n_namesz;
  // Copy as many bits of the build ID as will fit
  // into the GUID space.
  my_memset(identifier, 0, kMDGUIDSize);
  memcpy(identifier, build_id,
         std::min(kMDGUIDSize, (size_t)note_header->n_descsz));

  return true;
}

// Attempt to locate a .note.gnu.build-id section in an ELF binary
// and copy as many bytes of it as will fit into |identifier|.
static bool FindElfBuildIDNote(const void *elf_mapped_base,
                               uint8_t identifier[kMDGUIDSize]) {
  void* note_section;
  int note_size, elfclass;
  if (!FindElfSection(elf_mapped_base, ".note.gnu.build-id", SHT_NOTE,
                      (const void**)&note_section, &note_size, &elfclass) ||
      note_size == 0) {
    return false;
  }

  if (elfclass == ELFCLASS32) {
    return ElfClassBuildIDNoteIdentifier<ElfClass32>(note_section, identifier);
  } else if (elfclass == ELFCLASS64) {
    return ElfClassBuildIDNoteIdentifier<ElfClass64>(note_section, identifier);
  }

  return false;
}

// Attempt to locate the .text section of an ELF binary and generate
// a simple hash by XORing the first page worth of bytes into |identifier|.
static bool HashElfTextSection(const void *elf_mapped_base,
                               uint8_t identifier[kMDGUIDSize]) {
  void* text_section;
  int text_size;
  if (!FindElfSection(elf_mapped_base, ".text", SHT_PROGBITS,
                      (const void**)&text_section, &text_size, NULL) ||
      text_size == 0) {
    return false;
  }

  my_memset(identifier, 0, kMDGUIDSize);
  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(text_section);
  const uint8_t* ptr_end = ptr + std::min(text_size, 4096);
  while (ptr < ptr_end) {
    for (unsigned i = 0; i < kMDGUIDSize; i++)
      identifier[i] ^= ptr[i];
    ptr += kMDGUIDSize;
  }
  return true;
}

// static
bool FileID::ElfFileIdentifierFromMappedFile(const void* base,
                                             uint8_t identifier[kMDGUIDSize]) {
  // Look for a build id note first.
  if (FindElfBuildIDNote(base, identifier))
    return true;

  // Fall back on hashing the first page of the text section.
  return HashElfTextSection(base, identifier);
}

bool FileID::ElfFileIdentifier(uint8_t identifier[kMDGUIDSize]) {
  MemoryMappedFile mapped_file(path_);
  if (!mapped_file.data())  // Should probably check if size >= ElfW(Ehdr)?
    return false;

  return ElfFileIdentifierFromMappedFile(mapped_file.data(), identifier);
}

// static
void FileID::ConvertIdentifierToString(const uint8_t identifier[kMDGUIDSize],
                                       char* buffer, int buffer_length) {
  uint8_t identifier_swapped[kMDGUIDSize];

  // Endian-ness swap to match dump processor expectation.
  memcpy(identifier_swapped, identifier, kMDGUIDSize);
  uint32_t* data1 = reinterpret_cast<uint32_t*>(identifier_swapped);
  *data1 = htonl(*data1);
  uint16_t* data2 = reinterpret_cast<uint16_t*>(identifier_swapped + 4);
  *data2 = htons(*data2);
  uint16_t* data3 = reinterpret_cast<uint16_t*>(identifier_swapped + 6);
  *data3 = htons(*data3);

  int buffer_idx = 0;
  for (unsigned int idx = 0;
       (buffer_idx < buffer_length) && (idx < kMDGUIDSize);
       ++idx) {
    int hi = (identifier_swapped[idx] >> 4) & 0x0F;
    int lo = (identifier_swapped[idx]) & 0x0F;

    if (idx == 4 || idx == 6 || idx == 8 || idx == 10)
      buffer[buffer_idx++] = '-';

    buffer[buffer_idx++] = (hi >= 10) ? 'A' + hi - 10 : '0' + hi;
    buffer[buffer_idx++] = (lo >= 10) ? 'A' + lo - 10 : '0' + lo;
  }

  // NULL terminate
  buffer[(buffer_idx < buffer_length) ? buffer_idx : buffer_idx - 1] = 0;
}

}  // namespace google_breakpad
