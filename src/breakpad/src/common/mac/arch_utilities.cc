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

#include "common/mac/arch_utilities.h"

#include <mach-o/arch.h>
#include <stdio.h>
#include <string.h>

#ifndef CPU_TYPE_ARM
#define CPU_TYPE_ARM (static_cast<cpu_type_t>(12))
#endif  // CPU_TYPE_ARM

#ifndef CPU_SUBTYPE_ARM_V7
#define CPU_SUBTYPE_ARM_V7 (static_cast<cpu_subtype_t>(9))
#endif  // CPU_SUBTYPE_ARM_V7

#ifndef CPU_SUBTYPE_ARM_V7S
#define CPU_SUBTYPE_ARM_V7S (static_cast<cpu_subtype_t>(11))
#endif  // CPU_SUBTYPE_ARM_V7S

#ifndef CPU_TYPE_ARM64
#define CPU_TYPE_ARM64 (static_cast<cpu_type_t>(16777228))
#endif  // CPU_TYPE_ARM64

#ifndef CPU_SUBTYPE_ARM64_ALL
#define CPU_SUBTYPE_ARM64_ALL (static_cast<cpu_type_t>(0))
#endif  // CPU_SUBTYPE_ARM64_ALL

namespace {

const NXArchInfo* ArchInfo_arm64() {
  NXArchInfo* arm64 = new NXArchInfo;
  *arm64 = *NXGetArchInfoFromCpuType(CPU_TYPE_ARM,
                                     CPU_SUBTYPE_ARM_V7);
  arm64->name = "arm64";
  arm64->cputype = CPU_TYPE_ARM64;
  arm64->cpusubtype = CPU_SUBTYPE_ARM64_ALL;
  arm64->description = "arm 64";
  return arm64;
}

const NXArchInfo* ArchInfo_armv7s() {
  NXArchInfo* armv7s = new NXArchInfo;
  *armv7s = *NXGetArchInfoFromCpuType(CPU_TYPE_ARM,
                                      CPU_SUBTYPE_ARM_V7);
  armv7s->name = "armv7s";
  armv7s->cpusubtype = CPU_SUBTYPE_ARM_V7S;
  armv7s->description = "arm v7s";
  return armv7s;
}

}  // namespace

namespace google_breakpad {

const NXArchInfo* BreakpadGetArchInfoFromName(const char* arch_name) {
  // TODO: Remove this when the OS knows about arm64.
  if (!strcmp("arm64", arch_name))
    return BreakpadGetArchInfoFromCpuType(CPU_TYPE_ARM64,
                                          CPU_SUBTYPE_ARM64_ALL);

  // TODO: Remove this when the OS knows about armv7s.
  if (!strcmp("armv7s", arch_name))
    return BreakpadGetArchInfoFromCpuType(CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7S);

  return NXGetArchInfoFromName(arch_name);
}

const NXArchInfo* BreakpadGetArchInfoFromCpuType(cpu_type_t cpu_type,
                                                 cpu_subtype_t cpu_subtype) {
  // TODO: Remove this when the OS knows about arm64.
  if (cpu_type == CPU_TYPE_ARM64 && cpu_subtype == CPU_SUBTYPE_ARM64_ALL) {
    static const NXArchInfo* arm64 = ArchInfo_arm64();
    return arm64;
  }

  // TODO: Remove this when the OS knows about armv7s.
  if (cpu_type == CPU_TYPE_ARM && cpu_subtype == CPU_SUBTYPE_ARM_V7S) {
    static const NXArchInfo* armv7s = ArchInfo_armv7s();
    return armv7s;
  }

  return NXGetArchInfoFromCpuType(cpu_type, cpu_subtype);
}

}  // namespace google_breakpad
