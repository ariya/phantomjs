// Copyright (c) 2010 Google Inc.
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

// basic_source_line_resolver.h: BasicSourceLineResolver is derived from
// SourceLineResolverBase, and is a concrete implementation of
// SourceLineResolverInterface, using address map files produced by a
// compatible writer, e.g. PDBSourceLineWriter.
//
// see "processor/source_line_resolver_base.h"
// and "source_line_resolver_interface.h" for more documentation.

#ifndef GOOGLE_BREAKPAD_PROCESSOR_BASIC_SOURCE_LINE_RESOLVER_H__
#define GOOGLE_BREAKPAD_PROCESSOR_BASIC_SOURCE_LINE_RESOLVER_H__

#include <map>

#include "google_breakpad/processor/source_line_resolver_base.h"

namespace google_breakpad {

using std::string;
using std::map;

class BasicSourceLineResolver : public SourceLineResolverBase {
 public:
  BasicSourceLineResolver();
  virtual ~BasicSourceLineResolver() { }

  using SourceLineResolverBase::LoadModule;
  using SourceLineResolverBase::LoadModuleUsingMapBuffer;
  using SourceLineResolverBase::LoadModuleUsingMemoryBuffer;
  using SourceLineResolverBase::ShouldDeleteMemoryBufferAfterLoadModule;
  using SourceLineResolverBase::UnloadModule;
  using SourceLineResolverBase::HasModule;
  using SourceLineResolverBase::FillSourceLineInfo;
  using SourceLineResolverBase::FindWindowsFrameInfo;
  using SourceLineResolverBase::FindCFIFrameInfo;

 private:
  // friend declarations:
  friend class BasicModuleFactory;
  friend class ModuleComparer;
  friend class ModuleSerializer;
  template<class> friend class SimpleSerializer;

  // Function derives from SourceLineResolverBase::Function.
  struct Function;
  // Module implements SourceLineResolverBase::Module interface.
  class Module;

  // Disallow unwanted copy ctor and assignment operator
  BasicSourceLineResolver(const BasicSourceLineResolver&);
  void operator=(const BasicSourceLineResolver&);
};

}  // namespace google_breakpad

#endif  // GOOGLE_BREAKPAD_PROCESSOR_BASIC_SOURCE_LINE_RESOLVER_H__
