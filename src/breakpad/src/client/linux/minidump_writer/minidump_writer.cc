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

// This code writes out minidump files:
//   http://msdn.microsoft.com/en-us/library/ms680378(VS.85,loband).aspx
//
// Minidumps are a Microsoft format which Breakpad uses for recording crash
// dumps. This code has to run in a compromised environment (the address space
// may have received SIGSEGV), thus the following rules apply:
//   * You may not enter the dynamic linker. This means that we cannot call
//     any symbols in a shared library (inc libc). Because of this we replace
//     libc functions in linux_libc_support.h.
//   * You may not call syscalls via the libc wrappers. This rule is a subset
//     of the first rule but it bears repeating. We have direct wrappers
//     around the system calls in linux_syscall_support.h.
//   * You may not malloc. There's an alternative allocator in memory.h and
//     a canonical instance in the LinuxDumper object. We use the placement
//     new form to allocate objects and we don't delete them.

#include "client/linux/minidump_writer/minidump_writer.h"
#include "client/minidump_file_writer-inl.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#if !defined(__ANDROID__)
#include <link.h>
#endif
#include <stdio.h>
#if !defined(__ANDROID__)
#include <sys/ucontext.h>
#include <sys/user.h>
#endif
#include <sys/utsname.h>
#include <unistd.h>

#include <algorithm>

#include "client/minidump_file_writer.h"
#include "google_breakpad/common/minidump_format.h"

#if defined(__ANDROID__)
#include "client/linux/android_link.h"
#include "client/linux/android_ucontext.h"
#endif
#include "client/linux/handler/exception_handler.h"
#include "client/linux/minidump_writer/line_reader.h"
#include "client/linux/minidump_writer/linux_dumper.h"
#include "client/linux/minidump_writer/linux_ptrace_dumper.h"
#include "client/linux/minidump_writer/minidump_extension_linux.h"
#include "client/minidump_file_writer.h"
#include "common/linux/linux_libc_support.h"
#include "google_breakpad/common/minidump_format.h"
#include "third_party/lss/linux_syscall_support.h"

// Minidump defines register structures which are different from the raw
// structures which we get from the kernel. These are platform specific
// functions to juggle the ucontext and user structures into minidump format.
#if defined(__i386)
typedef MDRawContextX86 RawContextCPU;

// Write a uint16_t to memory
//   out: memory location to write to
//   v: value to write.
static void U16(void* out, uint16_t v) {
  memcpy(out, &v, sizeof(v));
}

// Write a uint32_t to memory
//   out: memory location to write to
//   v: value to write.
static void U32(void* out, uint32_t v) {
  memcpy(out, &v, sizeof(v));
}

// Juggle an x86 user_(fp|fpx|)regs_struct into minidump format
//   out: the minidump structure
//   info: the collection of register structures.
static void CPUFillFromThreadInfo(MDRawContextX86 *out,
                                  const google_breakpad::ThreadInfo &info) {
  out->context_flags = MD_CONTEXT_X86_ALL;

  out->dr0 = info.dregs[0];
  out->dr1 = info.dregs[1];
  out->dr2 = info.dregs[2];
  out->dr3 = info.dregs[3];
  // 4 and 5 deliberatly omitted because they aren't included in the minidump
  // format.
  out->dr6 = info.dregs[6];
  out->dr7 = info.dregs[7];

  out->gs = info.regs.xgs;
  out->fs = info.regs.xfs;
  out->es = info.regs.xes;
  out->ds = info.regs.xds;

  out->edi = info.regs.edi;
  out->esi = info.regs.esi;
  out->ebx = info.regs.ebx;
  out->edx = info.regs.edx;
  out->ecx = info.regs.ecx;
  out->eax = info.regs.eax;

  out->ebp = info.regs.ebp;
  out->eip = info.regs.eip;
  out->cs = info.regs.xcs;
  out->eflags = info.regs.eflags;
  out->esp = info.regs.esp;
  out->ss = info.regs.xss;

  out->float_save.control_word = info.fpregs.cwd;
  out->float_save.status_word = info.fpregs.swd;
  out->float_save.tag_word = info.fpregs.twd;
  out->float_save.error_offset = info.fpregs.fip;
  out->float_save.error_selector = info.fpregs.fcs;
  out->float_save.data_offset = info.fpregs.foo;
  out->float_save.data_selector = info.fpregs.fos;

  // 8 registers * 10 bytes per register.
  memcpy(out->float_save.register_area, info.fpregs.st_space, 10 * 8);

  // This matches the Intel fpsave format.
  U16(out->extended_registers + 0, info.fpregs.cwd);
  U16(out->extended_registers + 2, info.fpregs.swd);
  U16(out->extended_registers + 4, info.fpregs.twd);
  U16(out->extended_registers + 6, info.fpxregs.fop);
  U32(out->extended_registers + 8, info.fpxregs.fip);
  U16(out->extended_registers + 12, info.fpxregs.fcs);
  U32(out->extended_registers + 16, info.fpregs.foo);
  U16(out->extended_registers + 20, info.fpregs.fos);
  U32(out->extended_registers + 24, info.fpxregs.mxcsr);

  memcpy(out->extended_registers + 32, &info.fpxregs.st_space, 128);
  memcpy(out->extended_registers + 160, &info.fpxregs.xmm_space, 128);
}

// Juggle an x86 ucontext into minidump format
//   out: the minidump structure
//   info: the collection of register structures.
static void CPUFillFromUContext(MDRawContextX86 *out, const ucontext *uc,
                                const struct _libc_fpstate* fp) {
  const greg_t* regs = uc->uc_mcontext.gregs;

  out->context_flags = MD_CONTEXT_X86_FULL |
                       MD_CONTEXT_X86_FLOATING_POINT;

  out->gs = regs[REG_GS];
  out->fs = regs[REG_FS];
  out->es = regs[REG_ES];
  out->ds = regs[REG_DS];

  out->edi = regs[REG_EDI];
  out->esi = regs[REG_ESI];
  out->ebx = regs[REG_EBX];
  out->edx = regs[REG_EDX];
  out->ecx = regs[REG_ECX];
  out->eax = regs[REG_EAX];

  out->ebp = regs[REG_EBP];
  out->eip = regs[REG_EIP];
  out->cs = regs[REG_CS];
  out->eflags = regs[REG_EFL];
  out->esp = regs[REG_UESP];
  out->ss = regs[REG_SS];

  out->float_save.control_word = fp->cw;
  out->float_save.status_word = fp->sw;
  out->float_save.tag_word = fp->tag;
  out->float_save.error_offset = fp->ipoff;
  out->float_save.error_selector = fp->cssel;
  out->float_save.data_offset = fp->dataoff;
  out->float_save.data_selector = fp->datasel;

  // 8 registers * 10 bytes per register.
  memcpy(out->float_save.register_area, fp->_st, 10 * 8);
}

#elif defined(__x86_64)
typedef MDRawContextAMD64 RawContextCPU;

static void CPUFillFromThreadInfo(MDRawContextAMD64 *out,
                                  const google_breakpad::ThreadInfo &info) {
  out->context_flags = MD_CONTEXT_AMD64_FULL |
                       MD_CONTEXT_AMD64_SEGMENTS;

  out->cs = info.regs.cs;

  out->ds = info.regs.ds;
  out->es = info.regs.es;
  out->fs = info.regs.fs;
  out->gs = info.regs.gs;

  out->ss = info.regs.ss;
  out->eflags = info.regs.eflags;

  out->dr0 = info.dregs[0];
  out->dr1 = info.dregs[1];
  out->dr2 = info.dregs[2];
  out->dr3 = info.dregs[3];
  // 4 and 5 deliberatly omitted because they aren't included in the minidump
  // format.
  out->dr6 = info.dregs[6];
  out->dr7 = info.dregs[7];

  out->rax = info.regs.rax;
  out->rcx = info.regs.rcx;
  out->rdx = info.regs.rdx;
  out->rbx = info.regs.rbx;

  out->rsp = info.regs.rsp;

  out->rbp = info.regs.rbp;
  out->rsi = info.regs.rsi;
  out->rdi = info.regs.rdi;
  out->r8 = info.regs.r8;
  out->r9 = info.regs.r9;
  out->r10 = info.regs.r10;
  out->r11 = info.regs.r11;
  out->r12 = info.regs.r12;
  out->r13 = info.regs.r13;
  out->r14 = info.regs.r14;
  out->r15 = info.regs.r15;

  out->rip = info.regs.rip;

  out->flt_save.control_word = info.fpregs.cwd;
  out->flt_save.status_word = info.fpregs.swd;
  out->flt_save.tag_word = info.fpregs.ftw;
  out->flt_save.error_opcode = info.fpregs.fop;
  out->flt_save.error_offset = info.fpregs.rip;
  out->flt_save.error_selector = 0;  // We don't have this.
  out->flt_save.data_offset = info.fpregs.rdp;
  out->flt_save.data_selector = 0;   // We don't have this.
  out->flt_save.mx_csr = info.fpregs.mxcsr;
  out->flt_save.mx_csr_mask = info.fpregs.mxcr_mask;
  memcpy(&out->flt_save.float_registers, &info.fpregs.st_space, 8 * 16);
  memcpy(&out->flt_save.xmm_registers, &info.fpregs.xmm_space, 16 * 16);
}

static void CPUFillFromUContext(MDRawContextAMD64 *out, const ucontext *uc,
                                const struct _libc_fpstate* fpregs) {
  const greg_t* regs = uc->uc_mcontext.gregs;

  out->context_flags = MD_CONTEXT_AMD64_FULL;

  out->cs = regs[REG_CSGSFS] & 0xffff;

  out->fs = (regs[REG_CSGSFS] >> 32) & 0xffff;
  out->gs = (regs[REG_CSGSFS] >> 16) & 0xffff;

  out->eflags = regs[REG_EFL];

  out->rax = regs[REG_RAX];
  out->rcx = regs[REG_RCX];
  out->rdx = regs[REG_RDX];
  out->rbx = regs[REG_RBX];

  out->rsp = regs[REG_RSP];
  out->rbp = regs[REG_RBP];
  out->rsi = regs[REG_RSI];
  out->rdi = regs[REG_RDI];
  out->r8 = regs[REG_R8];
  out->r9 = regs[REG_R9];
  out->r10 = regs[REG_R10];
  out->r11 = regs[REG_R11];
  out->r12 = regs[REG_R12];
  out->r13 = regs[REG_R13];
  out->r14 = regs[REG_R14];
  out->r15 = regs[REG_R15];

  out->rip = regs[REG_RIP];

  out->flt_save.control_word = fpregs->cwd;
  out->flt_save.status_word = fpregs->swd;
  out->flt_save.tag_word = fpregs->ftw;
  out->flt_save.error_opcode = fpregs->fop;
  out->flt_save.error_offset = fpregs->rip;
  out->flt_save.data_offset = fpregs->rdp;
  out->flt_save.error_selector = 0;  // We don't have this.
  out->flt_save.data_selector = 0;  // We don't have this.
  out->flt_save.mx_csr = fpregs->mxcsr;
  out->flt_save.mx_csr_mask = fpregs->mxcr_mask;
  memcpy(&out->flt_save.float_registers, &fpregs->_st, 8 * 16);
  memcpy(&out->flt_save.xmm_registers, &fpregs->_xmm, 16 * 16);
}

#elif defined(__ARMEL__)
typedef MDRawContextARM RawContextCPU;

static void CPUFillFromThreadInfo(MDRawContextARM *out,
                                  const google_breakpad::ThreadInfo &info) {
  out->context_flags = MD_CONTEXT_ARM_FULL;

  for (int i = 0; i < MD_CONTEXT_ARM_GPR_COUNT; ++i)
    out->iregs[i] = info.regs.uregs[i];
  // No CPSR register in ThreadInfo(it's not accessible via ptrace)
  out->cpsr = 0;
#if !defined(__ANDROID__)
  out->float_save.fpscr = info.fpregs.fpsr |
    (static_cast<u_int64_t>(info.fpregs.fpcr) << 32);
  // TODO: sort this out, actually collect floating point registers
  memset(&out->float_save.regs, 0, sizeof(out->float_save.regs));
  memset(&out->float_save.extra, 0, sizeof(out->float_save.extra));
#endif
}

static void CPUFillFromUContext(MDRawContextARM *out, const ucontext *uc,
                                const struct _libc_fpstate* fpregs) {
  out->context_flags = MD_CONTEXT_ARM_FULL;

  out->iregs[0] = uc->uc_mcontext.arm_r0;
  out->iregs[1] = uc->uc_mcontext.arm_r1;
  out->iregs[2] = uc->uc_mcontext.arm_r2;
  out->iregs[3] = uc->uc_mcontext.arm_r3;
  out->iregs[4] = uc->uc_mcontext.arm_r4;
  out->iregs[5] = uc->uc_mcontext.arm_r5;
  out->iregs[6] = uc->uc_mcontext.arm_r6;
  out->iregs[7] = uc->uc_mcontext.arm_r7;
  out->iregs[8] = uc->uc_mcontext.arm_r8;
  out->iregs[9] = uc->uc_mcontext.arm_r9;
  out->iregs[10] = uc->uc_mcontext.arm_r10;

  out->iregs[11] = uc->uc_mcontext.arm_fp;
  out->iregs[12] = uc->uc_mcontext.arm_ip;
  out->iregs[13] = uc->uc_mcontext.arm_sp;
  out->iregs[14] = uc->uc_mcontext.arm_lr;
  out->iregs[15] = uc->uc_mcontext.arm_pc;

  out->cpsr = uc->uc_mcontext.arm_cpsr;

  // TODO: fix this after fixing ExceptionHandler
  out->float_save.fpscr = 0;
  memset(&out->float_save.regs, 0, sizeof(out->float_save.regs));
  memset(&out->float_save.extra, 0, sizeof(out->float_save.extra));
}

#else
#error "This code has not been ported to your platform yet."
#endif

namespace google_breakpad {

class MinidumpWriter {
 public:
  MinidumpWriter(const char* filename,
                 const ExceptionHandler::CrashContext* context,
                 const MappingList& mappings,
                 LinuxDumper* dumper)
      : filename_(filename),
        ucontext_(context ? &context->context : NULL),
#if !defined(__ARM_EABI__)
        float_state_(context ? &context->float_state : NULL),
#else
        // TODO: fix this after fixing ExceptionHandler
        float_state_(NULL),
#endif
        dumper_(dumper),
        memory_blocks_(dumper_->allocator()),
        mapping_list_(mappings) {
  }

  bool Init() {
    return dumper_->Init() && minidump_writer_.Open(filename_) &&
           dumper_->ThreadsSuspend();
  }

  ~MinidumpWriter() {
    minidump_writer_.Close();
    dumper_->ThreadsResume();
  }

  bool Dump() {
    // The dynamic linker makes information available that helps gdb find all
    // DSOs loaded into the program. If we can access this information, we dump
    // it to a MD_LINUX_DSO_DEBUG stream.
    struct r_debug* r_debug = NULL;
    uint32_t dynamic_length = 0;
#if !defined(__ANDROID__)
    // This code assumes the crashing process is the same as this process and
    // may hang or take a long time to complete if not so.
    // Thus, we skip this code for a post-mortem based dump.
    if (!dumper_->IsPostMortem()) {
      // The Android NDK is missing structure definitions for most of this.
      // For now, it's simpler just to skip it.
      for (int i = 0;;) {
        ElfW(Dyn) dyn;
        dynamic_length += sizeof(dyn);
        // NOTE: Use of _DYNAMIC assumes this is the same process as the
        // crashing process. This loop will go forever if it's out of bounds.
        dumper_->CopyFromProcess(&dyn, GetCrashThread(), _DYNAMIC+i++,
                                 sizeof(dyn));
        if (dyn.d_tag == DT_DEBUG) {
          r_debug = (struct r_debug*)dyn.d_un.d_ptr;
          continue;
        } else if (dyn.d_tag == DT_NULL) {
          break;
        }
      }
    }
#endif

    // A minidump file contains a number of tagged streams. This is the number
    // of stream which we write.
    unsigned kNumWriters = 12;
    if (r_debug)
      ++kNumWriters;

    TypedMDRVA<MDRawHeader> header(&minidump_writer_);
    TypedMDRVA<MDRawDirectory> dir(&minidump_writer_);
    if (!header.Allocate())
      return false;
    if (!dir.AllocateArray(kNumWriters))
      return false;
    memset(header.get(), 0, sizeof(MDRawHeader));

    header.get()->signature = MD_HEADER_SIGNATURE;
    header.get()->version = MD_HEADER_VERSION;
    header.get()->time_date_stamp = time(NULL);
    header.get()->stream_count = kNumWriters;
    header.get()->stream_directory_rva = dir.position();

    unsigned dir_index = 0;
    MDRawDirectory dirent;

    if (!WriteThreadListStream(&dirent))
      return false;
    dir.CopyIndex(dir_index++, &dirent);

    if (!WriteMappings(&dirent))
      return false;
    dir.CopyIndex(dir_index++, &dirent);

    if (!WriteMemoryListStream(&dirent))
      return false;
    dir.CopyIndex(dir_index++, &dirent);

    if (!WriteExceptionStream(&dirent))
      return false;
    dir.CopyIndex(dir_index++, &dirent);

    if (!WriteSystemInfoStream(&dirent))
      return false;
    dir.CopyIndex(dir_index++, &dirent);

    dirent.stream_type = MD_LINUX_CPU_INFO;
    if (!WriteFile(&dirent.location, "/proc/cpuinfo"))
      NullifyDirectoryEntry(&dirent);
    dir.CopyIndex(dir_index++, &dirent);

    dirent.stream_type = MD_LINUX_PROC_STATUS;
    if (!WriteProcFile(&dirent.location, GetCrashThread(), "status"))
      NullifyDirectoryEntry(&dirent);
    dir.CopyIndex(dir_index++, &dirent);

    dirent.stream_type = MD_LINUX_LSB_RELEASE;
    if (!WriteFile(&dirent.location, "/etc/lsb-release"))
      NullifyDirectoryEntry(&dirent);
    dir.CopyIndex(dir_index++, &dirent);

    dirent.stream_type = MD_LINUX_CMD_LINE;
    if (!WriteProcFile(&dirent.location, GetCrashThread(), "cmdline"))
      NullifyDirectoryEntry(&dirent);
    dir.CopyIndex(dir_index++, &dirent);

    dirent.stream_type = MD_LINUX_ENVIRON;
    if (!WriteProcFile(&dirent.location, GetCrashThread(), "environ"))
      NullifyDirectoryEntry(&dirent);
    dir.CopyIndex(dir_index++, &dirent);

    dirent.stream_type = MD_LINUX_AUXV;
    if (!WriteProcFile(&dirent.location, GetCrashThread(), "auxv"))
      NullifyDirectoryEntry(&dirent);
    dir.CopyIndex(dir_index++, &dirent);

    dirent.stream_type = MD_LINUX_MAPS;
    if (!WriteProcFile(&dirent.location, GetCrashThread(), "maps"))
      NullifyDirectoryEntry(&dirent);
    dir.CopyIndex(dir_index++, &dirent);

    if (r_debug) {
      dirent.stream_type = MD_LINUX_DSO_DEBUG;
      if (!WriteDSODebugStream(&dirent, r_debug, dynamic_length))
        NullifyDirectoryEntry(&dirent);
      dir.CopyIndex(dir_index++, &dirent);
    }

    // If you add more directory entries, don't forget to update kNumWriters,
    // above.

    dumper_->ThreadsResume();
    return true;
  }

  // Check if the top of the stack is part of a system call that has been
  // redirected by the seccomp sandbox. If so, try to pop the stack frames
  // all the way back to the point where the interception happened.
  void PopSeccompStackFrame(RawContextCPU* cpu, const MDRawThread& thread,
                            uint8_t* stack_copy) {
#if defined(__x86_64)
    u_int64_t bp = cpu->rbp;
    u_int64_t top = thread.stack.start_of_memory_range;
    for (int i = 4; i--; ) {
      if (bp < top ||
          bp + sizeof(bp) > thread.stack.start_of_memory_range +
          thread.stack.memory.data_size ||
          bp & 1) {
        break;
      }
      uint64_t old_top = top;
      top = bp;
      u_int8_t* bp_addr = stack_copy + bp - thread.stack.start_of_memory_range;
      memcpy(&bp, bp_addr, sizeof(bp));
      if (bp == 0xDEADBEEFDEADBEEFull) {
        struct {
          uint64_t r15;
          uint64_t r14;
          uint64_t r13;
          uint64_t r12;
          uint64_t r11;
          uint64_t r10;
          uint64_t r9;
          uint64_t r8;
          uint64_t rdi;
          uint64_t rsi;
          uint64_t rdx;
          uint64_t rcx;
          uint64_t rbx;
          uint64_t deadbeef;
          uint64_t rbp;
          uint64_t fakeret;
          uint64_t ret;
          /* char redzone[128]; */
        } seccomp_stackframe;
        if (top - offsetof(typeof(seccomp_stackframe), deadbeef) < old_top ||
            top - offsetof(typeof(seccomp_stackframe), deadbeef) +
            sizeof(seccomp_stackframe) >
            thread.stack.start_of_memory_range+thread.stack.memory.data_size) {
          break;
        }
        memcpy(&seccomp_stackframe,
               bp_addr - offsetof(typeof(seccomp_stackframe), deadbeef),
               sizeof(seccomp_stackframe));
        cpu->rbx = seccomp_stackframe.rbx;
        cpu->rcx = seccomp_stackframe.rcx;
        cpu->rdx = seccomp_stackframe.rdx;
        cpu->rsi = seccomp_stackframe.rsi;
        cpu->rdi = seccomp_stackframe.rdi;
        cpu->rbp = seccomp_stackframe.rbp;
        cpu->rsp = top + 4*sizeof(uint64_t) + 128;
        cpu->r8  = seccomp_stackframe.r8;
        cpu->r9  = seccomp_stackframe.r9;
        cpu->r10 = seccomp_stackframe.r10;
        cpu->r11 = seccomp_stackframe.r11;
        cpu->r12 = seccomp_stackframe.r12;
        cpu->r13 = seccomp_stackframe.r13;
        cpu->r14 = seccomp_stackframe.r14;
        cpu->r15 = seccomp_stackframe.r15;
        cpu->rip = seccomp_stackframe.fakeret;
        return;
      }
    }
#elif defined(__i386)
    u_int32_t bp = cpu->ebp;
    u_int32_t top = thread.stack.start_of_memory_range;
    for (int i = 4; i--; ) {
      if (bp < top ||
          bp + sizeof(bp) > thread.stack.start_of_memory_range +
          thread.stack.memory.data_size ||
          bp & 1) {
        break;
      }
      uint32_t old_top = top;
      top = bp;
      u_int8_t* bp_addr = stack_copy + bp - thread.stack.start_of_memory_range;
      memcpy(&bp, bp_addr, sizeof(bp));
      if (bp == 0xDEADBEEFu) {
        struct {
          uint32_t edi;
          uint32_t esi;
          uint32_t edx;
          uint32_t ecx;
          uint32_t ebx;
          uint32_t deadbeef;
          uint32_t ebp;
          uint32_t fakeret;
          uint32_t ret;
        } seccomp_stackframe;
        if (top - offsetof(typeof(seccomp_stackframe), deadbeef) < old_top ||
            top - offsetof(typeof(seccomp_stackframe), deadbeef) +
            sizeof(seccomp_stackframe) >
            thread.stack.start_of_memory_range+thread.stack.memory.data_size) {
          break;
        }
        memcpy(&seccomp_stackframe,
               bp_addr - offsetof(typeof(seccomp_stackframe), deadbeef),
               sizeof(seccomp_stackframe));
        cpu->ebx = seccomp_stackframe.ebx;
        cpu->ecx = seccomp_stackframe.ecx;
        cpu->edx = seccomp_stackframe.edx;
        cpu->esi = seccomp_stackframe.esi;
        cpu->edi = seccomp_stackframe.edi;
        cpu->ebp = seccomp_stackframe.ebp;
        cpu->esp = top + 4*sizeof(void*);
        cpu->eip = seccomp_stackframe.fakeret;
        return;
      }
    }
#endif
  }

  // Write information about the threads.
  bool WriteThreadListStream(MDRawDirectory* dirent) {
    const unsigned num_threads = dumper_->threads().size();

    TypedMDRVA<uint32_t> list(&minidump_writer_);
    if (!list.AllocateObjectAndArray(num_threads, sizeof(MDRawThread)))
      return false;

    dirent->stream_type = MD_THREAD_LIST_STREAM;
    dirent->location = list.location();

    *list.get() = num_threads;

    for (unsigned i = 0; i < num_threads; ++i) {
      MDRawThread thread;
      my_memset(&thread, 0, sizeof(thread));
      thread.thread_id = dumper_->threads()[i];
      // We have a different source of information for the crashing thread. If
      // we used the actual state of the thread we would find it running in the
      // signal handler with the alternative stack, which would be deeply
      // unhelpful.
      if (static_cast<pid_t>(thread.thread_id) == GetCrashThread() &&
          !dumper_->IsPostMortem()) {
        const void* stack;
        size_t stack_len;
        if (!dumper_->GetStackInfo(&stack, &stack_len, GetStackPointer()))
          return false;
        UntypedMDRVA memory(&minidump_writer_);
        if (!memory.Allocate(stack_len))
          return false;
        uint8_t* stack_copy = reinterpret_cast<uint8_t*>(Alloc(stack_len));
        dumper_->CopyFromProcess(stack_copy, thread.thread_id, stack,
                                 stack_len);
        memory.Copy(stack_copy, stack_len);
        thread.stack.start_of_memory_range = (uintptr_t) (stack);
        thread.stack.memory = memory.location();
        memory_blocks_.push_back(thread.stack);

        // Copy 256 bytes around crashing instruction pointer to minidump.
        const size_t kIPMemorySize = 256;
        u_int64_t ip = GetInstructionPointer();
        // Bound it to the upper and lower bounds of the memory map
        // it's contained within. If it's not in mapped memory,
        // don't bother trying to write it.
        bool ip_is_mapped = false;
        MDMemoryDescriptor ip_memory_d;
        for (unsigned j = 0; j < dumper_->mappings().size(); ++j) {
          const MappingInfo& mapping = *dumper_->mappings()[j];
          if (ip >= mapping.start_addr &&
              ip < mapping.start_addr + mapping.size) {
            ip_is_mapped = true;
            // Try to get 128 bytes before and after the IP, but
            // settle for whatever's available.
            ip_memory_d.start_of_memory_range =
              std::max(mapping.start_addr,
                       uintptr_t(ip - (kIPMemorySize / 2)));
            uintptr_t end_of_range =
              std::min(uintptr_t(ip + (kIPMemorySize / 2)),
                       uintptr_t(mapping.start_addr + mapping.size));
            ip_memory_d.memory.data_size =
              end_of_range - ip_memory_d.start_of_memory_range;
            break;
          }
        }

        if (ip_is_mapped) {
          UntypedMDRVA ip_memory(&minidump_writer_);
          if (!ip_memory.Allocate(ip_memory_d.memory.data_size))
            return false;
          uint8_t* memory_copy =
              reinterpret_cast<uint8_t*>(Alloc(ip_memory_d.memory.data_size));
          dumper_->CopyFromProcess(
              memory_copy,
              thread.thread_id,
              reinterpret_cast<void*>(ip_memory_d.start_of_memory_range),
              ip_memory_d.memory.data_size);
          ip_memory.Copy(memory_copy, ip_memory_d.memory.data_size);
          ip_memory_d.memory = ip_memory.location();
          memory_blocks_.push_back(ip_memory_d);
        }

        TypedMDRVA<RawContextCPU> cpu(&minidump_writer_);
        if (!cpu.Allocate())
          return false;
        my_memset(cpu.get(), 0, sizeof(RawContextCPU));
        CPUFillFromUContext(cpu.get(), ucontext_, float_state_);
        PopSeccompStackFrame(cpu.get(), thread, stack_copy);
        thread.thread_context = cpu.location();
        crashing_thread_context_ = cpu.location();
      } else {
        ThreadInfo info;
        if (!dumper_->GetThreadInfoByIndex(i, &info))
          return false;
        UntypedMDRVA memory(&minidump_writer_);
        if (!memory.Allocate(info.stack_len))
          return false;
        uint8_t* stack_copy = reinterpret_cast<uint8_t*>(Alloc(info.stack_len));
        dumper_->CopyFromProcess(stack_copy, thread.thread_id, info.stack,
                                 info.stack_len);
        memory.Copy(stack_copy, info.stack_len);
        thread.stack.start_of_memory_range = (uintptr_t)(info.stack);
        thread.stack.memory = memory.location();
        memory_blocks_.push_back(thread.stack);

        TypedMDRVA<RawContextCPU> cpu(&minidump_writer_);
        if (!cpu.Allocate())
          return false;
        my_memset(cpu.get(), 0, sizeof(RawContextCPU));
        CPUFillFromThreadInfo(cpu.get(), info);
        PopSeccompStackFrame(cpu.get(), thread, stack_copy);
        thread.thread_context = cpu.location();
        if (dumper_->threads()[i] == GetCrashThread()) {
          assert(dumper_->IsPostMortem());
          crashing_thread_context_ = cpu.location();
        }
      }

      list.CopyIndexAfterObject(i, &thread, sizeof(thread));
    }

    return true;
  }

  static bool ShouldIncludeMapping(const MappingInfo& mapping) {
    if (mapping.name[0] == 0 ||  // only want modules with filenames.
        mapping.offset ||  // only want to include one mapping per shared lib.
        mapping.size < 4096) {  // too small to get a signature for.
      return false;
    }

    return true;
  }

  // If there is caller-provided information about this mapping
  // in the mapping_list_ list, return true. Otherwise, return false.
  bool HaveMappingInfo(const MappingInfo& mapping) {
    for (MappingList::const_iterator iter = mapping_list_.begin();
         iter != mapping_list_.end();
         ++iter) {
      // Ignore any mappings that are wholly contained within
      // mappings in the mapping_info_ list.
      if (mapping.start_addr >= iter->first.start_addr &&
          (mapping.start_addr + mapping.size) <=
          (iter->first.start_addr + iter->first.size)) {
        return true;
      }
    }
    return false;
  }

  // Write information about the mappings in effect. Because we are using the
  // minidump format, the information about the mappings is pretty limited.
  // Because of this, we also include the full, unparsed, /proc/$x/maps file in
  // another stream in the file.
  bool WriteMappings(MDRawDirectory* dirent) {
    const unsigned num_mappings = dumper_->mappings().size();
    unsigned num_output_mappings = mapping_list_.size();

    for (unsigned i = 0; i < dumper_->mappings().size(); ++i) {
      const MappingInfo& mapping = *dumper_->mappings()[i];
      if (ShouldIncludeMapping(mapping) && !HaveMappingInfo(mapping))
        num_output_mappings++;
    }

    TypedMDRVA<uint32_t> list(&minidump_writer_);
    if (!list.AllocateObjectAndArray(num_output_mappings, MD_MODULE_SIZE))
      return false;

    dirent->stream_type = MD_MODULE_LIST_STREAM;
    dirent->location = list.location();
    *list.get() = num_output_mappings;

    // First write all the mappings from the dumper
    unsigned int j = 0;
    for (unsigned i = 0; i < num_mappings; ++i) {
      const MappingInfo& mapping = *dumper_->mappings()[i];
      if (!ShouldIncludeMapping(mapping) || HaveMappingInfo(mapping))
        continue;

      MDRawModule mod;
      if (!FillRawModule(mapping, true, i, mod, NULL))
        return false;
      list.CopyIndexAfterObject(j++, &mod, MD_MODULE_SIZE);
    }
    // Next write all the mappings provided by the caller
    for (MappingList::const_iterator iter = mapping_list_.begin();
         iter != mapping_list_.end();
         ++iter) {
      MDRawModule mod;
      if (!FillRawModule(iter->first, false, 0, mod, iter->second))
        return false;
      list.CopyIndexAfterObject(j++, &mod, MD_MODULE_SIZE);
    }

    return true;
  }

  // Fill the MDRawModule |mod| with information about the provided
  // |mapping|. If |identifier| is non-NULL, use it instead of calculating
  // a file ID from the mapping.
  bool FillRawModule(const MappingInfo& mapping,
                     bool member,
                     unsigned int mapping_id,
                     MDRawModule& mod,
                     const u_int8_t* identifier) {
    my_memset(&mod, 0, MD_MODULE_SIZE);

    mod.base_of_image = mapping.start_addr;
    mod.size_of_image = mapping.size;
    const size_t filepath_len = my_strlen(mapping.name);

    // Figure out file name from path
    const char* filename_ptr = mapping.name + filepath_len - 1;
    while (filename_ptr >= mapping.name) {
      if (*filename_ptr == '/')
        break;
      filename_ptr--;
    }
    filename_ptr++;

    const size_t filename_len = mapping.name + filepath_len - filename_ptr;

    uint8_t cv_buf[MDCVInfoPDB70_minsize + NAME_MAX];
    uint8_t* cv_ptr = cv_buf;
    UntypedMDRVA cv(&minidump_writer_);
    if (!cv.Allocate(MDCVInfoPDB70_minsize + filename_len + 1))
      return false;

    const uint32_t cv_signature = MD_CVINFOPDB70_SIGNATURE;
    memcpy(cv_ptr, &cv_signature, sizeof(cv_signature));
    cv_ptr += sizeof(cv_signature);
    uint8_t* signature = cv_ptr;
    cv_ptr += sizeof(MDGUID);
    if (identifier) {
      // GUID was provided by caller.
      memcpy(signature, identifier, sizeof(MDGUID));
    } else {
      dumper_->ElfFileIdentifierForMapping(mapping, member,
                                           mapping_id, signature);
    }
    my_memset(cv_ptr, 0, sizeof(uint32_t));  // Set age to 0 on Linux.
    cv_ptr += sizeof(uint32_t);

    // Write pdb_file_name
    memcpy(cv_ptr, filename_ptr, filename_len + 1);
    cv.Copy(cv_buf, MDCVInfoPDB70_minsize + filename_len + 1);

    mod.cv_record = cv.location();

    MDLocationDescriptor ld;
    if (!minidump_writer_.WriteString(mapping.name, filepath_len, &ld))
      return false;
    mod.module_name_rva = ld.rva;
    return true;
  }

  bool WriteMemoryListStream(MDRawDirectory* dirent) {
    TypedMDRVA<uint32_t> list(&minidump_writer_);
    if (!list.AllocateObjectAndArray(memory_blocks_.size(),
                                     sizeof(MDMemoryDescriptor)))
      return false;

    dirent->stream_type = MD_MEMORY_LIST_STREAM;
    dirent->location = list.location();

    *list.get() = memory_blocks_.size();

    for (size_t i = 0; i < memory_blocks_.size(); ++i) {
      list.CopyIndexAfterObject(i, &memory_blocks_[i],
                                sizeof(MDMemoryDescriptor));
    }
    return true;
  }

  bool WriteExceptionStream(MDRawDirectory* dirent) {
    TypedMDRVA<MDRawExceptionStream> exc(&minidump_writer_);
    if (!exc.Allocate())
      return false;
    my_memset(exc.get(), 0, sizeof(MDRawExceptionStream));

    dirent->stream_type = MD_EXCEPTION_STREAM;
    dirent->location = exc.location();

    exc.get()->thread_id = GetCrashThread();
    exc.get()->exception_record.exception_code = dumper_->crash_signal();
    exc.get()->exception_record.exception_address = dumper_->crash_address();
    exc.get()->thread_context = crashing_thread_context_;

    return true;
  }

  bool WriteSystemInfoStream(MDRawDirectory* dirent) {
    TypedMDRVA<MDRawSystemInfo> si(&minidump_writer_);
    if (!si.Allocate())
      return false;
    my_memset(si.get(), 0, sizeof(MDRawSystemInfo));

    dirent->stream_type = MD_SYSTEM_INFO_STREAM;
    dirent->location = si.location();

    WriteCPUInformation(si.get());
    WriteOSInformation(si.get());

    return true;
  }

  bool WriteDSODebugStream(MDRawDirectory* dirent, struct r_debug* r_debug,
                           uint32_t dynamic_length) {
#if defined(__ANDROID__)
    return false;
#else
    // The caller provided us with a pointer to "struct r_debug". We can
    // look up the "r_map" field to get a linked list of all loaded DSOs.
    // Our list of DSOs potentially is different from the ones in the crashing
    // process. So, we have to be careful to never dereference pointers
    // directly. Instead, we use CopyFromProcess() everywhere.
    // See <link.h> for a more detailed discussion of the how the dynamic
    // loader communicates with debuggers.

    // Count the number of loaded DSOs
    int dso_count = 0;
    struct r_debug debug_entry;
    dumper_->CopyFromProcess(&debug_entry, GetCrashThread(), r_debug,
                             sizeof(debug_entry));
    for (struct link_map* ptr = debug_entry.r_map; ptr; ) {
      struct link_map map;
      dumper_->CopyFromProcess(&map, GetCrashThread(), ptr, sizeof(map));
      ptr = map.l_next;
      dso_count++;
    }

    MDRVA linkmap_rva = minidump_writer_.kInvalidMDRVA;
    if (dso_count > 0) {
      // If we have at least one DSO, create an array of MDRawLinkMap
      // entries in the minidump file.
      TypedMDRVA<MDRawLinkMap> linkmap(&minidump_writer_);
      if (!linkmap.AllocateArray(dso_count))
        return false;
      linkmap_rva = linkmap.location().rva;
      int idx = 0;

      // Iterate over DSOs and write their information to mini dump
      for (struct link_map* ptr = debug_entry.r_map; ptr; ) {
        struct link_map map;
        dumper_->CopyFromProcess(&map, GetCrashThread(), ptr, sizeof(map));
        ptr = map.l_next;
        char filename[257] = { 0 };
        if (map.l_name) {
          dumper_->CopyFromProcess(filename, GetCrashThread(), map.l_name,
                                   sizeof(filename) - 1);
        }
        MDLocationDescriptor location;
        if (!minidump_writer_.WriteString(filename, 0, &location))
          return false;
        MDRawLinkMap entry;
        entry.name = location.rva;
        entry.addr = (void*)map.l_addr;
        entry.ld = (void*)map.l_ld;
        linkmap.CopyIndex(idx++, &entry);
      }
    }

    // Write MD_LINUX_DSO_DEBUG record
    TypedMDRVA<MDRawDebug> debug(&minidump_writer_);
    if (!debug.AllocateObjectAndArray(1, dynamic_length))
      return false;
    my_memset(debug.get(), 0, sizeof(MDRawDebug));
    dirent->stream_type = MD_LINUX_DSO_DEBUG;
    dirent->location = debug.location();

    debug.get()->version = debug_entry.r_version;
    debug.get()->map = linkmap_rva;
    debug.get()->dso_count = dso_count;
    debug.get()->brk = (void*)debug_entry.r_brk;
    debug.get()->ldbase = (void*)debug_entry.r_ldbase;
    debug.get()->dynamic = (void*)&_DYNAMIC;

    char *dso_debug_data = new char[dynamic_length];
    dumper_->CopyFromProcess(dso_debug_data, GetCrashThread(), &_DYNAMIC,
                             dynamic_length);
    debug.CopyIndexAfterObject(0, dso_debug_data, dynamic_length);
    delete[] dso_debug_data;

    return true;
#endif
  }

 private:
  void* Alloc(unsigned bytes) {
    return dumper_->allocator()->Alloc(bytes);
  }

  pid_t GetCrashThread() const {
    return dumper_->crash_thread();
  }

#if defined(__i386)
  uintptr_t GetStackPointer() {
    return ucontext_->uc_mcontext.gregs[REG_ESP];
  }

  uintptr_t GetInstructionPointer() {
    return ucontext_->uc_mcontext.gregs[REG_EIP];
  }
#elif defined(__x86_64)
  uintptr_t GetStackPointer() {
    return ucontext_->uc_mcontext.gregs[REG_RSP];
  }

  uintptr_t GetInstructionPointer() {
    return ucontext_->uc_mcontext.gregs[REG_RIP];
  }
#elif defined(__ARM_EABI__)
  uintptr_t GetStackPointer() {
    return ucontext_->uc_mcontext.arm_sp;
  }

  uintptr_t GetInstructionPointer() {
    return ucontext_->uc_mcontext.arm_ip;
  }
#else
#error "This code has not been ported to your platform yet."
#endif

  void NullifyDirectoryEntry(MDRawDirectory* dirent) {
    dirent->stream_type = 0;
    dirent->location.data_size = 0;
    dirent->location.rva = 0;
  }

  bool WriteCPUInformation(MDRawSystemInfo* sys_info) {
    char vendor_id[sizeof(sys_info->cpu.x86_cpu_info.vendor_id) + 1] = {0};
    static const char vendor_id_name[] = "vendor_id";
    static const size_t vendor_id_name_length = sizeof(vendor_id_name) - 1;

    struct CpuInfoEntry {
      const char* info_name;
      int value;
      bool found;
    } cpu_info_table[] = {
      { "processor", -1, false },
      { "model", 0, false },
      { "stepping",  0, false },
      { "cpu family", 0, false },
    };

    // processor_architecture should always be set, do this first
    sys_info->processor_architecture =
#if defined(__i386)
        MD_CPU_ARCHITECTURE_X86;
#elif defined(__x86_64)
        MD_CPU_ARCHITECTURE_AMD64;
#elif defined(__arm__)
        MD_CPU_ARCHITECTURE_ARM;
#else
#error "Unknown CPU arch"
#endif

    const int fd = sys_open("/proc/cpuinfo", O_RDONLY, 0);
    if (fd < 0)
      return false;

    {
      PageAllocator allocator;
      LineReader* const line_reader = new(allocator) LineReader(fd);
      const char* line;
      unsigned line_len;
      while (line_reader->GetNextLine(&line, &line_len)) {
        for (size_t i = 0;
             i < sizeof(cpu_info_table) / sizeof(cpu_info_table[0]);
             i++) {
          CpuInfoEntry* entry = &cpu_info_table[i];
          if (entry->found && i)
            continue;
          if (!strncmp(line, entry->info_name, strlen(entry->info_name))) {
            const char* value = strchr(line, ':');
            if (!value)
              continue;

            // the above strncmp only matches the prefix, it might be the wrong
            // line. i.e. we matched "model name" instead of "model".
            // check and make sure there is only spaces between the prefix and
            // the colon.
            const char* space_ptr = line + strlen(entry->info_name);
            for (; space_ptr < value; space_ptr++) {
              if (!isspace(*space_ptr)) {
                break;
              }
            }
            if (space_ptr != value)
              continue;

            sscanf(++value, " %d", &(entry->value));
            entry->found = true;
          }
        }

        // special case for vendor_id
        if (!strncmp(line, vendor_id_name, vendor_id_name_length)) {
          const char* value = strchr(line, ':');
          if (!value)
            goto popline;

          // skip ':" and all the spaces that follows
          do {
            value++;
          } while (isspace(*value));

          if (*value) {
            size_t length = strlen(value);
            if (length == 0)
              goto popline;
            // we don't want the trailing newline
            if (value[length - 1] == '\n')
              length--;
            // ensure we have space for the value
            if (length < sizeof(vendor_id))
              strncpy(vendor_id, value, length);
          }
        }

 popline:
        line_reader->PopLine(line_len);
      }
      sys_close(fd);
    }

    // make sure we got everything we wanted
    for (size_t i = 0;
         i < sizeof(cpu_info_table) / sizeof(cpu_info_table[0]);
         i++) {
      if (!cpu_info_table[i].found) {
        return false;
      }
    }
    // /proc/cpuinfo contains cpu id, change it into number by adding one.
    cpu_info_table[0].value++;

    sys_info->number_of_processors = cpu_info_table[0].value;
    sys_info->processor_level      = cpu_info_table[3].value;
    sys_info->processor_revision   = cpu_info_table[1].value << 8 |
                                     cpu_info_table[2].value;

    if (vendor_id[0] != '\0') {
      memcpy(sys_info->cpu.x86_cpu_info.vendor_id, vendor_id,
             sizeof(sys_info->cpu.x86_cpu_info.vendor_id));
    }
    return true;
  }

  bool WriteFile(MDLocationDescriptor* result, const char* filename) {
    const int fd = sys_open(filename, O_RDONLY, 0);
    if (fd < 0)
      return false;

    // We can't stat the files because several of the files that we want to
    // read are kernel seqfiles, which always have a length of zero. So we have
    // to read as much as we can into a buffer.
    static const unsigned kBufSize = 1024 - 2*sizeof(void*);
    struct Buffers {
      Buffers* next;
      size_t len;
      uint8_t data[kBufSize];
    } *buffers = reinterpret_cast<Buffers*>(Alloc(sizeof(Buffers)));
    buffers->next = NULL;
    buffers->len = 0;

    size_t total = 0;
    for (Buffers* bufptr = buffers;;) {
      ssize_t r;
      do {
        r = sys_read(fd, &bufptr->data[bufptr->len], kBufSize - bufptr->len);
      } while (r == -1 && errno == EINTR);

      if (r < 1)
        break;

      total += r;
      bufptr->len += r;
      if (bufptr->len == kBufSize) {
        bufptr->next = reinterpret_cast<Buffers*>(Alloc(sizeof(Buffers)));
        bufptr = bufptr->next;
        bufptr->next = NULL;
        bufptr->len = 0;
      }
    }
    sys_close(fd);

    if (!total)
      return false;

    UntypedMDRVA memory(&minidump_writer_);
    if (!memory.Allocate(total))
      return false;
    for (MDRVA pos = memory.position(); buffers; buffers = buffers->next) {
      // Check for special case of a zero-length buffer.  This should only
      // occur if a file's size happens to be a multiple of the buffer's
      // size, in which case the final sys_read() will have resulted in
      // zero bytes being read after the final buffer was just allocated.
      if (buffers->len == 0) {
        // This can only occur with final buffer.
        assert(buffers->next == NULL);
        continue;
      }
      memory.Copy(pos, &buffers->data, buffers->len);
      pos += buffers->len;
    }
    *result = memory.location();
    return true;
  }

  bool WriteOSInformation(MDRawSystemInfo* sys_info) {
    sys_info->platform_id = MD_OS_LINUX;

    struct utsname uts;
    if (uname(&uts))
      return false;

    static const size_t buf_len = 512;
    char buf[buf_len] = {0};
    size_t space_left = buf_len - 1;
    const char* info_table[] = {
      uts.sysname,
      uts.release,
      uts.version,
      uts.machine,
      NULL
    };
    bool first_item = true;
    for (const char** cur_info = info_table; *cur_info; cur_info++) {
      static const char* separator = " ";
      size_t separator_len = strlen(separator);
      size_t info_len = strlen(*cur_info);
      if (info_len == 0)
        continue;

      if (space_left < info_len + (first_item ? 0 : separator_len))
        break;

      if (!first_item) {
        strcat(buf, separator);
        space_left -= separator_len;
      }

      first_item = false;
      strcat(buf, *cur_info);
      space_left -= info_len;
    }

    MDLocationDescriptor location;
    if (!minidump_writer_.WriteString(buf, 0, &location))
      return false;
    sys_info->csd_version_rva = location.rva;

    return true;
  }

  bool WriteProcFile(MDLocationDescriptor* result, pid_t pid,
                     const char* filename) {
    char buf[NAME_MAX];
    if (!dumper_->BuildProcPath(buf, pid, filename))
      return false;
    return WriteFile(result, buf);
  }

  const char* const filename_;  // output filename
  const struct ucontext* const ucontext_;  // also from the signal handler
  const struct _libc_fpstate* const float_state_;  // ditto
  LinuxDumper* dumper_;
  MinidumpFileWriter minidump_writer_;
  MDLocationDescriptor crashing_thread_context_;
  // Blocks of memory written to the dump. These are all currently
  // written while writing the thread list stream, but saved here
  // so a memory list stream can be written afterwards.
  wasteful_vector<MDMemoryDescriptor> memory_blocks_;
  // Additional information about some mappings provided by the caller.
  const MappingList& mapping_list_;
};

bool WriteMinidump(const char* filename, pid_t crashing_process,
                   const void* blob, size_t blob_size) {
  MappingList m;
  return WriteMinidump(filename, crashing_process, blob, blob_size, m);
}

bool WriteMinidump(const char* filename, pid_t crashing_process,
                   const void* blob, size_t blob_size,
                   const MappingList& mappings) {
  if (blob_size != sizeof(ExceptionHandler::CrashContext))
    return false;
  const ExceptionHandler::CrashContext* context =
      reinterpret_cast<const ExceptionHandler::CrashContext*>(blob);
  LinuxPtraceDumper dumper(crashing_process);
  dumper.set_crash_address(
      reinterpret_cast<uintptr_t>(context->siginfo.si_addr));
  dumper.set_crash_signal(context->siginfo.si_signo);
  dumper.set_crash_thread(context->tid);
  MinidumpWriter writer(filename, context, mappings, &dumper);
  if (!writer.Init())
    return false;
  return writer.Dump();
}

bool WriteMinidump(const char* filename,
                   const MappingList& mappings,
                   LinuxDumper* dumper) {
  MinidumpWriter writer(filename, NULL, mappings, dumper);
  if (!writer.Init())
    return false;
  return writer.Dump();
}

}  // namespace google_breakpad
