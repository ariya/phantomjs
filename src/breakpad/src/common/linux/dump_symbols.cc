// Copyright (c) 2011 Google Inc.
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

// Restructured in 2009 by: Jim Blandy <jimb@mozilla.com> <jimb@red-bean.com>

// dump_symbols.cc: implement google_breakpad::WriteSymbolFile:
// Find all the debugging info in a file and dump it as a Breakpad symbol file.

#include "common/linux/dump_symbols.h"

#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <link.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "common/dwarf/bytereader-inl.h"
#include "common/dwarf/dwarf2diehandler.h"
#include "common/dwarf_cfi_to_module.h"
#include "common/dwarf_cu_to_module.h"
#include "common/dwarf_line_to_module.h"
#include "common/linux/elf_symbols_to_module.h"
#include "common/linux/file_id.h"
#include "common/module.h"
#include "common/stabs_reader.h"
#include "common/stabs_to_module.h"

// This namespace contains helper functions.
namespace {

using google_breakpad::DwarfCFIToModule;
using google_breakpad::DwarfCUToModule;
using google_breakpad::DwarfLineToModule;
using google_breakpad::Module;
using google_breakpad::StabsToModule;

//
// FDWrapper
//
// Wrapper class to make sure opened file is closed.
//
class FDWrapper {
 public:
  explicit FDWrapper(int fd) :
    fd_(fd) {}
  ~FDWrapper() {
    if (fd_ != -1)
      close(fd_);
  }
  int get() {
    return fd_;
  }
  int release() {
    int fd = fd_;
    fd_ = -1;
    return fd;
  }
 private:
  int fd_;
};

//
// MmapWrapper
//
// Wrapper class to make sure mapped regions are unmapped.
//
class MmapWrapper {
 public:
  MmapWrapper() : is_set_(false) {}
  ~MmapWrapper() {
    assert(is_set_);
    if (base_ != NULL) {
      assert(size_ > 0);
      munmap(base_, size_);
    }
  }
  void set(void *mapped_address, size_t mapped_size) {
    is_set_ = true;
    base_ = mapped_address;
    size_ = mapped_size;
  }
  void release() {
    assert(is_set_);
    base_ = NULL;
    size_ = 0;
  }

 private:
  bool is_set_;
  void *base_;
  size_t size_;
};


// Fix offset into virtual address by adding the mapped base into offsets.
// Make life easier when want to find something by offset.
static void FixAddress(void *obj_base) {
  ElfW(Addr) base = reinterpret_cast<ElfW(Addr)>(obj_base);
  ElfW(Ehdr) *elf_header = static_cast<ElfW(Ehdr) *>(obj_base);
  elf_header->e_phoff += base;
  elf_header->e_shoff += base;
  ElfW(Shdr) *sections = reinterpret_cast<ElfW(Shdr) *>(elf_header->e_shoff);
  for (int i = 0; i < elf_header->e_shnum; ++i)
    sections[i].sh_offset += base;
}

// Find the preferred loading address of the binary.
static ElfW(Addr) GetLoadingAddress(const ElfW(Phdr) *program_headers,
                                    int nheader) {
  for (int i = 0; i < nheader; ++i) {
    const ElfW(Phdr) &header = program_headers[i];
    // For executable, it is the PT_LOAD segment with offset to zero.
    if (header.p_type == PT_LOAD &&
        header.p_offset == 0)
      return header.p_vaddr;
  }
  // For other types of ELF, return 0.
  return 0;
}

static bool IsValidElf(const ElfW(Ehdr) *elf_header) {
  return memcmp(elf_header, ELFMAG, SELFMAG) == 0;
}

static const ElfW(Shdr) *FindSectionByName(const char *name,
                                           const ElfW(Shdr) *sections,
                                           const ElfW(Shdr) *section_names,
                                           int nsection) {
  assert(name != NULL);
  assert(sections != NULL);
  assert(nsection > 0);

  int name_len = strlen(name);
  if (name_len == 0)
    return NULL;

  // Find the end of the section name section, to make sure that
  // comparisons don't run off the end of the section.
  const char *names_end =
    reinterpret_cast<char*>(section_names->sh_offset + section_names->sh_size);

  for (int i = 0; i < nsection; ++i) {
    const char *section_name =
      reinterpret_cast<char*>(section_names->sh_offset + sections[i].sh_name);
    if (names_end - section_name >= name_len + 1 &&
        strcmp(name, section_name) == 0) {
      if (sections[i].sh_type == SHT_NOBITS) {
        fprintf(stderr,
                "Section %s found, but ignored because type=SHT_NOBITS.\n",
                name);
        return NULL;
      }
      return sections + i;
    }
  }
  return NULL;
}

static bool LoadStabs(const ElfW(Ehdr) *elf_header,
                      const ElfW(Shdr) *stab_section,
                      const ElfW(Shdr) *stabstr_section,
                      const bool big_endian,
                      Module *module) {
  // A callback object to handle data from the STABS reader.
  StabsToModule handler(module);
  // Find the addresses of the STABS data, and create a STABS reader object.
  // On Linux, STABS entries always have 32-bit values, regardless of the
  // address size of the architecture whose code they're describing, and
  // the strings are always "unitized".
  uint8_t *stabs = reinterpret_cast<uint8_t *>(stab_section->sh_offset);
  uint8_t *stabstr = reinterpret_cast<uint8_t *>(stabstr_section->sh_offset);
  google_breakpad::StabsReader reader(stabs, stab_section->sh_size,
                                      stabstr, stabstr_section->sh_size,
                                      big_endian, 4, true, &handler);
  // Read the STABS data, and do post-processing.
  if (!reader.Process())
    return false;
  handler.Finalize();
  return true;
}

// A line-to-module loader that accepts line number info parsed by
// dwarf2reader::LineInfo and populates a Module and a line vector
// with the results.
class DumperLineToModule: public DwarfCUToModule::LineToModuleFunctor {
 public:
  // Create a line-to-module converter using BYTE_READER.
  explicit DumperLineToModule(dwarf2reader::ByteReader *byte_reader)
      : byte_reader_(byte_reader) { }
  void operator()(const char *program, uint64 length,
                  Module *module, std::vector<Module::Line> *lines) {
    DwarfLineToModule handler(module, lines);
    dwarf2reader::LineInfo parser(program, length, byte_reader_, &handler);
    parser.Start();
  }
 private:
  dwarf2reader::ByteReader *byte_reader_;
};

static bool LoadDwarf(const std::string &dwarf_filename,
                      const ElfW(Ehdr) *elf_header,
                      const bool big_endian,
                      Module *module) {
  const dwarf2reader::Endianness endianness = big_endian ?
      dwarf2reader::ENDIANNESS_BIG : dwarf2reader::ENDIANNESS_LITTLE;
  dwarf2reader::ByteReader byte_reader(endianness);

  // Construct a context for this file.
  DwarfCUToModule::FileContext file_context(dwarf_filename, module);

  // Build a map of the ELF file's sections.
  const ElfW(Shdr) *sections
      = reinterpret_cast<ElfW(Shdr) *>(elf_header->e_shoff);
  int num_sections = elf_header->e_shnum;
  const ElfW(Shdr) *section_names = sections + elf_header->e_shstrndx;
  for (int i = 0; i < num_sections; i++) {
    const ElfW(Shdr) *section = &sections[i];
    std::string name = reinterpret_cast<const char *>(section_names->sh_offset +
                                                      section->sh_name);
    const char *contents = reinterpret_cast<const char *>(section->sh_offset);
    uint64 length = section->sh_size;
    file_context.section_map[name] = std::make_pair(contents, length);
  }

  // Parse all the compilation units in the .debug_info section.
  DumperLineToModule line_to_module(&byte_reader);
  std::pair<const char *, uint64> debug_info_section
      = file_context.section_map[".debug_info"];
  // We should never have been called if the file doesn't have a
  // .debug_info section.
  assert(debug_info_section.first);
  uint64 debug_info_length = debug_info_section.second;
  for (uint64 offset = 0; offset < debug_info_length;) {
    // Make a handler for the root DIE that populates MODULE with the
    // data we find.
    DwarfCUToModule::WarningReporter reporter(dwarf_filename, offset);
    DwarfCUToModule root_handler(&file_context, &line_to_module, &reporter);
    // Make a Dwarf2Handler that drives our DIEHandler.
    dwarf2reader::DIEDispatcher die_dispatcher(&root_handler);
    // Make a DWARF parser for the compilation unit at OFFSET.
    dwarf2reader::CompilationUnit reader(file_context.section_map,
                                         offset,
                                         &byte_reader,
                                         &die_dispatcher);
    // Process the entire compilation unit; get the offset of the next.
    offset += reader.Start();
  }
  return true;
}

// Fill REGISTER_NAMES with the register names appropriate to the
// machine architecture given in HEADER, indexed by the register
// numbers used in DWARF call frame information. Return true on
// success, or false if we don't recognize HEADER's machine
// architecture.
static bool DwarfCFIRegisterNames(const ElfW(Ehdr) *elf_header,
                                  std::vector<std::string> *register_names) {
  switch (elf_header->e_machine) {
    case EM_386:
      *register_names = DwarfCFIToModule::RegisterNames::I386();
      return true;
    case EM_ARM:
      *register_names = DwarfCFIToModule::RegisterNames::ARM();
      return true;
    case EM_X86_64:
      *register_names = DwarfCFIToModule::RegisterNames::X86_64();
      return true;
    default:
      return false;
  }
}

static bool LoadDwarfCFI(const std::string &dwarf_filename,
                         const ElfW(Ehdr) *elf_header,
                         const char *section_name,
                         const ElfW(Shdr) *section,
                         const bool eh_frame,
                         const ElfW(Shdr) *got_section,
                         const ElfW(Shdr) *text_section,
                         const bool big_endian,
                         Module *module) {
  // Find the appropriate set of register names for this file's
  // architecture.
  std::vector<std::string> register_names;
  if (!DwarfCFIRegisterNames(elf_header, &register_names)) {
    fprintf(stderr, "%s: unrecognized ELF machine architecture '%d';"
            " cannot convert DWARF call frame information\n",
            dwarf_filename.c_str(), elf_header->e_machine);
    return false;
  }

  const dwarf2reader::Endianness endianness = big_endian ?
      dwarf2reader::ENDIANNESS_BIG : dwarf2reader::ENDIANNESS_LITTLE;

  // Find the call frame information and its size.
  const char *cfi = reinterpret_cast<const char *>(section->sh_offset);
  size_t cfi_size = section->sh_size;

  // Plug together the parser, handler, and their entourages.
  DwarfCFIToModule::Reporter module_reporter(dwarf_filename, section_name);
  DwarfCFIToModule handler(module, register_names, &module_reporter);
  dwarf2reader::ByteReader byte_reader(endianness);
  // Since we're using the ElfW macro, we're not actually capable of
  // processing both ELF32 and ELF64 files with the same program; that
  // would take a bit more work. But this will work out well enough.
  if (elf_header->e_ident[EI_CLASS] == ELFCLASS32)
    byte_reader.SetAddressSize(4);
  else if (elf_header->e_ident[EI_CLASS] == ELFCLASS64)
    byte_reader.SetAddressSize(8);
  else {
    fprintf(stderr, "%s: bad file class in ELF header: %d\n",
            dwarf_filename.c_str(), elf_header->e_ident[EI_CLASS]);
    return false;
  }
  // Provide the base addresses for .eh_frame encoded pointers, if
  // possible.
  byte_reader.SetCFIDataBase(section->sh_addr, cfi);
  if (got_section)
    byte_reader.SetDataBase(got_section->sh_addr);
  if (text_section)
    byte_reader.SetTextBase(text_section->sh_addr);

  dwarf2reader::CallFrameInfo::Reporter dwarf_reporter(dwarf_filename,
                                                       section_name);
  dwarf2reader::CallFrameInfo parser(cfi, cfi_size,
                                     &byte_reader, &handler, &dwarf_reporter,
                                     eh_frame);
  parser.Start();
  return true;
}

bool LoadELF(const std::string &obj_file, MmapWrapper* map_wrapper,
             ElfW(Ehdr) **elf_header) {
  int obj_fd = open(obj_file.c_str(), O_RDONLY);
  if (obj_fd < 0) {
    fprintf(stderr, "Failed to open ELF file '%s': %s\n",
            obj_file.c_str(), strerror(errno));
    return false;
  }
  FDWrapper obj_fd_wrapper(obj_fd);
  struct stat st;
  if (fstat(obj_fd, &st) != 0 && st.st_size <= 0) {
    fprintf(stderr, "Unable to fstat ELF file '%s': %s\n",
            obj_file.c_str(), strerror(errno));
    return false;
  }
  void *obj_base = mmap(NULL, st.st_size,
                        PROT_READ | PROT_WRITE, MAP_PRIVATE, obj_fd, 0);
  if (obj_base == MAP_FAILED) {
    fprintf(stderr, "Failed to mmap ELF file '%s': %s\n",
            obj_file.c_str(), strerror(errno));
    return false;
  }
  map_wrapper->set(obj_base, st.st_size);
  *elf_header = reinterpret_cast<ElfW(Ehdr) *>(obj_base);
  if (!IsValidElf(*elf_header)) {
    fprintf(stderr, "Not a valid ELF file: %s\n", obj_file.c_str());
    return false;
  }
  return true;
}

// Get the endianness of ELF_HEADER. If it's invalid, return false.
bool ElfEndianness(const ElfW(Ehdr) *elf_header, bool *big_endian) {
  if (elf_header->e_ident[EI_DATA] == ELFDATA2LSB) {
    *big_endian = false;
    return true;
  }
  if (elf_header->e_ident[EI_DATA] == ELFDATA2MSB) {
    *big_endian = true;
    return true;
  }

  fprintf(stderr, "bad data encoding in ELF header: %d\n",
          elf_header->e_ident[EI_DATA]);
  return false;
}

// Read the .gnu_debuglink and get the debug file name. If anything goes
// wrong, return an empty string.
static std::string ReadDebugLink(const ElfW(Shdr) *debuglink_section,
                                 const std::string &obj_file,
                                 const std::string &debug_dir) {
  char *debuglink = reinterpret_cast<char *>(debuglink_section->sh_offset);
  size_t debuglink_len = strlen(debuglink) + 5;  // '\0' + CRC32.
  debuglink_len = 4 * ((debuglink_len + 3) / 4);  // Round to nearest 4 bytes.

  // Sanity check.
  if (debuglink_len != debuglink_section->sh_size) {
    fprintf(stderr, "Mismatched .gnu_debuglink string / section size: "
            "%zx %zx\n", debuglink_len, debuglink_section->sh_size);
    return "";
  }

  std::string debuglink_path = debug_dir + "/" + debuglink;
  int debuglink_fd = open(debuglink_path.c_str(), O_RDONLY);
  if (debuglink_fd < 0) {
    fprintf(stderr, "Failed to open debug ELF file '%s' for '%s': %s\n",
            debuglink_path.c_str(), obj_file.c_str(), strerror(errno));
    return "";
  }
  FDWrapper debuglink_fd_wrapper(debuglink_fd);
  // TODO(thestig) check the CRC-32 at the end of the .gnu_debuglink
  // section.

  return debuglink_path;
}

//
// LoadSymbolsInfo
//
// Holds the state between the two calls to LoadSymbols() in case we have to
// follow the .gnu_debuglink section and load debug information from a
// different file.
//
class LoadSymbolsInfo {
 public:
  explicit LoadSymbolsInfo(const std::string &dbg_dir) :
    debug_dir_(dbg_dir),
    has_loading_addr_(false) {}

  // Keeps track of which sections have been loaded so we don't accidentally
  // load it twice from two different files.
  void LoadedSection(const std::string &section) {
    if (loaded_sections_.count(section) == 0) {
      loaded_sections_.insert(section);
    } else {
      fprintf(stderr, "Section %s has already been loaded.\n",
              section.c_str());
    }
  }

  // We expect the ELF file and linked debug file to have the same preferred
  // loading address.
  void set_loading_addr(ElfW(Addr) addr, const std::string &filename) {
    if (!has_loading_addr_) {
      loading_addr_ = addr;
      loaded_file_ = filename;
      return;
    }

    if (addr != loading_addr_) {
      fprintf(stderr,
              "ELF file '%s' and debug ELF file '%s' "
              "have different load addresses.\n",
              loaded_file_.c_str(), filename.c_str());
      assert(false);
    }
  }

  // Setters and getters
  const std::string &debug_dir() const {
    return debug_dir_;
  }

  std::string debuglink_file() const {
    return debuglink_file_;
  }
  void set_debuglink_file(std::string file) {
    debuglink_file_ = file;
  }

 private:
  const std::string &debug_dir_;  // Directory with the debug ELF file.

  std::string debuglink_file_;  // Full path to the debug ELF file.

  bool has_loading_addr_;  // Indicate if LOADING_ADDR_ is valid.

  ElfW(Addr) loading_addr_;  // Saves the preferred loading address from the
                             // first call to LoadSymbols().

  std::string loaded_file_;  // Name of the file loaded from the first call to
                             // LoadSymbols().

  std::set<std::string> loaded_sections_;  // Tracks the Loaded ELF sections
                                           // between calls to LoadSymbols().
};

static bool LoadSymbols(const std::string &obj_file,
                        const bool big_endian,
                        ElfW(Ehdr) *elf_header,
                        const bool read_gnu_debug_link,
                        LoadSymbolsInfo *info,
                        Module *module) {
  // Translate all offsets in section headers into address.
  FixAddress(elf_header);
  ElfW(Addr) loading_addr = GetLoadingAddress(
      reinterpret_cast<ElfW(Phdr) *>(elf_header->e_phoff),
      elf_header->e_phnum);
  module->SetLoadAddress(loading_addr);
  info->set_loading_addr(loading_addr, obj_file);

  const ElfW(Shdr) *sections =
      reinterpret_cast<ElfW(Shdr) *>(elf_header->e_shoff);
  const ElfW(Shdr) *section_names = sections + elf_header->e_shstrndx;
  bool found_debug_info_section = false;
  bool found_usable_info = false;

  // Look for STABS debugging information, and load it if present.
  const ElfW(Shdr) *stab_section
      = FindSectionByName(".stab", sections, section_names,
                          elf_header->e_shnum);
  if (stab_section) {
    const ElfW(Shdr) *stabstr_section = stab_section->sh_link + sections;
    if (stabstr_section) {
      found_debug_info_section = true;
      found_usable_info = true;
      info->LoadedSection(".stab");
      if (!LoadStabs(elf_header, stab_section, stabstr_section, big_endian,
                     module)) {
        fprintf(stderr, "%s: \".stab\" section found, but failed to load STABS"
                " debugging information\n", obj_file.c_str());
      }
    }
  }

  // Look for DWARF debugging information, and load it if present.
  const ElfW(Shdr) *dwarf_section
      = FindSectionByName(".debug_info", sections, section_names,
                          elf_header->e_shnum);
  if (dwarf_section) {
    found_debug_info_section = true;
    found_usable_info = true;
    info->LoadedSection(".debug_info");
    if (!LoadDwarf(obj_file, elf_header, big_endian, module))
      fprintf(stderr, "%s: \".debug_info\" section found, but failed to load "
              "DWARF debugging information\n", obj_file.c_str());
  }

  // Dwarf Call Frame Information (CFI) is actually independent from
  // the other DWARF debugging information, and can be used alone.
  const ElfW(Shdr) *dwarf_cfi_section =
      FindSectionByName(".debug_frame", sections, section_names,
                          elf_header->e_shnum);
  if (dwarf_cfi_section) {
    // Ignore the return value of this function; even without call frame
    // information, the other debugging information could be perfectly
    // useful.
    info->LoadedSection(".debug_frame");
    bool result =
      LoadDwarfCFI(obj_file, elf_header, ".debug_frame",
                   dwarf_cfi_section, false, 0, 0, big_endian, module);
    found_usable_info = found_usable_info || result;
  }

  // Linux C++ exception handling information can also provide
  // unwinding data.
  const ElfW(Shdr) *eh_frame_section =
      FindSectionByName(".eh_frame", sections, section_names,
                        elf_header->e_shnum);
  if (eh_frame_section) {
    // Pointers in .eh_frame data may be relative to the base addresses of
    // certain sections. Provide those sections if present.
    const ElfW(Shdr) *got_section =
      FindSectionByName(".got", sections, section_names, elf_header->e_shnum);
    const ElfW(Shdr) *text_section =
      FindSectionByName(".text", sections, section_names,
                        elf_header->e_shnum);
    info->LoadedSection(".eh_frame");
    // As above, ignore the return value of this function.
    bool result =
      LoadDwarfCFI(obj_file, elf_header, ".eh_frame", eh_frame_section, true,
                   got_section, text_section, big_endian, module);
    found_usable_info = found_usable_info || result;
  }

  if (!found_debug_info_section) {
    fprintf(stderr, "%s: file contains no debugging information"
            " (no \".stab\" or \".debug_info\" sections)\n",
            obj_file.c_str());

    // Failed, but maybe we can find a .gnu_debuglink section?
    if (read_gnu_debug_link) {
      const ElfW(Shdr) *gnu_debuglink_section
          = FindSectionByName(".gnu_debuglink", sections, section_names,
                              elf_header->e_shnum);
      if (gnu_debuglink_section) {
        if (!info->debug_dir().empty()) {
          std::string debuglink_file =
              ReadDebugLink(gnu_debuglink_section, obj_file, info->debug_dir());
          info->set_debuglink_file(debuglink_file);
        } else {
          fprintf(stderr, ".gnu_debuglink section found in '%s', "
                  "but no debug path specified.\n", obj_file.c_str());
        }
      } else {
        fprintf(stderr, "%s does not contain a .gnu_debuglink section.\n",
                obj_file.c_str());
      }
    } else {
      // The caller doesn't want to consult .gnu_debuglink.
      // See if there are export symbols available.
      const ElfW(Shdr) *dynsym_section =
        FindSectionByName(".dynsym", sections, section_names,
                          elf_header->e_shnum);
      const ElfW(Shdr) *dynstr_section =
        FindSectionByName(".dynstr", sections, section_names,
                          elf_header->e_shnum);
      if (dynsym_section && dynstr_section) {
        info->LoadedSection(".dynsym");
        fprintf(stderr, "Have .dynsym + .dynstr\n");

        uint8_t* dynsyms =
          reinterpret_cast<uint8_t*>(dynsym_section->sh_offset);
        uint8_t* dynstrs =
          reinterpret_cast<uint8_t*>(dynstr_section->sh_offset);
        bool result =
          ELFSymbolsToModule(dynsyms,
                             dynsym_section->sh_size,
                             dynstrs,
                             dynstr_section->sh_size,
                             big_endian,
                             // This could change to something more useful
                             // when support for dumping cross-architecture
                             // symbols is finished.
                             sizeof(ElfW(Addr)),
                             module);
        found_usable_info = found_usable_info || result;
      }

      // Return true if some usable information was found, since
      // the caller doesn't want to use .gnu_debuglink.
      return found_usable_info;
    }

    // No debug info was found, let the user try again with .gnu_debuglink
    // if present.
    return false;
  }

  return true;
}

// Return the breakpad symbol file identifier for the architecture of
// ELF_HEADER.
const char *ElfArchitecture(const ElfW(Ehdr) *elf_header) {
  ElfW(Half) arch = elf_header->e_machine;
  switch (arch) {
    case EM_386:        return "x86";
    case EM_ARM:        return "arm";
    case EM_MIPS:       return "mips";
    case EM_PPC64:      return "ppc64";
    case EM_PPC:        return "ppc";
    case EM_S390:       return "s390";
    case EM_SPARC:      return "sparc";
    case EM_SPARCV9:    return "sparcv9";
    case EM_X86_64:     return "x86_64";
    default: return NULL;
  }
}

// Format the Elf file identifier in IDENTIFIER as a UUID with the
// dashes removed.
std::string FormatIdentifier(unsigned char identifier[16]) {
  char identifier_str[40];
  google_breakpad::FileID::ConvertIdentifierToString(
      identifier,
      identifier_str,
      sizeof(identifier_str));
  std::string id_no_dash;
  for (int i = 0; identifier_str[i] != '\0'; ++i)
    if (identifier_str[i] != '-')
      id_no_dash += identifier_str[i];
  // Add an extra "0" by the end.  PDB files on Windows have an 'age'
  // number appended to the end of the file identifier; this isn't
  // really used or necessary on other platforms, but let's preserve
  // the pattern.
  id_no_dash += '0';
  return id_no_dash;
}

// Return the non-directory portion of FILENAME: the portion after the
// last slash, or the whole filename if there are no slashes.
std::string BaseFileName(const std::string &filename) {
  // Lots of copies!  basename's behavior is less than ideal.
  char *c_filename = strdup(filename.c_str());
  std::string base = basename(c_filename);
  free(c_filename);
  return base;
}

}  // namespace

namespace google_breakpad {

// Not explicitly exported, but not static so it can be used in unit tests.
// Ideally obj_file would be const, but internally this code does write
// to some ELF header fields to make its work simpler.
bool WriteSymbolFileInternal(uint8_t* obj_file,
                             const std::string &obj_filename,
                             const std::string &debug_dir,
                             bool cfi,
                             std::ostream &sym_stream) {
  ElfW(Ehdr) *elf_header = reinterpret_cast<ElfW(Ehdr) *>(obj_file);

  if (!IsValidElf(elf_header)) {
    fprintf(stderr, "Not a valid ELF file: %s\n", obj_filename.c_str());
    return false;
  }

  unsigned char identifier[16];
  if (!google_breakpad::FileID::ElfFileIdentifierFromMappedFile(elf_header,
                                                                identifier)) {
    fprintf(stderr, "%s: unable to generate file identifier\n",
            obj_filename.c_str());
    return false;
  }

  const char *architecture = ElfArchitecture(elf_header);
  if (!architecture) {
    fprintf(stderr, "%s: unrecognized ELF machine architecture: %d\n",
            obj_filename.c_str(), elf_header->e_machine);
    return false;
  }

  // Figure out what endianness this file is.
  bool big_endian;
  if (!ElfEndianness(elf_header, &big_endian))
    return false;

  std::string name = BaseFileName(obj_filename);
  std::string os = "Linux";
  std::string id = FormatIdentifier(identifier);

  LoadSymbolsInfo info(debug_dir);
  Module module(name, os, architecture, id);
  if (!LoadSymbols(obj_filename, big_endian, elf_header, !debug_dir.empty(),
                   &info, &module)) {
    const std::string debuglink_file = info.debuglink_file();
    if (debuglink_file.empty())
      return false;

    // Load debuglink ELF file.
    fprintf(stderr, "Found debugging info in %s\n", debuglink_file.c_str());
    MmapWrapper debug_map_wrapper;
    ElfW(Ehdr) *debug_elf_header = NULL;
    if (!LoadELF(debuglink_file, &debug_map_wrapper, &debug_elf_header))
      return false;
    // Sanity checks to make sure everything matches up.
    const char *debug_architecture = ElfArchitecture(debug_elf_header);
    if (!debug_architecture) {
      fprintf(stderr, "%s: unrecognized ELF machine architecture: %d\n",
              debuglink_file.c_str(), debug_elf_header->e_machine);
      return false;
    }
    if (strcmp(architecture, debug_architecture)) {
      fprintf(stderr, "%s with ELF machine architecture %s does not match "
              "%s with ELF architecture %s\n",
              debuglink_file.c_str(), debug_architecture,
              obj_filename.c_str(), architecture);
      return false;
    }

    bool debug_big_endian;
    if (!ElfEndianness(debug_elf_header, &debug_big_endian))
      return false;
    if (debug_big_endian != big_endian) {
      fprintf(stderr, "%s and %s does not match in endianness\n",
              obj_filename.c_str(), debuglink_file.c_str());
      return false;
    }

    if (!LoadSymbols(debuglink_file, debug_big_endian, debug_elf_header,
                     false, &info, &module)) {
      return false;
    }
  }
  if (!module.Write(sym_stream, cfi))
    return false;

  return true;
}

bool WriteSymbolFile(const std::string &obj_file,
                     const std::string &debug_dir,
                     bool cfi,
                     std::ostream &sym_stream) {
  MmapWrapper map_wrapper;
  ElfW(Ehdr) *elf_header = NULL;
  if (!LoadELF(obj_file, &map_wrapper, &elf_header))
    return false;

  return WriteSymbolFileInternal(reinterpret_cast<uint8_t*>(elf_header),
                                 obj_file, debug_dir, cfi, sym_stream);
}

}  // namespace google_breakpad
