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

// stackwalker_x86_unittest.cc: Unit tests for StackwalkerX86 class.

#include <string>
#include <vector>

#include "breakpad_googletest_includes.h"
#include "common/test_assembler.h"
#include "google_breakpad/common/minidump_format.h"
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/source_line_resolver_interface.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "processor/stackwalker_unittest_utils.h"
#include "processor/stackwalker_x86.h"
#include "processor/windows_frame_info.h"

using google_breakpad::BasicSourceLineResolver;
using google_breakpad::CallStack;
using google_breakpad::StackFrame;
using google_breakpad::StackFrameX86;
using google_breakpad::StackwalkerX86;
using google_breakpad::SystemInfo;
using google_breakpad::WindowsFrameInfo;
using google_breakpad::test_assembler::kLittleEndian;
using google_breakpad::test_assembler::Label;
using google_breakpad::test_assembler::Section;
using std::string;
using std::vector;
using testing::_;
using testing::Return;
using testing::SetArgumentPointee;
using testing::Test;

class StackwalkerX86Fixture {
 public:
  StackwalkerX86Fixture()
    : stack_section(kLittleEndian),
      // Give the two modules reasonable standard locations and names
      // for tests to play with.
      module1(0x40000000, 0x10000, "module1", "version1"),
      module2(0x50000000, 0x10000, "module2", "version2") {
    // Identify the system as a Linux system.
    system_info.os = "Linux";
    system_info.os_short = "linux";
    system_info.os_version = "Salacious Skink";
    system_info.cpu = "x86";
    system_info.cpu_info = "";

    // Put distinctive values in the raw CPU context.
    BrandContext(&raw_context);

    // Create some modules with some stock debugging information.
    modules.Add(&module1);
    modules.Add(&module2);

    // By default, none of the modules have symbol info; call
    // SetModuleSymbols to override this.
    EXPECT_CALL(supplier, GetCStringSymbolData(_, _, _, _))
      .WillRepeatedly(Return(MockSymbolSupplier::NOT_FOUND));
  }

  // Set the Breakpad symbol information that supplier should return for
  // MODULE to INFO.
  void SetModuleSymbols(MockCodeModule *module, const string &info) {
    unsigned int buffer_size = info.size() + 1;
    char *buffer = reinterpret_cast<char*>(operator new(buffer_size));
    strcpy(buffer, info.c_str());
    EXPECT_CALL(supplier, GetCStringSymbolData(module, &system_info, _, _))
      .WillRepeatedly(DoAll(SetArgumentPointee<3>(buffer),
                            Return(MockSymbolSupplier::FOUND)));
  }

  // Populate stack_region with the contents of stack_section. Use
  // stack_section.start() as the region's starting address.
  void RegionFromSection() {
    string contents;
    ASSERT_TRUE(stack_section.GetContents(&contents));
    stack_region.Init(stack_section.start().Value(), contents);
  }

  // Fill RAW_CONTEXT with pseudo-random data, for round-trip checking.
  void BrandContext(MDRawContextX86 *raw_context) {
    u_int8_t x = 173;
    for (size_t i = 0; i < sizeof(*raw_context); i++)
      reinterpret_cast<u_int8_t *>(raw_context)[i] = (x += 17);
  }
  
  SystemInfo system_info;
  MDRawContextX86 raw_context;
  Section stack_section;
  MockMemoryRegion stack_region;
  MockCodeModule module1;
  MockCodeModule module2;
  MockCodeModules modules;
  MockSymbolSupplier supplier;
  BasicSourceLineResolver resolver;
  CallStack call_stack;
  const vector<StackFrame *> *frames;
};

class SanityCheck: public StackwalkerX86Fixture, public Test { };

TEST_F(SanityCheck, NoResolver) {
  stack_section.start() = 0x80000000;
  stack_section.D32(0).D32(0); // end-of-stack marker
  RegionFromSection();
  raw_context.eip = 0x40000200;
  raw_context.ebp = 0x80000000;

  StackwalkerX86 walker(&system_info, &raw_context, &stack_region, &modules,
                        NULL, NULL);
  // This should succeed, even without a resolver or supplier.
  ASSERT_TRUE(walker.Walk(&call_stack));
  frames = call_stack.frames();
  StackFrameX86 *frame = static_cast<StackFrameX86 *>(frames->at(0));
  // Check that the values from the original raw context made it
  // through to the context in the stack frame.
  EXPECT_EQ(0, memcmp(&raw_context, &frame->context, sizeof(raw_context)));
}

class GetContextFrame: public StackwalkerX86Fixture, public Test { };

TEST_F(GetContextFrame, Simple) {
  stack_section.start() = 0x80000000;
  stack_section.D32(0).D32(0); // end-of-stack marker
  RegionFromSection();
  raw_context.eip = 0x40000200;
  raw_context.ebp = 0x80000000;

  StackwalkerX86 walker(&system_info, &raw_context, &stack_region, &modules,
                        &supplier, &resolver);
  ASSERT_TRUE(walker.Walk(&call_stack));
  frames = call_stack.frames();
  StackFrameX86 *frame = static_cast<StackFrameX86 *>(frames->at(0));
  // Check that the values from the original raw context made it
  // through to the context in the stack frame.
  EXPECT_EQ(0, memcmp(&raw_context, &frame->context, sizeof(raw_context)));
}

class GetCallerFrame: public StackwalkerX86Fixture, public Test { };

// Walk a traditional frame. A traditional frame saves the caller's
// %ebp just below the return address, and has its own %ebp pointing
// at the saved %ebp.
TEST_F(GetCallerFrame, Traditional) {
  stack_section.start() = 0x80000000;
  Label frame0_ebp, frame1_ebp;
  stack_section
    .Append(12, 0)                      // frame 0: space
    .Mark(&frame0_ebp)                  // frame 0 %ebp points here
    .D32(frame1_ebp)                    // frame 0: saved %ebp
    .D32(0x40008679)                    // frame 0: return address
    .Append(8, 0)                       // frame 1: space
    .Mark(&frame1_ebp)                  // frame 1 %ebp points here
    .D32(0)                             // frame 1: saved %ebp (stack end)
    .D32(0);                            // frame 1: return address (stack end)
  RegionFromSection();
  raw_context.eip = 0x4000c7a5;
  raw_context.esp = stack_section.start().Value();
  raw_context.ebp = frame0_ebp.Value();

  StackwalkerX86 walker(&system_info, &raw_context, &stack_region, &modules,
                        &supplier, &resolver);
  ASSERT_TRUE(walker.Walk(&call_stack));
  frames = call_stack.frames();
  ASSERT_EQ(2U, frames->size());

  StackFrameX86 *frame0 = static_cast<StackFrameX86 *>(frames->at(0));
  EXPECT_EQ(StackFrame::FRAME_TRUST_CONTEXT, frame0->trust);
  EXPECT_EQ(StackFrameX86::CONTEXT_VALID_ALL, frame0->context_validity);
  EXPECT_EQ(0x4000c7a5U, frame0->instruction);
  EXPECT_EQ(0x4000c7a5U, frame0->context.eip);
  EXPECT_EQ(frame0_ebp.Value(), frame0->context.ebp);
  EXPECT_EQ(NULL, frame0->windows_frame_info);

  StackFrameX86 *frame1 = static_cast<StackFrameX86 *>(frames->at(1));
  EXPECT_EQ(StackFrame::FRAME_TRUST_FP, frame1->trust);
  ASSERT_EQ((StackFrameX86::CONTEXT_VALID_EIP
             | StackFrameX86::CONTEXT_VALID_ESP
             | StackFrameX86::CONTEXT_VALID_EBP),
            frame1->context_validity);
  EXPECT_EQ(0x40008679U, frame1->instruction + 1);
  EXPECT_EQ(0x40008679U, frame1->context.eip);
  EXPECT_EQ(frame1_ebp.Value(), frame1->context.ebp);
  EXPECT_EQ(NULL, frame1->windows_frame_info);
}

// Walk a traditional frame, but use a bogus %ebp value, forcing a scan
// of the stack for something that looks like a return address.
TEST_F(GetCallerFrame, TraditionalScan) {
  stack_section.start() = 0x80000000;
  Label frame1_ebp;
  stack_section
    // frame 0
    .D32(0xf065dc76)    // locals area:
    .D32(0x46ee2167)    // garbage that doesn't look like
    .D32(0xbab023ec)    // a return address
    .D32(frame1_ebp)    // saved %ebp (%ebp fails to point here, forcing scan)
    .D32(0x4000129d)    // return address
    // frame 1
    .Append(8, 0)       // space
    .Mark(&frame1_ebp)  // %ebp points here
    .D32(0)             // saved %ebp (stack end)
    .D32(0);            // return address (stack end)

  RegionFromSection();
  raw_context.eip = 0x4000f49d;
  raw_context.esp = stack_section.start().Value();
  // Make the frame pointer bogus, to make the stackwalker scan the stack
  // for something that looks like a return address.
  raw_context.ebp = 0xd43eed6e;

  StackwalkerX86 walker(&system_info, &raw_context, &stack_region, &modules,
                        &supplier, &resolver);
  ASSERT_TRUE(walker.Walk(&call_stack));
  frames = call_stack.frames();
  ASSERT_EQ(2U, frames->size());

  StackFrameX86 *frame0 = static_cast<StackFrameX86 *>(frames->at(0));
  EXPECT_EQ(StackFrame::FRAME_TRUST_CONTEXT, frame0->trust);
  ASSERT_EQ(StackFrameX86::CONTEXT_VALID_ALL, frame0->context_validity);
  EXPECT_EQ(0x4000f49dU, frame0->instruction);
  EXPECT_EQ(0x4000f49dU, frame0->context.eip);
  EXPECT_EQ(stack_section.start().Value(), frame0->context.esp);
  EXPECT_EQ(0xd43eed6eU, frame0->context.ebp);
  EXPECT_EQ(NULL, frame0->windows_frame_info);

  StackFrameX86 *frame1 = static_cast<StackFrameX86 *>(frames->at(1));
  EXPECT_EQ(StackFrame::FRAME_TRUST_SCAN, frame1->trust);
  // I'd argue that CONTEXT_VALID_EBP shouldn't be here, since the
  // walker does not actually fetch the EBP after a scan (forcing the
  // next frame to be scanned as well). But let's grandfather the existing
  // behavior in for now.
  ASSERT_EQ((StackFrameX86::CONTEXT_VALID_EIP
             | StackFrameX86::CONTEXT_VALID_ESP
             | StackFrameX86::CONTEXT_VALID_EBP),
            frame1->context_validity);
  EXPECT_EQ(0x4000129dU, frame1->instruction + 1);
  EXPECT_EQ(0x4000129dU, frame1->context.eip);
  EXPECT_EQ(0x80000014U, frame1->context.esp);
  EXPECT_EQ(0xd43eed6eU, frame1->context.ebp);
  EXPECT_EQ(NULL, frame1->windows_frame_info);
}

// Force scanning for a return address a long way down the stack
TEST_F(GetCallerFrame, TraditionalScanLongWay) {
  stack_section.start() = 0x80000000;
  Label frame1_ebp;
  stack_section
    // frame 0
    .D32(0xf065dc76)    // locals area:
    .D32(0x46ee2167)    // garbage that doesn't look like
    .D32(0xbab023ec)    // a return address
    .Append(20 * 4, 0)  // a bunch of space
    .D32(frame1_ebp)    // saved %ebp (%ebp fails to point here, forcing scan)
    .D32(0x4000129d)    // return address
    // frame 1
    .Append(8, 0)       // space
    .Mark(&frame1_ebp)  // %ebp points here
    .D32(0)             // saved %ebp (stack end)
    .D32(0);            // return address (stack end)

  RegionFromSection();
  raw_context.eip = 0x4000f49d;
  raw_context.esp = stack_section.start().Value();
  // Make the frame pointer bogus, to make the stackwalker scan the stack
  // for something that looks like a return address.
  raw_context.ebp = 0xd43eed6e;

  StackwalkerX86 walker(&system_info, &raw_context, &stack_region, &modules,
                        &supplier, &resolver);
  ASSERT_TRUE(walker.Walk(&call_stack));
  frames = call_stack.frames();
  ASSERT_EQ(2U, frames->size());

  StackFrameX86 *frame0 = static_cast<StackFrameX86 *>(frames->at(0));
  EXPECT_EQ(StackFrame::FRAME_TRUST_CONTEXT, frame0->trust);
  ASSERT_EQ(StackFrameX86::CONTEXT_VALID_ALL, frame0->context_validity);
  EXPECT_EQ(0x4000f49dU, frame0->instruction);
  EXPECT_EQ(0x4000f49dU, frame0->context.eip);
  EXPECT_EQ(stack_section.start().Value(), frame0->context.esp);
  EXPECT_EQ(0xd43eed6eU, frame0->context.ebp);
  EXPECT_EQ(NULL, frame0->windows_frame_info);

  StackFrameX86 *frame1 = static_cast<StackFrameX86 *>(frames->at(1));
  EXPECT_EQ(StackFrame::FRAME_TRUST_SCAN, frame1->trust);
  // I'd argue that CONTEXT_VALID_EBP shouldn't be here, since the
  // walker does not actually fetch the EBP after a scan (forcing the
  // next frame to be scanned as well). But let's grandfather the existing
  // behavior in for now.
  ASSERT_EQ((StackFrameX86::CONTEXT_VALID_EIP
             | StackFrameX86::CONTEXT_VALID_ESP
             | StackFrameX86::CONTEXT_VALID_EBP),
            frame1->context_validity);
  EXPECT_EQ(0x4000129dU, frame1->instruction + 1);
  EXPECT_EQ(0x4000129dU, frame1->context.eip);
  EXPECT_EQ(0x80000064U, frame1->context.esp);
  EXPECT_EQ(0xd43eed6eU, frame1->context.ebp);
  EXPECT_EQ(NULL, frame1->windows_frame_info);
}

// Use Windows frame data (a "STACK WIN 4" record, from a
// FrameTypeFrameData DIA record) to walk a stack frame.
TEST_F(GetCallerFrame, WindowsFrameData) {
  SetModuleSymbols(&module1,
                   "STACK WIN 4 aa85 176 0 0 4 10 4 0 1"
                   " $T2 $esp .cbSavedRegs + ="
                   " $T0 .raSearchStart ="
                   " $eip $T0 ^ ="
                   " $esp $T0 4 + ="
                   " $ebx $T2 4  - ^ ="
                   " $edi $T2 8  - ^ ="
                   " $esi $T2 12 - ^ ="
                   " $ebp $T2 16 - ^ =\n");
  Label frame1_esp, frame1_ebp;
  stack_section.start() = 0x80000000;
  stack_section
    // frame 0
    .D32(frame1_ebp)                    // saved regs: %ebp
    .D32(0xa7120d1a)                    //             %esi
    .D32(0x630891be)                    //             %edi
    .D32(0x9068a878)                    //             %ebx
    .D32(0xa08ea45f)                    // locals: unused
    .D32(0x40001350)                    // return address
    // frame 1
    .Mark(&frame1_esp)
    .Append(12, 0)                      // empty space
    .Mark(&frame1_ebp)
    .D32(0)                             // saved %ebp (stack end)
    .D32(0);                            // saved %eip (stack end)

  RegionFromSection();
  raw_context.eip = 0x4000aa85;
  raw_context.esp = stack_section.start().Value();
  raw_context.ebp = 0xf052c1de;         // should not be needed to walk frame

  StackwalkerX86 walker(&system_info, &raw_context, &stack_region, &modules,
                        &supplier, &resolver);
  ASSERT_TRUE(walker.Walk(&call_stack));
  frames = call_stack.frames();
  ASSERT_EQ(2U, frames->size());

  StackFrameX86 *frame0 = static_cast<StackFrameX86 *>(frames->at(0));
  EXPECT_EQ(StackFrame::FRAME_TRUST_CONTEXT, frame0->trust);
  ASSERT_EQ(StackFrameX86::CONTEXT_VALID_ALL, frame0->context_validity);
  EXPECT_EQ(0x4000aa85U, frame0->instruction);
  EXPECT_EQ(0x4000aa85U, frame0->context.eip);
  EXPECT_EQ(stack_section.start().Value(), frame0->context.esp);
  EXPECT_EQ(0xf052c1deU, frame0->context.ebp);
  EXPECT_TRUE(frame0->windows_frame_info != NULL);

  StackFrameX86 *frame1 = static_cast<StackFrameX86 *>(frames->at(1));
  EXPECT_EQ(StackFrame::FRAME_TRUST_CFI, frame1->trust);
  ASSERT_EQ((StackFrameX86::CONTEXT_VALID_EIP
             | StackFrameX86::CONTEXT_VALID_ESP
             | StackFrameX86::CONTEXT_VALID_EBP
             | StackFrameX86::CONTEXT_VALID_EBX
             | StackFrameX86::CONTEXT_VALID_ESI
             | StackFrameX86::CONTEXT_VALID_EDI),
            frame1->context_validity);
  EXPECT_EQ(0x40001350U, frame1->instruction + 1);
  EXPECT_EQ(0x40001350U, frame1->context.eip);
  EXPECT_EQ(frame1_esp.Value(), frame1->context.esp);
  EXPECT_EQ(frame1_ebp.Value(), frame1->context.ebp);
  EXPECT_EQ(0x9068a878U, frame1->context.ebx);
  EXPECT_EQ(0xa7120d1aU, frame1->context.esi);
  EXPECT_EQ(0x630891beU, frame1->context.edi);
  EXPECT_EQ(NULL, frame1->windows_frame_info);
}

// Use Windows frame data (a "STACK WIN 4" record, from a
// FrameTypeFrameData DIA record) to walk a stack frame where the stack
// is aligned and we must search
TEST_F(GetCallerFrame, WindowsFrameDataAligned) {
  SetModuleSymbols(&module1,
                   "STACK WIN 4 aa85 176 0 0 4 4 8 0 1"
		   " $T1 .raSearch ="
		   " $T0 $T1 4 - 8 @ ="
		   " $ebp $T1 4 - ^ ="
		   " $eip $T1 ^ ="
		   " $esp $T1 4 + =");
  Label frame1_esp, frame1_ebp;
  stack_section.start() = 0x80000000;
  stack_section
    // frame 0
    .D32(0x0ffa0ffa)                    // unused saved register
    .D32(0xdeaddead)                    // locals
    .D32(0xbeefbeef)
    .D32(0)                             // 8-byte alignment
    .D32(frame1_ebp)
    .D32(0x5000129d)                    // return address
    // frame 1
    .Mark(&frame1_esp)
    .D32(0x1)                           // parameter
    .Mark(&frame1_ebp)
    .D32(0)                             // saved %ebp (stack end)
    .D32(0);                            // saved %eip (stack end)

  RegionFromSection();
  raw_context.eip = 0x4000aa85;
  raw_context.esp = stack_section.start().Value();
  raw_context.ebp = 0xf052c1de;         // should not be needed to walk frame

  StackwalkerX86 walker(&system_info, &raw_context, &stack_region, &modules,
                        &supplier, &resolver);
  ASSERT_TRUE(walker.Walk(&call_stack));
  frames = call_stack.frames();
  ASSERT_EQ(2U, frames->size());

  StackFrameX86 *frame0 = static_cast<StackFrameX86 *>(frames->at(0));
  EXPECT_EQ(StackFrame::FRAME_TRUST_CONTEXT, frame0->trust);
  ASSERT_EQ(StackFrameX86::CONTEXT_VALID_ALL, frame0->context_validity);
  EXPECT_EQ(0x4000aa85U, frame0->instruction);
  EXPECT_EQ(0x4000aa85U, frame0->context.eip);
  EXPECT_EQ(stack_section.start().Value(), frame0->context.esp);
  EXPECT_EQ(0xf052c1deU, frame0->context.ebp);
  EXPECT_TRUE(frame0->windows_frame_info != NULL);

  StackFrameX86 *frame1 = static_cast<StackFrameX86 *>(frames->at(1));
  EXPECT_EQ(StackFrame::FRAME_TRUST_CFI, frame1->trust);
  ASSERT_EQ((StackFrameX86::CONTEXT_VALID_EIP
             | StackFrameX86::CONTEXT_VALID_ESP
             | StackFrameX86::CONTEXT_VALID_EBP),
            frame1->context_validity);
  EXPECT_EQ(0x5000129dU, frame1->instruction + 1);
  EXPECT_EQ(0x5000129dU, frame1->context.eip);
  EXPECT_EQ(frame1_esp.Value(), frame1->context.esp);
  EXPECT_EQ(frame1_ebp.Value(), frame1->context.ebp);
  EXPECT_EQ(NULL, frame1->windows_frame_info);
}

// Use Windows frame data (a "STACK WIN 4" record, from a
// FrameTypeFrameData DIA record) to walk a frame, and depend on the
// parameter size from the callee as well.
TEST_F(GetCallerFrame, WindowsFrameDataParameterSize) {
  SetModuleSymbols(&module1, "FUNC 1000 100 c module1::wheedle\n");
  SetModuleSymbols(&module2,
                   // Note bogus parameter size in FUNC record; the stack walker
                   // should prefer the STACK WIN record, and see '4' below.
                   "FUNC aa85 176 beef module2::whine\n"
                   "STACK WIN 4 aa85 176 0 0 4 10 4 0 1"
                   " $T2 $esp .cbLocals + .cbSavedRegs + ="
                   " $T0 .raSearchStart ="
                   " $eip $T0 ^ ="
                   " $esp $T0 4 + ="
                   " $ebp $T0 20 - ^ ="
                   " $ebx $T0 8 - ^ =\n");
  Label frame0_esp, frame0_ebp;
  Label frame1_esp;
  Label frame2_esp, frame2_ebp;
  stack_section.start() = 0x80000000;
  stack_section
    // frame 0, in module1::wheedle.  Traditional frame.
    .Mark(&frame0_esp)
    .Append(16, 0)      // frame space
    .Mark(&frame0_ebp)
    .D32(0x6fa902e0)    // saved %ebp.  Not a frame pointer.
    .D32(0x5000aa95)    // return address, in module2::whine
    // frame 1, in module2::whine.  FrameData frame.
    .Mark(&frame1_esp)
    .D32(0xbaa0cb7a)    // argument 3 passed to module1::wheedle
    .D32(0xbdc92f9f)    // argument 2
    .D32(0x0b1d8442)    // argument 1
    .D32(frame2_ebp)    // saved %ebp
    .D32(0xb1b90a15)    // unused
    .D32(0xf18e072d)    // unused
    .D32(0x2558c7f3)    // saved %ebx
    .D32(0x0365e25e)    // unused
    .D32(0x2a179e38)    // return address; $T0 points here
    // frame 2, in no module
    .Mark(&frame2_esp)
    .Append(12, 0)      // empty space
    .Mark(&frame2_ebp)
    .D32(0)             // saved %ebp (stack end)
    .D32(0);            // saved %eip (stack end)

  RegionFromSection();
  raw_context.eip = 0x40001004; // in module1::wheedle
  raw_context.esp = stack_section.start().Value();
  raw_context.ebp = frame0_ebp.Value();

  StackwalkerX86 walker(&system_info, &raw_context, &stack_region, &modules,
                        &supplier, &resolver);
  ASSERT_TRUE(walker.Walk(&call_stack));
  frames = call_stack.frames();
  ASSERT_EQ(3U, frames->size());

  StackFrameX86 *frame0 = static_cast<StackFrameX86 *>(frames->at(0));
  EXPECT_EQ(StackFrame::FRAME_TRUST_CONTEXT, frame0->trust);
  ASSERT_EQ(StackFrameX86::CONTEXT_VALID_ALL, frame0->context_validity);
  EXPECT_EQ(0x40001004U, frame0->instruction);
  EXPECT_EQ(0x40001004U, frame0->context.eip);
  EXPECT_EQ(frame0_esp.Value(), frame0->context.esp);
  EXPECT_EQ(frame0_ebp.Value(), frame0->context.ebp);
  EXPECT_EQ(&module1, frame0->module);
  EXPECT_EQ("module1::wheedle", frame0->function_name);
  EXPECT_EQ(0x40001000U, frame0->function_base);
  // The FUNC record for module1::wheedle should have produced a
  // WindowsFrameInfo structure with only the parameter size valid.
  ASSERT_TRUE(frame0->windows_frame_info != NULL);
  EXPECT_EQ(WindowsFrameInfo::VALID_PARAMETER_SIZE,
            frame0->windows_frame_info->valid);
  EXPECT_EQ(12U, frame0->windows_frame_info->parameter_size);

  StackFrameX86 *frame1 = static_cast<StackFrameX86 *>(frames->at(1));
  EXPECT_EQ(StackFrame::FRAME_TRUST_FP, frame1->trust);
  ASSERT_EQ((StackFrameX86::CONTEXT_VALID_EIP
             | StackFrameX86::CONTEXT_VALID_ESP
             | StackFrameX86::CONTEXT_VALID_EBP),
            frame1->context_validity);
  EXPECT_EQ(0x5000aa95U, frame1->instruction + 1);
  EXPECT_EQ(0x5000aa95U, frame1->context.eip);
  EXPECT_EQ(frame1_esp.Value(), frame1->context.esp);
  EXPECT_EQ(0x6fa902e0U, frame1->context.ebp);
  EXPECT_EQ(&module2, frame1->module);
  EXPECT_EQ("module2::whine", frame1->function_name);
  EXPECT_EQ(0x5000aa85U, frame1->function_base);
  ASSERT_TRUE(frame1->windows_frame_info != NULL);
  EXPECT_EQ(WindowsFrameInfo::VALID_ALL, frame1->windows_frame_info->valid);
  // This should not see the 0xbeef parameter size from the FUNC
  // record, but should instead see the STACK WIN record.
  EXPECT_EQ(4U, frame1->windows_frame_info->parameter_size);

  StackFrameX86 *frame2 = static_cast<StackFrameX86 *>(frames->at(2));
  EXPECT_EQ(StackFrame::FRAME_TRUST_CFI, frame2->trust);
  ASSERT_EQ((StackFrameX86::CONTEXT_VALID_EIP
             | StackFrameX86::CONTEXT_VALID_ESP
             | StackFrameX86::CONTEXT_VALID_EBP
             | StackFrameX86::CONTEXT_VALID_EBX),
            frame2->context_validity);
  EXPECT_EQ(0x2a179e38U, frame2->instruction + 1);
  EXPECT_EQ(0x2a179e38U, frame2->context.eip);
  EXPECT_EQ(frame2_esp.Value(), frame2->context.esp);
  EXPECT_EQ(frame2_ebp.Value(), frame2->context.ebp);
  EXPECT_EQ(0x2558c7f3U, frame2->context.ebx);
  EXPECT_EQ(NULL, frame2->module);
  EXPECT_EQ(NULL, frame2->windows_frame_info);
}

// Use Windows frame data (a "STACK WIN 4" record, from a
// FrameTypeFrameData DIA record) to walk a stack frame, where the
// expression fails to yield both an $eip and an $ebp value, and the stack
// walker must scan.
TEST_F(GetCallerFrame, WindowsFrameDataScan) {
  SetModuleSymbols(&module1,
                   "STACK WIN 4 c8c 111 0 0 4 10 4 0 1 bad program string\n");
  // Mark frame 1's PC as the end of the stack.
  SetModuleSymbols(&module2,
                   "FUNC 7c38 accf 0 module2::function\n"
                   "STACK WIN 4 7c38 accf 0 0 4 10 4 0 1 $eip 0 = $ebp 0 =\n");
  Label frame1_esp;
  stack_section.start() = 0x80000000;
  stack_section
    // frame 0
    .Append(16, 0x2a)                   // unused, garbage
    .D32(0x50007ce9)                    // return address
    // frame 1
    .Mark(&frame1_esp)
    .Append(8, 0);                      // empty space

  RegionFromSection();
  raw_context.eip = 0x40000c9c;
  raw_context.esp = stack_section.start().Value();
  raw_context.ebp = 0x2ae314cd;         // should not be needed to walk frame

  StackwalkerX86 walker(&system_info, &raw_context, &stack_region, &modules,
                        &supplier, &resolver);
  ASSERT_TRUE(walker.Walk(&call_stack));
  frames = call_stack.frames();
  ASSERT_EQ(2U, frames->size());

  StackFrameX86 *frame0 = static_cast<StackFrameX86 *>(frames->at(0));
  EXPECT_EQ(StackFrame::FRAME_TRUST_CONTEXT, frame0->trust);
  ASSERT_EQ(StackFrameX86::CONTEXT_VALID_ALL, frame0->context_validity);
  EXPECT_EQ(0x40000c9cU, frame0->instruction);
  EXPECT_EQ(0x40000c9cU, frame0->context.eip);
  EXPECT_EQ(stack_section.start().Value(), frame0->context.esp);
  EXPECT_EQ(0x2ae314cdU, frame0->context.ebp);
  EXPECT_TRUE(frame0->windows_frame_info != NULL);

  StackFrameX86 *frame1 = static_cast<StackFrameX86 *>(frames->at(1));
  EXPECT_EQ(StackFrame::FRAME_TRUST_SCAN, frame1->trust);
  // I'd argue that CONTEXT_VALID_EBP shouldn't be here, since the walker
  // does not actually fetch the EBP after a scan (forcing the next frame
  // to be scanned as well). But let's grandfather the existing behavior in
  // for now.
  ASSERT_EQ((StackFrameX86::CONTEXT_VALID_EIP
             | StackFrameX86::CONTEXT_VALID_ESP
             | StackFrameX86::CONTEXT_VALID_EBP),
            frame1->context_validity);
  EXPECT_EQ(0x50007ce9U, frame1->instruction + 1);
  EXPECT_EQ(0x50007ce9U, frame1->context.eip);
  EXPECT_EQ(frame1_esp.Value(), frame1->context.esp);
  EXPECT_TRUE(frame1->windows_frame_info != NULL);
}

// Use Windows frame data (a "STACK WIN 4" record, from a
// FrameTypeFrameData DIA record) to walk a stack frame, where the
// expression yields an $eip that falls outside of any module, and the
// stack walker must scan.
TEST_F(GetCallerFrame, WindowsFrameDataBadEIPScan) {
  SetModuleSymbols(&module1,
                   "STACK WIN 4 6e6 e7 0 0 0 8 4 0 1"
                   // A traditional frame, actually.
                   " $eip $ebp 4 + ^ = $esp $ebp 8 + = $ebp $ebp ^ =\n");
  // Mark frame 1's PC as the end of the stack.
  SetModuleSymbols(&module2,
                   "FUNC cfdb 8406 0 module2::function\n"
                   "STACK WIN 4 cfdb 8406 0 0 0 0 0 0 1 $eip 0 = $ebp 0 =\n");
  stack_section.start() = 0x80000000;

  // In this stack, the context's %ebp is pointing at the wrong place, so
  // the stack walker needs to scan to find the return address, and then
  // scan again to find the caller's saved %ebp.
  Label frame0_ebp, frame1_ebp, frame1_esp;
  stack_section
    // frame 0
    .Append(8, 0x2a)            // garbage
    .Mark(&frame0_ebp)          // frame 0 %ebp points here, but should point
                                // at *** below
    // The STACK WIN record says that the following two values are
    // frame 1's saved %ebp and return address, but the %ebp is wrong;
    // they're garbage. The stack walker will scan for the right values.
    .D32(0x3d937b2b)            // alleged to be frame 1's saved %ebp
    .D32(0x17847f5b)            // alleged to be frame 1's return address
    .D32(frame1_ebp)            // frame 1's real saved %ebp; scan will find
    .D32(0x2b2b2b2b)            // first word of realigned register save area
    // *** frame 0 %ebp ought to be pointing here
    .D32(0x2c2c2c2c)            // realigned locals area
    .D32(0x5000d000)            // frame 1's real saved %eip; scan will find
    // Frame 1, in module2::function. The STACK WIN record describes
    // this as the oldest frame, without referring to its contents, so
    // we needn't to provide any actual data here.
    .Mark(&frame1_esp)
    .Mark(&frame1_ebp)          // frame 1 %ebp points here
    // A dummy value for frame 1's %ebp to point at. The scan recognizes the
    // saved %ebp because it points to a valid word in the stack memory region.
    .D32(0x2d2d2d2d);

  RegionFromSection();
  raw_context.eip = 0x40000700;
  raw_context.esp = stack_section.start().Value();
  raw_context.ebp = frame0_ebp.Value();

  StackwalkerX86 walker(&system_info, &raw_context, &stack_region, &modules,
                        &supplier, &resolver);
  ASSERT_TRUE(walker.Walk(&call_stack));
  frames = call_stack.frames();
  ASSERT_EQ(2U, frames->size());

  StackFrameX86 *frame0 = static_cast<StackFrameX86 *>(frames->at(0));
  EXPECT_EQ(StackFrame::FRAME_TRUST_CONTEXT, frame0->trust);
  ASSERT_EQ(StackFrameX86::CONTEXT_VALID_ALL, frame0->context_validity);
  EXPECT_EQ(0x40000700U, frame0->instruction);
  EXPECT_EQ(0x40000700U, frame0->context.eip);
  EXPECT_EQ(stack_section.start().Value(), frame0->context.esp);
  EXPECT_EQ(frame0_ebp.Value(), frame0->context.ebp);
  EXPECT_TRUE(frame0->windows_frame_info != NULL);

  StackFrameX86 *frame1 = static_cast<StackFrameX86 *>(frames->at(1));
  EXPECT_EQ(StackFrame::FRAME_TRUST_CFI_SCAN, frame1->trust);
  // I'd argue that CONTEXT_VALID_EBP shouldn't be here, since the
  // walker does not actually fetch the EBP after a scan (forcing the
  // next frame to be scanned as well). But let's grandfather the existing
  // behavior in for now.
  ASSERT_EQ((StackFrameX86::CONTEXT_VALID_EIP
             | StackFrameX86::CONTEXT_VALID_ESP
             | StackFrameX86::CONTEXT_VALID_EBP),
            frame1->context_validity);
  EXPECT_EQ(0x5000d000U, frame1->instruction + 1);
  EXPECT_EQ(0x5000d000U, frame1->context.eip);
  EXPECT_EQ(frame1_esp.Value(), frame1->context.esp);
  EXPECT_EQ(frame1_ebp.Value(), frame1->context.ebp);
  EXPECT_TRUE(frame1->windows_frame_info != NULL);
}

// Use Windows FrameTypeFPO data to walk a stack frame for a function that
// does not modify %ebp from the value it had in the caller.
TEST_F(GetCallerFrame, WindowsFPOUnchangedEBP) {
  SetModuleSymbols(&module1,
                   // Note bogus parameter size in FUNC record; the walker
                   // should prefer the STACK WIN record, and see the '8' below.
                   "FUNC e8a8 100 feeb module1::discombobulated\n"
                   "STACK WIN 0 e8a8 100 0 0 8 4 10 0 0 0\n");
  Label frame0_esp;
  Label frame1_esp, frame1_ebp;
  stack_section.start() = 0x80000000;
  stack_section
    // frame 0, in module1::wheedle.  FrameTypeFPO (STACK WIN 0) frame.
    .Mark(&frame0_esp)
    // no outgoing parameters; this is the youngest frame.
    .D32(0x7c521352)    // four bytes of saved registers
    .Append(0x10, 0x42) // local area
    .D32(0x40009b5b)    // return address, in module1, no function
    // frame 1, in module1, no function.
    .Mark(&frame1_esp)
    .D32(0xf60ea7fc)    // junk
    .Mark(&frame1_ebp)
    .D32(0)             // saved %ebp (stack end)
    .D32(0);            // saved %eip (stack end)

  RegionFromSection();
  raw_context.eip = 0x4000e8b8; // in module1::whine
  raw_context.esp = stack_section.start().Value();
  // Frame pointer unchanged from caller.
  raw_context.ebp = frame1_ebp.Value();

  StackwalkerX86 walker(&system_info, &raw_context, &stack_region, &modules,
                        &supplier, &resolver);
  ASSERT_TRUE(walker.Walk(&call_stack));
  frames = call_stack.frames();
  ASSERT_EQ(2U, frames->size());

  StackFrameX86 *frame0 = static_cast<StackFrameX86 *>(frames->at(0));
  EXPECT_EQ(StackFrame::FRAME_TRUST_CONTEXT, frame0->trust);
  ASSERT_EQ(StackFrameX86::CONTEXT_VALID_ALL, frame0->context_validity);
  EXPECT_EQ(0x4000e8b8U, frame0->instruction);
  EXPECT_EQ(0x4000e8b8U, frame0->context.eip);
  EXPECT_EQ(frame0_esp.Value(), frame0->context.esp);
  EXPECT_EQ(frame1_ebp.Value(), frame0->context.ebp); // unchanged from caller
  EXPECT_EQ(&module1, frame0->module);
  EXPECT_EQ("module1::discombobulated", frame0->function_name);
  EXPECT_EQ(0x4000e8a8U, frame0->function_base);
  // The STACK WIN record for module1::discombobulated should have
  // produced a fully populated WindowsFrameInfo structure.
  ASSERT_TRUE(frame0->windows_frame_info != NULL);
  EXPECT_EQ(WindowsFrameInfo::VALID_ALL, frame0->windows_frame_info->valid);
  EXPECT_EQ(0x10U, frame0->windows_frame_info->local_size);

  StackFrameX86 *frame1 = static_cast<StackFrameX86 *>(frames->at(1));
  EXPECT_EQ(StackFrame::FRAME_TRUST_CFI, frame1->trust);
  ASSERT_EQ((StackFrameX86::CONTEXT_VALID_EIP
             | StackFrameX86::CONTEXT_VALID_ESP
             | StackFrameX86::CONTEXT_VALID_EBP),
            frame1->context_validity);
  EXPECT_EQ(0x40009b5bU, frame1->instruction + 1);
  EXPECT_EQ(0x40009b5bU, frame1->context.eip);
  EXPECT_EQ(frame1_esp.Value(), frame1->context.esp);
  EXPECT_EQ(frame1_ebp.Value(), frame1->context.ebp);
  EXPECT_EQ(&module1, frame1->module);
  EXPECT_EQ("", frame1->function_name);
  EXPECT_EQ(NULL, frame1->windows_frame_info);
}

// Use Windows FrameTypeFPO data to walk a stack frame for a function
// that uses %ebp for its own purposes, saving the value it had in the
// caller in the standard place in the saved register area.
TEST_F(GetCallerFrame, WindowsFPOUsedEBP) {
  SetModuleSymbols(&module1,
                   // Note bogus parameter size in FUNC record; the walker
                   // should prefer the STACK WIN record, and see the '8' below.
                   "FUNC 9aa8 e6 abbe module1::RaisedByTheAliens\n"
                   "STACK WIN 0 9aa8 e6 a 0 10 8 4 0 0 1\n");
  Label frame0_esp;
  Label frame1_esp, frame1_ebp;
  stack_section.start() = 0x80000000;
  stack_section
    // frame 0, in module1::wheedle.  FrameTypeFPO (STACK WIN 0) frame.
    .Mark(&frame0_esp)
    // no outgoing parameters; this is the youngest frame.
    .D32(frame1_ebp)    // saved register area: saved %ebp
    .D32(0xb68bd5f9)    // saved register area: something else
    .D32(0xd25d05fc)    // local area
    .D32(0x4000debe)    // return address, in module1, no function
    // frame 1, in module1, no function.
    .Mark(&frame1_esp)
    .D32(0xf0c9a974)    // junk
    .Mark(&frame1_ebp)
    .D32(0)             // saved %ebp (stack end)
    .D32(0);            // saved %eip (stack end)

  RegionFromSection();
  raw_context.eip = 0x40009ab8; // in module1::RaisedByTheAliens
  raw_context.esp = stack_section.start().Value();
  // RaisedByTheAliens uses %ebp for its own mysterious purposes.
  raw_context.ebp = 0xecbdd1a5;

  StackwalkerX86 walker(&system_info, &raw_context, &stack_region, &modules,
                        &supplier, &resolver);
  ASSERT_TRUE(walker.Walk(&call_stack));
  frames = call_stack.frames();
  ASSERT_EQ(2U, frames->size());

  StackFrameX86 *frame0 = static_cast<StackFrameX86 *>(frames->at(0));
  EXPECT_EQ(StackFrame::FRAME_TRUST_CONTEXT, frame0->trust);
  ASSERT_EQ(StackFrameX86::CONTEXT_VALID_ALL, frame0->context_validity);
  EXPECT_EQ(0x40009ab8U, frame0->instruction);
  EXPECT_EQ(0x40009ab8U, frame0->context.eip);
  EXPECT_EQ(frame0_esp.Value(), frame0->context.esp);
  EXPECT_EQ(0xecbdd1a5, frame0->context.ebp);
  EXPECT_EQ(&module1, frame0->module);
  EXPECT_EQ("module1::RaisedByTheAliens", frame0->function_name);
  EXPECT_EQ(0x40009aa8U, frame0->function_base);
  // The STACK WIN record for module1::RaisedByTheAliens should have
  // produced a fully populated WindowsFrameInfo structure.
  ASSERT_TRUE(frame0->windows_frame_info != NULL);
  EXPECT_EQ(WindowsFrameInfo::VALID_ALL, frame0->windows_frame_info->valid);
  EXPECT_EQ("", frame0->windows_frame_info->program_string);
  EXPECT_TRUE(frame0->windows_frame_info->allocates_base_pointer);

  StackFrameX86 *frame1 = static_cast<StackFrameX86 *>(frames->at(1));
  EXPECT_EQ(StackFrame::FRAME_TRUST_CFI, frame1->trust);
  ASSERT_EQ((StackFrameX86::CONTEXT_VALID_EIP
             | StackFrameX86::CONTEXT_VALID_ESP
             | StackFrameX86::CONTEXT_VALID_EBP),
            frame1->context_validity);
  EXPECT_EQ(0x4000debeU, frame1->instruction + 1);
  EXPECT_EQ(0x4000debeU, frame1->context.eip);
  EXPECT_EQ(frame1_esp.Value(), frame1->context.esp);
  EXPECT_EQ(frame1_ebp.Value(), frame1->context.ebp);
  EXPECT_EQ(&module1, frame1->module);
  EXPECT_EQ("", frame1->function_name);
  EXPECT_EQ(NULL, frame1->windows_frame_info);
}

struct CFIFixture: public StackwalkerX86Fixture {
  CFIFixture() {
    // Provide a bunch of STACK CFI records; individual tests walk to the
    // caller from every point in this series, expecting to find the same
    // set of register values.
    SetModuleSymbols(&module1,
                     // The youngest frame's function.
                     "FUNC 4000 1000 10 enchiridion\n"
                     // Initially, just a return address.
                     "STACK CFI INIT 4000 100 .cfa: $esp 4 + .ra: .cfa 4 - ^\n"
                     // Push %ebx.
                     "STACK CFI 4001 .cfa: $esp 8 + $ebx: .cfa 8 - ^\n"
                     // Move %esi into %ebx.  Weird, but permitted.
                     "STACK CFI 4002 $esi: $ebx\n"
                     // Allocate frame space, and save %edi.
                     "STACK CFI 4003 .cfa: $esp 20 + $edi: .cfa 16 - ^\n"
                     // Put the return address in %edi.
                     "STACK CFI 4005 .ra: $edi\n"
                     // Save %ebp, and use it as a frame pointer.
                     "STACK CFI 4006 .cfa: $ebp 8 + $ebp: .cfa 12 - ^\n"

                     // The calling function.
                     "FUNC 5000 1000 10 epictetus\n"
                     // Mark it as end of stack.
                     "STACK CFI INIT 5000 1000 .cfa: $esp .ra 0\n");

    // Provide some distinctive values for the caller's registers.
    expected.esp = 0x80000000;
    expected.eip = 0x40005510;
    expected.ebp = 0xc0d4aab9;
    expected.ebx = 0x60f20ce6;
    expected.esi = 0x53d1379d;
    expected.edi = 0xafbae234;

    // By default, registers are unchanged.
    raw_context = expected;
  }

  // Walk the stack, using stack_section as the contents of the stack
  // and raw_context as the current register values. (Set
  // raw_context.esp to the stack's starting address.) Expect two
  // stack frames; in the older frame, expect the callee-saves
  // registers to have values matching those in 'expected'.
  void CheckWalk() {
    RegionFromSection();
    raw_context.esp = stack_section.start().Value();

    StackwalkerX86 walker(&system_info, &raw_context, &stack_region, &modules,
                          &supplier, &resolver);
    ASSERT_TRUE(walker.Walk(&call_stack));
    frames = call_stack.frames();
    ASSERT_EQ(2U, frames->size());

    StackFrameX86 *frame0 = static_cast<StackFrameX86 *>(frames->at(0));
    EXPECT_EQ(StackFrame::FRAME_TRUST_CONTEXT, frame0->trust);
    ASSERT_EQ(StackFrameX86::CONTEXT_VALID_ALL, frame0->context_validity);
    EXPECT_EQ("enchiridion", frame0->function_name);
    EXPECT_EQ(0x40004000U, frame0->function_base);
    ASSERT_TRUE(frame0->windows_frame_info != NULL);
    ASSERT_EQ(WindowsFrameInfo::VALID_PARAMETER_SIZE,
              frame0->windows_frame_info->valid);
    ASSERT_TRUE(frame0->cfi_frame_info != NULL);

    StackFrameX86 *frame1 = static_cast<StackFrameX86 *>(frames->at(1));
    EXPECT_EQ(StackFrame::FRAME_TRUST_CFI, frame1->trust);
    ASSERT_EQ((StackFrameX86::CONTEXT_VALID_EIP |
               StackFrameX86::CONTEXT_VALID_ESP |
               StackFrameX86::CONTEXT_VALID_EBP |
               StackFrameX86::CONTEXT_VALID_EBX |
               StackFrameX86::CONTEXT_VALID_ESI |
               StackFrameX86::CONTEXT_VALID_EDI),
              frame1->context_validity);
    EXPECT_EQ(expected.eip, frame1->context.eip);
    EXPECT_EQ(expected.esp, frame1->context.esp);
    EXPECT_EQ(expected.ebp, frame1->context.ebp);
    EXPECT_EQ(expected.ebx, frame1->context.ebx);
    EXPECT_EQ(expected.esi, frame1->context.esi);
    EXPECT_EQ(expected.edi, frame1->context.edi);
    EXPECT_EQ("epictetus", frame1->function_name);
  }

  // The values the stack walker should find for the caller's registers.
  MDRawContextX86 expected;
};

class CFI: public CFIFixture, public Test { };

TEST_F(CFI, At4000) {
  Label frame1_esp = expected.esp;
  stack_section
    .D32(0x40005510)            // return address
    .Mark(&frame1_esp);         // This effectively sets stack_section.start().
  raw_context.eip = 0x40004000;
  CheckWalk();
}

TEST_F(CFI, At4001) {
  Label frame1_esp = expected.esp;
  stack_section
    .D32(0x60f20ce6)            // saved %ebx
    .D32(0x40005510)            // return address
    .Mark(&frame1_esp);         // This effectively sets stack_section.start().
  raw_context.eip = 0x40004001;
  raw_context.ebx = 0x91aa9a8b; // callee's %ebx value
  CheckWalk();
}

TEST_F(CFI, At4002) {
  Label frame1_esp = expected.esp;
  stack_section
    .D32(0x60f20ce6)            // saved %ebx
    .D32(0x40005510)            // return address
    .Mark(&frame1_esp);         // This effectively sets stack_section.start().
  raw_context.eip = 0x40004002;
  raw_context.ebx = 0x53d1379d; // saved %esi
  raw_context.esi = 0xa5c790ed; // callee's %esi value
  CheckWalk();
}

TEST_F(CFI, At4003) {
  Label frame1_esp = expected.esp;
  stack_section
    .D32(0x56ec3db7)            // garbage
    .D32(0xafbae234)            // saved %edi
    .D32(0x53d67131)            // garbage
    .D32(0x60f20ce6)            // saved %ebx
    .D32(0x40005510)            // return address
    .Mark(&frame1_esp);         // This effectively sets stack_section.start().
  raw_context.eip = 0x40004003;
  raw_context.ebx = 0x53d1379d; // saved %esi
  raw_context.esi = 0xa97f229d; // callee's %esi
  raw_context.edi = 0xb05cc997; // callee's %edi
  CheckWalk();
}

// The results here should be the same as those at module offset
// 0x4003.
TEST_F(CFI, At4004) {
  Label frame1_esp = expected.esp;
  stack_section
    .D32(0xe29782c2)            // garbage
    .D32(0xafbae234)            // saved %edi
    .D32(0x5ba29ce9)            // garbage
    .D32(0x60f20ce6)            // saved %ebx
    .D32(0x40005510)            // return address
    .Mark(&frame1_esp);         // This effectively sets stack_section.start().
  raw_context.eip = 0x40004004;
  raw_context.ebx = 0x53d1379d; // saved %esi
  raw_context.esi = 0x0fb7dc4e; // callee's %esi
  raw_context.edi = 0x993b4280; // callee's %edi
  CheckWalk();
}

TEST_F(CFI, At4005) {
  Label frame1_esp = expected.esp;
  stack_section
    .D32(0xe29782c2)            // garbage
    .D32(0xafbae234)            // saved %edi
    .D32(0x5ba29ce9)            // garbage
    .D32(0x60f20ce6)            // saved %ebx
    .D32(0x8036cc02)            // garbage
    .Mark(&frame1_esp);         // This effectively sets stack_section.start().
  raw_context.eip = 0x40004005;
  raw_context.ebx = 0x53d1379d; // saved %esi
  raw_context.esi = 0x0fb7dc4e; // callee's %esi
  raw_context.edi = 0x40005510; // return address
  CheckWalk();
}

TEST_F(CFI, At4006) {
  Label frame0_ebp;
  Label frame1_esp = expected.esp;
  stack_section
    .D32(0xdcdd25cd)            // garbage
    .D32(0xafbae234)            // saved %edi
    .D32(0xc0d4aab9)            // saved %ebp
    .Mark(&frame0_ebp)          // frame pointer points here
    .D32(0x60f20ce6)            // saved %ebx
    .D32(0x8036cc02)            // garbage
    .Mark(&frame1_esp);         // This effectively sets stack_section.start().
  raw_context.eip = 0x40004006;
  raw_context.ebp = frame0_ebp.Value();
  raw_context.ebx = 0x53d1379d; // saved %esi
  raw_context.esi = 0x743833c9; // callee's %esi
  raw_context.edi = 0x40005510; // return address
  CheckWalk();
}

