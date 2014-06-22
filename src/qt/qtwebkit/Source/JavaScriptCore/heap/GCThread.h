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

#ifndef GCThread_h
#define GCThread_h

#include <GCThreadSharedData.h>
#include <wtf/Deque.h>
#include <wtf/OwnPtr.h>
#include <wtf/Threading.h>

namespace JSC {

class CopyVisitor;
class GCThreadSharedData;
class SlotVisitor;

class GCThread {
public:
    GCThread(GCThreadSharedData&, SlotVisitor*, CopyVisitor*);

    SlotVisitor* slotVisitor();
    CopyVisitor* copyVisitor();
    ThreadIdentifier threadID();
    void initializeThreadID(ThreadIdentifier);

    static void gcThreadStartFunc(void*);

private:
    void gcThreadMain();
    GCPhase waitForNextPhase();

    ThreadIdentifier m_threadID;
    GCThreadSharedData& m_shared;
    OwnPtr<SlotVisitor> m_slotVisitor;
    OwnPtr<CopyVisitor> m_copyVisitor;
};

} // namespace JSC

#endif
