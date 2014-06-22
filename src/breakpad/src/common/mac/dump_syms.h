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

// dump_syms.h: Declaration of google_breakpad::DumpSymbols, a class for
// reading debugging information from Mach-O files and writing it out as a
// Breakpad symbol file.

#include <Foundation/Foundation.h>
#include <mach-o/loader.h>
#include <stdio.h>
#include <stdlib.h>

#include <ostream>
#include <string>
#include <vector>

#include "common/byte_cursor.h"
#include "common/mac/macho_reader.h"
#include "common/module.h"
#include "common/symbol_data.h"

namespace google_breakpad {

class DumpSymbols {
 public:
  DumpSymbols(SymbolData symbol_data, bool handle_inter_cu_refs)
      : symbol_data_(symbol_data),
        handle_inter_cu_refs_(handle_inter_cu_refs),
        input_pathname_(),
        object_filename_(),
        contents_(),
        selected_object_file_(),
        selected_object_name_() { }
  ~DumpSymbols() {
    [input_pathname_ release];
    [object_filename_ release];
    [contents_ release];
  }

  // Prepare to read debugging information from |filename|. |filename| may be
  // the name of a universal binary, a Mach-O file, or a dSYM bundle
  // containing either of the above. On success, return true; if there is a
  // problem reading |filename|, report it and return false.
  //
  // (This class uses NSString for filenames and related values,
  // because the Mac Foundation framework seems to support
  // filename-related operations more fully on NSString values.)
  bool Read(NSString *filename);

  // If this dumper's file includes an object file for |cpu_type| and
  // |cpu_subtype|, then select that object file for dumping, and return
  // true. Otherwise, return false, and leave this dumper's selected
  // architecture unchanged.
  //
  // By default, if this dumper's file contains only one object file, then
  // the dumper will dump those symbols; and if it contains more than one
  // object file, then the dumper will dump the object file whose
  // architecture matches that of this dumper program.
  bool SetArchitecture(cpu_type_t cpu_type, cpu_subtype_t cpu_subtype);

  // If this dumper's file includes an object file for |arch_name|, then select
  // that object file for dumping, and return true. Otherwise, return false,
  // and leave this dumper's selected architecture unchanged.
  //
  // By default, if this dumper's file contains only one object file, then
  // the dumper will dump those symbols; and if it contains more than one
  // object file, then the dumper will dump the object file whose
  // architecture matches that of this dumper program.
  bool SetArchitecture(const std::string &arch_name);

  // Return a pointer to an array of 'struct fat_arch' structures,
  // describing the object files contained in this dumper's file. Set
  // *|count| to the number of elements in the array. The returned array is
  // owned by this DumpSymbols instance.
  //
  // If there are no available architectures, this function
  // may return NULL.
  const struct fat_arch *AvailableArchitectures(size_t *count) {
    *count = object_files_.size();
    if (object_files_.size() > 0)
      return &object_files_[0];
    return NULL;
  }

  // Read the selected object file's debugging information, and write it out to
  // |stream|. Return true on success; if an error occurs, report it and
  // return false.
  bool WriteSymbolFile(std::ostream &stream);

  // As above, but simply return the debugging information in module
  // instead of writing it to a stream. The caller owns the resulting
  // module object and must delete it when finished.
  bool ReadSymbolData(Module** module);

 private:
  // Used internally.
  class DumperLineToModule;
  class LoadCommandDumper;

  // Return an identifier string for the file this DumpSymbols is dumping.
  std::string Identifier();

  // Read debugging information from |dwarf_sections|, which was taken from
  // |macho_reader|, and add it to |module|. On success, return true;
  // on failure, report the problem and return false.
  bool ReadDwarf(google_breakpad::Module *module,
                 const mach_o::Reader &macho_reader,
                 const mach_o::SectionMap &dwarf_sections,
                 bool handle_inter_cu_refs) const;

  // Read DWARF CFI or .eh_frame data from |section|, belonging to
  // |macho_reader|, and record it in |module|.  If |eh_frame| is true,
  // then the data is .eh_frame-format data; otherwise, it is standard DWARF
  // .debug_frame data. On success, return true; on failure, report
  // the problem and return false.
  bool ReadCFI(google_breakpad::Module *module,
               const mach_o::Reader &macho_reader,
               const mach_o::Section &section,
               bool eh_frame) const;

  // The selection of what type of symbol data to read/write.
  const SymbolData symbol_data_;

  // Whether to handle references between compilation units.
  const bool handle_inter_cu_refs_;

  // The name of the file or bundle whose symbols this will dump.
  // This is the path given to Read, for use in error messages.
  NSString *input_pathname_;

  // The name of the file this DumpSymbols will actually read debugging
  // information from. Normally, this is the same as input_pathname_, but if
  // filename refers to a dSYM bundle, then this is the resource file
  // within that bundle.
  NSString *object_filename_;

  // The complete contents of object_filename_, mapped into memory.
  NSData *contents_;

  // A vector of fat_arch structures describing the object files
  // object_filename_ contains. If object_filename_ refers to a fat binary,
  // this may have more than one element; if it refers to a Mach-O file, this
  // has exactly one element.
  vector<struct fat_arch> object_files_;

  // The object file in object_files_ selected to dump, or NULL if
  // SetArchitecture hasn't been called yet.
  const struct fat_arch *selected_object_file_;

  // A string that identifies the selected object file, for use in error
  // messages.  This is usually object_filename_, but if that refers to a
  // fat binary, it includes an indication of the particular architecture
  // within that binary.
  string selected_object_name_;
};

}  // namespace google_breakpad
