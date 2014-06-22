//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// debug.cpp: Debugging utilities.

#include "common/debug.h"
#include "common/system.h"
#include <d3d9.h>

namespace gl
{

typedef void (WINAPI *PerfOutputFunction)(D3DCOLOR, LPCWSTR);

static void output(bool traceFileDebugOnly, PerfOutputFunction perfFunc, const char *format, va_list vararg)
{
#if !defined(ANGLE_DISABLE_PERF)
    if (perfActive())
    {
        char message[32768];
        int len = vsprintf_s(message, format, vararg);
        if (len < 0)
        {
            return;
        }

        // There are no ASCII variants of these D3DPERF functions.
        wchar_t wideMessage[32768];
        for (int i = 0; i < len; ++i)
        {
            wideMessage[i] = message[i];
        }
        wideMessage[len] = 0;

        perfFunc(0, wideMessage);
    }
#endif

#if !defined(ANGLE_DISABLE_TRACE)
#if defined(NDEBUG)
    if (traceFileDebugOnly)
    {
        return;
    }
#endif

    FILE* file = fopen(TRACE_OUTPUT_FILE, "a");
    if (file)
    {
        vfprintf(file, format, vararg);
        fclose(file);
    }
#endif
}

void trace(bool traceFileDebugOnly, const char *format, ...)
{
    va_list vararg;
    va_start(vararg, format);
#if defined(ANGLE_DISABLE_PERF)
    output(traceFileDebugOnly, NULL, format, vararg);
#else
    output(traceFileDebugOnly, D3DPERF_SetMarker, format, vararg);
#endif
    va_end(vararg);
}

bool perfActive()
{
#if defined(ANGLE_DISABLE_PERF)
    return false;
#else
    static bool active = D3DPERF_GetStatus() != 0;
    return active;
#endif
}

ScopedPerfEventHelper::ScopedPerfEventHelper(const char* format, ...)
{
#if !defined(ANGLE_DISABLE_PERF)
#if defined(ANGLE_DISABLE_TRACE)
    if (!perfActive())
    {
        return;
    }
#endif
    va_list vararg;
    va_start(vararg, format);
    output(true, reinterpret_cast<PerfOutputFunction>(D3DPERF_BeginEvent), format, vararg);
    va_end(vararg);
#endif
}

ScopedPerfEventHelper::~ScopedPerfEventHelper()
{
#if !defined(ANGLE_DISABLE_PERF)
    if (perfActive())
    {
        D3DPERF_EndEvent();
    }
#endif
}
}
