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
 *  The MemorySampler class samples a number of internal and external memory 
 *  metrics every second while running. Sample data is written to a log file. 
 *  Sampling occurs over a duration specified when started. If duration is set 
 *  to 0 (default), the memory sampler will run indefinitely until the stop 
 *  function is called. MemorySampler allows the option of sampling "in use" 
 *  memory (committed memory minus free list byte count) or committed memory for
 *  any allocator which keeps a free list. This includes FastMalloc and the 
 *  JavaScriptCore heap at this time.
 *  The following memory metrics are recorded:
 *
 *      WebCore
 *          - FastMalloc allocations bytes              (in use or committed)
 *      JavaScriptCore
 *          - Garbage collector heap bytes              (in use or committed)
 *          - Stack bytes                               (committed only!)
 *          - JIT Code bytes                            (committed only!)
 *      Malloc zones
 *          - In use bytes for the following zones:
 *              * Default zone                          (in use or committed)
 *              * DispCon zone                          (in use or committed)
 *              * Purgable zone                         (in use or committed)
 *      Task Info
 *          - Resident size memory (RSIZE)
 */

#ifndef WebMemorySampler_h
#define WebMemorySampler_h

#if ENABLE(MEMORY_SAMPLER)

#include "SandboxExtension.h"
#include <WebCore/FileSystem.h>
#include <WebCore/Timer.h>
#include <wtf/Noncopyable.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

struct SystemMallocStats;

struct WebMemoryStatistics {
    Vector<String> keys;
    Vector<size_t> values;
};
    
class WebMemorySampler {
    WTF_MAKE_NONCOPYABLE(WebMemorySampler);
public:
    static WebMemorySampler* shared();
    void start(const double interval = 0);
    void start(const SandboxExtension::Handle&, const String&, const double interval = 0);
    void stop();
    bool isRunning() const;
    
private:
    WebMemorySampler();
    ~WebMemorySampler();
    
    void initializeTempLogFile();
    void initializeSandboxedLogFile(const SandboxExtension::Handle&, const String&);
    void writeHeaders();
    void initializeTimers(double);
    void sampleTimerFired(WebCore::Timer<WebMemorySampler>*);
    void stopTimerFired(WebCore::Timer<WebMemorySampler>*);
    void appendCurrentMemoryUsageToFile(WebCore::PlatformFileHandle&);
    void sendMemoryPressureEvent();
    
    SystemMallocStats sampleSystemMalloc() const;
    size_t sampleProcessCommittedBytes() const;
    WebMemoryStatistics sampleWebKit() const;
    String processName() const;
    
    WebCore::PlatformFileHandle m_sampleLogFile;
    String m_sampleLogFilePath;
    WebCore::Timer<WebMemorySampler> m_sampleTimer;
    WebCore::Timer<WebMemorySampler> m_stopTimer;
    bool m_isRunning;
    double m_runningTime;
    RefPtr<SandboxExtension> m_sampleLogSandboxExtension;
};

}

#endif

#endif
