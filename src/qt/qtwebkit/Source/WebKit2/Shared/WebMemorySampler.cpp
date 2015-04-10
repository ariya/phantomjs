/*
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "config.h"
#include "WebMemorySampler.h"

#if ENABLE(MEMORY_SAMPLER)

#include <stdio.h>
#include <unistd.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

using namespace WebCore;

namespace WebKit {

static const char separator = '\t';

WebMemorySampler* WebMemorySampler::shared()
{
    static WebMemorySampler* sharedMemorySampler;
    if (!sharedMemorySampler)
        sharedMemorySampler = new WebMemorySampler();
    return sharedMemorySampler;
}

WebMemorySampler::WebMemorySampler() 
    : m_sampleTimer(this, &WebMemorySampler::sampleTimerFired)
    , m_stopTimer(this, &WebMemorySampler::stopTimerFired)
    , m_isRunning(false)
    , m_runningTime(0)
{
}

void WebMemorySampler::start(const double interval) 
{
    if (m_isRunning) 
        return;
    
    initializeTempLogFile();
    initializeTimers(interval);
}

void WebMemorySampler::start(const SandboxExtension::Handle& sampleLogFileHandle, const String& sampleLogFilePath, const double interval) 
{
    if (m_isRunning) 
        return;
    
    // If we are on a system without SandboxExtension the handle and filename will be empty
    if (sampleLogFilePath.isEmpty()) {
        start(interval);
        return;
    }
        
    initializeSandboxedLogFile(sampleLogFileHandle, sampleLogFilePath);
    initializeTimers(interval);
   
}

void WebMemorySampler::initializeTimers(double interval)
{
    m_sampleTimer.startRepeating(1);
    printf("Started memory sampler for process %s %d", processName().utf8().data(), getpid());
    if (interval > 0) {
        m_stopTimer.startOneShot(interval);
        printf(" for a interval of %g seconds", interval);
    }
    printf("; Sampler log file stored at: %s\n", m_sampleLogFilePath.utf8().data());
    m_runningTime = interval;
    m_isRunning = true;
}

void WebMemorySampler::stop() 
{
    if (!m_isRunning) 
        return;
    m_sampleTimer.stop();
    m_sampleLogFile = 0;
    printf("Stopped memory sampler for process %s %d\n", processName().utf8().data(), getpid());
    // Flush stdout buffer so python script can be guaranteed to read up to this point.
    fflush(stdout);
    m_isRunning = false;
    
    if (m_stopTimer.isActive())
        m_stopTimer.stop();
    
    if (m_sampleLogSandboxExtension) {
        m_sampleLogSandboxExtension->revoke();
        m_sampleLogSandboxExtension = nullptr;
    }    
}

bool WebMemorySampler::isRunning() const
{
    return m_isRunning;
}
    
void WebMemorySampler::initializeTempLogFile()
{
    m_sampleLogFilePath = openTemporaryFile(processName(), m_sampleLogFile);
    writeHeaders();
}

void WebMemorySampler::initializeSandboxedLogFile(const SandboxExtension::Handle& sampleLogSandboxHandle, const String& sampleLogFilePath)
{
    m_sampleLogSandboxExtension = SandboxExtension::create(sampleLogSandboxHandle);
    if (m_sampleLogSandboxExtension)
        m_sampleLogSandboxExtension->consume();
    m_sampleLogFilePath = sampleLogFilePath;
    m_sampleLogFile = openFile(m_sampleLogFilePath, OpenForWrite);
    writeHeaders();
}

void WebMemorySampler::writeHeaders()
{
    String processDetails = String::format("Process: %s Pid: %d\n", processName().utf8().data(), getpid());

    CString utf8String = processDetails.utf8();
    writeToFile(m_sampleLogFile, utf8String.data(), utf8String.length());
    
    StringBuilder header; 
    WebMemoryStatistics stats = sampleWebKit();
    if (!stats.keys.isEmpty()) {
        for (size_t i = 0; i < stats.keys.size(); ++i) {
            header.append(separator);
            header.append(stats.keys[i]);
        }
    }
    header.append('\n');
    utf8String = header.toString().utf8();
    writeToFile(m_sampleLogFile, utf8String.data(), utf8String.length());
}

void WebMemorySampler::sampleTimerFired(Timer<WebMemorySampler>*)
{
    sendMemoryPressureEvent();
    appendCurrentMemoryUsageToFile(m_sampleLogFile);
}

void WebMemorySampler::stopTimerFired(Timer<WebMemorySampler>*)
{
    if (!m_isRunning)
        return;
    printf("%g seconds elapsed. Stopping memory sampler...\n", m_runningTime);
    stop();
}

void WebMemorySampler::appendCurrentMemoryUsageToFile(PlatformFileHandle&)
{
    // Collect statistics from allocators and get RSIZE metric
    StringBuilder statString;
    WebMemoryStatistics memoryStats = sampleWebKit();

    if (!memoryStats.values.isEmpty()) {
        statString.append(separator);
        for (size_t i = 0; i < memoryStats.values.size(); ++i) {
            statString.append(separator);
            statString.appendNumber(memoryStats.values[i]);
        }
    }
    statString.append('\n');

    CString utf8String = statString.toString().utf8();
    writeToFile(m_sampleLogFile, utf8String.data(), utf8String.length());
}

}

#endif
