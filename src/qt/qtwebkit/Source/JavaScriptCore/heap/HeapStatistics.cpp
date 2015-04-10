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

#include "config.h"
#include "HeapStatistics.h"

#include "Heap.h"
#include "JSObject.h"
#include "Operations.h"
#include "Options.h"
#include <stdlib.h>
#if OS(UNIX)
#include <sys/resource.h>
#endif
#include <wtf/CurrentTime.h>
#include <wtf/DataLog.h>
#include <wtf/Deque.h>

namespace JSC {

double HeapStatistics::s_startTime = 0.0;
double HeapStatistics::s_endTime = 0.0;
Vector<double>* HeapStatistics::s_pauseTimeStarts = 0;
Vector<double>* HeapStatistics::s_pauseTimeEnds = 0;

#if OS(UNIX) 

void HeapStatistics::initialize()
{
    ASSERT(Options::recordGCPauseTimes());
    s_startTime = WTF::monotonicallyIncreasingTime();
    s_pauseTimeStarts = new Vector<double>();
    s_pauseTimeEnds = new Vector<double>();
}

void HeapStatistics::recordGCPauseTime(double start, double end)
{
    ASSERT(Options::recordGCPauseTimes());
    ASSERT(s_pauseTimeStarts);
    ASSERT(s_pauseTimeEnds);
    s_pauseTimeStarts->append(start);
    s_pauseTimeEnds->append(end);
}

void HeapStatistics::logStatistics()
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
#if USE(CF) || OS(UNIX)
    char* vmName = getenv("JSVMName");
    char* suiteName = getenv("JSSuiteName");
    char* benchmarkName = getenv("JSBenchmarkName");
#else
#error "The HeapStatistics module is not supported on this platform."
#endif
    if (!vmName || !suiteName || !benchmarkName)
        dataLogF("HeapStatistics: {\"max_rss\": %ld", usage.ru_maxrss);
    else
        dataLogF("HeapStatistics: {\"max_rss\": %ld, \"vm_name\": \"%s\", \"suite_name\": \"%s\", \"benchmark_name\": \"%s\"", 
            usage.ru_maxrss, vmName, suiteName, benchmarkName); 

    if (Options::recordGCPauseTimes()) {
        dataLogF(", \"pause_times\": [");
        Vector<double>::iterator startIt = s_pauseTimeStarts->begin();
        Vector<double>::iterator endIt = s_pauseTimeEnds->begin();
        if (startIt != s_pauseTimeStarts->end() && endIt != s_pauseTimeEnds->end()) {
            dataLogF("[%f, %f]", *startIt, *endIt);
            ++startIt;
            ++endIt;
        }
        while (startIt != s_pauseTimeStarts->end() && endIt != s_pauseTimeEnds->end()) {
            dataLogF(", [%f, %f]", *startIt, *endIt);
            ++startIt;
            ++endIt;
        }
        dataLogF("], \"start_time\": %f, \"end_time\": %f", s_startTime, s_endTime);
    }
    dataLogF("}\n");
}

void HeapStatistics::exitWithFailure()
{
    ASSERT(Options::logHeapStatisticsAtExit());
    s_endTime = WTF::monotonicallyIncreasingTime();
    logStatistics();
    exit(-1);
}

void HeapStatistics::reportSuccess()
{
    ASSERT(Options::logHeapStatisticsAtExit());
    s_endTime = WTF::monotonicallyIncreasingTime();
    logStatistics();
}

#else

void HeapStatistics::initialize()
{
}

void HeapStatistics::recordGCPauseTime(double, double)
{
}

void HeapStatistics::logStatistics()
{
}

void HeapStatistics::exitWithFailure()
{
}

void HeapStatistics::reportSuccess()
{
}

#endif // OS(UNIX)

size_t HeapStatistics::parseMemoryAmount(char* s)
{
    size_t multiplier = 1;
    char* afterS;
    size_t value = strtol(s, &afterS, 10);
    char next = afterS[0];
    switch (next) {
    case 'K':
        multiplier = KB;
        break;
    case 'M':
        multiplier = MB;
        break;
    case 'G':
        multiplier = GB;
        break;
    default:
        break;
    }
    return value * multiplier;
}

class StorageStatistics : public MarkedBlock::VoidFunctor {
public:
    StorageStatistics();

    void operator()(JSCell*);

    size_t objectWithOutOfLineStorageCount();
    size_t objectCount();

    size_t storageSize();
    size_t storageCapacity();

private:
    size_t m_objectWithOutOfLineStorageCount;
    size_t m_objectCount;
    size_t m_storageSize;
    size_t m_storageCapacity;
};

inline StorageStatistics::StorageStatistics()
    : m_objectWithOutOfLineStorageCount(0)
    , m_objectCount(0)
    , m_storageSize(0)
    , m_storageCapacity(0)
{
}

inline void StorageStatistics::operator()(JSCell* cell)
{
    if (!cell->isObject())
        return;

    JSObject* object = jsCast<JSObject*>(cell);
    if (hasIndexedProperties(object->structure()->indexingType()))
        return;

    if (object->structure()->isUncacheableDictionary())
        return;

    ++m_objectCount;
    if (!object->hasInlineStorage())
        ++m_objectWithOutOfLineStorageCount;
    m_storageSize += object->structure()->totalStorageSize() * sizeof(WriteBarrierBase<Unknown>);
    m_storageCapacity += object->structure()->totalStorageCapacity() * sizeof(WriteBarrierBase<Unknown>); 
}

inline size_t StorageStatistics::objectWithOutOfLineStorageCount()
{
    return m_objectWithOutOfLineStorageCount;
}

inline size_t StorageStatistics::objectCount()
{
    return m_objectCount;
}

inline size_t StorageStatistics::storageSize()
{
    return m_storageSize;
}

inline size_t StorageStatistics::storageCapacity()
{
    return m_storageCapacity;
}

void HeapStatistics::showObjectStatistics(Heap* heap)
{
    dataLogF("\n=== Heap Statistics: ===\n");
    dataLogF("size: %ldkB\n", static_cast<long>(heap->m_sizeAfterLastCollect / KB));
    dataLogF("capacity: %ldkB\n", static_cast<long>(heap->capacity() / KB));
    dataLogF("pause time: %lfms\n\n", heap->m_lastGCLength);

    StorageStatistics storageStatistics;
    heap->m_objectSpace.forEachLiveCell(storageStatistics);
    dataLogF("wasted .property storage: %ldkB (%ld%%)\n",
        static_cast<long>(
            (storageStatistics.storageCapacity() - storageStatistics.storageSize()) / KB),
        static_cast<long>(
            (storageStatistics.storageCapacity() - storageStatistics.storageSize()) * 100
                / storageStatistics.storageCapacity()));
    dataLogF("objects with out-of-line .property storage: %ld (%ld%%)\n",
        static_cast<long>(
            storageStatistics.objectWithOutOfLineStorageCount()),
        static_cast<long>(
            storageStatistics.objectWithOutOfLineStorageCount() * 100
                / storageStatistics.objectCount()));
}

} // namespace JSC
