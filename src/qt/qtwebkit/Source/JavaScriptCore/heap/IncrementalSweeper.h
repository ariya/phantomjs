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

#ifndef IncrementalSweeper_h
#define IncrementalSweeper_h

#include "HeapTimer.h"
#include "MarkedBlock.h"
#include <wtf/HashSet.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>

namespace JSC {

class Heap;

class IncrementalSweeper : public HeapTimer {
public:
    static PassOwnPtr<IncrementalSweeper> create(Heap*);
    void startSweeping(Vector<MarkedBlock*>&);
    virtual void doWork();
    void sweepNextBlock();
    void willFinishSweeping();

private:
#if USE(CF) || PLATFORM(BLACKBERRY) || PLATFORM(QT)
#if USE(CF)
    IncrementalSweeper(Heap*, CFRunLoopRef);
#else
    IncrementalSweeper(Heap*);
#endif
    
    void doSweep(double startTime);
    void scheduleTimer();
    void cancelTimer();
    
    unsigned m_currentBlockToSweepIndex;
    Vector<MarkedBlock*>& m_blocksToSweep;
#else
    
    IncrementalSweeper(VM*);
    
#endif
};

} // namespace JSC

#endif
