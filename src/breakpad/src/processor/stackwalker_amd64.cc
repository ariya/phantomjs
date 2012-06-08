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

// stackwalker_amd64.cc: amd64-specific stackwalker.
//
// See stackwalker_amd64.h for documentation.
//
// Author: Mark Mentovai, Ted Mielczarek


#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/memory_region.h"
#include "google_breakpad/processor/source_line_resolver_interface.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "processor/cfi_frame_info.h"
#include "processor/logging.h"
#include "processor/scoped_ptr.h"
#include "processor/stackwalker_amd64.h"

namespace google_breakpad {


const StackwalkerAMD64::CFIWalker::RegisterSet
StackwalkerAMD64::cfi_register_map_[] = {
  // It may seem like $rip and $rsp are callee-saves, because the callee is
  // responsible for having them restored upon return. But the callee_saves
  // flags here really means that the walker should assume they're
  // unchanged if the CFI doesn't mention them --- clearly wrong for $rip
  // and $rsp.
  { "$rax", NULL, false,
    StackFrameAMD64::CONTEXT_VALID_RAX, &MDRawContextAMD64::rax },
  { "$rdx", NULL, false,
    StackFrameAMD64::CONTEXT_VALID_RDX, &MDRawContextAMD64::rdx },
  { "$rcx", NULL, false,
    StackFrameAMD64::CONTEXT_VALID_RCX, &MDRawContextAMD64::rcx },
  { "$rbx", NULL, true,
    StackFrameAMD64::CONTEXT_VALID_RBX, &MDRawContextAMD64::rbx },
  { "$rsi", NULL, false,
    StackFrameAMD64::CONTEXT_VALID_RSI, &MDRawContextAMD64::rsi },
  { "$rdi", NULL, false,
    StackFrameAMD64::CONTEXT_VALID_RDI, &MDRawContextAMD64::rdi },
  { "$rbp", NULL, true,
    StackFrameAMD64::CONTEXT_VALID_RBP, &MDRawContextAMD64::rbp },
  { "$rsp", ".cfa", false,
    StackFrameAMD64::CONTEXT_VALID_RSP, &MDRawContextAMD64::rsp },
  { "$r8", NULL, false,
    StackFrameAMD64::CONTEXT_VALID_R8,  &MDRawContextAMD64::r8 },
  { "$r9", NULL, false,
    StackFrameAMD64::CONTEXT_VALID_R9,  &MDRawContextAMD64::r9 },
  { "$r10", NULL, false,
    StackFrameAMD64::CONTEXT_VALID_R10, &MDRawContextAMD64::r10 },
  { "$r11", NULL, false,
    StackFrameAMD64::CONTEXT_VALID_R11, &MDRawContextAMD64::r11 },
  { "$r12", NULL, true,
    StackFrameAMD64::CONTEXT_VALID_R12, &MDRawContextAMD64::r12 },
  { "$r13", NULL, true,
    StackFrameAMD64::CONTEXT_VALID_R13, &MDRawContextAMD64::r13 },
  { "$r14", NULL, true,
    StackFrameAMD64::CONTEXT_VALID_R14, &MDRawContextAMD64::r14 },
  { "$r15", NULL, true,
    StackFrameAMD64::CONTEXT_VALID_R15, &MDRawContextAMD64::r15 },
  { "$rip", ".ra", false,
    StackFrameAMD64::CONTEXT_VALID_RIP, &MDRawContextAMD64::rip },
};

StackwalkerAMD64::StackwalkerAMD64(const SystemInfo *system_info,
                                   const MDRawContextAMD64 *context,
                                   MemoryRegion *memory,
                                   const CodeModules *modules,
                                   SymbolSupplier *supplier,
                                   SourceLineResolverInterface *resolver)
    : Stackwalker(system_info, memory, modules, supplier, resolver),
      context_(context),
      cfi_walker_(cfi_register_map_,
                  (sizeof(cfi_register_map_) / sizeof(cfi_register_map_[0]))) {
}


StackFrame* StackwalkerAMD64::GetContextFrame() {
  if (!context_ || !memory_) {
    BPLOG(ERROR) << "Can't get context frame without context or memory";
    return NULL;
  }

  StackFrameAMD64 *frame = new StackFrameAMD64();

  // The instruction pointer is stored directly in a register, so pull it
  // straight out of the CPU context structure.
  frame->context = *context_;
  frame->context_validity = StackFrameAMD64::CONTEXT_VALID_ALL;
  frame->trust = StackFrame::FRAME_TRUST_CONTEXT;
  frame->instruction = frame->context.rip;

  return frame;
}

StackFrameAMD64 *StackwalkerAMD64::GetCallerByCFIFrameInfo(
    const vector<StackFrame *> &frames,
    CFIFrameInfo *cfi_frame_info) {
  StackFrameAMD64 *last_frame = static_cast<StackFrameAMD64*>(frames.back());

  scoped_ptr<StackFrameAMD64> frame(new StackFrameAMD64());
  if (!cfi_walker_
      .FindCallerRegisters(*memory_, *cfi_frame_info,
                           last_frame->context, last_frame->context_validity,
                           &frame->context, &frame->context_validity))
    return NULL;

  // Make sure we recovered all the essentials.
  static const int essentials = (StackFrameAMD64::CONTEXT_VALID_RIP
                                 | StackFrameAMD64::CONTEXT_VALID_RSP);
  if ((frame->context_validity & essentials) != essentials)
    return NULL;

  frame->trust = StackFrame::FRAME_TRUST_CFI;
  return frame.release();
}

StackFrameAMD64 *StackwalkerAMD64::GetCallerByStackScan(
    const vector<StackFrame *> &frames) {
  StackFrameAMD64 *last_frame = static_cast<StackFrameAMD64 *>(frames.back());
  u_int64_t last_rsp = last_frame->context.rsp;
  u_int64_t caller_rip_address, caller_rip;

  if (!ScanForReturnAddress(last_rsp, &caller_rip_address, &caller_rip)) {
    // No plausible return address was found.
    return NULL;
  }

  // Create a new stack frame (ownership will be transferred to the caller)
  // and fill it in.
  StackFrameAMD64 *frame = new StackFrameAMD64();

  frame->trust = StackFrame::FRAME_TRUST_SCAN;
  frame->context = last_frame->context;
  frame->context.rip = caller_rip;
  // The caller's %rsp is directly underneath the return address pushed by
  // the call.
  frame->context.rsp = caller_rip_address + 8;
  frame->context_validity = StackFrameAMD64::CONTEXT_VALID_RIP |
                            StackFrameAMD64::CONTEXT_VALID_RSP;

  // Other unwinders give up if they don't have an %rbp value, so see if we
  // can pass some plausible value on.
  if (last_frame->context_validity & StackFrameAMD64::CONTEXT_VALID_RBP) {
    // Functions typically push their caller's %rbp immediately upon entry,
    // and then set %rbp to point to that. So if the callee's %rbp is
    // pointing to the first word below the alleged return address, presume
    // that the caller's %rbp is saved there.
    if (caller_rip_address - 8 == last_frame->context.rbp) {
      u_int64_t caller_rbp = 0;
      if (memory_->GetMemoryAtAddress(last_frame->context.rbp, &caller_rbp) &&
          caller_rbp > caller_rip_address) {
        frame->context.rbp = caller_rbp;
        frame->context_validity |= StackFrameAMD64::CONTEXT_VALID_RBP;
      }
    } else if (last_frame->context.rbp >= caller_rip_address + 8) {
      // If the callee's %rbp is plausible as a value for the caller's
      // %rbp, presume that the callee left it unchanged.
      frame->context.rbp = last_frame->context.rbp;
      frame->context_validity |= StackFrameAMD64::CONTEXT_VALID_RBP;
    }
  }

  return frame;
}

StackFrame* StackwalkerAMD64::GetCallerFrame(const CallStack *stack) {
  if (!memory_ || !stack) {
    BPLOG(ERROR) << "Can't get caller frame without memory or stack";
    return NULL;
  }

  const vector<StackFrame *> &frames = *stack->frames();
  StackFrameAMD64 *last_frame = static_cast<StackFrameAMD64 *>(frames.back());
  scoped_ptr<StackFrameAMD64> new_frame;

  // If we have DWARF CFI information, use it.
  scoped_ptr<CFIFrameInfo> cfi_frame_info(
      resolver_ ? resolver_->FindCFIFrameInfo(last_frame) : NULL);
  if (cfi_frame_info.get())
    new_frame.reset(GetCallerByCFIFrameInfo(frames, cfi_frame_info.get()));

  // If CFI failed, or there wasn't CFI available, fall back
  // to stack scanning.
  if (!new_frame.get()) {
    new_frame.reset(GetCallerByStackScan(frames));
  }

  // If nothing worked, tell the caller.
  if (!new_frame.get())
    return NULL;

  // Treat an instruction address of 0 as end-of-stack.
  if (new_frame->context.rip == 0)
    return NULL;

  // If the new stack pointer is at a lower address than the old, then
  // that's clearly incorrect. Treat this as end-of-stack to enforce
  // progress and avoid infinite loops.
  if (new_frame->context.rsp <= last_frame->context.rsp)
    return NULL;

  // new_frame->context.rip is the return address, which is one instruction
  // past the CALL that caused us to arrive at the callee. Set
  // new_frame->instruction to one less than that. This won't reference the
  // beginning of the CALL instruction, but it's guaranteed to be within
  // the CALL, which is sufficient to get the source line information to
  // match up with the line that contains a function call. Callers that
  // require the exact return address value may access the context.rip
  // field of StackFrameAMD64.
  new_frame->instruction = new_frame->context.rip - 1;

  return new_frame.release();
}

}  // namespace google_breakpad
