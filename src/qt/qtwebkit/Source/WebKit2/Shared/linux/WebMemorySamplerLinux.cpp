/*
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Samsung Electronics
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
 */

#include "config.h"
#include "WebMemorySampler.h"

#if ENABLE(MEMORY_SAMPLER)

#include "NotImplemented.h"
#include <JavaScriptCore/MemoryStatistics.h>
#include <WebCore/JSDOMWindow.h>
#include <runtime/JSLock.h>
#include <runtime/Operations.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <wtf/CurrentTime.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;
using namespace JSC;
using namespace WTF;

namespace WebKit {

struct ApplicationMemoryStats {
    size_t totalProgramSize;
    size_t residentSetSize;
    size_t sharedSize;
    size_t textSize;
    size_t librarySize;
    size_t dataStackSize;
    size_t dirtyPageSize;
};

static const unsigned int maxBuffer = 128;
static const unsigned int maxProcessPath = 35;

static inline String nextToken(FILE* file)
{
    ASSERT(file);
    if (!file)
        return String();

    char buffer[maxBuffer] = {0, };
    unsigned int index = 0;
    while (index < maxBuffer) {
        char ch = fgetc(file);
        if (ch == EOF || (isASCIISpace(ch) && index)) // Break on non-initial ASCII space.
            break;
        if (!isASCIISpace(ch)) {
            buffer[index] = ch;
            index++;
        }
    }

    return String(buffer);
}

static inline void appendKeyValuePair(WebMemoryStatistics& stats, const String& key, size_t value)
{
    stats.keys.append(key);
    stats.values.append(value);
}

static ApplicationMemoryStats sampleMemoryAllocatedForApplication()
{
    ApplicationMemoryStats applicationStats;
    char processPath[maxProcessPath];
    snprintf(processPath, maxProcessPath, "/proc/self/statm");
    FILE* statmFileDescriptor = fopen(processPath, "r");
    if (!statmFileDescriptor)
        return applicationStats;

    applicationStats.totalProgramSize = nextToken(statmFileDescriptor).toInt();
    applicationStats.residentSetSize = nextToken(statmFileDescriptor).toInt();
    applicationStats.sharedSize = nextToken(statmFileDescriptor).toInt();
    applicationStats.textSize = nextToken(statmFileDescriptor).toInt();
    applicationStats.librarySize = nextToken(statmFileDescriptor).toInt();
    applicationStats.dataStackSize = nextToken(statmFileDescriptor).toInt();
    applicationStats.dirtyPageSize = nextToken(statmFileDescriptor).toInt();

    fclose(statmFileDescriptor);

    return applicationStats;
}

String WebMemorySampler::processName() const
{
    char processPath[maxProcessPath];
    snprintf(processPath, maxProcessPath, "/proc/self/status");
    FILE* statusFileDescriptor = fopen(processPath, "r");
    if (!statusFileDescriptor)
        return String();
        
    nextToken(statusFileDescriptor);
    String processName = nextToken(statusFileDescriptor);

    fclose(statusFileDescriptor);

    return processName;
}

WebMemoryStatistics WebMemorySampler::sampleWebKit() const
{
    WebMemoryStatistics webKitMemoryStats;

    double now = currentTime();

    appendKeyValuePair(webKitMemoryStats, ASCIILiteral("Timestamp"), now);

    ApplicationMemoryStats applicationStats = sampleMemoryAllocatedForApplication();

    appendKeyValuePair(webKitMemoryStats, ASCIILiteral("Total Program Size"), applicationStats.totalProgramSize);
    appendKeyValuePair(webKitMemoryStats, ASCIILiteral("RSS"), applicationStats.residentSetSize);
    appendKeyValuePair(webKitMemoryStats, ASCIILiteral("Shared"), applicationStats.sharedSize);
    appendKeyValuePair(webKitMemoryStats, ASCIILiteral("Text"), applicationStats.textSize);
    appendKeyValuePair(webKitMemoryStats, ASCIILiteral("Library"), applicationStats.librarySize);
    appendKeyValuePair(webKitMemoryStats, ASCIILiteral("Data/Stack"), applicationStats.dataStackSize);
    appendKeyValuePair(webKitMemoryStats, ASCIILiteral("Dirty"), applicationStats.dirtyPageSize);

    size_t totalBytesInUse = 0;
    size_t totalBytesCommitted = 0;

#if ENABLE(GLOBAL_FASTMALLOC_NEW)
    FastMallocStatistics fastMallocStatistics = WTF::fastMallocStatistics();
    size_t fastMallocBytesInUse = fastMallocStatistics.committedVMBytes - fastMallocStatistics.freeListBytes;
    size_t fastMallocBytesCommitted = fastMallocStatistics.committedVMBytes;
    totalBytesInUse += fastMallocBytesInUse;
    totalBytesCommitted += fastMallocBytesCommitted;

    appendKeyValuePair(webKitMemoryStats, ASCIILiteral("Fast Malloc In Use"), fastMallocBytesInUse);
    appendKeyValuePair(webKitMemoryStats, ASCIILiteral("Fast Malloc Committed Memory"), fastMallocBytesCommitted);
#endif

    size_t jscHeapBytesInUse = JSDOMWindow::commonVM()->heap.size();
    size_t jscHeapBytesCommitted = JSDOMWindow::commonVM()->heap.capacity();
    totalBytesInUse += jscHeapBytesInUse;
    totalBytesCommitted += jscHeapBytesCommitted;

    GlobalMemoryStatistics globalMemoryStats = globalMemoryStatistics();
    totalBytesInUse += globalMemoryStats.stackBytes + globalMemoryStats.JITBytes;
    totalBytesCommitted += globalMemoryStats.stackBytes + globalMemoryStats.JITBytes;

    appendKeyValuePair(webKitMemoryStats, ASCIILiteral("JavaScript Heap In Use"), jscHeapBytesInUse);
    appendKeyValuePair(webKitMemoryStats, ASCIILiteral("JavaScript Heap Commited Memory"), jscHeapBytesCommitted);
    
    appendKeyValuePair(webKitMemoryStats, ASCIILiteral("JavaScript Stack Bytes"), globalMemoryStats.stackBytes);
    appendKeyValuePair(webKitMemoryStats, ASCIILiteral("JavaScript JIT Bytes"), globalMemoryStats.JITBytes);

    appendKeyValuePair(webKitMemoryStats, ASCIILiteral("Total Memory In Use"), totalBytesInUse);
    appendKeyValuePair(webKitMemoryStats, ASCIILiteral("Total Committed Memory"), totalBytesCommitted);

    struct sysinfo systemInfo;
    if (!sysinfo(&systemInfo)) {
        appendKeyValuePair(webKitMemoryStats, ASCIILiteral("System Total Bytes"), systemInfo.totalram);
        appendKeyValuePair(webKitMemoryStats, ASCIILiteral("Available Bytes"), systemInfo.freeram);
        appendKeyValuePair(webKitMemoryStats, ASCIILiteral("Shared Bytes"), systemInfo.sharedram);
        appendKeyValuePair(webKitMemoryStats, ASCIILiteral("Buffer Bytes"), systemInfo.bufferram);
        appendKeyValuePair(webKitMemoryStats, ASCIILiteral("Total Swap Bytes"), systemInfo.totalswap);
        appendKeyValuePair(webKitMemoryStats, ASCIILiteral("Available Swap Bytes"), systemInfo.freeswap);
    }   

    return webKitMemoryStats;
}

void WebMemorySampler::sendMemoryPressureEvent()
{
    notImplemented();
}

}
#endif
