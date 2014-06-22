//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef __OSINCLUDE_H
#define __OSINCLUDE_H

//
// This file contains contains os-specific datatypes and
// declares any os-specific functions.
//

#if defined(_WIN32) || defined(_WIN64)
#define ANGLE_OS_WIN
#elif defined(__APPLE__) || defined(__linux__) || \
      defined(__FreeBSD__) || defined(__OpenBSD__) || \
      defined(__sun) || defined(ANDROID) || \
      defined(__GLIBC__) || defined(__GNU__) || \
      defined(__QNX__)
#define ANGLE_OS_POSIX
#else
#error Unsupported platform.
#endif

#if defined(ANGLE_OS_WIN)
#define STRICT
#define VC_EXTRALEAN 1
#include <windows.h>
#elif defined(ANGLE_OS_POSIX)
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#endif  // ANGLE_OS_WIN


#include "compiler/debug.h"

//
// Thread Local Storage Operations
//
#if defined(ANGLE_OS_WIN)
typedef DWORD OS_TLSIndex;
#define OS_INVALID_TLS_INDEX (TLS_OUT_OF_INDEXES)
#elif defined(ANGLE_OS_POSIX)
typedef pthread_key_t OS_TLSIndex;
#define OS_INVALID_TLS_INDEX (static_cast<OS_TLSIndex>(-1))
#endif  // ANGLE_OS_WIN

OS_TLSIndex OS_AllocTLSIndex();
bool OS_SetTLSValue(OS_TLSIndex nIndex, void *lpvValue);
bool OS_FreeTLSIndex(OS_TLSIndex nIndex);

inline void* OS_GetTLSValue(OS_TLSIndex nIndex)
{
    ASSERT(nIndex != OS_INVALID_TLS_INDEX);
#if defined(ANGLE_OS_WIN)
    return TlsGetValue(nIndex);
#elif defined(ANGLE_OS_POSIX)
    return pthread_getspecific(nIndex);
#endif  // ANGLE_OS_WIN
}

#endif // __OSINCLUDE_H
