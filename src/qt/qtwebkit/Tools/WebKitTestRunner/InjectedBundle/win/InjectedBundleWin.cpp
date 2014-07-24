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
#include "InjectedBundle.h"

namespace WTR {

static HANDLE webProcessCrashingEvent;

static LONG WINAPI exceptionFilter(EXCEPTION_POINTERS*)
{
    // Let the UI process know right away that we crashed. It might take a long time for us to
    // finish crashing if a crash log is being saved.
    ::SetEvent(webProcessCrashingEvent);

    return EXCEPTION_CONTINUE_SEARCH;
}

void InjectedBundle::platformInitialize(WKTypeRef initializationUserData)
{
    ::SetUnhandledExceptionFilter(exceptionFilter);

    ASSERT_ARG(initializationUserData, initializationUserData);
    ASSERT_ARG(initializationUserData, WKGetTypeID(initializationUserData) == WKStringGetTypeID());

    WKStringRef string = static_cast<WKStringRef>(initializationUserData);
    Vector<char> buffer(WKStringGetMaximumUTF8CStringSize(string));
    WKStringGetUTF8CString(string, buffer.data(), buffer.size());

    // The UI process should already have created this event. We're just getting another HANDLE to it.
    webProcessCrashingEvent = ::CreateEventA(0, FALSE, FALSE, buffer.data());
    ASSERT(webProcessCrashingEvent);
}

} // namespace WTR
