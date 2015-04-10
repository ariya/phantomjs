# Copyright (C) 2011 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

require "config"

# Interesting invariant, which we take advantage of: branching instructions
# always begin with "b", and no non-branching instructions begin with "b".
# Terminal instructions are "jmp" and "ret".

MACRO_INSTRUCTIONS =
    [
     "addi",
     "andi",
     "lshifti",
     "lshiftp",
     "lshiftq",
     "muli",
     "negi",
     "negp",
     "negq",
     "noti",
     "ori",
     "rshifti",
     "urshifti",
     "rshiftp",
     "urshiftp",
     "rshiftq",
     "urshiftq",
     "subi",
     "xori",
     "loadi",
     "loadis",
     "loadb",
     "loadbs",
     "loadh",
     "loadhs",
     "storei",
     "storeb",
     "loadd",
     "moved",
     "stored",
     "addd",
     "divd",
     "subd",
     "muld",
     "sqrtd",
     "ci2d",
     "fii2d", # usage: fii2d <gpr with least significant bits>, <gpr with most significant bits>, <fpr>
     "fd2ii", # usage: fd2ii <fpr>, <gpr with least significant bits>, <gpr with most significant bits>
     "fq2d",
     "fd2q",
     "bdeq",
     "bdneq",
     "bdgt",
     "bdgteq",
     "bdlt",
     "bdlteq",
     "bdequn",
     "bdnequn",
     "bdgtun",
     "bdgtequn",
     "bdltun",
     "bdltequn",
     "btd2i",
     "td2i",
     "bcd2i",
     "movdz",
     "pop",
     "push",
     "move",
     "sxi2q",
     "zxi2q",
     "nop",
     "bieq",
     "bineq",
     "bia",
     "biaeq",
     "bib",
     "bibeq",
     "bigt",
     "bigteq",
     "bilt",
     "bilteq",
     "bbeq",
     "bbneq",
     "bba",
     "bbaeq",
     "bbb",
     "bbbeq",
     "bbgt",
     "bbgteq",
     "bblt",
     "bblteq",
     "btis",
     "btiz",
     "btinz",
     "btbs",
     "btbz",
     "btbnz",
     "jmp",
     "baddio",
     "baddis",
     "baddiz",
     "baddinz",
     "bsubio",
     "bsubis",
     "bsubiz",
     "bsubinz",
     "bmulio",
     "bmulis",
     "bmuliz",
     "bmulinz",
     "borio",
     "boris",
     "boriz",
     "borinz",
     "break",
     "call",
     "ret",
     "cbeq",
     "cbneq",
     "cba",
     "cbaeq",
     "cbb",
     "cbbeq",
     "cbgt",
     "cbgteq",
     "cblt",
     "cblteq",
     "cieq",
     "cineq",
     "cia",
     "ciaeq",
     "cib",
     "cibeq",
     "cigt",
     "cigteq",
     "cilt",
     "cilteq",
     "tis",
     "tiz",
     "tinz",
     "tbs",
     "tbz",
     "tbnz",
     "tps",
     "tpz",
     "tpnz",
     "peek",
     "poke",
     "bpeq",
     "bpneq",
     "bpa",
     "bpaeq",
     "bpb",
     "bpbeq",
     "bpgt",
     "bpgteq",
     "bplt",
     "bplteq",
     "addp",
     "mulp",
     "andp",
     "orp",
     "subp",
     "xorp",
     "loadp",
     "cpeq",
     "cpneq",
     "cpa",
     "cpaeq",
     "cpb",
     "cpbeq",
     "cpgt",
     "cpgteq",
     "cplt",
     "cplteq",
     "storep",
     "btps",
     "btpz",
     "btpnz",
     "baddpo",
     "baddps",
     "baddpz",
     "baddpnz",
     "tqs",
     "tqz",
     "tqnz",
     "peekq",
     "pokeq",
     "bqeq",
     "bqneq",
     "bqa",
     "bqaeq",
     "bqb",
     "bqbeq",
     "bqgt",
     "bqgteq",
     "bqlt",
     "bqlteq",
     "addq",
     "mulq",
     "andq",
     "orq",
     "subq",
     "xorq",
     "loadq",
     "cqeq",
     "cqneq",
     "cqa",
     "cqaeq",
     "cqb",
     "cqbeq",
     "cqgt",
     "cqgteq",
     "cqlt",
     "cqlteq",
     "storeq",
     "btqs",
     "btqz",
     "btqnz",
     "baddqo",
     "baddqs",
     "baddqz",
     "baddqnz",
     "bo",
     "bs",
     "bz",
     "bnz",
     "leai",
     "leap",
    ]

X86_INSTRUCTIONS =
    [
     "cdqi",
     "idivi"
    ]

ARM_INSTRUCTIONS =
    [
     "smulli",
     "addis",
     "subis",
     "oris"
    ]

MIPS_INSTRUCTIONS =
    [
    "la",
    "movz",
    "movn",
    "slt",
    "sltu",
    "pichdr"
    ]

SH4_INSTRUCTIONS =
    [
    "shllx",
    "shlrx",
    "shld",
    "shad",
    "bdnan",
    "loaddReversedAndIncrementAddress",
    "storedReversedAndDecrementAddress",
    "ldspr",
    "stspr"
    ]

CXX_INSTRUCTIONS =
    [
     "cloopCrash",           # no operands
     "cloopCallJSFunction",  # operands: callee
     "cloopCallNative",      # operands: callee
     "cloopCallSlowPath",    # operands: callTarget, currentFrame, currentPC

     # For debugging only:
     # Takes no operands but simply emits whatever follows in // comments as
     # a line of C++ code in the generated LLIntAssembly.h file. This can be
     # used to insert instrumentation into the interpreter loop to inspect
     # variables of interest. Do not leave these instructions in production
     # code.
     "cloopDo",              # no operands
    ]

INSTRUCTIONS = MACRO_INSTRUCTIONS + X86_INSTRUCTIONS + ARM_INSTRUCTIONS + MIPS_INSTRUCTIONS + SH4_INSTRUCTIONS + CXX_INSTRUCTIONS

INSTRUCTION_PATTERN = Regexp.new('\\A((' + INSTRUCTIONS.join(')|(') + '))\\Z')

def isBranch(instruction)
    instruction =~ /^b/
end

def hasFallThrough(instruction)
    instruction != "ret" and instruction != "jmp"
end

