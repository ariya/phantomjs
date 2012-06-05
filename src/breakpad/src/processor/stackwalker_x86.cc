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

// stackwalker_x86.cc: x86-specific stackwalker.
//
// See stackwalker_x86.h for documentation.
//
// Author: Mark Mentovai


#include "processor/postfix_evaluator-inl.h"

#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/code_modules.h"
#include "google_breakpad/processor/memory_region.h"
#include "google_breakpad/processor/source_line_resolver_interface.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "processor/logging.h"
#include "processor/scoped_ptr.h"
#include "processor/stackwalker_x86.h"
#include "processor/windows_frame_info.h"
#include "processor/cfi_frame_info.h"

namespace google_breakpad {


const StackwalkerX86::CFIWalker::RegisterSet
StackwalkerX86::cfi_register_map_[] = {
  // It may seem like $eip and $esp are callee-saves, because (with Unix or
  // cdecl calling conventions) the callee is responsible for having them
  // restored upon return. But the callee_saves flags here really means
  // that the walker should assume they're unchanged if the CFI doesn't
  // mention them, which is clearly wrong for $eip and $esp.
  { "$eip", ".ra",  false,
    StackFrameX86::CONTEXT_VALID_EIP, &MDRawContextX86::eip },
  { "$esp", ".cfa", false,
    StackFrameX86::CONTEXT_VALID_ESP, &MDRawContextX86::esp },
  { "$ebp", NULL,   true,
    StackFrameX86::CONTEXT_VALID_EBP, &MDRawContextX86::ebp },
  { "$eax", NULL,   false,
    StackFrameX86::CONTEXT_VALID_EAX, &MDRawContextX86::eax },
  { "$ebx", NULL,   true,
    StackFrameX86::CONTEXT_VALID_EBX, &MDRawContextX86::ebx },
  { "$ecx", NULL,   false,
    StackFrameX86::CONTEXT_VALID_ECX, &MDRawContextX86::ecx },
  { "$edx", NULL,   false,
    StackFrameX86::CONTEXT_VALID_EDX, &MDRawContextX86::edx },
  { "$esi", NULL,   true,
    StackFrameX86::CONTEXT_VALID_ESI, &MDRawContextX86::esi },
  { "$edi", NULL,   true,
    StackFrameX86::CONTEXT_VALID_EDI, &MDRawContextX86::edi },
};

StackwalkerX86::StackwalkerX86(const SystemInfo *system_info,
                               const MDRawContextX86 *context,
                               MemoryRegion *memory,
                               const CodeModules *modules,
                               SymbolSupplier *supplier,
                               SourceLineResolverInterface *resolver)
    : Stackwalker(system_info, memory, modules, supplier, resolver),
      context_(context),
      cfi_walker_(cfi_register_map_,
                  (sizeof(cfi_register_map_) / sizeof(cfi_register_map_[0]))) {
  if (memory_->GetBase() + memory_->GetSize() - 1 > 0xffffffff) {
    // The x86 is a 32-bit CPU, the limits of the supplied stack are invalid.
    // Mark memory_ = NULL, which will cause stackwalking to fail.
    BPLOG(ERROR) << "Memory out of range for stackwalking: " <<
                    HexString(memory_->GetBase()) << "+" <<
                    HexString(memory_->GetSize());
    memory_ = NULL;
  }
}

StackFrameX86::~StackFrameX86() {
  if (windows_frame_info)
    delete windows_frame_info;
  windows_frame_info = NULL;
  if (cfi_frame_info)
    delete cfi_frame_info;
  cfi_frame_info = NULL;
}

StackFrame *StackwalkerX86::GetContextFrame() {
  if (!context_ || !memory_) {
    BPLOG(ERROR) << "Can't get context frame without context or memory";
    return NULL;
  }

  StackFrameX86 *frame = new StackFrameX86();

  // The instruction pointer is stored directly in a register, so pull it
  // straight out of the CPU context structure.
  frame->context = *context_;
  frame->context_validity = StackFrameX86::CONTEXT_VALID_ALL;
  frame->trust = StackFrame::FRAME_TRUST_CONTEXT;
  frame->instruction = frame->context.eip;

  return frame;
}

StackFrameX86 *StackwalkerX86::GetCallerByWindowsFrameInfo(
    const vector<StackFrame *> &frames,
    WindowsFrameInfo *last_frame_info) {
  StackFrame::FrameTrust trust = StackFrame::FRAME_TRUST_NONE;

  StackFrameX86 *last_frame = static_cast<StackFrameX86 *>(frames.back());

  // Save the stack walking info we found, in case we need it later to
  // find the callee of the frame we're constructing now.
  last_frame->windows_frame_info = last_frame_info;

  // This function only covers the full STACK WIN case. If
  // last_frame_info is VALID_PARAMETER_SIZE-only, then we should
  // assume the traditional frame format or use some other strategy.
  if (last_frame_info->valid != WindowsFrameInfo::VALID_ALL)
    return NULL;

  // This stackwalker sets each frame's %esp to its value immediately prior
  // to the CALL into the callee.  This means that %esp points to the last
  // callee argument pushed onto the stack, which may not be where %esp points
  // after the callee returns.  Specifically, the value is correct for the
  // cdecl calling convention, but not other conventions.  The cdecl
  // convention requires a caller to pop its callee's arguments from the
  // stack after the callee returns.  This is usually accomplished by adding
  // the known size of the arguments to %esp.  Other calling conventions,
  // including stdcall, thiscall, and fastcall, require the callee to pop any
  // parameters stored on the stack before returning.  This is usually
  // accomplished by using the RET n instruction, which pops n bytes off
  // the stack after popping the return address.
  //
  // Because each frame's %esp will point to a location on the stack after
  // callee arguments have been PUSHed, when locating things in a stack frame
  // relative to %esp, the size of the arguments to the callee need to be
  // taken into account.  This seems a little bit unclean, but it's better
  // than the alternative, which would need to take these same things into
  // account, but only for cdecl functions.  With this implementation, we get
  // to be agnostic about each function's calling convention.  Furthermore,
  // this is how Windows debugging tools work, so it means that the %esp
  // values produced by this stackwalker directly correspond to the %esp
  // values you'll see there.
  //
  // If the last frame has no callee (because it's the context frame), just
  // set the callee parameter size to 0: the stack pointer can't point to
  // callee arguments because there's no callee.  This is correct as long
  // as the context wasn't captured while arguments were being pushed for
  // a function call.  Note that there may be functions whose parameter sizes
  // are unknown, 0 is also used in that case.  When that happens, it should
  // be possible to walk to the next frame without reference to %esp.

  u_int32_t last_frame_callee_parameter_size = 0;
  int frames_already_walked = frames.size();
  if (frames_already_walked >= 2) {
    const StackFrameX86 *last_frame_callee
        = static_cast<StackFrameX86 *>(frames[frames_already_walked - 2]);
    WindowsFrameInfo *last_frame_callee_info
        = last_frame_callee->windows_frame_info;
    if (last_frame_callee_info &&
        (last_frame_callee_info->valid
         & WindowsFrameInfo::VALID_PARAMETER_SIZE)) {
      last_frame_callee_parameter_size =
          last_frame_callee_info->parameter_size;
    }
  }

  // Set up the dictionary for the PostfixEvaluator.  %ebp and %esp are used
  // in each program string, and their previous values are known, so set them
  // here.
  PostfixEvaluator<u_int32_t>::DictionaryType dictionary;
  // Provide the current register values.
  dictionary["$ebp"] = last_frame->context.ebp;
  dictionary["$esp"] = last_frame->context.esp;
  // Provide constants from the debug info for last_frame and its callee.
  // .cbCalleeParams is a Breakpad extension that allows us to use the
  // PostfixEvaluator engine when certain types of debugging information
  // are present without having to write the constants into the program
  // string as literals.
  dictionary[".cbCalleeParams"] = last_frame_callee_parameter_size;
  dictionary[".cbSavedRegs"] = last_frame_info->saved_register_size;
  dictionary[".cbLocals"] = last_frame_info->local_size;

  u_int32_t raSearchStart = last_frame->context.esp +
                            last_frame_callee_parameter_size +
                            last_frame_info->local_size +
                            last_frame_info->saved_register_size;
  u_int32_t found; // dummy value
  // Scan up to three words above the calculated search value, in case
  // the stack was aligned to a quadword boundary.
  ScanForReturnAddress(raSearchStart, &raSearchStart, &found, 3);
  
  // The difference between raSearch and raSearchStart is unknown,
  // but making them the same seems to work well in practice.
  dictionary[".raSearchStart"] = raSearchStart;
  dictionary[".raSearch"] = raSearchStart;

  dictionary[".cbParams"] = last_frame_info->parameter_size;

  // Decide what type of program string to use. The program string is in
  // postfix notation and will be passed to PostfixEvaluator::Evaluate.
  // Given the dictionary and the program string, it is possible to compute
  // the return address and the values of other registers in the calling
  // function. Because of bugs described below, the stack may need to be
  // scanned for these values. The results of program string evaluation
  // will be used to determine whether to scan for better values.
  string program_string;
  bool recover_ebp = true;

  trust = StackFrame::FRAME_TRUST_CFI;
  if (!last_frame_info->program_string.empty()) {
    // The FPO data has its own program string, which will tell us how to
    // get to the caller frame, and may even fill in the values of
    // nonvolatile registers and provide pointers to local variables and
    // parameters.  In some cases, particularly with program strings that use
    // .raSearchStart, the stack may need to be scanned afterward.
    program_string = last_frame_info->program_string;
  } else if (last_frame_info->allocates_base_pointer) {
    // The function corresponding to the last frame doesn't use the frame
    // pointer for conventional purposes, but it does allocate a new
    // frame pointer and use it for its own purposes.  Its callee's
    // information is still accessed relative to %esp, and the previous
    // value of %ebp can be recovered from a location in its stack frame,
    // within the saved-register area.
    //
    // Functions that fall into this category use the %ebp register for
    // a purpose other than the frame pointer.  They restore the caller's
    // %ebp before returning.  These functions create their stack frame
    // after a CALL by decrementing the stack pointer in an amount
    // sufficient to store local variables, and then PUSHing saved
    // registers onto the stack.  Arguments to a callee function, if any,
    // are PUSHed after that.  Walking up to the caller, therefore,
    // can be done solely with calculations relative to the stack pointer
    // (%esp).  The return address is recovered from the memory location
    // above the known sizes of the callee's parameters, saved registers,
    // and locals.  The caller's stack pointer (the value of %esp when
    // the caller executed CALL) is the location immediately above the
    // saved return address.  The saved value of %ebp to be restored for
    // the caller is at a known location in the saved-register area of
    // the stack frame.
    //
    // For this type of frame, MSVC 14 (from Visual Studio 8/2005) in
    // link-time code generation mode (/LTCG and /GL) can generate erroneous
    // debugging data.  The reported size of saved registers can be 0,
    // which is clearly an error because these frames must, at the very
    // least, save %ebp.  For this reason, in addition to those given above
    // about the use of .raSearchStart, the stack may need to be scanned
    // for a better return address and a better frame pointer after the
    // program string is evaluated.
    //
    // %eip_new = *(%esp_old + callee_params + saved_regs + locals)
    // %ebp_new = *(%esp_old + callee_params + saved_regs - 8)
    // %esp_new = %esp_old + callee_params + saved_regs + locals + 4
    program_string = "$eip .raSearchStart ^ = "
        "$ebp $esp .cbCalleeParams + .cbSavedRegs + 8 - ^ = "
        "$esp .raSearchStart 4 + =";
  } else {
    // The function corresponding to the last frame doesn't use %ebp at
    // all.  The callee frame is located relative to %esp.
    //
    // The called procedure's instruction pointer and stack pointer are
    // recovered in the same way as the case above, except that no
    // frame pointer (%ebp) is used at all, so it is not saved anywhere
    // in the callee's stack frame and does not need to be recovered.
    // Because %ebp wasn't used in the callee, whatever value it has
    // is the value that it had in the caller, so it can be carried
    // straight through without bringing its validity into question.
    //
    // Because of the use of .raSearchStart, the stack will possibly be
    // examined to locate a better return address after program string
    // evaluation.  The stack will not be examined to locate a saved
    // %ebp value, because these frames do not save (or use) %ebp.
    //
    // %eip_new = *(%esp_old + callee_params + saved_regs + locals)
    // %esp_new = %esp_old + callee_params + saved_regs + locals + 4
    // %ebp_new = %ebp_old
    program_string = "$eip .raSearchStart ^ = "
        "$esp .raSearchStart 4 + =";
    recover_ebp = false;
  }

  // Now crank it out, making sure that the program string set at least the
  // two required variables.
  PostfixEvaluator<u_int32_t> evaluator =
      PostfixEvaluator<u_int32_t>(&dictionary, memory_);
  PostfixEvaluator<u_int32_t>::DictionaryValidityType dictionary_validity;
  if (!evaluator.Evaluate(program_string, &dictionary_validity) ||
      dictionary_validity.find("$eip") == dictionary_validity.end() ||
      dictionary_validity.find("$esp") == dictionary_validity.end()) {
    // Program string evaluation failed. It may be that %eip is not somewhere
    // with stack frame info, and %ebp is pointing to non-stack memory, so
    // our evaluation couldn't succeed. We'll scan the stack for a return
    // address. This can happen if the stack is in a module for which
    // we don't have symbols, and that module is compiled without a
    // frame pointer.
    u_int32_t location_start = last_frame->context.esp;
    u_int32_t location, eip;
    if (!ScanForReturnAddress(location_start, &location, &eip)) {
      // if we can't find an instruction pointer even with stack scanning,
      // give up.
      return NULL;
    }

    // This seems like a reasonable return address. Since program string
    // evaluation failed, use it and set %esp to the location above the
    // one where the return address was found.
    dictionary["$eip"] = eip;
    dictionary["$esp"] = location + 4;
    trust = StackFrame::FRAME_TRUST_SCAN;
  }

  // Since this stack frame did not use %ebp in a traditional way,
  // locating the return address isn't entirely deterministic. In that
  // case, the stack can be scanned to locate the return address.
  //
  // However, if program string evaluation resulted in both %eip and
  // %ebp values of 0, trust that the end of the stack has been
  // reached and don't scan for anything else.
  if (dictionary["$eip"] != 0 || dictionary["$ebp"] != 0) {
    int offset = 0;

    // This scan can only be done if a CodeModules object is available, to
    // check that candidate return addresses are in fact inside a module.
    //
    // TODO(mmentovai): This ignores dynamically-generated code.  One possible
    // solution is to check the minidump's memory map to see if the candidate
    // %eip value comes from a mapped executable page, although this would
    // require dumps that contain MINIDUMP_MEMORY_INFO, which the Breakpad
    // client doesn't currently write (it would need to call MiniDumpWriteDump
    // with the MiniDumpWithFullMemoryInfo type bit set).  Even given this
    // ability, older OSes (pre-XP SP2) and CPUs (pre-P4) don't enforce
    // an independent execute privilege on memory pages.

    u_int32_t eip = dictionary["$eip"];
    if (modules_ && !modules_->GetModuleForAddress(eip)) {
      // The instruction pointer at .raSearchStart was invalid, so start
      // looking one 32-bit word above that location.
      u_int32_t location_start = dictionary[".raSearchStart"] + 4;
      u_int32_t location;
      if (ScanForReturnAddress(location_start, &location, &eip)) {
        // This is a better return address that what program string
        // evaluation found.  Use it, and set %esp to the location above the
        // one where the return address was found.
        dictionary["$eip"] = eip;
        dictionary["$esp"] = location + 4;
        offset = location - location_start;
        trust = StackFrame::FRAME_TRUST_CFI_SCAN;
      }
    }

    // When trying to recover the previous value of the frame pointer (%ebp),
    // start looking at the lowest possible address in the saved-register
    // area, and look at the entire saved register area, increased by the
    // size of |offset| to account for additional data that may be on the
    // stack.  The scan is performed from the highest possible address to
    // the lowest, because we expect that the function's prolog would have
    // saved %ebp early.
    u_int32_t ebp = dictionary["$ebp"];
    u_int32_t value;  // throwaway variable to check pointer validity
    if (recover_ebp && !memory_->GetMemoryAtAddress(ebp, &value)) {
      int fp_search_bytes = last_frame_info->saved_register_size + offset;
      u_int32_t location_end = last_frame->context.esp +
                               last_frame_callee_parameter_size;

      for (u_int32_t location = location_end + fp_search_bytes;
           location >= location_end;
           location -= 4) {
        if (!memory_->GetMemoryAtAddress(location, &ebp))
          break;

        if (memory_->GetMemoryAtAddress(ebp, &value)) {
          // The candidate value is a pointer to the same memory region
          // (the stack).  Prefer it as a recovered %ebp result.
          dictionary["$ebp"] = ebp;
          break;
        }
      }
    }
  }

  // Create a new stack frame (ownership will be transferred to the caller)
  // and fill it in.
  StackFrameX86 *frame = new StackFrameX86();

  frame->trust = trust;
  frame->context = last_frame->context;
  frame->context.eip = dictionary["$eip"];
  frame->context.esp = dictionary["$esp"];
  frame->context.ebp = dictionary["$ebp"];
  frame->context_validity = StackFrameX86::CONTEXT_VALID_EIP |
                                StackFrameX86::CONTEXT_VALID_ESP |
                                StackFrameX86::CONTEXT_VALID_EBP;

  // These are nonvolatile (callee-save) registers, and the program string
  // may have filled them in.
  if (dictionary_validity.find("$ebx") != dictionary_validity.end()) {
    frame->context.ebx = dictionary["$ebx"];
    frame->context_validity |= StackFrameX86::CONTEXT_VALID_EBX;
  }
  if (dictionary_validity.find("$esi") != dictionary_validity.end()) {
    frame->context.esi = dictionary["$esi"];
    frame->context_validity |= StackFrameX86::CONTEXT_VALID_ESI;
  }
  if (dictionary_validity.find("$edi") != dictionary_validity.end()) {
    frame->context.edi = dictionary["$edi"];
    frame->context_validity |= StackFrameX86::CONTEXT_VALID_EDI;
  }

  return frame;
}

StackFrameX86 *StackwalkerX86::GetCallerByCFIFrameInfo(
    const vector<StackFrame*> &frames,
    CFIFrameInfo *cfi_frame_info) {
  StackFrameX86 *last_frame = static_cast<StackFrameX86*>(frames.back());
  last_frame->cfi_frame_info = cfi_frame_info;

  scoped_ptr<StackFrameX86> frame(new StackFrameX86());
  if (!cfi_walker_
      .FindCallerRegisters(*memory_, *cfi_frame_info,
                           last_frame->context, last_frame->context_validity,
                           &frame->context, &frame->context_validity))
    return NULL;
  
  // Make sure we recovered all the essentials.
  static const int essentials = (StackFrameX86::CONTEXT_VALID_EIP
                                 | StackFrameX86::CONTEXT_VALID_ESP
                                 | StackFrameX86::CONTEXT_VALID_EBP);
  if ((frame->context_validity & essentials) != essentials)
    return NULL;

  frame->trust = StackFrame::FRAME_TRUST_CFI;

  return frame.release();
}

StackFrameX86 *StackwalkerX86::GetCallerByEBPAtBase(
    const vector<StackFrame *> &frames) {
  StackFrame::FrameTrust trust;
  StackFrameX86 *last_frame = static_cast<StackFrameX86 *>(frames.back());
  u_int32_t last_esp = last_frame->context.esp;
  u_int32_t last_ebp = last_frame->context.ebp;

  // Assume that the standard %ebp-using x86 calling convention is in
  // use.
  //
  // The typical x86 calling convention, when frame pointers are present,
  // is for the calling procedure to use CALL, which pushes the return
  // address onto the stack and sets the instruction pointer (%eip) to
  // the entry point of the called routine.  The called routine then
  // PUSHes the calling routine's frame pointer (%ebp) onto the stack
  // before copying the stack pointer (%esp) to the frame pointer (%ebp).
  // Therefore, the calling procedure's frame pointer is always available
  // by dereferencing the called procedure's frame pointer, and the return
  // address is always available at the memory location immediately above
  // the address pointed to by the called procedure's frame pointer.  The
  // calling procedure's stack pointer (%esp) is 8 higher than the value
  // of the called procedure's frame pointer at the time the calling
  // procedure made the CALL: 4 bytes for the return address pushed by the
  // CALL itself, and 4 bytes for the callee's PUSH of the caller's frame
  // pointer.
  //
  // %eip_new = *(%ebp_old + 4)
  // %esp_new = %ebp_old + 8
  // %ebp_new = *(%ebp_old)

  u_int32_t caller_eip, caller_esp, caller_ebp;

  if (memory_->GetMemoryAtAddress(last_ebp + 4, &caller_eip) &&
      memory_->GetMemoryAtAddress(last_ebp, &caller_ebp)) {
    caller_esp = last_ebp + 8;
    trust = StackFrame::FRAME_TRUST_FP;
  } else {
    // We couldn't read the memory %ebp refers to. It may be that %ebp
    // is pointing to non-stack memory. We'll scan the stack for a
    // return address. This can happen if last_frame is executing code
    // for a module for which we don't have symbols, and that module
    // is compiled without a frame pointer.
    if (!ScanForReturnAddress(last_esp, &caller_esp, &caller_eip)) {
      // if we can't find an instruction pointer even with stack scanning,
      // give up.
      return NULL;
    }

    // ScanForReturnAddress found a reasonable return address. Advance
    // %esp to the location above the one where the return address was
    // found. Assume that %ebp is unchanged.
    caller_esp += 4;
    caller_ebp = last_ebp;

    trust = StackFrame::FRAME_TRUST_SCAN;
  }

  // Create a new stack frame (ownership will be transferred to the caller)
  // and fill it in.
  StackFrameX86 *frame = new StackFrameX86();

  frame->trust = trust;
  frame->context = last_frame->context;
  frame->context.eip = caller_eip;
  frame->context.esp = caller_esp;
  frame->context.ebp = caller_ebp;
  frame->context_validity = StackFrameX86::CONTEXT_VALID_EIP |
                            StackFrameX86::CONTEXT_VALID_ESP |
                            StackFrameX86::CONTEXT_VALID_EBP;

  return frame;
}

StackFrame *StackwalkerX86::GetCallerFrame(const CallStack *stack) {
  if (!memory_ || !stack) {
    BPLOG(ERROR) << "Can't get caller frame without memory or stack";
    return NULL;
  }

  const vector<StackFrame *> &frames = *stack->frames();
  StackFrameX86 *last_frame = static_cast<StackFrameX86 *>(frames.back());
  scoped_ptr<StackFrameX86> new_frame;

  // If the resolver has Windows stack walking information, use that.
  WindowsFrameInfo *windows_frame_info
      = resolver_ ? resolver_->FindWindowsFrameInfo(last_frame) : NULL;
  if (windows_frame_info)
    new_frame.reset(GetCallerByWindowsFrameInfo(frames, windows_frame_info));

  // If the resolver has DWARF CFI information, use that.
  if (!new_frame.get()) {
    CFIFrameInfo *cfi_frame_info = 
        resolver_ ? resolver_->FindCFIFrameInfo(last_frame) : NULL;
    if (cfi_frame_info)
      new_frame.reset(GetCallerByCFIFrameInfo(frames, cfi_frame_info));
  }

  // Otherwise, hope that the program was using a traditional frame structure.
  if (!new_frame.get())
    new_frame.reset(GetCallerByEBPAtBase(frames));

  // If nothing worked, tell the caller.
  if (!new_frame.get())
    return NULL;
  
  // Treat an instruction address of 0 as end-of-stack.
  if (new_frame->context.eip == 0)
    return NULL;

  // If the new stack pointer is at a lower address than the old, then
  // that's clearly incorrect. Treat this as end-of-stack to enforce
  // progress and avoid infinite loops.
  if (new_frame->context.esp <= last_frame->context.esp)
    return NULL;

  // new_frame->context.eip is the return address, which is one instruction
  // past the CALL that caused us to arrive at the callee. Set
  // new_frame->instruction to one less than that. This won't reference the
  // beginning of the CALL instruction, but it's guaranteed to be within
  // the CALL, which is sufficient to get the source line information to
  // match up with the line that contains a function call. Callers that
  // require the exact return address value may access the context.eip
  // field of StackFrameX86.
  new_frame->instruction = new_frame->context.eip - 1;

  return new_frame.release();
}

}  // namespace google_breakpad
