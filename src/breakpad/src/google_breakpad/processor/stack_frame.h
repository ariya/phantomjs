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

#ifndef GOOGLE_BREAKPAD_PROCESSOR_STACK_FRAME_H__
#define GOOGLE_BREAKPAD_PROCESSOR_STACK_FRAME_H__

#include <string>
#include "google_breakpad/common/breakpad_types.h"

namespace google_breakpad {

class CodeModule;

using std::string;

struct StackFrame {
  // Indicates how well the instruction pointer derived during
  // stack walking is trusted. Since the stack walker can resort to
  // stack scanning, it can wind up with dubious frames.
  // In rough order of "trust metric".
  enum FrameTrust {
    FRAME_TRUST_NONE,     // Unknown
    FRAME_TRUST_SCAN,     // Scanned the stack, found this
    FRAME_TRUST_CFI_SCAN, // Scanned the stack using call frame info, found this
    FRAME_TRUST_FP,       // Derived from frame pointer
    FRAME_TRUST_CFI,      // Derived from call frame info
    FRAME_TRUST_CONTEXT   // Given as instruction pointer in a context
  };

  StackFrame()
      : instruction(),
        module(NULL),
        function_name(),
        function_base(),
        source_file_name(),
        source_line(),
        source_line_base(),
        trust(FRAME_TRUST_NONE) {}
  virtual ~StackFrame() {}

  // Return a string describing how this stack frame was found
  // by the stackwalker.
  string trust_description() const {
    switch (trust) {
      case StackFrame::FRAME_TRUST_CONTEXT:
        return "given as instruction pointer in context";
      case StackFrame::FRAME_TRUST_CFI:
        return "call frame info";
      case StackFrame::FRAME_TRUST_CFI_SCAN:
        return "call frame info with scanning";
      case StackFrame::FRAME_TRUST_FP:
        return "previous frame's frame pointer";
      case StackFrame::FRAME_TRUST_SCAN:
        return "stack scanning";
      default:
        return "unknown";
    }
  };

  // The program counter location as an absolute virtual address.  For the
  // innermost called frame in a stack, this will be an exact program counter
  // or instruction pointer value.  For all other frames, this will be within
  // the instruction that caused execution to branch to a called function,
  // but may not necessarily point to the exact beginning of that instruction.
  u_int64_t instruction;

  // The module in which the instruction resides.
  const CodeModule *module;

  // The function name, may be omitted if debug symbols are not available.
  string function_name;

  // The start address of the function, may be omitted if debug symbols
  // are not available.
  u_int64_t function_base;

  // The source file name, may be omitted if debug symbols are not available.
  string source_file_name;

  // The (1-based) source line number, may be omitted if debug symbols are
  // not available.
  int source_line;

  // The start address of the source line, may be omitted if debug symbols
  // are not available.
  u_int64_t source_line_base;

  // Amount of trust the stack walker has in the instruction pointer
  // of this frame.
  FrameTrust trust;
};

}  // namespace google_breakpad

#endif  // GOOGLE_BREAKPAD_PROCESSOR_STACK_FRAME_H__
