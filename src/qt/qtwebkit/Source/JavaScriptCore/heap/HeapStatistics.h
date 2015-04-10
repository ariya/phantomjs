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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HeapStatistics_h
#define HeapStatistics_h

#include "JSExportMacros.h"
#include <wtf/Deque.h>

namespace JSC {

class Heap;

class HeapStatistics {
public:
    NO_RETURN static void exitWithFailure();
    JS_EXPORT_PRIVATE static void reportSuccess();

    static void initialize();
    static void recordGCPauseTime(double start, double end);
    static size_t parseMemoryAmount(char*);

    static void showObjectStatistics(Heap*);

    static const size_t KB = 1024;
    static const size_t MB = 1024 * KB;
    static const size_t GB = 1024 * MB;

private:
    static void logStatistics();
    static Vector<double>* s_pauseTimeStarts;
    static Vector<double>* s_pauseTimeEnds;
    static double s_startTime;
    static double s_endTime;
};

} // namespace JSC

#endif
