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
#include "DisplaySleepDisabler.h"

#include <IOKit/pwr_mgt/IOPMLib.h>
#include <wtf/RetainPtr.h>

namespace WebCore {

static const double systemActivityInterval = 1;

DisplaySleepDisabler::DisplaySleepDisabler(const char* reason)
    : m_disableDisplaySleepAssertion(0)
{
#if !PLATFORM(IOS)
    RetainPtr<CFStringRef> reasonCF = adoptCF(CFStringCreateWithCString(kCFAllocatorDefault, reason, kCFStringEncodingUTF8));
    IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn, reasonCF.get(), &m_disableDisplaySleepAssertion);
#else
    UNUSED_PARAM(reason);
    IOPMAssertionCreate(kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn, &m_disableDisplaySleepAssertion);
    m_systemActivityTimer.startRepeating(systemActivityInterval);
#endif
}

DisplaySleepDisabler::~DisplaySleepDisabler()
{
    IOPMAssertionRelease(m_disableDisplaySleepAssertion);
}

}
