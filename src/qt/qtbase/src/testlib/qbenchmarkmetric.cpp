/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/private/qbenchmarkmetric_p.h>

/*!
  \enum QTest::QBenchmarkMetric
  \since 4.7

  This enum lists all the things that can be benchmarked.

  \value FramesPerSecond        Frames per second
  \value BitsPerSecond          Bits per second
  \value BytesPerSecond         Bytes per second
  \value WalltimeMilliseconds   Clock time in milliseconds
  \value WalltimeNanoseconds    Clock time in nanoseconds
  \value BytesAllocated         Memory usage in bytes
  \value Events                 Event count
  \value CPUTicks               CPU time
  \value CPUMigrations          Process migrations between CPUs
  \value CPUCycles              CPU cycles
  \value BusCycles              Bus cycles
  \value StalledCycles          Cycles stalled
  \value InstructionReads       Instruction reads
  \value Instructions           Instructions executed
  \value BranchInstructions     Branch-type instructions
  \value BranchMisses           Branch instructions that were mispredicted
  \value CacheReferences        Cache accesses of any type
  \value CacheMisses            Cache misses of any type
  \value CacheReads             Cache reads / loads
  \value CacheReadMisses        Cache read / load misses
  \value CacheWrites            Cache writes / stores
  \value CacheWriteMisses       Cache write / store misses
  \value CachePrefetches        Cache prefetches
  \value CachePrefetchMisses    Cache prefetch misses
  \value ContextSwitches        Context switches
  \value PageFaults             Page faults of any type
  \value MinorPageFaults        Minor page faults
  \value MajorPageFaults        Major page faults
  \value AlignmentFaults        Faults caused due to misalignment
  \value EmulationFaults        Faults that needed software emulation

  \sa QTest::benchmarkMetricName(), QTest::benchmarkMetricUnit()

  Note that \c WalltimeNanoseconds and \c BytesAllocated are
  only provided for use via \l setBenchmarkResult(), and results
  in those metrics are not able to be provided automatically
  by the QTest framework.
 */

/*!
  \relates QTest
  \since 4.7
  Returns the enum value \a metric as a character string.
 */
const char * QTest::benchmarkMetricName(QBenchmarkMetric metric)
{
    switch (metric) {
    case FramesPerSecond:
        return "FramesPerSecond";
    case BitsPerSecond:
        return "BitsPerSecond";
    case BytesPerSecond:
        return "BytesPerSecond";
    case WalltimeMilliseconds:
        return "WalltimeMilliseconds";
    case Events:
        return "Events";
    case CPUTicks:
        return "CPUTicks";
    case CPUMigrations:
        return "CPUMigrations";
    case CPUCycles:
        return "CPUCycles";
    case BusCycles:
        return "BusCycles";
    case StalledCycles:
        return "StalledCycles";
    case InstructionReads:
        return "InstructionReads";
    case Instructions:
        return "Instructions";
    case WalltimeNanoseconds:
        return "WalltimeNanoseconds";
    case BytesAllocated:
        return "BytesAllocated";
    case BranchInstructions:
        return "BranchInstructions";
    case BranchMisses:
        return "BranchMisses";
    case CacheReferences:
        return "CacheReferences";
    case CacheReads:
        return "CacheReads";
    case CacheWrites:
        return "CacheWrites";
    case CachePrefetches:
        return "CachePrefetches";
    case CacheMisses:
        return "CacheMisses";
    case CacheReadMisses:
        return "CacheReadMisses";
    case CacheWriteMisses:
        return "CacheWriteMisses";
    case CachePrefetchMisses:
        return "CachePrefetchMisses";
    case ContextSwitches:
        return "ContextSwitches";
    case PageFaults:
        return "PageFaults";
    case MinorPageFaults:
        return "MinorPageFaults";
    case MajorPageFaults:
        return "MajorPageFaults";
    case AlignmentFaults:
        return "AlignmentFaults";
    case EmulationFaults:
        return "EmulationFaults";
    default:
        return "";
    }
};

/*!
  \relates QTest
  \since 4.7
  Retuns the units of measure for the specified \a metric.
 */
const char * QTest::benchmarkMetricUnit(QBenchmarkMetric metric)
{
    switch (metric) {
    case FramesPerSecond:
        return "fps";
    case BitsPerSecond:
        return "bits/s";
    case BytesPerSecond:
        return "bytes/s";
    case WalltimeMilliseconds:
        return "msecs";
    case Events:
        return "events";
    case CPUTicks:
        return "CPU ticks";
    case CPUMigrations:
        return "CPU migrations";
    case CPUCycles:
        return "CPU cycles";
    case BusCycles:
        return "bus cycles";
    case StalledCycles:
        return "stalled cycles";
    case InstructionReads:
        return "instruction reads";
    case Instructions:
        return "instructions";
    case WalltimeNanoseconds:
        return "nsecs";
    case BytesAllocated:
        return "bytes";
    case BranchInstructions:
        return "branch instructions";
    case BranchMisses:
        return "branch misses";
    case CacheReferences:
        return "cache references";
    case CacheReads:
        return "cache loads";
    case CacheWrites:
        return "cache stores";
    case CachePrefetches:
        return "cache prefetches";
    case CacheMisses:
        return "cache misses";
    case CacheReadMisses:
        return "cache load misses";
    case CacheWriteMisses:
        return "cache store misses";
    case CachePrefetchMisses:
        return "cache prefetch misses";
    case ContextSwitches:
        return "context switches";
    case PageFaults:
        return "page faults";
    case MinorPageFaults:
        return "minor page faults";
    case MajorPageFaults:
        return "major page faults";
    case AlignmentFaults:
        return "alignment faults";
    case EmulationFaults:
        return "emulation faults";
    default:
        return "";
    }
}

