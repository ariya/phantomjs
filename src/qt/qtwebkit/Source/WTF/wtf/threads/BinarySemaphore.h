/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef BinarySemaphore_h
#define BinarySemaphore_h

#include <wtf/Noncopyable.h>
#include <wtf/ThreadingPrimitives.h>

namespace WTF {

class BinarySemaphore {
    WTF_MAKE_NONCOPYABLE(BinarySemaphore);

public:
    WTF_EXPORT_PRIVATE BinarySemaphore();
    WTF_EXPORT_PRIVATE ~BinarySemaphore();

    WTF_EXPORT_PRIVATE void signal();
    WTF_EXPORT_PRIVATE bool wait(double absoluteTime);

#if OS(WINDOWS)
    HANDLE event() const { return m_event; }
#endif

private:
#if OS(WINDOWS)
    HANDLE m_event;
#else
    bool m_isSet;

    Mutex m_mutex;
    ThreadCondition m_condition;
#endif
};

} // namespace WTF

using WTF::BinarySemaphore;

#endif // BinarySemaphore_h
