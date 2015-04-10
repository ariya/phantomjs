/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef ExitKind_h
#define ExitKind_h

namespace JSC {

enum ExitKind {
    ExitKindUnset,
    BadType, // We exited because a type prediction was wrong.
    BadFunction, // We exited because we made an incorrect assumption about what function we would see.
    BadExecutable, // We exited because we made an incorrect assumption about what executable we would see.
    BadCache, // We exited because an inline cache was wrong.
    BadWeakConstantCache, // We exited because a cache on a weak constant (usually a prototype) was wrong.
    BadIndexingType, // We exited because an indexing type was wrong.
    Overflow, // We exited because of overflow.
    NegativeZero, // We exited because we encountered negative zero.
    StoreToHole, // We had a store to a hole.
    LoadFromHole, // We had a load from a hole.
    OutOfBounds, // We had an out-of-bounds access to an array.
    StoreToHoleOrOutOfBounds, // We're simultaneously speculating that we're in bounds and not accessing a hole, and one of those things didn't pan out.
    InadequateCoverage, // We exited because we ended up in code that didn't have profiling coverage.
    ArgumentsEscaped, // We exited because arguments escaped but we didn't expect them to.
    NotStringObject, // We exited because we shouldn't have attempted to optimize string object access.
    Uncountable, // We exited for none of the above reasons, and we should not count it. Most uses of this should be viewed as a FIXME.
    UncountableWatchpoint, // We exited because of a watchpoint, which isn't counted because watchpoints do tracking themselves.
    WatchdogTimerFired // We exited because we need to service the watchdog timer.
};

const char* exitKindToString(ExitKind);
bool exitKindIsCountable(ExitKind);

} // namespace JSC

namespace WTF {

class PrintStream;
void printInternal(PrintStream&, JSC::ExitKind);

} // namespace WTF

#endif // ExitKind_h

