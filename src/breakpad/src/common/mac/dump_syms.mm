// -*- mode: c++ -*-

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

// Author: Jim Blandy <jimb@mozilla.com> <jimb@red-bean.com>

// dump_syms.mm: Create a symbol file for use with minidumps

#include "common/mac/dump_syms.h"

#include <Foundation/Foundation.h>
#include <mach-o/arch.h>
#include <mach-o/fat.h>
#include <stdio.h>

#include <ostream>
#include <string>
#include <vector>

#include "common/dwarf/bytereader-inl.h"
#include "common/dwarf/dwarf2reader.h"
#include "common/dwarf_cfi_to_module.h"
#include "common/dwarf_cu_to_module.h"
#include "common/dwarf_line_to_module.h"
#include "common/mac/file_id.h"
#include "common/mac/arch_utilities.h"
#include "common/mac/macho_reader.h"
#include "common/module.h"
#include "common/scoped_ptr.h"
#include "common/stabs_reader.h"
#include "common/stabs_to_module.h"
#include "common/symbol_data.h"

#ifndef CPU_TYPE_ARM
#define CPU_TYPE_ARM (static_cast<cpu_type_t>(12))
#endif //  CPU_TYPE_ARM

#ifndef CPU_TYPE_ARM64
#define CPU_TYPE_ARM64 (static_cast<cpu_type_t>(16777228))
#endif  // CPU_TYPE_ARM64

using dwarf2reader::ByteReader;
using google_breakpad::DwarfCUToModule;
using google_breakpad::DwarfLineToModule;
using google_breakpad::FileID;
using google_breakpad::mach_o::FatReader;
using google_breakpad::mach_o::Section;
using google_breakpad::mach_o::Segment;
using google_breakpad::Module;
using google_breakpad::StabsReader;
using google_breakpad::StabsToModule;
using google_breakpad::scoped_ptr;
using std::make_pair;
using std::pair;
using std::string;
using std::vector;

namespace google_breakpad {

bool DumpSymbols::Read(NSString *filename) {
  if (![[NSFileManager defaultManager] fileExistsAtPath:filename]) {
    fprintf(stderr, "Object file does not exist: %s\n",
            [filename fileSystemRepresentation]);
    return false;
  }

  input_pathname_ = [filename retain];

  // Does this filename refer to a dSYM bundle?
  NSBundle *bundle = [NSBundle bundleWithPath:input_pathname_];

  if (bundle) {
    // Filenames referring to bundles usually have names of the form
    // "<basename>.dSYM"; however, if the user has specified a wrapper
    // suffix (the WRAPPER_SUFFIX and WRAPPER_EXTENSION build settings),
    // then the name may have the form "<basename>.<extension>.dSYM". In
    // either case, the resource name for the file containing the DWARF
    // info within the bundle is <basename>.
    //
    // Since there's no way to tell how much to strip off, remove one
    // extension at a time, and use the first one that
    // pathForResource:ofType:inDirectory likes.
    NSString *base_name = [input_pathname_ lastPathComponent];
    NSString *dwarf_resource;

    do {
      NSString *new_base_name = [base_name stringByDeletingPathExtension];

      // If stringByDeletingPathExtension returned the name unchanged, then
      // there's nothing more for us to strip off --- lose.
      if ([new_base_name isEqualToString:base_name]) {
        fprintf(stderr, "Unable to find DWARF-bearing file in bundle: %s\n",
                [input_pathname_ fileSystemRepresentation]);
        return false;
      }

      // Take the shortened result as our new base_name.
      base_name = new_base_name;

      // Try to find a DWARF resource in the bundle under the new base_name.
      dwarf_resource = [bundle pathForResource:base_name
                        ofType:nil inDirectory:@"DWARF"];
    } while (!dwarf_resource);

    object_filename_ = [dwarf_resource retain];
  } else {
    object_filename_ = [input_pathname_ retain];
  }

  // Read the file's contents into memory.
  //
  // The documentation for dataWithContentsOfMappedFile says:
  //
  //     Because of file mapping restrictions, this method should only be
  //     used if the file is guaranteed to exist for the duration of the
  //     data object’s existence. It is generally safer to use the
  //     dataWithContentsOfFile: method.
  //
  // I gather this means that OS X doesn't have (or at least, that method
  // doesn't use) a form of mapping like Linux's MAP_PRIVATE, where the
  // process appears to get its own copy of the data, and changes to the
  // file don't affect memory and vice versa).
  NSError *error;
  contents_ = [NSData dataWithContentsOfFile:object_filename_
                                     options:0
                                       error:&error];
  if (!contents_) {
    fprintf(stderr, "Error reading object file: %s: %s\n",
            [object_filename_ fileSystemRepresentation],
            [[error localizedDescription] UTF8String]);
    return false;
  }
  [contents_ retain];

  // Get the list of object files present in the file.
  FatReader::Reporter fat_reporter([object_filename_
                                    fileSystemRepresentation]);
  FatReader fat_reader(&fat_reporter);
  if (!fat_reader.Read(reinterpret_cast<const uint8_t *>([contents_ bytes]),
                       [contents_ length])) {
    return false;
  }

  // Get our own copy of fat_reader's object file list.
  size_t object_files_count;
  const struct fat_arch *object_files =
    fat_reader.object_files(&object_files_count);
  if (object_files_count == 0) {
    fprintf(stderr, "Fat binary file contains *no* architectures: %s\n",
            [object_filename_ fileSystemRepresentation]);
    return false;
  }
  object_files_.resize(object_files_count);
  memcpy(&object_files_[0], object_files,
         sizeof(struct fat_arch) * object_files_count);

  return true;
}

bool DumpSymbols::SetArchitecture(cpu_type_t cpu_type,
                                  cpu_subtype_t cpu_subtype) {
  // Find the best match for the architecture the user requested.
  const struct fat_arch *best_match
    = NXFindBestFatArch(cpu_type, cpu_subtype, &object_files_[0],
                        static_cast<uint32_t>(object_files_.size()));
  if (!best_match) return false;

  // Record the selected object file.
  selected_object_file_ = best_match;
  return true;
}

bool DumpSymbols::SetArchitecture(const std::string &arch_name) {
  bool arch_set = false;
  const NXArchInfo *arch_info =
      google_breakpad::BreakpadGetArchInfoFromName(arch_name.c_str());
  if (arch_info) {
    arch_set = SetArchitecture(arch_info->cputype, arch_info->cpusubtype);
  }
  return arch_set;
}

string DumpSymbols::Identifier() {
  FileID file_id([object_filename_ fileSystemRepresentation]);
  unsigned char identifier_bytes[16];
  cpu_type_t cpu_type = selected_object_file_->cputype;
  cpu_subtype_t cpu_subtype = selected_object_file_->cpusubtype;
  if (!file_id.MachoIdentifier(cpu_type, cpu_subtype, identifier_bytes)) {
    fprintf(stderr, "Unable to calculate UUID of mach-o binary %s!\n",
            [object_filename_ fileSystemRepresentation]);
    return "";
  }

  char identifier_string[40];
  FileID::ConvertIdentifierToString(identifier_bytes, identifier_string,
                                    sizeof(identifier_string));

  string compacted(identifier_string);
  for(size_t i = compacted.find('-'); i != string::npos;
      i = compacted.find('-', i))
    compacted.erase(i, 1);

  return compacted;
}

// A line-to-module loader that accepts line number info parsed by
// dwarf2reader::LineInfo and populates a Module and a line vector
// with the results.
class DumpSymbols::DumperLineToModule:
      public DwarfCUToModule::LineToModuleHandler {
 public:
  // Create a line-to-module converter using BYTE_READER.
  DumperLineToModule(dwarf2reader::ByteReader *byte_reader)
      : byte_reader_(byte_reader) { }

  void StartCompilationUnit(const string& compilation_dir) {
    compilation_dir_ = compilation_dir;
  }

  void ReadProgram(const char *program, uint64 length,
                   Module *module, vector<Module::Line> *lines) {
    DwarfLineToModule handler(module, compilation_dir_, lines);
    dwarf2reader::LineInfo parser(program, length, byte_reader_, &handler);
    parser.Start();
  }
 private:
  string compilation_dir_;
  dwarf2reader::ByteReader *byte_reader_;  // WEAK
};

bool DumpSymbols::ReadDwarf(google_breakpad::Module *module,
                            const mach_o::Reader &macho_reader,
                            const mach_o::SectionMap &dwarf_sections,
                            bool handle_inter_cu_refs) const {
  // Build a byte reader of the appropriate endianness.
  ByteReader byte_reader(macho_reader.big_endian()
                         ? dwarf2reader::ENDIANNESS_BIG
                         : dwarf2reader::ENDIANNESS_LITTLE);

  // Construct a context for this file.
  DwarfCUToModule::FileContext file_context(selected_object_name_,
                                            module,
                                            handle_inter_cu_refs);

  // Build a dwarf2reader::SectionMap from our mach_o::SectionMap.
  for (mach_o::SectionMap::const_iterator it = dwarf_sections.begin();
       it != dwarf_sections.end(); ++it) {
    file_context.AddSectionToSectionMap(
        it->first,
        reinterpret_cast<const char *>(it->second.contents.start),
        it->second.contents.Size());
  }

  // Find the __debug_info section.
  dwarf2reader::SectionMap::const_iterator debug_info_entry =
      file_context.section_map().find("__debug_info");
  assert(debug_info_entry != file_context.section_map().end());
  const std::pair<const char*, uint64>& debug_info_section =
      debug_info_entry->second;
  // There had better be a __debug_info section!
  if (!debug_info_section.first) {
    fprintf(stderr, "%s: __DWARF segment of file has no __debug_info section\n",
            selected_object_name_.c_str());
    return false;
  }

  // Build a line-to-module loader for the root handler to use.
  DumperLineToModule line_to_module(&byte_reader);

  // Walk the __debug_info section, one compilation unit at a time.
  uint64 debug_info_length = debug_info_section.second;
  for (uint64 offset = 0; offset < debug_info_length;) {
    // Make a handler for the root DIE that populates MODULE with the
    // debug info.
    DwarfCUToModule::WarningReporter reporter(selected_object_name_,
                                              offset);
    DwarfCUToModule root_handler(&file_context, &line_to_module, &reporter);
    // Make a Dwarf2Handler that drives our DIEHandler.
    dwarf2reader::DIEDispatcher die_dispatcher(&root_handler);
    // Make a DWARF parser for the compilation unit at OFFSET.
    dwarf2reader::CompilationUnit dwarf_reader(file_context.section_map(),
                                               offset,
                                               &byte_reader,
                                               &die_dispatcher);
    // Process the entire compilation unit; get the offset of the next.
    offset += dwarf_reader.Start();
  }

  return true;
}

bool DumpSymbols::ReadCFI(google_breakpad::Module *module,
                          const mach_o::Reader &macho_reader,
                          const mach_o::Section &section,
                          bool eh_frame) const {
  // Find the appropriate set of register names for this file's
  // architecture.
  vector<string> register_names;
  switch (macho_reader.cpu_type()) {
    case CPU_TYPE_X86:
      register_names = DwarfCFIToModule::RegisterNames::I386();
      break;
    case CPU_TYPE_X86_64:
      register_names = DwarfCFIToModule::RegisterNames::X86_64();
      break;
    case CPU_TYPE_ARM:
      register_names = DwarfCFIToModule::RegisterNames::ARM();
      break;
    case CPU_TYPE_ARM64:
      register_names = DwarfCFIToModule::RegisterNames::ARM64();
      break;
    default: {
      const NXArchInfo *arch = google_breakpad::BreakpadGetArchInfoFromCpuType(
          macho_reader.cpu_type(), macho_reader.cpu_subtype());
      fprintf(stderr, "%s: cannot convert DWARF call frame information for ",
              selected_object_name_.c_str());
      if (arch)
        fprintf(stderr, "architecture '%s'", arch->name);
      else
        fprintf(stderr, "architecture %d,%d",
                macho_reader.cpu_type(), macho_reader.cpu_subtype());
      fprintf(stderr, " to Breakpad symbol file: no register name table\n");
      return false;
    }
  }

  // Find the call frame information and its size.
  const char *cfi = reinterpret_cast<const char *>(section.contents.start);
  size_t cfi_size = section.contents.Size();

  // Plug together the parser, handler, and their entourages.
  DwarfCFIToModule::Reporter module_reporter(selected_object_name_,
                                             section.section_name);
  DwarfCFIToModule handler(module, register_names, &module_reporter);
  dwarf2reader::ByteReader byte_reader(macho_reader.big_endian() ?
                                       dwarf2reader::ENDIANNESS_BIG :
                                       dwarf2reader::ENDIANNESS_LITTLE);
  byte_reader.SetAddressSize(macho_reader.bits_64() ? 8 : 4);
  // At the moment, according to folks at Apple and some cursory
  // investigation, Mac OS X only uses DW_EH_PE_pcrel-based pointers, so
  // this is the only base address the CFI parser will need.
  byte_reader.SetCFIDataBase(section.address, cfi);

  dwarf2reader::CallFrameInfo::Reporter dwarf_reporter(selected_object_name_,
                                                       section.section_name);
  dwarf2reader::CallFrameInfo parser(cfi, cfi_size,
                                     &byte_reader, &handler, &dwarf_reporter,
                                     eh_frame);
  parser.Start();
  return true;
}

// A LoadCommandHandler that loads whatever debugging data it finds into a
// Module.
class DumpSymbols::LoadCommandDumper:
      public mach_o::Reader::LoadCommandHandler {
 public:
  // Create a load command dumper handling load commands from READER's
  // file, and adding data to MODULE.
  LoadCommandDumper(const DumpSymbols &dumper,
                    google_breakpad::Module *module,
                    const mach_o::Reader &reader,
                    SymbolData symbol_data,
                    bool handle_inter_cu_refs)
      : dumper_(dumper),
        module_(module),
        reader_(reader),
        symbol_data_(symbol_data),
        handle_inter_cu_refs_(handle_inter_cu_refs) { }

  bool SegmentCommand(const mach_o::Segment &segment);
  bool SymtabCommand(const ByteBuffer &entries, const ByteBuffer &strings);

 private:
  const DumpSymbols &dumper_;
  google_breakpad::Module *module_;  // WEAK
  const mach_o::Reader &reader_;
  const SymbolData symbol_data_;
  const bool handle_inter_cu_refs_;
};

bool DumpSymbols::LoadCommandDumper::SegmentCommand(const Segment &segment) {
  mach_o::SectionMap section_map;
  if (!reader_.MapSegmentSections(segment, &section_map))
    return false;

  if (segment.name == "__TEXT") {
    module_->SetLoadAddress(segment.vmaddr);
    if (symbol_data_ != NO_CFI) {
      mach_o::SectionMap::const_iterator eh_frame =
          section_map.find("__eh_frame");
      if (eh_frame != section_map.end()) {
        // If there is a problem reading this, don't treat it as a fatal error.
        dumper_.ReadCFI(module_, reader_, eh_frame->second, true);
      }
    }
    return true;
  }

  if (segment.name == "__DWARF") {
    if (symbol_data_ != ONLY_CFI) {
      if (!dumper_.ReadDwarf(module_, reader_, section_map,
                             handle_inter_cu_refs_)) {
        return false;
      }
    }
    if (symbol_data_ != NO_CFI) {
      mach_o::SectionMap::const_iterator debug_frame
          = section_map.find("__debug_frame");
      if (debug_frame != section_map.end()) {
        // If there is a problem reading this, don't treat it as a fatal error.
        dumper_.ReadCFI(module_, reader_, debug_frame->second, false);
      }
    }
  }

  return true;
}

bool DumpSymbols::LoadCommandDumper::SymtabCommand(const ByteBuffer &entries,
                                                   const ByteBuffer &strings) {
  StabsToModule stabs_to_module(module_);
  // Mac OS X STABS are never "unitized", and the size of the 'value' field
  // matches the address size of the executable.
  StabsReader stabs_reader(entries.start, entries.Size(),
                           strings.start, strings.Size(),
                           reader_.big_endian(),
                           reader_.bits_64() ? 8 : 4,
                           true,
                           &stabs_to_module);
  if (!stabs_reader.Process())
    return false;
  stabs_to_module.Finalize();
  return true;
}

bool DumpSymbols::ReadSymbolData(Module** out_module) {
  // Select an object file, if SetArchitecture hasn't been called to set one
  // explicitly.
  if (!selected_object_file_) {
    // If there's only one architecture, that's the one.
    if (object_files_.size() == 1)
      selected_object_file_ = &object_files_[0];
    else {
      // Look for an object file whose architecture matches our own.
      const NXArchInfo *local_arch = NXGetLocalArchInfo();
      if (!SetArchitecture(local_arch->cputype, local_arch->cpusubtype)) {
        fprintf(stderr, "%s: object file contains more than one"
                " architecture, none of which match the current"
                " architecture; specify an architecture explicitly"
                " with '-a ARCH' to resolve the ambiguity\n",
                [object_filename_ fileSystemRepresentation]);
        return false;
      }
    }
  }

  assert(selected_object_file_);

  // Find the name of the selected file's architecture, to appear in
  // the MODULE record and in error messages.
  const NXArchInfo *selected_arch_info =
      google_breakpad::BreakpadGetArchInfoFromCpuType(
          selected_object_file_->cputype, selected_object_file_->cpusubtype);

  const char *selected_arch_name = selected_arch_info->name;
  if (strcmp(selected_arch_name, "i386") == 0)
    selected_arch_name = "x86";

  // Produce a name to use in error messages that includes the
  // filename, and the architecture, if there is more than one.
  selected_object_name_ = [object_filename_ UTF8String];
  if (object_files_.size() > 1) {
    selected_object_name_ += ", architecture ";
    selected_object_name_ + selected_arch_name;
  }

  // Compute a module name, to appear in the MODULE record.
  NSString *module_name = [object_filename_ lastPathComponent];

  // Choose an identifier string, to appear in the MODULE record.
  string identifier = Identifier();
  if (identifier.empty())
    return false;
  identifier += "0";

  // Create a module to hold the debugging information.
  scoped_ptr<Module> module(new Module([module_name UTF8String],
                                       "mac",
                                       selected_arch_name,
                                       identifier));

  // Parse the selected object file.
  mach_o::Reader::Reporter reporter(selected_object_name_);
  mach_o::Reader reader(&reporter);
  if (!reader.Read(reinterpret_cast<const uint8_t *>([contents_ bytes])
                   + selected_object_file_->offset,
                   selected_object_file_->size,
                   selected_object_file_->cputype,
                   selected_object_file_->cpusubtype))
    return false;

  // Walk its load commands, and deal with whatever is there.
  LoadCommandDumper load_command_dumper(*this, module.get(), reader,
                                        symbol_data_, handle_inter_cu_refs_);
  if (!reader.WalkLoadCommands(&load_command_dumper))
    return false;

  *out_module = module.release();

  return true;
}

bool DumpSymbols::WriteSymbolFile(std::ostream &stream) {
  Module* module = NULL;

  if (ReadSymbolData(&module) && module) {
    bool res = module->Write(stream, symbol_data_);
    delete module;
    return res;
  }

  return false;
}

}  // namespace google_breakpad
