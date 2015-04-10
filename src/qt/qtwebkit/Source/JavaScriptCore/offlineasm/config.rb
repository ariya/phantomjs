# Copyright (C) 2012 Apple Inc. All rights reserved.
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

$preferredCommentStartColumn = 60


# Turns on dumping of the count of labels.
# For example,  the output will look like this:
#
#    ...
#    OFFLINE_ASM_LOCAL_LABEL(_offlineasm_4_functionArityCheck__continue)  // Local Label 24 .
#    ...
#    OFFLINE_ASM_GLOBAL_LABEL(llint_op_enter)  // Global Label 8 .
#    ...
#
$enableLabelCountComments = false

# Turns on dumping of source file and line numbers in the output file.
# For example,  the output will look like this:
#
#    ...
#    "\tmovq -8(%r13), %rcx\n"   // JavaScriptCore/llint/LowLevelInterpreter64.asm:185
#    "\tmovl 52(%rcx), %ecx\n"   // JavaScriptCore/llint/LowLevelInterpreter64.asm:186
#    ...
#
$enableCodeOriginComments = true

# Turns on recording and dumping of annotations in the generated output file.
# An annotations can be specified for each instruction in the source asm files.
# For example, the output will look like this:
#
#     ...
#    "\tmovq -8(%r13), %rcx\n"   // t2<CodeBlock> = cfr.CodeBlock
#    "\tmovl 52(%rcx), %ecx\n"   // t2<size_t> = t2<CodeBlock>.m_numVars
#     ...
#
$enableInstrAnnotations = false
