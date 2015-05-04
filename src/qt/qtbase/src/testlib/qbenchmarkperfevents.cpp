/****************************************************************************
**
** Copyright (C) 2013 Intel Corporation.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qbenchmarkperfevents_p.h"
#include "qbenchmarkmetric.h"
#include "qbenchmark_p.h"

#ifdef QTESTLIB_USE_PERF_EVENTS

// include the qcore_unix_p.h without core-private
// we only use inline functions anyway
#include "../corelib/kernel/qcore_unix_p.h"

#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include <sys/syscall.h>
#include <sys/ioctl.h>

#include "3rdparty/linux_perf_event_p.h"

// for PERF_TYPE_HW_CACHE, the config is a bitmask
// lowest 8 bits: cache type
// bits 8 to 15: cache operation
// bits 16 to 23: cache result
#define CACHE_L1D_READ              (PERF_COUNT_HW_CACHE_L1D | PERF_COUNT_HW_CACHE_OP_READ << 8 | PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)
#define CACHE_L1D_WRITE             (PERF_COUNT_HW_CACHE_L1D | PERF_COUNT_HW_CACHE_OP_WRITE << 8 | PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)
#define CACHE_L1D_PREFETCH          (PERF_COUNT_HW_CACHE_L1D | PERF_COUNT_HW_CACHE_OP_PREFETCH << 8 | PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)
#define CACHE_L1I_READ              (PERF_COUNT_HW_CACHE_L1I | PERF_COUNT_HW_CACHE_OP_READ << 8 | PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)
#define CACHE_L1I_PREFETCH          (PERF_COUNT_HW_CACHE_L1I | PERF_COUNT_HW_CACHE_OP_PREFETCH << 8 | PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)
#define CACHE_LLC_READ              (PERF_COUNT_HW_CACHE_LL  | PERF_COUNT_HW_CACHE_OP_READ << 8 | PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)
#define CACHE_LLC_WRITE             (PERF_COUNT_HW_CACHE_LL  | PERF_COUNT_HW_CACHE_OP_WRITE << 8| PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)
#define CACHE_LLC_PREFETCH          (PERF_COUNT_HW_CACHE_LL  | PERF_COUNT_HW_CACHE_OP_PREFETCH << 8 | PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)
#define CACHE_L1D_READ_MISS         (PERF_COUNT_HW_CACHE_L1D | PERF_COUNT_HW_CACHE_OP_READ << 8 | PERF_COUNT_HW_CACHE_RESULT_MISS << 16)
#define CACHE_L1D_WRITE_MISS        (PERF_COUNT_HW_CACHE_L1D | PERF_COUNT_HW_CACHE_OP_WRITE << 8 | PERF_COUNT_HW_CACHE_RESULT_MISS << 16)
#define CACHE_L1D_PREFETCH_MISS     (PERF_COUNT_HW_CACHE_L1D | PERF_COUNT_HW_CACHE_OP_PREFETCH << 8 | PERF_COUNT_HW_CACHE_RESULT_MISS << 16)
#define CACHE_L1I_READ_MISS         (PERF_COUNT_HW_CACHE_L1I | PERF_COUNT_HW_CACHE_OP_READ << 8 | PERF_COUNT_HW_CACHE_RESULT_MISS << 16)
#define CACHE_L1I_PREFETCH_MISS     (PERF_COUNT_HW_CACHE_L1I | PERF_COUNT_HW_CACHE_OP_PREFETCH << 8 | PERF_COUNT_HW_CACHE_RESULT_MISS << 16)
#define CACHE_LLC_READ_MISS         (PERF_COUNT_HW_CACHE_LL  | PERF_COUNT_HW_CACHE_OP_READ << 8 | PERF_COUNT_HW_CACHE_RESULT_MISS << 16)
#define CACHE_LLC_WRITE_MISS        (PERF_COUNT_HW_CACHE_LL  | PERF_COUNT_HW_CACHE_OP_WRITE << 8 | PERF_COUNT_HW_CACHE_RESULT_MISS << 16)
#define CACHE_LLC_PREFETCH_MISS     (PERF_COUNT_HW_CACHE_LL  | PERF_COUNT_HW_CACHE_OP_PREFETCH << 8 | PERF_COUNT_HW_CACHE_RESULT_MISS << 16)
#define CACHE_BRANCH_READ           (PERF_COUNT_HW_CACHE_BPU | PERF_COUNT_HW_CACHE_OP_READ << 8 | PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)
#define CACHE_BRANCH_READ_MISS      (PERF_COUNT_HW_CACHE_BPU | PERF_COUNT_HW_CACHE_OP_READ << 8 | PERF_COUNT_HW_CACHE_RESULT_MISS << 16)

QT_BEGIN_NAMESPACE

static perf_event_attr attr;

static void initPerf()
{
    static bool done;
    if (!done) {
        memset(&attr, 0, sizeof attr);
        attr.size = sizeof attr;
        attr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
        attr.disabled = true; // we'll enable later
        attr.inherit = true; // let children processes inherit the monitoring
        attr.pinned = true; // keep it running in the hardware
        attr.inherit_stat = true; // aggregate all the info from child processes
        attr.task = true; // trace fork/exits

        // set a default performance counter: CPU cycles
        attr.type = PERF_TYPE_HARDWARE;
        attr.config = PERF_COUNT_HW_CPU_CYCLES; // default

        done = true;
    }
}

/*!
    \class QBenchmarkPerfEvents
    \brief The Linux perf events benchmark backend

    This benchmark backend uses the Linux Performance Counters interface,
    introduced with the Linux kernel v2.6.31. The interface is done by one
    system call (perf_event_open) which takes an attribute structure and
    returns a file descriptor.

    More information:
     \li design docs: tools/perf/design.txt <http://lxr.linux.no/linux/tools/perf/design.txt>
     \li sample tool: tools/perf/builtin-stat.c <http://lxr.linux.no/linux/tools/perf/builtin-stat.c>
    (note: as of v3.3.1, the documentation is out-of-date with the kernel
    interface, so reading the source code of existing tools is necessary)

    This benchlib backend monitors the current process as well as child process
    launched. We do not try to benchmark in kernel or hypervisor mode, as that
    usually requires elevated privileges.
 */

static int perf_event_open(perf_event_attr *attr, pid_t pid, int cpu, int group_fd, unsigned long flags)
{
#ifdef SYS_perf_event_open
    return syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags);
#else
    Q_UNUSED(attr);
    Q_UNUSED(pid);
    Q_UNUSED(cpu);
    Q_UNUSED(group_fd);
    Q_UNUSED(flags);
    errno = ENOSYS;
    return -1;
#endif
}

bool QBenchmarkPerfEventsMeasurer::isAvailable()
{
    // this generates an EFAULT because attr == NULL if perf_event_open is available
    // if the kernel is too old, it generates ENOSYS
    return perf_event_open(0, 0, 0, 0, 0) == -1 && errno != ENOSYS;
}

/* Event list structure
   The following table provides the list of supported events

   Event type   Event counter           Unit            Name and aliases
   HARDWARE     CPU_CYCLES              CPUCycles       cycles  cpu-cycles
   HARDWARE     INSTRUCTIONS            Instructions    instructions
   HARDWARE     CACHE_REFERENCES        CacheReferences cache-references
   HARDWARE     CACHE_MISSES            CacheMisses     cache-misses
   HARDWARE     BRANCH_INSTRUCTIONS     BranchInstructions branch-instructions branches
   HARDWARE     BRANCH_MISSES           BranchMisses    branch-misses
   HARDWARE     BUS_CYCLES              BusCycles       bus-cycles
   HARDWARE     STALLED_CYCLES_FRONTEND StalledCycles   stalled-cycles-frontend idle-cycles-frontend
   HARDWARE     STALLED_CYCLES_BACKEND  StalledCycles   stalled-cycles-backend idle-cycles-backend
   SOFTWARE     CPU_CLOCK               WalltimeMilliseconds cpu-clock
   SOFTWARE     TASK_CLOCK              WalltimeMilliseconds task-clock
   SOFTWARE     PAGE_FAULTS             PageFaults      page-faults faults
   SOFTWARE     PAGE_FAULTS_MAJ         MajorPageFaults major-faults
   SOFTWARE     PAGE_FAULTS_MIN         MinorPageFaults minor-faults
   SOFTWARE     CONTEXT_SWITCHES        ContextSwitches context-switches cs
   SOFTWARE     CPU_MIGRATIONS          CPUMigrations   cpu-migrations migrations
   SOFTWARE     ALIGNMENT_FAULTS        AlignmentFaults alignment-faults
   SOFTWARE     EMULATION_FAULTS        EmulationFaults emulation-faults
   HW_CACHE     L1D_READ                CacheReads      l1d-cache-reads l1d-cache-loads l1d-reads l1d-loads
   HW_CACHE     L1D_WRITE               CacheWrites     l1d-cache-writes l1d-cache-stores l1d-writes l1d-stores
   HW_CACHE     L1D_PREFETCH            CachePrefetches l1d-cache-prefetches l1d-prefetches
   HW_CACHE     L1I_READ                CacheReads      l1i-cache-reads l1i-cache-loads l1i-reads l1i-loads
   HW_CACHE     L1I_PREFETCH            CachePrefetches l1i-cache-prefetches l1i-prefetches
   HW_CACHE     LLC_READ                CacheReads      llc-cache-reads llc-cache-loads llc-loads llc-reads
   HW_CACHE     LLC_WRITE               CacheWrites     llc-cache-writes llc-cache-stores llc-writes llc-stores
   HW_CACHE     LLC_PREFETCH            CachePrefetches llc-cache-prefetches llc-prefetches
   HW_CACHE     L1D_READ_MISS           CacheReads      l1d-cache-read-misses l1d-cache-load-misses l1d-read-misses l1d-load-misses
   HW_CACHE     L1D_WRITE_MISS          CacheWrites     l1d-cache-write-misses l1d-cache-store-misses l1d-write-misses l1d-store-misses
   HW_CACHE     L1D_PREFETCH_MISS       CachePrefetches l1d-cache-prefetch-misses l1d-prefetch-misses
   HW_CACHE     L1I_READ_MISS           CacheReads      l1i-cache-read-misses l1i-cache-load-misses l1i-read-misses l1i-load-misses
   HW_CACHE     L1I_PREFETCH_MISS       CachePrefetches l1i-cache-prefetch-misses l1i-prefetch-misses
   HW_CACHE     LLC_READ_MISS           CacheReads      llc-cache-read-misses llc-cache-load-misses llc-read-misses llc-load-misses
   HW_CACHE     LLC_WRITE_MISS          CacheWrites     llc-cache-write-misses llc-cache-store-misses llc-write-misses llc-store-misses
   HW_CACHE     LLC_PREFETCH_MISS       CachePrefetches llc-cache-prefetch-misses llc-prefetch-misses
   HW_CACHE     BRANCH_READ             BranchInstructions branch-reads branch-loads branch-predicts
   HW_CACHE     BRANCH_READ_MISS        BranchMisses    branch-mispredicts branch-read-misses branch-load-misses

   Use the following Perl script to re-generate the list
=== cut perl ===
#!/usr/bin/env perl
# Load all entries into %map
while (<STDIN>) {
    m/^\s*(.*)\s*$/;
    @_ = split /\s+/, $1;
    $type = shift @_;
    $id = ($type eq "HARDWARE" ? "PERF_COUNT_HW_" :
       $type eq "SOFTWARE" ? "PERF_COUNT_SW_" :
       $type eq "HW_CACHE" ? "CACHE_" : "") . shift @_;
    $unit = shift @_;

    for $string (@_) {
    die "$string was already seen!" if defined($map{$string});
    $map{$string} = [-1, $type, $id, $unit];
    push @strings, $string;
    }
}

# sort the map and print the string list
@strings = sort @strings;
print "static const char eventlist_strings[] = \n";
$counter = 0;
for $entry (@strings) {
    print "    \"$entry\\0\"\n";
    $map{$entry}[0] = $counter;
    $counter += 1 + length $entry;
}

# print the table
print "    \"\\0\";\n\nstatic const Events eventlist[] = {\n";
for $entry (sort @strings) {
    printf "    { %3d, PERF_TYPE_%s, %s, QTest::%s },\n",
        $map{$entry}[0],
    $map{$entry}[1],
        $map{$entry}[2],
        $map{$entry}[3];
}
print "    {   0, PERF_TYPE_MAX, 0, QTest::Events }\n};\n";
=== cut perl ===
*/

struct Events {
    unsigned offset;
    quint32 type;
    quint64 event_id;
    QTest::QBenchmarkMetric metric;
};

/* -- BEGIN GENERATED CODE -- */
static const char eventlist_strings[] =
    "alignment-faults\0"
    "branch-instructions\0"
    "branch-load-misses\0"
    "branch-loads\0"
    "branch-mispredicts\0"
    "branch-misses\0"
    "branch-predicts\0"
    "branch-read-misses\0"
    "branch-reads\0"
    "branches\0"
    "bus-cycles\0"
    "cache-misses\0"
    "cache-references\0"
    "context-switches\0"
    "cpu-clock\0"
    "cpu-cycles\0"
    "cpu-migrations\0"
    "cs\0"
    "cycles\0"
    "emulation-faults\0"
    "faults\0"
    "idle-cycles-backend\0"
    "idle-cycles-frontend\0"
    "instructions\0"
    "l1d-cache-load-misses\0"
    "l1d-cache-loads\0"
    "l1d-cache-prefetch-misses\0"
    "l1d-cache-prefetches\0"
    "l1d-cache-read-misses\0"
    "l1d-cache-reads\0"
    "l1d-cache-store-misses\0"
    "l1d-cache-stores\0"
    "l1d-cache-write-misses\0"
    "l1d-cache-writes\0"
    "l1d-load-misses\0"
    "l1d-loads\0"
    "l1d-prefetch-misses\0"
    "l1d-prefetches\0"
    "l1d-read-misses\0"
    "l1d-reads\0"
    "l1d-store-misses\0"
    "l1d-stores\0"
    "l1d-write-misses\0"
    "l1d-writes\0"
    "l1i-cache-load-misses\0"
    "l1i-cache-loads\0"
    "l1i-cache-prefetch-misses\0"
    "l1i-cache-prefetches\0"
    "l1i-cache-read-misses\0"
    "l1i-cache-reads\0"
    "l1i-load-misses\0"
    "l1i-loads\0"
    "l1i-prefetch-misses\0"
    "l1i-prefetches\0"
    "l1i-read-misses\0"
    "l1i-reads\0"
    "llc-cache-load-misses\0"
    "llc-cache-loads\0"
    "llc-cache-prefetch-misses\0"
    "llc-cache-prefetches\0"
    "llc-cache-read-misses\0"
    "llc-cache-reads\0"
    "llc-cache-store-misses\0"
    "llc-cache-stores\0"
    "llc-cache-write-misses\0"
    "llc-cache-writes\0"
    "llc-load-misses\0"
    "llc-loads\0"
    "llc-prefetch-misses\0"
    "llc-prefetches\0"
    "llc-read-misses\0"
    "llc-reads\0"
    "llc-store-misses\0"
    "llc-stores\0"
    "llc-write-misses\0"
    "llc-writes\0"
    "major-faults\0"
    "migrations\0"
    "minor-faults\0"
    "page-faults\0"
    "stalled-cycles-backend\0"
    "stalled-cycles-frontend\0"
    "task-clock\0"
    "\0";

static const Events eventlist[] = {
    {   0, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_ALIGNMENT_FAULTS, QTest::AlignmentFaults },
    {  17, PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS, QTest::BranchInstructions },
    {  37, PERF_TYPE_HW_CACHE, CACHE_BRANCH_READ_MISS, QTest::BranchMisses },
    {  56, PERF_TYPE_HW_CACHE, CACHE_BRANCH_READ, QTest::BranchInstructions },
    {  69, PERF_TYPE_HW_CACHE, CACHE_BRANCH_READ_MISS, QTest::BranchMisses },
    {  88, PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES, QTest::BranchMisses },
    { 102, PERF_TYPE_HW_CACHE, CACHE_BRANCH_READ, QTest::BranchInstructions },
    { 118, PERF_TYPE_HW_CACHE, CACHE_BRANCH_READ_MISS, QTest::BranchMisses },
    { 137, PERF_TYPE_HW_CACHE, CACHE_BRANCH_READ, QTest::BranchInstructions },
    { 150, PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS, QTest::BranchInstructions },
    { 159, PERF_TYPE_HARDWARE, PERF_COUNT_HW_BUS_CYCLES, QTest::BusCycles },
    { 170, PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES, QTest::CacheMisses },
    { 183, PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES, QTest::CacheReferences },
    { 200, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CONTEXT_SWITCHES, QTest::ContextSwitches },
    { 217, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_CLOCK, QTest::WalltimeMilliseconds },
    { 227, PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, QTest::CPUCycles },
    { 238, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_MIGRATIONS, QTest::CPUMigrations },
    { 253, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CONTEXT_SWITCHES, QTest::ContextSwitches },
    { 256, PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, QTest::CPUCycles },
    { 263, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_EMULATION_FAULTS, QTest::EmulationFaults },
    { 280, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS, QTest::PageFaults },
    { 287, PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_BACKEND, QTest::StalledCycles },
    { 307, PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_FRONTEND, QTest::StalledCycles },
    { 328, PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, QTest::Instructions },
    { 341, PERF_TYPE_HW_CACHE, CACHE_L1D_READ_MISS, QTest::CacheReads },
    { 363, PERF_TYPE_HW_CACHE, CACHE_L1D_READ, QTest::CacheReads },
    { 379, PERF_TYPE_HW_CACHE, CACHE_L1D_PREFETCH_MISS, QTest::CachePrefetches },
    { 405, PERF_TYPE_HW_CACHE, CACHE_L1D_PREFETCH, QTest::CachePrefetches },
    { 426, PERF_TYPE_HW_CACHE, CACHE_L1D_READ_MISS, QTest::CacheReads },
    { 448, PERF_TYPE_HW_CACHE, CACHE_L1D_READ, QTest::CacheReads },
    { 464, PERF_TYPE_HW_CACHE, CACHE_L1D_WRITE_MISS, QTest::CacheWrites },
    { 487, PERF_TYPE_HW_CACHE, CACHE_L1D_WRITE, QTest::CacheWrites },
    { 504, PERF_TYPE_HW_CACHE, CACHE_L1D_WRITE_MISS, QTest::CacheWrites },
    { 527, PERF_TYPE_HW_CACHE, CACHE_L1D_WRITE, QTest::CacheWrites },
    { 544, PERF_TYPE_HW_CACHE, CACHE_L1D_READ_MISS, QTest::CacheReads },
    { 560, PERF_TYPE_HW_CACHE, CACHE_L1D_READ, QTest::CacheReads },
    { 570, PERF_TYPE_HW_CACHE, CACHE_L1D_PREFETCH_MISS, QTest::CachePrefetches },
    { 590, PERF_TYPE_HW_CACHE, CACHE_L1D_PREFETCH, QTest::CachePrefetches },
    { 605, PERF_TYPE_HW_CACHE, CACHE_L1D_READ_MISS, QTest::CacheReads },
    { 621, PERF_TYPE_HW_CACHE, CACHE_L1D_READ, QTest::CacheReads },
    { 631, PERF_TYPE_HW_CACHE, CACHE_L1D_WRITE_MISS, QTest::CacheWrites },
    { 648, PERF_TYPE_HW_CACHE, CACHE_L1D_WRITE, QTest::CacheWrites },
    { 659, PERF_TYPE_HW_CACHE, CACHE_L1D_WRITE_MISS, QTest::CacheWrites },
    { 676, PERF_TYPE_HW_CACHE, CACHE_L1D_WRITE, QTest::CacheWrites },
    { 687, PERF_TYPE_HW_CACHE, CACHE_L1I_READ_MISS, QTest::CacheReads },
    { 709, PERF_TYPE_HW_CACHE, CACHE_L1I_READ, QTest::CacheReads },
    { 725, PERF_TYPE_HW_CACHE, CACHE_L1I_PREFETCH_MISS, QTest::CachePrefetches },
    { 751, PERF_TYPE_HW_CACHE, CACHE_L1I_PREFETCH, QTest::CachePrefetches },
    { 772, PERF_TYPE_HW_CACHE, CACHE_L1I_READ_MISS, QTest::CacheReads },
    { 794, PERF_TYPE_HW_CACHE, CACHE_L1I_READ, QTest::CacheReads },
    { 810, PERF_TYPE_HW_CACHE, CACHE_L1I_READ_MISS, QTest::CacheReads },
    { 826, PERF_TYPE_HW_CACHE, CACHE_L1I_READ, QTest::CacheReads },
    { 836, PERF_TYPE_HW_CACHE, CACHE_L1I_PREFETCH_MISS, QTest::CachePrefetches },
    { 856, PERF_TYPE_HW_CACHE, CACHE_L1I_PREFETCH, QTest::CachePrefetches },
    { 871, PERF_TYPE_HW_CACHE, CACHE_L1I_READ_MISS, QTest::CacheReads },
    { 887, PERF_TYPE_HW_CACHE, CACHE_L1I_READ, QTest::CacheReads },
    { 897, PERF_TYPE_HW_CACHE, CACHE_LLC_READ_MISS, QTest::CacheReads },
    { 919, PERF_TYPE_HW_CACHE, CACHE_LLC_READ, QTest::CacheReads },
    { 935, PERF_TYPE_HW_CACHE, CACHE_LLC_PREFETCH_MISS, QTest::CachePrefetches },
    { 961, PERF_TYPE_HW_CACHE, CACHE_LLC_PREFETCH, QTest::CachePrefetches },
    { 982, PERF_TYPE_HW_CACHE, CACHE_LLC_READ_MISS, QTest::CacheReads },
    { 1004, PERF_TYPE_HW_CACHE, CACHE_LLC_READ, QTest::CacheReads },
    { 1020, PERF_TYPE_HW_CACHE, CACHE_LLC_WRITE_MISS, QTest::CacheWrites },
    { 1043, PERF_TYPE_HW_CACHE, CACHE_LLC_WRITE, QTest::CacheWrites },
    { 1060, PERF_TYPE_HW_CACHE, CACHE_LLC_WRITE_MISS, QTest::CacheWrites },
    { 1083, PERF_TYPE_HW_CACHE, CACHE_LLC_WRITE, QTest::CacheWrites },
    { 1100, PERF_TYPE_HW_CACHE, CACHE_LLC_READ_MISS, QTest::CacheReads },
    { 1116, PERF_TYPE_HW_CACHE, CACHE_LLC_READ, QTest::CacheReads },
    { 1126, PERF_TYPE_HW_CACHE, CACHE_LLC_PREFETCH_MISS, QTest::CachePrefetches },
    { 1146, PERF_TYPE_HW_CACHE, CACHE_LLC_PREFETCH, QTest::CachePrefetches },
    { 1161, PERF_TYPE_HW_CACHE, CACHE_LLC_READ_MISS, QTest::CacheReads },
    { 1177, PERF_TYPE_HW_CACHE, CACHE_LLC_READ, QTest::CacheReads },
    { 1187, PERF_TYPE_HW_CACHE, CACHE_LLC_WRITE_MISS, QTest::CacheWrites },
    { 1204, PERF_TYPE_HW_CACHE, CACHE_LLC_WRITE, QTest::CacheWrites },
    { 1215, PERF_TYPE_HW_CACHE, CACHE_LLC_WRITE_MISS, QTest::CacheWrites },
    { 1232, PERF_TYPE_HW_CACHE, CACHE_LLC_WRITE, QTest::CacheWrites },
    { 1243, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS_MAJ, QTest::MajorPageFaults },
    { 1256, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_MIGRATIONS, QTest::CPUMigrations },
    { 1267, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS_MIN, QTest::MinorPageFaults },
    { 1280, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS, QTest::PageFaults },
    { 1292, PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_BACKEND, QTest::StalledCycles },
    { 1315, PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_FRONTEND, QTest::StalledCycles },
    { 1339, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_TASK_CLOCK, QTest::WalltimeMilliseconds },
    {   0, PERF_TYPE_MAX, 0, QTest::Events }
};
/* -- END GENERATED CODE -- */

QTest::QBenchmarkMetric QBenchmarkPerfEventsMeasurer::metricForEvent(quint32 type, quint64 event_id)
{
    const Events *ptr = eventlist;
    for ( ; ptr->type != PERF_TYPE_MAX; ++ptr) {
        if (ptr->type == type && ptr->event_id == event_id)
            return ptr->metric;
    }
    return QTest::Events;
}

void QBenchmarkPerfEventsMeasurer::setCounter(const char *name)
{
    initPerf();
    const char *colon = strchr(name, ':');
    int n = colon ? colon - name : strlen(name);
    const Events *ptr = eventlist;
    for ( ; ptr->type != PERF_TYPE_MAX; ++ptr) {
        int c = strncmp(name, eventlist_strings + ptr->offset, n);
        if (c == 0)
            break;
        if (c < 0) {
            fprintf(stderr, "ERROR: Performance counter type '%s' is unknown\n", name);
            exit(1);
        }
    }

    attr.type = ptr->type;
    attr.config = ptr->event_id;

    // now parse the attributes
    if (!colon)
        return;
    while (*++colon) {
        switch (*colon) {
        case 'u':
            attr.exclude_user = true;
            break;
        case 'k':
            attr.exclude_kernel = true;
            break;
        case 'h':
            attr.exclude_hv = true;
            break;
        case 'G':
            attr.exclude_guest = true;
            break;
        case 'H':
            attr.exclude_host = true;
            break;
        default:
            fprintf(stderr, "ERROR: Unknown attribute '%c'\n", *colon);
            exit(1);
        }
    }
}

void QBenchmarkPerfEventsMeasurer::listCounters()
{
    if (!isAvailable()) {
        printf("Performance counters are not available on this system\n");
        return;
    }

    printf("The following performance counters are available:\n");
    const Events *ptr = eventlist;
    for ( ; ptr->type != PERF_TYPE_MAX; ++ptr) {
        printf("  %-30s [%s]\n", eventlist_strings + ptr->offset,
               ptr->type == PERF_TYPE_HARDWARE ? "hardware" :
               ptr->type == PERF_TYPE_SOFTWARE ? "software" :
               ptr->type == PERF_TYPE_HW_CACHE ? "cache" : "other");
    }

    printf("\nAttributes can be specified by adding a colon and the following:\n"
           "  u - exclude measuring in the userspace\n"
           "  k - exclude measuring in kernel mode\n"
           "  h - exclude measuring in the hypervisor\n"
           "  G - exclude measuring when running virtualized (guest VM)\n"
           "  H - exclude measuring when running non-virtualized (host system)\n"
           "Attributes can be combined, for example: -perfcounter branch-mispredicts:kh\n");
}

QBenchmarkPerfEventsMeasurer::QBenchmarkPerfEventsMeasurer()
    : fd(-1)
{
}

QBenchmarkPerfEventsMeasurer::~QBenchmarkPerfEventsMeasurer()
{
    qt_safe_close(fd);
}

void QBenchmarkPerfEventsMeasurer::init()
{
}

void QBenchmarkPerfEventsMeasurer::start()
{

    initPerf();
    if (fd == -1) {
        // pid == 0 -> attach to the current process
        // cpu == -1 -> monitor on all CPUs
        // group_fd == -1 -> this is the group leader
        // flags == 0 -> reserved, must be zero
        fd = perf_event_open(&attr, 0, -1, -1, 0);
        if (fd == -1) {
            perror("QBenchmarkPerfEventsMeasurer::start: perf_event_open");
            exit(1);
        } else {
            ::fcntl(fd, F_SETFD, FD_CLOEXEC);
        }
    }

    // enable the counter
    ::ioctl(fd, PERF_EVENT_IOC_RESET);
    ::ioctl(fd, PERF_EVENT_IOC_ENABLE);
}

qint64 QBenchmarkPerfEventsMeasurer::checkpoint()
{
    ::ioctl(fd, PERF_EVENT_IOC_DISABLE);
    qint64 value = readValue();
    ::ioctl(fd, PERF_EVENT_IOC_ENABLE);
    return value;
}

qint64 QBenchmarkPerfEventsMeasurer::stop()
{
    // disable the counter
    ::ioctl(fd, PERF_EVENT_IOC_DISABLE);
    return readValue();
}

bool QBenchmarkPerfEventsMeasurer::isMeasurementAccepted(qint64)
{
    return true;
}

int QBenchmarkPerfEventsMeasurer::adjustIterationCount(int)
{
    return 1;
}

int QBenchmarkPerfEventsMeasurer::adjustMedianCount(int)
{
    return 1;
}

QTest::QBenchmarkMetric QBenchmarkPerfEventsMeasurer::metricType()
{
    return metricForEvent(attr.type, attr.config);
}

static quint64 rawReadValue(int fd)
{
    /* from the kernel docs:
     * struct read_format {
     *  { u64           value;
     *    { u64         time_enabled; } && PERF_FORMAT_TOTAL_TIME_ENABLED
     *    { u64         time_running; } && PERF_FORMAT_TOTAL_TIME_RUNNING
     *    { u64         id;           } && PERF_FORMAT_ID
     *  } && !PERF_FORMAT_GROUP
     */

    struct read_format {
        quint64 value;
        quint64 time_enabled;
        quint64 time_running;
    } results;

    size_t nread = 0;
    while (nread < sizeof results) {
        char *ptr = reinterpret_cast<char *>(&results);
        qint64 r = qt_safe_read(fd, ptr + nread, sizeof results - nread);
        if (r == -1) {
            perror("QBenchmarkPerfEventsMeasurer::readValue: reading the results");
            exit(1);
        }
        nread += quint64(r);
    }

    if (results.time_running == results.time_enabled)
        return results.value;

    // scale the results, though this shouldn't happen!
    return results.value * (double(results.time_running) / double(results.time_enabled));
}

qint64 QBenchmarkPerfEventsMeasurer::readValue()
{
    quint64 raw = rawReadValue(fd);
    if (metricType() == QTest::WalltimeMilliseconds) {
        // perf returns nanoseconds
        return raw / 1000000;
    }
    return raw;
}

QT_END_NAMESPACE

#endif
