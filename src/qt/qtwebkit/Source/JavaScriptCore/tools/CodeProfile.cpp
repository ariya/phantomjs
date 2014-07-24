/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "CodeProfile.h"

#include "CodeBlock.h"
#include "CodeProfiling.h"
#include "LinkBuffer.h"
#include "ProfileTreeNode.h"
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(MAC)
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#endif

namespace JSC {

// Map from CodeType enum to a corresponding name.
const char* CodeProfile::s_codeTypeNames[CodeProfile::NumberOfCodeTypes] = {
    "[[EngineCode]]",
    "[[GlobalThunk]]",
    "[[RegExpCode]]",
    "[[DFGJIT]]",
    "[[BaselineOnly]]",
    "[[BaselineProfile]]",
    "[[BaselineOSR]]",
    "[[EngineFrame]]"
};

// Helper function, find the symbol name for a pc in JSC.
static const char* symbolName(void* address)
{
#if PLATFORM(MAC)
    Dl_info info;
    if (!dladdr(address, &info) || !info.dli_sname)
        return "<unknown>";

    const char* mangledName = info.dli_sname;
    const char* cxaDemangled = abi::__cxa_demangle(mangledName, 0, 0, 0);
    return cxaDemangled ? cxaDemangled : mangledName;
#else
    UNUSED_PARAM(address);
    return "<unknown>";
#endif
}

// Helper function, truncate traces to prune the output & make very verbose mode a little more readable.
static bool truncateTrace(const char* symbolName)
{
    return !strcmp(symbolName, "JSC::BytecodeGenerator::generate()")
        || !strcmp(symbolName, "JSC::Parser<JSC::Lexer<unsigned char> >::parseInner()")
        || !strcmp(symbolName, "WTF::fastMalloc(unsigned long)")
        || !strcmp(symbolName, "WTF::calculateUTCOffset()")
        || !strcmp(symbolName, "JSC::DFG::ByteCodeParser::parseCodeBlock()");
        
}

// Each trace consists of a sequence of zero or more 'EngineFrame' entries,
// followed by a sample in JIT code, or one or more 'EngineFrame' entries,
// followed by a 'EngineCode' terminator.
void CodeProfile::sample(void* pc, void** framePointer)
{
    // Disallow traces containing only a 'EngineCode' terminator, without any 'EngineFrame' frames.
    if (!framePointer)
        return;

    while (framePointer) {
        CodeType type;

#if ENABLE(JIT)
        // Determine if this sample fell in JIT code, and if so, from which JIT & why.
        void* ownerUID = CodeProfiling::getOwnerUIDForPC(pc);

        if (!ownerUID)
            type = EngineFrame;
        else if (ownerUID == GLOBAL_THUNK_ID)
            type = GlobalThunk;
        else if (ownerUID == REGEXP_CODE_ID)
            type = RegExpCode;
        else {
            CodeBlock* codeBlock = static_cast<CodeBlock*>(ownerUID);
            if (codeBlock->getJITType() == JITCode::DFGJIT)
                type = DFGJIT;
            else if (codeBlock->canCompileWithDFGState() != DFG::CanCompile)
                type = BaselineOnly;
            else if (codeBlock->replacement())
                type = BaselineOSR;
            else
                type = BaselineProfile;
        }
#else
        type = EngineFrame;
#endif

        // A sample in JIT code terminates the trace.
        m_samples.append(CodeRecord(pc, type));
        if (type != EngineFrame)
            return;

#if PLATFORM(MAC) && CPU(X86_64)
        // Walk up the stack.
        pc = framePointer[1];
        framePointer = reinterpret_cast<void**>(*framePointer);
#elif OS(LINUX) && CPU(X86)
        // Don't unwind the stack as some dependent third party libraries
        // may be compiled with -fomit-frame-pointer.
        framePointer = 0;
#else
        // This platform is not yet supported!
        RELEASE_ASSERT_NOT_REACHED();
#endif
    }

    // If we get here, we walked the entire stack without finding any frames of JIT code.
    m_samples.append(CodeRecord(0, EngineCode));
}

void CodeProfile::report()
{
    dataLogF("<CodeProfiling %s:%d>\n", m_file.data(), m_lineNo);

    // How many frames of C-code to print - 0, if not verbose, 1 if verbose, up to 1024 if very verbose.
    unsigned recursionLimit = CodeProfiling::beVeryVerbose() ? 1024 : CodeProfiling::beVerbose();

    ProfileTreeNode profile;

    // Walk through the sample buffer.
    size_t trace = 0;
    while (trace < m_samples.size()) {

        // All traces are zero or more 'EngineFrame's, followed by a non-'EngineFrame'.
        // Scan to find the last sample in the trace.
        size_t lastInTrace = trace;
        while (m_samples[lastInTrace].type == EngineFrame)
            ++lastInTrace;

        // We use the last sample type to look up a name (used as a bucket in the profiler).
        ProfileTreeNode* callbacks = profile.sampleChild(s_codeTypeNames[m_samples[lastInTrace].type]);

        // If there are any samples in C-code, add up to recursionLimit of them into the profile tree.
        size_t lastEngineFrame = lastInTrace;
        for (unsigned count = 0; lastEngineFrame > trace && count < recursionLimit; ++count) {
            --lastEngineFrame;
            ASSERT(m_samples[lastEngineFrame].type == EngineFrame);
            const char* name = symbolName(m_samples[lastEngineFrame].pc);
            callbacks = callbacks->sampleChild(name);
            if (truncateTrace(name))
                break;
        }

        // Move on to the next trace.
        trace = lastInTrace + 1;
        ASSERT(trace <= m_samples.size());
    }

    // Output the profile tree.
    dataLogF("Total samples: %lld\n", static_cast<long long>(profile.childCount()));
    profile.dump();
    
    for (size_t i = 0 ; i < m_children.size(); ++i)
        m_children[i]->report();

    dataLogF("</CodeProfiling %s:%d>\n", m_file.data(), m_lineNo);
}

}
