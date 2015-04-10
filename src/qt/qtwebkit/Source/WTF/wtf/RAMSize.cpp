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
#include "RAMSize.h"

#include "StdLibExtras.h"
#if OS(DARWIN)
#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#elif OS(UNIX)
#include <unistd.h>
#elif OS(WINDOWS)
#include <windows.h>
#elif OS(QNX)
#include <sys/stat.h>
#endif

namespace WTF {

static const size_t ramSizeGuess = 128 * MB;

static size_t computeRAMSize()
{
#if OS(DARWIN)
    int mib[2];
    uint64_t ramSize;
    size_t length;

    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    length = sizeof(int64_t);
    int sysctlResult = sysctl(mib, 2, &ramSize, &length, 0, 0);
    if (sysctlResult == -1)
        return ramSizeGuess;
    return ramSize > std::numeric_limits<size_t>::max() ? std::numeric_limits<size_t>::max() : static_cast<size_t>(ramSize);
#elif OS(UNIX)
    long pages = sysconf(_SC_PHYS_PAGES);
    long pageSize = sysconf(_SC_PAGE_SIZE);
    if (pages == -1 || pageSize == -1)
        return ramSizeGuess;
    return pages * pageSize;
#elif OS(WINCE)
    MEMORYSTATUS status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    if (status.dwTotalPhys <= 0)
        return ramSizeGuess;
    return status.dwTotalPhys;
#elif OS(WINDOWS)
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    bool result = GlobalMemoryStatusEx(&status);
    if (!result)
        return ramSizeGuess;
    return status.ullTotalPhys;
#elif OS(QNX)
    struct stat mst;
    if (stat("/proc", &mst))
        return ramSizeGuess;
    return mst.st_size;
#endif
}

size_t ramSize()
{
    static const size_t ramSize = computeRAMSize();
    return ramSize;
}

} // namespace WTF
