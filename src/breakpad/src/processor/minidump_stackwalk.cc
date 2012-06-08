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

// minidump_stackwalk.cc: Process a minidump with MinidumpProcessor, printing
// the results, including stack traces.
//
// Author: Mark Mentovai

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/code_module.h"
#include "google_breakpad/processor/code_modules.h"
#include "google_breakpad/processor/minidump.h"
#include "google_breakpad/processor/minidump_processor.h"
#include "google_breakpad/processor/process_state.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "processor/logging.h"
#include "processor/pathname_stripper.h"
#include "processor/scoped_ptr.h"
#include "processor/simple_symbol_supplier.h"

namespace {

using std::string;
using std::vector;
using google_breakpad::BasicSourceLineResolver;
using google_breakpad::CallStack;
using google_breakpad::CodeModule;
using google_breakpad::CodeModules;
using google_breakpad::MinidumpModule;
using google_breakpad::MinidumpProcessor;
using google_breakpad::PathnameStripper;
using google_breakpad::ProcessState;
using google_breakpad::scoped_ptr;
using google_breakpad::SimpleSymbolSupplier;
using google_breakpad::StackFrame;
using google_breakpad::StackFramePPC;
using google_breakpad::StackFrameSPARC;
using google_breakpad::StackFrameX86;
using google_breakpad::StackFrameAMD64;
using google_breakpad::StackFrameARM;

// Separator character for machine readable output.
static const char kOutputSeparator = '|';

// PrintRegister prints a register's name and value to stdout.  It will
// print four registers on a line.  For the first register in a set,
// pass 0 for |start_col|.  For registers in a set, pass the most recent
// return value of PrintRegister.
// The caller is responsible for printing the final newline after a set
// of registers is completely printed, regardless of the number of calls
// to PrintRegister.
static const int kMaxWidth = 80;  // optimize for an 80-column terminal
static int PrintRegister(const char *name, u_int32_t value, int start_col) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), " %5s = 0x%08x", name, value);

  if (start_col + strlen(buffer) > kMaxWidth) {
    start_col = 0;
    printf("\n ");
  }
  fputs(buffer, stdout);

  return start_col + strlen(buffer);
}

// PrintRegister64 does the same thing, but for 64-bit registers.
static int PrintRegister64(const char *name, u_int64_t value, int start_col) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), " %5s = 0x%016" PRIx64 , name, value);

  if (start_col + strlen(buffer) > kMaxWidth) {
    start_col = 0;
    printf("\n ");
  }
  fputs(buffer, stdout);

  return start_col + strlen(buffer);
}

// StripSeparator takes a string |original| and returns a copy
// of the string with all occurences of |kOutputSeparator| removed.
static string StripSeparator(const string &original) {
  string result = original;
  string::size_type position = 0;
  while ((position = result.find(kOutputSeparator, position)) != string::npos) {
    result.erase(position, 1);
  }
  position = 0;
  while ((position = result.find('\n', position)) != string::npos) {
    result.erase(position, 1);
  }
  return result;
}

// PrintStack prints the call stack in |stack| to stdout, in a reasonably
// useful form.  Module, function, and source file names are displayed if
// they are available.  The code offset to the base code address of the
// source line, function, or module is printed, preferring them in that
// order.  If no source line, function, or module information is available,
// an absolute code offset is printed.
//
// If |cpu| is a recognized CPU name, relevant register state for each stack
// frame printed is also output, if available.
static void PrintStack(const CallStack *stack, const string &cpu) {
  int frame_count = stack->frames()->size();
  for (int frame_index = 0; frame_index < frame_count; ++frame_index) {
    const StackFrame *frame = stack->frames()->at(frame_index);
    printf("%2d  ", frame_index);

    if (frame->module) {
      printf("%s", PathnameStripper::File(frame->module->code_file()).c_str());
      if (!frame->function_name.empty()) {
        printf("!%s", frame->function_name.c_str());
        if (!frame->source_file_name.empty()) {
          string source_file = PathnameStripper::File(frame->source_file_name);
          printf(" [%s : %d + 0x%" PRIx64 "]",
                 source_file.c_str(),
                 frame->source_line,
                 frame->instruction - frame->source_line_base);
        } else {
          printf(" + 0x%" PRIx64, frame->instruction - frame->function_base);
        }
      } else {
        printf(" + 0x%" PRIx64,
               frame->instruction - frame->module->base_address());
      }
    } else {
      printf("0x%" PRIx64, frame->instruction);
    }
    printf("\n ");

    int sequence = 0;
    if (cpu == "x86") {
      const StackFrameX86 *frame_x86 =
        reinterpret_cast<const StackFrameX86*>(frame);

      if (frame_x86->context_validity & StackFrameX86::CONTEXT_VALID_EIP)
        sequence = PrintRegister("eip", frame_x86->context.eip, sequence);
      if (frame_x86->context_validity & StackFrameX86::CONTEXT_VALID_ESP)
        sequence = PrintRegister("esp", frame_x86->context.esp, sequence);
      if (frame_x86->context_validity & StackFrameX86::CONTEXT_VALID_EBP)
        sequence = PrintRegister("ebp", frame_x86->context.ebp, sequence);
      if (frame_x86->context_validity & StackFrameX86::CONTEXT_VALID_EBX)
        sequence = PrintRegister("ebx", frame_x86->context.ebx, sequence);
      if (frame_x86->context_validity & StackFrameX86::CONTEXT_VALID_ESI)
        sequence = PrintRegister("esi", frame_x86->context.esi, sequence);
      if (frame_x86->context_validity & StackFrameX86::CONTEXT_VALID_EDI)
        sequence = PrintRegister("edi", frame_x86->context.edi, sequence);
      if (frame_x86->context_validity == StackFrameX86::CONTEXT_VALID_ALL) {
        sequence = PrintRegister("eax", frame_x86->context.eax, sequence);
        sequence = PrintRegister("ecx", frame_x86->context.ecx, sequence);
        sequence = PrintRegister("edx", frame_x86->context.edx, sequence);
        sequence = PrintRegister("efl", frame_x86->context.eflags, sequence);
      }
    } else if (cpu == "ppc") {
      const StackFramePPC *frame_ppc =
        reinterpret_cast<const StackFramePPC*>(frame);

      if (frame_ppc->context_validity & StackFramePPC::CONTEXT_VALID_SRR0)
        sequence = PrintRegister("srr0", frame_ppc->context.srr0, sequence);
      if (frame_ppc->context_validity & StackFramePPC::CONTEXT_VALID_GPR1)
        sequence = PrintRegister("r1", frame_ppc->context.gpr[1], sequence);
    } else if (cpu == "amd64") {
      const StackFrameAMD64 *frame_amd64 =
        reinterpret_cast<const StackFrameAMD64*>(frame);

      if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_RBX)
        sequence = PrintRegister64("rbx", frame_amd64->context.rbx, sequence);
      if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_R12)
        sequence = PrintRegister64("r12", frame_amd64->context.r12, sequence);
      if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_R13)
        sequence = PrintRegister64("r13", frame_amd64->context.r13, sequence);
      if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_R14)
        sequence = PrintRegister64("r14", frame_amd64->context.r14, sequence);
      if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_R15)
        sequence = PrintRegister64("r15", frame_amd64->context.r15, sequence);
      if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_RIP)
        sequence = PrintRegister64("rip", frame_amd64->context.rip, sequence);
      if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_RSP)
        sequence = PrintRegister64("rsp", frame_amd64->context.rsp, sequence);
      if (frame_amd64->context_validity & StackFrameAMD64::CONTEXT_VALID_RBP)
        sequence = PrintRegister64("rbp", frame_amd64->context.rbp, sequence);
    } else if (cpu == "sparc") {
      const StackFrameSPARC *frame_sparc =
        reinterpret_cast<const StackFrameSPARC*>(frame);

      if (frame_sparc->context_validity & StackFrameSPARC::CONTEXT_VALID_SP)
        sequence = PrintRegister("sp", frame_sparc->context.g_r[14], sequence);
      if (frame_sparc->context_validity & StackFrameSPARC::CONTEXT_VALID_FP)
        sequence = PrintRegister("fp", frame_sparc->context.g_r[30], sequence);
      if (frame_sparc->context_validity & StackFrameSPARC::CONTEXT_VALID_PC)
        sequence = PrintRegister("pc", frame_sparc->context.pc, sequence);
    } else if (cpu == "arm") {
      const StackFrameARM *frame_arm =
        reinterpret_cast<const StackFrameARM*>(frame);

      // General-purpose callee-saves registers.
      if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R4)
        sequence = PrintRegister("r4", frame_arm->context.iregs[4], sequence);
      if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R5)
        sequence = PrintRegister("r5", frame_arm->context.iregs[5], sequence);
      if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R6)
        sequence = PrintRegister("r6", frame_arm->context.iregs[6], sequence);
      if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R7)
        sequence = PrintRegister("r7", frame_arm->context.iregs[7], sequence);
      if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R8)
        sequence = PrintRegister("r8", frame_arm->context.iregs[8], sequence);
      if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R9)
        sequence = PrintRegister("r9", frame_arm->context.iregs[9], sequence);
      if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_R10)
        sequence = PrintRegister("r10", frame_arm->context.iregs[10], sequence);

      // Registers with a dedicated or conventional purpose.
      if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_FP)
        sequence = PrintRegister("fp", frame_arm->context.iregs[11], sequence);
      if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_SP)
        sequence = PrintRegister("sp", frame_arm->context.iregs[13], sequence);
      if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_LR)
        sequence = PrintRegister("lr", frame_arm->context.iregs[14], sequence);
      if (frame_arm->context_validity & StackFrameARM::CONTEXT_VALID_PC)
        sequence = PrintRegister("pc", frame_arm->context.iregs[15], sequence);
    }
    printf("\n    Found by: %s\n", frame->trust_description().c_str());
  }
}

// PrintStackMachineReadable prints the call stack in |stack| to stdout,
// in the following machine readable pipe-delimited text format:
// thread number|frame number|module|function|source file|line|offset
//
// Module, function, source file, and source line may all be empty
// depending on availability.  The code offset follows the same rules as
// PrintStack above.
static void PrintStackMachineReadable(int thread_num, const CallStack *stack) {
  int frame_count = stack->frames()->size();
  for (int frame_index = 0; frame_index < frame_count; ++frame_index) {
    const StackFrame *frame = stack->frames()->at(frame_index);
    printf("%d%c%d%c", thread_num, kOutputSeparator, frame_index,
           kOutputSeparator);

    if (frame->module) {
      assert(!frame->module->code_file().empty());
      printf("%s", StripSeparator(PathnameStripper::File(
                     frame->module->code_file())).c_str());
      if (!frame->function_name.empty()) {
        printf("%c%s", kOutputSeparator,
               StripSeparator(frame->function_name).c_str());
        if (!frame->source_file_name.empty()) {
          printf("%c%s%c%d%c0x%" PRIx64,
                 kOutputSeparator,
                 StripSeparator(frame->source_file_name).c_str(),
                 kOutputSeparator,
                 frame->source_line,
                 kOutputSeparator,
                 frame->instruction - frame->source_line_base);
        } else {
          printf("%c%c%c0x%" PRIx64,
                 kOutputSeparator,  // empty source file
                 kOutputSeparator,  // empty source line
                 kOutputSeparator,
                 frame->instruction - frame->function_base);
        }
      } else {
        printf("%c%c%c%c0x%" PRIx64,
               kOutputSeparator,  // empty function name
               kOutputSeparator,  // empty source file
               kOutputSeparator,  // empty source line
               kOutputSeparator,
               frame->instruction - frame->module->base_address());
      }
    } else {
      // the printf before this prints a trailing separator for module name
      printf("%c%c%c%c0x%" PRIx64,
             kOutputSeparator,  // empty function name
             kOutputSeparator,  // empty source file
             kOutputSeparator,  // empty source line
             kOutputSeparator,
             frame->instruction);
    }
    printf("\n");
  }
}

static void PrintModules(const CodeModules *modules) {
  if (!modules)
    return;

  printf("\n");
  printf("Loaded modules:\n");

  u_int64_t main_address = 0;
  const CodeModule *main_module = modules->GetMainModule();
  if (main_module) {
    main_address = main_module->base_address();
  }

  unsigned int module_count = modules->module_count();
  for (unsigned int module_sequence = 0;
       module_sequence < module_count;
       ++module_sequence) {
    const CodeModule *module = modules->GetModuleAtSequence(module_sequence);
    u_int64_t base_address = module->base_address();
    printf("0x%08" PRIx64 " - 0x%08" PRIx64 "  %s  %s%s\n",
           base_address, base_address + module->size() - 1,
           PathnameStripper::File(module->code_file()).c_str(),
           module->version().empty() ? "???" : module->version().c_str(),
           main_module != NULL && base_address == main_address ?
               "  (main)" : "");
  }
}

// PrintModulesMachineReadable outputs a list of loaded modules,
// one per line, in the following machine-readable pipe-delimited
// text format:
// Module|{Module Filename}|{Version}|{Debug Filename}|{Debug Identifier}|
// {Base Address}|{Max Address}|{Main}
static void PrintModulesMachineReadable(const CodeModules *modules) {
  if (!modules)
    return;

  u_int64_t main_address = 0;
  const CodeModule *main_module = modules->GetMainModule();
  if (main_module) {
    main_address = main_module->base_address();
  }

  unsigned int module_count = modules->module_count();
  for (unsigned int module_sequence = 0;
       module_sequence < module_count;
       ++module_sequence) {
    const CodeModule *module = modules->GetModuleAtSequence(module_sequence);
    u_int64_t base_address = module->base_address();
    printf("Module%c%s%c%s%c%s%c%s%c0x%08" PRIx64 "%c0x%08" PRIx64 "%c%d\n",
           kOutputSeparator,
           StripSeparator(PathnameStripper::File(module->code_file())).c_str(),
           kOutputSeparator, StripSeparator(module->version()).c_str(),
           kOutputSeparator,
           StripSeparator(PathnameStripper::File(module->debug_file())).c_str(),
           kOutputSeparator,
           StripSeparator(module->debug_identifier()).c_str(),
           kOutputSeparator, base_address,
           kOutputSeparator, base_address + module->size() - 1,
           kOutputSeparator,
           main_module != NULL && base_address == main_address ? 1 : 0);
  }
}

static void PrintProcessState(const ProcessState& process_state) {
  // Print OS and CPU information.
  string cpu = process_state.system_info()->cpu;
  string cpu_info = process_state.system_info()->cpu_info;
  printf("Operating system: %s\n", process_state.system_info()->os.c_str());
  printf("                  %s\n",
         process_state.system_info()->os_version.c_str());
  printf("CPU: %s\n", cpu.c_str());
  if (!cpu_info.empty()) {
    // This field is optional.
    printf("     %s\n", cpu_info.c_str());
  }
  printf("     %d CPU%s\n",
         process_state.system_info()->cpu_count,
         process_state.system_info()->cpu_count != 1 ? "s" : "");
  printf("\n");

  // Print crash information.
  if (process_state.crashed()) {
    printf("Crash reason:  %s\n", process_state.crash_reason().c_str());
    printf("Crash address: 0x%" PRIx64 "\n", process_state.crash_address());
  } else {
    printf("No crash\n");
  }

  string assertion = process_state.assertion();
  if (!assertion.empty()) {
    printf("Assertion: %s\n", assertion.c_str());
  }

  // If the thread that requested the dump is known, print it first.
  int requesting_thread = process_state.requesting_thread();
  if (requesting_thread != -1) {
    printf("\n");
    printf("Thread %d (%s)\n",
          requesting_thread,
          process_state.crashed() ? "crashed" :
                                    "requested dump, did not crash");
    PrintStack(process_state.threads()->at(requesting_thread), cpu);
  }

  // Print all of the threads in the dump.
  int thread_count = process_state.threads()->size();
  for (int thread_index = 0; thread_index < thread_count; ++thread_index) {
    if (thread_index != requesting_thread) {
      // Don't print the crash thread again, it was already printed.
      printf("\n");
      printf("Thread %d\n", thread_index);
      PrintStack(process_state.threads()->at(thread_index), cpu);
    }
  }

  PrintModules(process_state.modules());
}

static void PrintProcessStateMachineReadable(const ProcessState& process_state)
{
  // Print OS and CPU information.
  // OS|{OS Name}|{OS Version}
  // CPU|{CPU Name}|{CPU Info}|{Number of CPUs}
  printf("OS%c%s%c%s\n", kOutputSeparator,
         StripSeparator(process_state.system_info()->os).c_str(),
         kOutputSeparator,
         StripSeparator(process_state.system_info()->os_version).c_str());
  printf("CPU%c%s%c%s%c%d\n", kOutputSeparator,
         StripSeparator(process_state.system_info()->cpu).c_str(),
         kOutputSeparator,
         // this may be empty
         StripSeparator(process_state.system_info()->cpu_info).c_str(),
         kOutputSeparator,
         process_state.system_info()->cpu_count);

  int requesting_thread = process_state.requesting_thread();

  // Print crash information.
  // Crash|{Crash Reason}|{Crash Address}|{Crashed Thread}
  printf("Crash%c", kOutputSeparator);
  if (process_state.crashed()) {
    printf("%s%c0x%" PRIx64 "%c",
           StripSeparator(process_state.crash_reason()).c_str(),
           kOutputSeparator, process_state.crash_address(), kOutputSeparator);
  } else {
    // print assertion info, if available, in place of crash reason,
    // instead of the unhelpful "No crash"
    string assertion = process_state.assertion();
    if (!assertion.empty()) {
      printf("%s%c%c", StripSeparator(assertion).c_str(),
             kOutputSeparator, kOutputSeparator);
    } else {
      printf("No crash%c%c", kOutputSeparator, kOutputSeparator);
    }
  }

  if (requesting_thread != -1) {
    printf("%d\n", requesting_thread);
  } else {
    printf("\n");
  }

  PrintModulesMachineReadable(process_state.modules());

  // blank line to indicate start of threads
  printf("\n");

  // If the thread that requested the dump is known, print it first.
  if (requesting_thread != -1) {
    PrintStackMachineReadable(requesting_thread,
                              process_state.threads()->at(requesting_thread));
  }

  // Print all of the threads in the dump.
  int thread_count = process_state.threads()->size();
  for (int thread_index = 0; thread_index < thread_count; ++thread_index) {
    if (thread_index != requesting_thread) {
      // Don't print the crash thread again, it was already printed.
      PrintStackMachineReadable(thread_index,
                                process_state.threads()->at(thread_index));
    }
  }
}

// Processes |minidump_file| using MinidumpProcessor.  |symbol_path|, if
// non-empty, is the base directory of a symbol storage area, laid out in
// the format required by SimpleSymbolSupplier.  If such a storage area
// is specified, it is made available for use by the MinidumpProcessor.
//
// Returns the value of MinidumpProcessor::Process.  If processing succeeds,
// prints identifying OS and CPU information from the minidump, crash
// information if the minidump was produced as a result of a crash, and
// call stacks for each thread contained in the minidump.  All information
// is printed to stdout.
static bool PrintMinidumpProcess(const string &minidump_file,
                                 const vector<string> &symbol_paths,
                                 bool machine_readable) {
  scoped_ptr<SimpleSymbolSupplier> symbol_supplier;
  if (!symbol_paths.empty()) {
    // TODO(mmentovai): check existence of symbol_path if specified?
    symbol_supplier.reset(new SimpleSymbolSupplier(symbol_paths));
  }

  BasicSourceLineResolver resolver;
  MinidumpProcessor minidump_processor(symbol_supplier.get(), &resolver);

  // Process the minidump.
  ProcessState process_state;
  if (minidump_processor.Process(minidump_file, &process_state) !=
      google_breakpad::PROCESS_OK) {
    BPLOG(ERROR) << "MinidumpProcessor::Process failed";
    return false;
  }

  if (machine_readable) {
    PrintProcessStateMachineReadable(process_state);
  } else {
    PrintProcessState(process_state);
  }

  return true;
}

}  // namespace

static void usage(const char *program_name) {
  fprintf(stderr, "usage: %s [-m] <minidump-file> [symbol-path ...]\n"
          "    -m : Output in machine-readable format\n",
          program_name);
}

int main(int argc, char **argv) {
  BPLOG_INIT(&argc, &argv);

  if (argc < 2) {
    usage(argv[0]);
    return 1;
  }

  const char *minidump_file;
  bool machine_readable;
  int symbol_path_arg;

  if (strcmp(argv[1], "-m") == 0) {
    if (argc < 3) {
      usage(argv[0]);
      return 1;
    }

    machine_readable = true;
    minidump_file = argv[2];
    symbol_path_arg = 3;
  } else {
    machine_readable = false;
    minidump_file = argv[1];
    symbol_path_arg = 2;
  }

  // extra arguments are symbol paths
  std::vector<std::string> symbol_paths;
  if (argc > symbol_path_arg) {
    for (int argi = symbol_path_arg; argi < argc; ++argi)
      symbol_paths.push_back(argv[argi]);
  }

  return PrintMinidumpProcess(minidump_file,
                              symbol_paths,
                              machine_readable) ? 0 : 1;
}
