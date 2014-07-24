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

#ifndef CodeProfiling_h
#define CodeProfiling_h

namespace WTF {

class MetaAllocator;
class MetaAllocatorTracker;

}

namespace JSC {

class CodeProfile;
class SourceCode;

class CodeProfiling {
    enum Mode {
        Disabled,
        Enabled,
        Verbose,
        VeryVerbose
    };

public:
    CodeProfiling(const SourceCode& source)
        : m_active(enabled())
    {
        if (m_active)
            begin(source);
    }

    ~CodeProfiling()
    {
        if (m_active)
            end();
    }

    static bool enabled() { return s_mode != Disabled; }
    static bool beVerbose() { return s_mode >= Verbose; }
    static bool beVeryVerbose() { return s_mode >= VeryVerbose; }

    static void notifyAllocator(WTF::MetaAllocator*);
    static void* getOwnerUIDForPC(void*);
    static void sample(void* pc, void** framePointer);

private:
    void begin(const SourceCode&);
    void end();

    bool m_active;

    static Mode s_mode;
    static WTF::MetaAllocatorTracker* s_tracker;
    static volatile CodeProfile* s_profileStack;
};

}

#endif // CodeProfiling_h

