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

// on_demand_symbol_supplier.h: Provides a Symbol Supplier that will create
// a breakpad symbol file on demand.

#ifndef TOOLS_MAC_CRASH_REPORT_ON_DEMAND_SYMBOL_SUPPLIER_H__
#define TOOLS_MAC_CRASH_REPORT_ON_DEMAND_SYMBOL_SUPPLIER_H__

#include <map>
#include <string>
#include "google_breakpad/processor/symbol_supplier.h"

namespace google_breakpad {

using std::map;
using std::string;
class MinidumpModule;

class OnDemandSymbolSupplier : public SymbolSupplier {
 public:
  // |search_dir| is the directory to search for alternative symbols with
  // the same name as the module in the minidump
  OnDemandSymbolSupplier(const string &search_dir,
                         const string &symbol_search_dir);
  virtual ~OnDemandSymbolSupplier() {}

  // Returns the path to the symbol file for the given module.
  virtual SymbolResult GetSymbolFile(const CodeModule *module,
                                     const SystemInfo *system_info,
                                     string *symbol_file);

  // Returns the path to the symbol file for the given module.
  virtual SymbolResult GetSymbolFile(const CodeModule *module,
                                     const SystemInfo *system_info,
                                     string *symbol_file,
                                     string *symbol_data);
  // Allocates data buffer on heap, and takes the ownership of
  // the data buffer.
  virtual SymbolResult GetCStringSymbolData(const CodeModule *module,
                                            const SystemInfo *system_info,
                                            string *symbol_file,
                                            char **symbol_data,
                                            size_t *symbol_data_size);

  // Delete the data buffer allocated for module in GetCStringSymbolData().
  virtual void FreeSymbolData(const CodeModule *module);

 protected:
  // Search directory
  string search_dir_;
  string symbol_search_dir_;

  // When we create a symbol file for a module, save the name of the module
  // and the path to that module's symbol file.
  map<string, string> module_file_map_;

  // Map of allocated data buffers, keyed by module->code_file().
  map<string, char *> memory_buffers_;

  // Return the name for |module|  This will be the value used as the key
  // to the |module_file_map_|.
  string GetNameForModule(const CodeModule *module);

  // Find the module on local system.  If the module resides in a different
  // location than the full path in the minidump, this will be the location
  // used.
  string GetLocalModulePath(const CodeModule *module);

  // Return the full path for |module|.
  string GetModulePath(const CodeModule *module);

  // Return the path to the symbol file for |module|.  If an empty string is
  // returned, then |module| doesn't have a symbol file.
  string GetModuleSymbolFile(const CodeModule *module);

  // Generate the breakpad symbol file for |module|.  Return true if successful.
  // File is generated in /tmp.
  bool GenerateSymbolFile(const CodeModule *module,
                          const SystemInfo *system_info);
};

}  // namespace google_breakpad

#endif  // TOOLS_MAC_CRASH_REPORT_ON_DEMAND_SYMBOL_SUPPLIER_H__
