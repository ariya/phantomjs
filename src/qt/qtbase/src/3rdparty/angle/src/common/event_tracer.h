// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMMON_EVENT_TRACER_H_
#define COMMON_EVENT_TRACER_H_

#if !defined(TRACE_ENTRY)
#if defined(_WIN32)
#define TRACE_ENTRY __stdcall
#else
#define TRACE_ENTRY
#endif // // _WIN32
#endif //TRACE_ENTRY

extern "C" {

typedef const unsigned char* (*GetCategoryEnabledFlagFunc)(const char* name);
typedef void (*AddTraceEventFunc)(char phase, const unsigned char* categoryGroupEnabled, const char* name,
                                  unsigned long long id, int numArgs, const char** argNames,
                                  const unsigned char* argTypes, const unsigned long long* argValues,
                                  unsigned char flags);

// extern "C" so that it has a reasonable name for GetProcAddress.
void TRACE_ENTRY SetTraceFunctionPointers(GetCategoryEnabledFlagFunc get_category_enabled_flag,
                                          AddTraceEventFunc add_trace_event_func);

}

namespace gl
{

const unsigned char* TraceGetTraceCategoryEnabledFlag(const char* name);

void TraceAddTraceEvent(char phase, const unsigned char* categoryGroupEnabled, const char* name, unsigned long long id,
                        int numArgs, const char** argNames, const unsigned char* argTypes,
                        const unsigned long long* argValues, unsigned char flags);

}

#endif  // COMMON_EVENT_TRACER_H_
