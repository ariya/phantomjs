// -*- mode: C++ -*-

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

// Original author: Jim Blandy <jimb@mozilla.com> <jimb@red-bean.com>

// Mock classes for writing stackwalker tests, shared amongst architectures.

#ifndef PROCESSOR_STACKWALKER_UNITTEST_UTILS_H_
#define PROCESSOR_STACKWALKER_UNITTEST_UTILS_H_

#include <stdlib.h>
#include <string>
#include <vector>

#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/processor/code_module.h"
#include "google_breakpad/processor/code_modules.h"
#include "google_breakpad/processor/memory_region.h"
#include "google_breakpad/processor/symbol_supplier.h"
#include "google_breakpad/processor/system_info.h"

class MockMemoryRegion: public google_breakpad::MemoryRegion {
 public:
  MockMemoryRegion(): base_address_(0) { }

  // Set this region's address and contents. If we have placed an
  // instance of this class in a test fixture class, individual tests
  // can use this to provide the region's contents.
  void Init(u_int64_t base_address, const std::string &contents) {
    base_address_ = base_address;
    contents_ = contents;
  }

  u_int64_t GetBase() const { return base_address_; }
  u_int32_t GetSize() const { return contents_.size(); }

  bool GetMemoryAtAddress(u_int64_t address, u_int8_t  *value) const {
    return GetMemoryLittleEndian(address, value);
  }
  bool GetMemoryAtAddress(u_int64_t address, u_int16_t *value) const {
    return GetMemoryLittleEndian(address, value);
  }
  bool GetMemoryAtAddress(u_int64_t address, u_int32_t *value) const {
    return GetMemoryLittleEndian(address, value);
  }
  bool GetMemoryAtAddress(u_int64_t address, u_int64_t *value) const {
    return GetMemoryLittleEndian(address, value);
  }

 private:
  // Fetch a little-endian value from ADDRESS in contents_ whose size
  // is BYTES, and store it in *VALUE. Return true on success.
  template<typename ValueType>
  bool GetMemoryLittleEndian(u_int64_t address, ValueType *value) const {
    if (address < base_address_ ||
        address - base_address_ + sizeof(ValueType) > contents_.size())
      return false;
    ValueType v = 0;
    int start = address - base_address_;
    // The loop condition is odd, but it's correct for size_t.
    for (size_t i = sizeof(ValueType) - 1; i < sizeof(ValueType); i--)
      v = (v << 8) | static_cast<unsigned char>(contents_[start + i]);
    *value = v;
    return true;
  }

  u_int64_t base_address_;
  std::string contents_;
};

class MockCodeModule: public google_breakpad::CodeModule {
 public:
  MockCodeModule(u_int64_t base_address, u_int64_t size,
                 const std::string &code_file, const std::string &version)
      : base_address_(base_address), size_(size), code_file_(code_file) { }

  u_int64_t base_address()       const { return base_address_; }
  u_int64_t size()               const { return size_; }
  std::string code_file()        const { return code_file_; }
  std::string code_identifier()  const { return code_file_; }
  std::string debug_file()       const { return code_file_; }
  std::string debug_identifier() const { return code_file_; }
  std::string version()          const { return version_; }
  const google_breakpad::CodeModule *Copy() const {
    abort(); // Tests won't use this.
  }

 private:
  u_int64_t base_address_;
  u_int64_t size_;
  std::string code_file_;
  std::string version_;
};

class MockCodeModules: public google_breakpad::CodeModules {
 public:  
  typedef google_breakpad::CodeModule CodeModule;
  typedef google_breakpad::CodeModules CodeModules;

  void Add(const MockCodeModule *module) { 
    modules_.push_back(module);
  }

  unsigned int module_count() const { return modules_.size(); }

  const CodeModule *GetModuleForAddress(u_int64_t address) const {
    for (ModuleVector::const_iterator i = modules_.begin();
         i != modules_.end(); i++) {
      const MockCodeModule *module = *i;
      if (module->base_address() <= address &&
          address - module->base_address() < module->size())
        return module;
    }
    return NULL;
  };

  const CodeModule *GetMainModule() const { return modules_[0]; }

  const CodeModule *GetModuleAtSequence(unsigned int sequence) const {
    return modules_.at(sequence);
  }

  const CodeModule *GetModuleAtIndex(unsigned int index) const {
    return modules_.at(index);
  }

  const CodeModules *Copy() const { abort(); } // Tests won't use this.

 private:  
  typedef std::vector<const MockCodeModule *> ModuleVector;
  ModuleVector modules_;
};

class MockSymbolSupplier: public google_breakpad::SymbolSupplier {
 public:
  typedef google_breakpad::CodeModule CodeModule;
  typedef google_breakpad::SystemInfo SystemInfo;
  MOCK_METHOD3(GetSymbolFile, SymbolResult(const CodeModule *module,
                                           const SystemInfo *system_info,
                                           std::string *symbol_file));
  MOCK_METHOD4(GetSymbolFile, SymbolResult(const CodeModule *module,
                                           const SystemInfo *system_info,
                                           std::string *symbol_file,
                                           std::string *symbol_data));
  MOCK_METHOD4(GetCStringSymbolData, SymbolResult(const CodeModule *module,
                                                  const SystemInfo *system_info,
                                                  std::string *symbol_file,
                                                  char **symbol_data));
  MOCK_METHOD1(FreeSymbolData, void(const CodeModule *module));
};

#endif // PROCESSOR_STACKWALKER_UNITTEST_UTILS_H_
