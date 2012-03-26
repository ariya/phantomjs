/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron Zwarich <cwzwarich@uwaterloo.ca>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TimeoutChecker.h"

#include "CallFrame.h"
#include "JSGlobalObject.h"

#if OS(DARWIN)
#include <mach/mach.h>
#elif OS(WINDOWS)
#include <windows.h>
#else
#include "CurrentTime.h"
#endif

#if PLATFORM(BREWMP)
#include <AEEStdLib.h>
#endif

using namespace std;

namespace JSC {

// Number of ticks before the first timeout check is done.
static const int ticksUntilFirstCheck = 1024;

// Number of milliseconds between each timeout check.
static const int intervalBetweenChecks = 1000;

// Returns the time the current thread has spent executing, in milliseconds.
static inline unsigned getCPUTime()
{
#if OS(DARWIN)
    mach_msg_type_number_t infoCount = THREAD_BASIC_INFO_COUNT;
    thread_basic_info_data_t info;

    // Get thread information
    mach_port_t threadPort = mach_thread_self();
    thread_info(threadPort, THREAD_BASIC_INFO, reinterpret_cast<thread_info_t>(&info), &infoCount);
    mach_port_deallocate(mach_task_self(), threadPort);
    
    unsigned time = info.user_time.seconds * 1000 + info.user_time.microseconds / 1000;
    time += info.system_time.seconds * 1000 + info.system_time.microseconds / 1000;
    
    return time;
#elif OS(WINDOWS)
    union {
        FILETIME fileTime;
        unsigned long long fileTimeAsLong;
    } userTime, kernelTime;
    
    // GetThreadTimes won't accept NULL arguments so we pass these even though
    // they're not used.
    FILETIME creationTime, exitTime;
    
    GetThreadTimes(GetCurrentThread(), &creationTime, &exitTime, &kernelTime.fileTime, &userTime.fileTime);
    
    return userTime.fileTimeAsLong / 10000 + kernelTime.fileTimeAsLong / 10000;
#elif OS(SYMBIAN)
    RThread current;
    TTimeIntervalMicroSeconds cpuTime;

    TInt err = current.GetCpuTime(cpuTime);
    ASSERT_WITH_MESSAGE(err == KErrNone, "GetCpuTime failed with %d", err);
    return cpuTime.Int64() / 1000;
#elif PLATFORM(BREWMP)
    // This function returns a continuously and linearly increasing millisecond
    // timer from the time the device was powered on.
    // There is only one thread in BREW, so this is enough.
    return GETUPTIMEMS();
#else
    // FIXME: We should return the time the current thread has spent executing.

    // use a relative time from first call in order to avoid an overflow
    static double firstTime = currentTime();
    return static_cast<unsigned> ((currentTime() - firstTime) * 1000);
#endif
}

TimeoutChecker::TimeoutChecker()
    : m_timeoutInterval(0)
    , m_startCount(0)
{
    reset();
}

void TimeoutChecker::reset()
{
    m_ticksUntilNextCheck = ticksUntilFirstCheck;
    m_timeAtLastCheck = 0;
    m_timeExecuting = 0;
}

bool TimeoutChecker::didTimeOut(ExecState* exec)
{
    unsigned currentTime = getCPUTime();
    
    if (!m_timeAtLastCheck) {
        // Suspicious amount of looping in a script -- start timing it
        m_timeAtLastCheck = currentTime;
        return false;
    }
    
    unsigned timeDiff = currentTime - m_timeAtLastCheck;
    
    if (timeDiff == 0)
        timeDiff = 1;
    
    m_timeExecuting += timeDiff;
    m_timeAtLastCheck = currentTime;
    
    // Adjust the tick threshold so we get the next checkTimeout call in the
    // interval specified in intervalBetweenChecks.
    m_ticksUntilNextCheck = static_cast<unsigned>((static_cast<float>(intervalBetweenChecks) / timeDiff) * m_ticksUntilNextCheck);
    // If the new threshold is 0 reset it to the default threshold. This can happen if the timeDiff is higher than the
    // preferred script check time interval.
    if (m_ticksUntilNextCheck == 0)
        m_ticksUntilNextCheck = ticksUntilFirstCheck;
    
    if (m_timeoutInterval && m_timeExecuting > m_timeoutInterval) {
        if (exec->dynamicGlobalObject()->shouldInterruptScript())
            return true;
        
        reset();
    }
    
    return false;
}

} // namespace JSC
