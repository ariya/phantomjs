/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#include "config.h"
#include "BinarySemaphore.h"

namespace WTF {

BinarySemaphore::BinarySemaphore()
    : m_event(::CreateEventW(0, FALSE, FALSE, 0))
{
}

BinarySemaphore::~BinarySemaphore()
{
    ::CloseHandle(m_event);
}

void BinarySemaphore::signal()
{
    ::SetEvent(m_event);
}

bool BinarySemaphore::wait(double absoluteTime)
{
    DWORD interval = absoluteTimeToWaitTimeoutInterval(absoluteTime);
    if (!interval) {
        // Consider the wait to have timed out, even if the event has already been signaled, to
        // match the WTF::ThreadCondition implementation.
        return false;
    }

    DWORD result = ::WaitForSingleObject(m_event, interval);
    switch (result) {
    case WAIT_OBJECT_0:
        // The event was signaled.
        return true;

    case WAIT_TIMEOUT:
        // The wait timed out.
        return false;

    case WAIT_FAILED:
        ASSERT_WITH_MESSAGE(false, "::WaitForSingleObject failed with error %lu", ::GetLastError());
        return false;

    default:
        ASSERT_WITH_MESSAGE(false, "::WaitForSingleObject returned unexpected result %lu", result);
        return false;
    }
}

} // namespace WTF
