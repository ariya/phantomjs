//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// debug.h: Debugging utilities.

#ifndef COMMON_DEBUG_H_
#define COMMON_DEBUG_H_

#include <stdio.h>
#include <assert.h>

#include "common/angleutils.h"

#if !defined(TRACE_OUTPUT_FILE)
#define TRACE_OUTPUT_FILE "debug.txt"
#endif

namespace gl
{
    // Outputs text to the debugging log, or the debugging window
    void trace(bool traceInDebugOnly, const char *format, ...);

    // Returns whether D3DPERF is active.
    bool perfActive();

    // Pairs a D3D begin event with an end event.
    class ScopedPerfEventHelper
    {
      public:
        ScopedPerfEventHelper(const char* format, ...);
        ~ScopedPerfEventHelper();

      private:
        DISALLOW_COPY_AND_ASSIGN(ScopedPerfEventHelper);
    };

    void InitializeDebugAnnotations();
    void UninitializeDebugAnnotations();
}

// A macro to output a trace of a function call and its arguments to the debugging log
#if defined(ANGLE_ENABLE_DEBUG_TRACE) || defined(ANGLE_ENABLE_DEBUG_ANNOTATIONS)
#define TRACE(message, ...) gl::trace(true, "trace: %s(%d): " message "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define TRACE(message, ...) (void(0))
#endif

// A macro to output a function call and its arguments to the debugging log, to denote an item in need of fixing.
#if defined(ANGLE_ENABLE_DEBUG_TRACE) || defined(ANGLE_ENABLE_DEBUG_ANNOTATIONS)
#define FIXME(message, ...) gl::trace(false, "fixme: %s(%d): " message "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define FIXME(message, ...) (void(0))
#endif

// A macro to output a function call and its arguments to the debugging log, in case of error.
#if defined(ANGLE_ENABLE_DEBUG_TRACE) || defined(ANGLE_ENABLE_DEBUG_ANNOTATIONS)
#define ERR(message, ...) gl::trace(false, "err: %s(%d): " message "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define ERR(message, ...) (void(0))
#endif

// A macro to log a performance event around a scope.
#if defined(ANGLE_ENABLE_DEBUG_TRACE) || defined(ANGLE_ENABLE_DEBUG_ANNOTATIONS)
#if defined(_MSC_VER)
#define EVENT(message, ...) gl::ScopedPerfEventHelper scopedPerfEventHelper ## __LINE__("%s" message "\n", __FUNCTION__, __VA_ARGS__);
#else
#define EVENT(message, ...) gl::ScopedPerfEventHelper scopedPerfEventHelper(message "\n", ##__VA_ARGS__);
#endif // _MSC_VER
#else
#define EVENT(message, ...) (void(0))
#endif

// A macro asserting a condition and outputting failures to the debug log
#if !defined(NDEBUG)
#define ASSERT(expression) do { \
    if(!(expression)) \
        ERR("\t! Assert failed in %s(%d): "#expression"\n", __FUNCTION__, __LINE__); \
        assert(expression); \
    } while(0)
#define UNUSED_ASSERTION_VARIABLE(variable)
#else
#define ASSERT(expression) (void(0))
#define UNUSED_ASSERTION_VARIABLE(variable) ((void)variable)
#endif

#ifndef ANGLE_ENABLE_DEBUG_TRACE
#define UNUSED_TRACE_VARIABLE(variable) ((void)variable)
#else
#define UNUSED_TRACE_VARIABLE(variable)
#endif

// A macro to indicate unimplemented functionality

#if defined (ANGLE_TEST_CONFIG)
#define NOASSERT_UNIMPLEMENTED 1
#endif

// Define NOASSERT_UNIMPLEMENTED to non zero to skip the assert fail in the unimplemented checks
// This will allow us to test with some automated test suites (eg dEQP) without crashing
#ifndef NOASSERT_UNIMPLEMENTED
#define NOASSERT_UNIMPLEMENTED 0
#endif

#if !defined(NDEBUG)
#define UNIMPLEMENTED() do { \
    FIXME("\t! Unimplemented: %s(%d)\n", __FUNCTION__, __LINE__); \
    assert(NOASSERT_UNIMPLEMENTED); \
    } while(0)
#else
    #define UNIMPLEMENTED() FIXME("\t! Unimplemented: %s(%d)\n", __FUNCTION__, __LINE__)
#endif

// A macro for code which is not expected to be reached under valid assumptions
#if !defined(NDEBUG)
#define UNREACHABLE() do { \
    ERR("\t! Unreachable reached: %s(%d)\n", __FUNCTION__, __LINE__); \
    assert(false); \
    } while(0)
#else
    #define UNREACHABLE() ERR("\t! Unreachable reached: %s(%d)\n", __FUNCTION__, __LINE__)
#endif

// A macro that determines whether an object has a given runtime type.
#if !defined(NDEBUG) && (!defined(_MSC_VER) || defined(_CPPRTTI)) && (!defined(__GNUC__) || __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 3) || defined(__GXX_RTTI))
#define HAS_DYNAMIC_TYPE(type, obj) (dynamic_cast<type >(obj) != NULL)
#else
#define HAS_DYNAMIC_TYPE(type, obj) true
#endif

// A macro functioning as a compile-time assert to validate constant conditions
#if (defined(_MSC_VER) && _MSC_VER >= 1600) || (defined(__GNUC__) && (__GNUC__ > 4 || __GNUC_MINOR__ >= 3))
#define META_ASSERT_MSG(condition, msg) static_assert(condition, msg)
#else
#define META_ASSERT_CONCAT(a, b) a ## b
#define META_ASSERT_CONCAT2(a, b) META_ASSERT_CONCAT(a, b)
#define META_ASSERT_MSG(condition, msg) typedef int META_ASSERT_CONCAT2(COMPILE_TIME_ASSERT_, __LINE__)[static_cast<bool>(condition)?1:-1]
#endif
#define META_ASSERT(condition) META_ASSERT_MSG(condition, "compile time assertion failed.")

#endif   // COMMON_DEBUG_H_
