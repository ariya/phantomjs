// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/event_tracer.h"

namespace gl
{

GetCategoryEnabledFlagFunc g_getCategoryEnabledFlag;
AddTraceEventFunc g_addTraceEvent;

}  // namespace gl

extern "C" {

void TRACE_ENTRY SetTraceFunctionPointers(GetCategoryEnabledFlagFunc getCategoryEnabledFlag,
                                        AddTraceEventFunc addTraceEvent)
{
    gl::g_getCategoryEnabledFlag = getCategoryEnabledFlag;
    gl::g_addTraceEvent = addTraceEvent;
}

}  // extern "C"

namespace gl
{

const unsigned char* TraceGetTraceCategoryEnabledFlag(const char* name)
{
    if (g_getCategoryEnabledFlag)
    {
        return g_getCategoryEnabledFlag(name);
    }
    static unsigned char disabled = 0;
    return &disabled;
}

void TraceAddTraceEvent(char phase, const unsigned char* categoryGroupEnabled, const char* name, unsigned long long id,
                        int numArgs, const char** argNames, const unsigned char* argTypes,
                        const unsigned long long* argValues, unsigned char flags)
{
    if (g_addTraceEvent)
    {
        g_addTraceEvent(phase, categoryGroupEnabled, name, id, numArgs, argNames, argTypes, argValues, flags);
    }
}

}  // namespace gl
