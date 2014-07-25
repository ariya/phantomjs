// Copyright (c) 2012, Google Inc.
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

#include "client/ios/handler/ios_exception_minidump_generator.h"

#include "google_breakpad/common/minidump_exception_mac.h"
#include "client/minidump_file_writer-inl.h"
#include "processor/scoped_ptr.h"

namespace {

const uint32_t kExpectedFinalFp = 4;
const uint32_t kExpectedFinalSp = 0;
const int kExceptionType = EXC_SOFTWARE;
const int kExceptionCode = MD_EXCEPTION_CODE_MAC_NS_EXCEPTION;

#ifdef HAS_ARM_SUPPORT
// Append the given 4 bytes value to the sp position of the stack represented
// by memory.
void AppendToMemory(uint8_t *memory, uint32_t sp, uint32_t data) {
  assert(sizeof(data) == 4);
  memcpy(memory + sp, &data, sizeof(data));
}
#endif

}  // namespace

namespace google_breakpad {

IosExceptionMinidumpGenerator::IosExceptionMinidumpGenerator(
    NSException *exception)
    : MinidumpGenerator(mach_task_self(), 0) {
  return_addresses_ = [[exception callStackReturnAddresses] retain];
  SetExceptionInformation(kExceptionType,
                          kExceptionCode,
                          0,
                          pthread_mach_thread_np(pthread_self()));
}

IosExceptionMinidumpGenerator::~IosExceptionMinidumpGenerator() {
  [return_addresses_ release];
}

bool IosExceptionMinidumpGenerator::WriteCrashingContext(
    MDLocationDescriptor *register_location) {
#ifdef HAS_ARM_SUPPORT
  TypedMDRVA<MDRawContextARM> context(&writer_);
  if (!context.Allocate())
    return false;
  *register_location = context.location();
  MDRawContextARM *context_ptr = context.get();
  memset(context_ptr, 0, sizeof(MDRawContextARM));
  context_ptr->context_flags = MD_CONTEXT_ARM_FULL;
  context_ptr->iregs[7] = kExpectedFinalFp;  // FP
  context_ptr->iregs[13] = kExpectedFinalSp;  // SP
  uint32_t pc = GetPCFromException();
  context_ptr->iregs[14] = pc;  // LR
  context_ptr->iregs[15] = pc;  // PC
  return true;
#else
  assert(false);
  return false;
#endif
}

uint32_t IosExceptionMinidumpGenerator::GetPCFromException() {
  return [[return_addresses_ objectAtIndex:0] unsignedIntegerValue];
}

bool IosExceptionMinidumpGenerator::WriteExceptionStream(
    MDRawDirectory *exception_stream) {
#ifdef HAS_ARM_SUPPORT
  TypedMDRVA<MDRawExceptionStream> exception(&writer_);

  if (!exception.Allocate())
    return false;

  exception_stream->stream_type = MD_EXCEPTION_STREAM;
  exception_stream->location = exception.location();
  MDRawExceptionStream *exception_ptr = exception.get();
  exception_ptr->thread_id = pthread_mach_thread_np(pthread_self());

  // This naming is confusing, but it is the proper translation from
  // mach naming to minidump naming.
  exception_ptr->exception_record.exception_code = kExceptionType;
  exception_ptr->exception_record.exception_flags = kExceptionCode;

  if (!WriteCrashingContext(&exception_ptr->thread_context))
    return false;

  exception_ptr->exception_record.exception_address = GetPCFromException();
  return true;
#else
  return MinidumpGenerator::WriteExceptionStream(exception_stream);
#endif
}

bool IosExceptionMinidumpGenerator::WriteThreadStream(mach_port_t thread_id,
                                                      MDRawThread *thread) {
#ifdef HAS_ARM_SUPPORT
  if (pthread_mach_thread_np(pthread_self()) != thread_id)
    return MinidumpGenerator::WriteThreadStream(thread_id, thread);

  size_t frame_count = [return_addresses_ count];
  UntypedMDRVA memory(&writer_);
  size_t size = 8 * (frame_count - 1) + 4;
  if (!memory.Allocate(size))
    return false;
  scoped_array<uint8_t> stack_memory(new uint8_t[size]);
  uint32_t sp = size - 4;
  uint32_t fp = 0;
  uint32_t lr = [[return_addresses_ lastObject] unsignedIntegerValue];
  for (int current_frame = frame_count - 2;
       current_frame >= 0;
       --current_frame) {
    AppendToMemory(stack_memory.get(), sp, fp);
    sp -= 4;
    fp = sp;
    AppendToMemory(stack_memory.get(), sp, lr);
    sp -= 4;
    lr = [[return_addresses_ objectAtIndex:current_frame] unsignedIntegerValue];
  }
  if (!memory.Copy(stack_memory.get(), size))
    return false;
  assert(sp == kExpectedFinalSp);
  assert(fp == kExpectedFinalFp);
  assert(lr == GetPCFromException());
  thread->stack.start_of_memory_range = sp;
  thread->stack.memory = memory.location();
  memory_blocks_.push_back(thread->stack);

  if (!WriteCrashingContext(&thread->thread_context))
    return false;

  thread->thread_id = thread_id;
  return true;
#else
  return MinidumpGenerator::WriteThreadStream(thread_id, thread);
#endif
}

}  // namespace google_breakpad
