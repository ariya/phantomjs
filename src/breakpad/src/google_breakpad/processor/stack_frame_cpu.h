// -*- mode: c++ -*-

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

// stack_frame_cpu.h: CPU-specific StackFrame extensions.
//
// These types extend the StackFrame structure to carry CPU-specific register
// state.  They are defined in this header instead of stack_frame.h to
// avoid the need to include minidump_format.h when only the generic
// StackFrame type is needed.
//
// Author: Mark Mentovai

#ifndef GOOGLE_BREAKPAD_PROCESSOR_STACK_FRAME_CPU_H__
#define GOOGLE_BREAKPAD_PROCESSOR_STACK_FRAME_CPU_H__

#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/stack_frame.h"

namespace google_breakpad {

struct WindowsFrameInfo;
class CFIFrameInfo;

struct StackFrameX86 : public StackFrame {
  // ContextValidity has one entry for each relevant hardware pointer
  // register (%eip and %esp) and one entry for each general-purpose
  // register. It's worthwhile having validity flags for caller-saves
  // registers: they are valid in the youngest frame, and such a frame
  // might save a callee-saves register in a caller-saves register, but
  // SimpleCFIWalker won't touch registers unless they're marked as valid.
  enum ContextValidity {
    CONTEXT_VALID_NONE = 0,
    CONTEXT_VALID_EIP  = 1 << 0,
    CONTEXT_VALID_ESP  = 1 << 1,
    CONTEXT_VALID_EBP  = 1 << 2,
    CONTEXT_VALID_EAX  = 1 << 3,
    CONTEXT_VALID_EBX  = 1 << 4,
    CONTEXT_VALID_ECX  = 1 << 5,
    CONTEXT_VALID_EDX  = 1 << 6,
    CONTEXT_VALID_ESI  = 1 << 7,
    CONTEXT_VALID_EDI  = 1 << 8,
    CONTEXT_VALID_ALL  = -1
  };

 StackFrameX86()
     : context(),
       context_validity(CONTEXT_VALID_NONE),
       windows_frame_info(NULL),
       cfi_frame_info(NULL) {}
  ~StackFrameX86();

  // Register state.  This is only fully valid for the topmost frame in a
  // stack.  In other frames, the values of nonvolatile registers may be
  // present, given sufficient debugging information.  Refer to
  // context_validity.
  MDRawContextX86 context;

  // context_validity is actually ContextValidity, but int is used because
  // the OR operator doesn't work well with enumerated types.  This indicates
  // which fields in context are valid.
  int context_validity;

  // Any stack walking information we found describing this.instruction.
  // These may be NULL if there is no such information for that address.
  WindowsFrameInfo *windows_frame_info;
  CFIFrameInfo *cfi_frame_info;
};

struct StackFramePPC : public StackFrame {
  // ContextValidity should eventually contain entries for the validity of
  // other nonvolatile (callee-save) registers as in
  // StackFrameX86::ContextValidity, but the ppc stackwalker doesn't currently
  // locate registers other than the ones listed here.
  enum ContextValidity {
    CONTEXT_VALID_NONE = 0,
    CONTEXT_VALID_SRR0 = 1 << 0,
    CONTEXT_VALID_GPR1 = 1 << 1,
    CONTEXT_VALID_ALL  = -1
  };

  StackFramePPC() : context(), context_validity(CONTEXT_VALID_NONE) {}

  // Register state.  This is only fully valid for the topmost frame in a
  // stack.  In other frames, the values of nonvolatile registers may be
  // present, given sufficient debugging information.  Refer to
  // context_validity.
  MDRawContextPPC context;

  // context_validity is actually ContextValidity, but int is used because
  // the OR operator doesn't work well with enumerated types.  This indicates
  // which fields in context are valid.
  int context_validity;
};

struct StackFrameAMD64 : public StackFrame {
  // ContextValidity has one entry for each register that we might be able
  // to recover.
  enum ContextValidity {
    CONTEXT_VALID_NONE  = 0,
    CONTEXT_VALID_RAX   = 1 << 0,
    CONTEXT_VALID_RDX   = 1 << 1,
    CONTEXT_VALID_RCX   = 1 << 2,
    CONTEXT_VALID_RBX   = 1 << 3,
    CONTEXT_VALID_RSI   = 1 << 4,
    CONTEXT_VALID_RDI   = 1 << 5,
    CONTEXT_VALID_RBP   = 1 << 6,
    CONTEXT_VALID_RSP   = 1 << 7,
    CONTEXT_VALID_R8    = 1 << 8,
    CONTEXT_VALID_R9    = 1 << 9,
    CONTEXT_VALID_R10   = 1 << 10,
    CONTEXT_VALID_R11   = 1 << 11,
    CONTEXT_VALID_R12   = 1 << 12,
    CONTEXT_VALID_R13   = 1 << 13,
    CONTEXT_VALID_R14   = 1 << 14,
    CONTEXT_VALID_R15   = 1 << 15,
    CONTEXT_VALID_RIP   = 1 << 16,
    CONTEXT_VALID_ALL  = -1
  };

  StackFrameAMD64() : context(), context_validity(CONTEXT_VALID_NONE) {}

  // Register state. This is only fully valid for the topmost frame in a
  // stack. In other frames, which registers are present depends on what
  // debugging information we had available. Refer to context_validity.
  MDRawContextAMD64 context;

  // For each register in context whose value has been recovered, we set
  // the corresponding CONTEXT_VALID_ bit in context_validity.
  //
  // context_validity's type should actually be ContextValidity, but
  // we use int instead because the bitwise inclusive or operator
  // yields an int when applied to enum values, and C++ doesn't
  // silently convert from ints to enums.
  int context_validity;
};

struct StackFrameSPARC : public StackFrame {
  // to be confirmed
  enum ContextValidity {
    CONTEXT_VALID_NONE = 0,
    CONTEXT_VALID_PC   = 1 << 0,
    CONTEXT_VALID_SP   = 1 << 1,
    CONTEXT_VALID_FP   = 1 << 2,
    CONTEXT_VALID_ALL  = -1
  };

  StackFrameSPARC() : context(), context_validity(CONTEXT_VALID_NONE) {}

  // Register state.  This is only fully valid for the topmost frame in a
  // stack.  In other frames, the values of nonvolatile registers may be
  // present, given sufficient debugging information.  Refer to
  // context_validity.
  MDRawContextSPARC context;

  // context_validity is actually ContextValidity, but int is used because
  // the OR operator doesn't work well with enumerated types.  This indicates
  // which fields in context are valid.
  int context_validity;
};

struct StackFrameARM : public StackFrame {
  // A flag for each register we might know.
  enum ContextValidity {
    CONTEXT_VALID_NONE = 0,
    CONTEXT_VALID_R0   = 1 << 0,
    CONTEXT_VALID_R1   = 1 << 1,
    CONTEXT_VALID_R2   = 1 << 2,
    CONTEXT_VALID_R3   = 1 << 3,
    CONTEXT_VALID_R4   = 1 << 4,
    CONTEXT_VALID_R5   = 1 << 5,
    CONTEXT_VALID_R6   = 1 << 6,
    CONTEXT_VALID_R7   = 1 << 7,
    CONTEXT_VALID_R8   = 1 << 8,
    CONTEXT_VALID_R9   = 1 << 9,
    CONTEXT_VALID_R10  = 1 << 10,
    CONTEXT_VALID_R11  = 1 << 11,
    CONTEXT_VALID_R12  = 1 << 12,
    CONTEXT_VALID_R13  = 1 << 13,
    CONTEXT_VALID_R14  = 1 << 14,
    CONTEXT_VALID_R15  = 1 << 15,
    CONTEXT_VALID_ALL  = ~CONTEXT_VALID_NONE,

    // Aliases for registers with dedicated or conventional roles.
    CONTEXT_VALID_FP   = CONTEXT_VALID_R11,
    CONTEXT_VALID_SP   = CONTEXT_VALID_R13,
    CONTEXT_VALID_LR   = CONTEXT_VALID_R14,
    CONTEXT_VALID_PC   = CONTEXT_VALID_R15
  };

  StackFrameARM() : context(), context_validity(CONTEXT_VALID_NONE) {}

  // Return the ContextValidity flag for register rN.
  static ContextValidity RegisterValidFlag(int n) {
    return ContextValidity(1 << n);
  } 

  // Register state.  This is only fully valid for the topmost frame in a
  // stack.  In other frames, the values of nonvolatile registers may be
  // present, given sufficient debugging information.  Refer to
  // context_validity.
  MDRawContextARM context;

  // For each register in context whose value has been recovered, we set
  // the corresponding CONTEXT_VALID_ bit in context_validity.
  //
  // context_validity's type should actually be ContextValidity, but
  // we use int instead because the bitwise inclusive or operator
  // yields an int when applied to enum values, and C++ doesn't
  // silently convert from ints to enums.
  int context_validity;
};

}  // namespace google_breakpad

#endif  // GOOGLE_BREAKPAD_PROCESSOR_STACK_FRAME_CPU_H__
