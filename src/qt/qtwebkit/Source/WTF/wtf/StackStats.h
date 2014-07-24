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

#ifndef StackStats_h
#define StackStats_h

#include "ExportMacros.h"
#include "ThreadingPrimitives.h"


// Define this flag to enable Stack stats collection. This feature is useful
// for getting a sample of native stack usage sizes.
//
// Enabling this will cause stats to be collected and written to a log file at
// various  instrumented points in the code. It will result in noticeable
// performance loss. Hence, this should only be enable when you want to do
// some stats location in your local build. This code is provided here as a
// convenience for collecting that data. It is not meant to be enabled by
// default on release or debug builds.

// #define ENABLE_STACK_STATS 1


namespace WTF {

#if !ENABLE(STACK_STATS) 

class StackStats {
public:
    // The CheckPoint class is for marking check points corresponding
    // each location in code where a stack recursion check is being done.

    class CheckPoint {
    public:
        CheckPoint() { }
    };

    class PerThreadStats {
    public:
        PerThreadStats() { }
    };

    class LayoutCheckPoint {
    public:
        LayoutCheckPoint() { }
    };

    static void initialize() { }
    static void probe() { }
};

#else // ENABLE(STACK_STATS)

class StackStats {
public:
    // The CheckPoint class is for marking check points corresponding
    // each location in code where a stack recursion check is being done.

    class CheckPoint {
    public:
        CheckPoint();
        ~CheckPoint();
    private:
        CheckPoint* m_prev;
    };

    class PerThreadStats {
    public:
        PerThreadStats();

    private:
        int m_reentryDepth;
        char* m_stackStart;
        CheckPoint* m_currentCheckPoint;

        friend class CheckPoint;
        friend class StackStats;
    };

    class LayoutCheckPoint {
    public:
        WTF_EXPORT_PRIVATE LayoutCheckPoint();
        WTF_EXPORT_PRIVATE ~LayoutCheckPoint();

    private:
        LayoutCheckPoint* m_prev;
        int m_depth;
    };

    // Initializes locks and the log file. Should only be called once.
    static void initialize();

    // Used for probing the stack at places where we suspect to be high
    // points of stack usage but are NOT check points where stack recursion
    // is checked.
    //
    // The more places where we add this probe, the more accurate our
    // stats data will be. However, adding too many probes will also
    // result in unnecessary performance loss. So, only add these probes
    // judiciously where appropriate.
    static void probe();

private:
    // CheckPoint management:
    static Mutex* s_sharedLock;
    static CheckPoint* s_topCheckPoint;
    static LayoutCheckPoint* s_firstLayoutCheckPoint;
    static LayoutCheckPoint* s_topLayoutCheckPoint;

    // High watermark stats:
    static int s_maxCheckPointDiff;
    static int s_maxStackHeight;
    static int s_maxReentryDepth;

    static int s_maxLayoutCheckPointDiff;
    static int s_maxTotalLayoutCheckPointDiff;
    static int s_maxLayoutReentryDepth;

    friend class CheckPoint;
    friend class LayoutCheckPoint;
};

#endif // ENABLE(STACK_STATS) 

} // namespace WTF

using WTF::StackStats;

#endif // StackStats_h
