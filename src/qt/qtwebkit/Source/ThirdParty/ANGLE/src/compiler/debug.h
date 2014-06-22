//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// debug.h: Debugging utilities.

#ifndef COMPILER_DEBUG_H_
#define COMPILER_DEBUG_H_

#include <assert.h>

#ifdef _DEBUG
#define TRACE_ENABLED  // define to enable debug message tracing
#endif  // _DEBUG

// Outputs text to the debug log
#ifdef TRACE_ENABLED

#ifdef  __cplusplus
extern "C" {
#endif  // __cplusplus
void Trace(const char* format, ...);
#ifdef  __cplusplus
}
#endif  // __cplusplus

#else   // TRACE_ENABLED

#define Trace(...) ((void)0)

#endif  // TRACE_ENABLED

// A macro asserting a condition and outputting failures to the debug log
#define ASSERT(expression) do { \
    if(!(expression)) \
        Trace("Assert failed: %s(%d): "#expression"\n", __FUNCTION__, __LINE__); \
    assert(expression); \
} while(0)

#define UNIMPLEMENTED() do { \
    Trace("Unimplemented invoked: %s(%d)\n", __FUNCTION__, __LINE__); \
    assert(false); \
} while(0)

#define UNREACHABLE() do { \
    Trace("Unreachable reached: %s(%d)\n", __FUNCTION__, __LINE__); \
    assert(false); \
} while(0)

#endif   // COMPILER_DEBUG_H_

