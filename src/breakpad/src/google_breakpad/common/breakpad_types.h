/* Copyright (c) 2006, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

/* breakpad_types.h: Precise-width types
 *
 * (This is C99 source, please don't corrupt it with C++.)
 *
 * This file ensures that types u_intN_t are defined for N = 8, 16, 32, and
 * 64.  Types of precise widths are crucial to the task of writing data
 * structures on one platform and reading them on another.
 *
 * Author: Mark Mentovai */

#ifndef GOOGLE_BREAKPAD_COMMON_BREAKPAD_TYPES_H__
#define GOOGLE_BREAKPAD_COMMON_BREAKPAD_TYPES_H__

#ifndef _WIN32

#include <sys/types.h>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif  /* __STDC_FORMAT_MACROS */
#include <inttypes.h>

#if defined(__SUNPRO_CC) || (defined(__GNUC__) && defined(__sun__))
typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;
#endif

#else  /* !_WIN32 */

#include <WTypes.h>

typedef unsigned __int8  u_int8_t;
typedef unsigned __int16 u_int16_t;
typedef unsigned __int32 u_int32_t;
typedef unsigned __int64 u_int64_t;

#endif  /* !_WIN32 */

typedef struct {
  u_int64_t high;
  u_int64_t low;
} u_int128_t;

typedef u_int64_t breakpad_time_t;

/* Try to get PRIx64 from inttypes.h, but if it's not defined, fall back to
 * llx, which is the format string for "long long" - this is a 64-bit
 * integral type on many systems. */
#ifndef PRIx64
#define PRIx64 "llx"
#endif  /* !PRIx64 */

#endif  /* GOOGLE_BREAKPAD_COMMON_BREAKPAD_TYPES_H__ */
