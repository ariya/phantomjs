/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2013 Apple Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

/* This prefix file should contain only: 
 *    1) files to precompile for faster builds
 *    2) in one case at least: OS-X-specific performance bug workarounds
 *    3) the special trick to catch us using new or delete without including "config.h"
 * The project should be able to build without this header, although we rarely test that.
 */

/* Things that need to be defined globally should go into "config.h". */

#include <wtf/Platform.h>

#if defined(__APPLE__)
#ifdef __cplusplus
#define NULL __null
#else
#define NULL ((void *)0)
#endif
#endif

#if OS(WINDOWS)
#if !USE(CURL)
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_ // Prevent inclusion of winsock.h in windows.h
#endif
#endif

// If we don't define these, they get defined in windef.h. 
// We want to use std::min and std::max
#ifdef __cplusplus
#define max max
#define min min
#endif

#else

#include <pthread.h>

#endif // OS(WINDOWS)

#include <sys/types.h>
#include <fcntl.h>
#if defined(__APPLE__)
#include <regex.h>
#endif

#include <setjmp.h>

#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if defined(__APPLE__)
#include <unistd.h>
#endif

#ifdef __cplusplus


#include <ciso646>

#if defined(_LIBCPP_VERSION)

// Add work around for a bug in libc++ that caused standard heap
// APIs to not compile <rdar://problem/10858112>.

#include <type_traits>

namespace WebCore {
    class TimerHeapReference;
}

_LIBCPP_BEGIN_NAMESPACE_STD

inline _LIBCPP_INLINE_VISIBILITY
const WebCore::TimerHeapReference& move(const WebCore::TimerHeapReference& t)
{
    return t;
}

_LIBCPP_END_NAMESPACE_STD

#endif // defined(_LIBCPP_VERSION)

#include <algorithm>
#include <cstddef>
#include <new>

#endif

#if defined(__APPLE__)
#include <sys/param.h>
#endif
#include <sys/stat.h>
#if defined(__APPLE__)
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <CoreFoundation/CoreFoundation.h>
#if PLATFORM(WIN_CAIRO)
#include <ConditionalMacros.h>
#include <windows.h>
#else

#if OS(WINDOWS)
#if USE(CG)

#if defined(_MSC_VER) && _MSC_VER <= 1600

#include <WebCore/WebCoreHeaderDetection.h>

#if HAVE(AVCF_LEGIBLE_OUTPUT)
// These must be defined before including CGFloat.h
// This can be removed once we move to VS2012 or newer
#include <wtf/ExportMacros.h>
#include <wtf/MathExtras.h>

#define isnan _isnan
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
#include <CoreGraphics/CGFloat.h>
#endif
#include <CoreGraphics/CoreGraphics.h>
#undef isnan
#endif
#endif

// FIXME <rdar://problem/8208868> Remove support for obsolete ColorSync API, CoreServices header in CoreGraphics
// We can remove this once the new ColorSync APIs are available in an internal Safari SDK.
#include <ColorSync/ColorSync.h>
#ifdef __COLORSYNCDEPRECATED__
#define COREGRAPHICS_INCLUDES_CORESERVICES_HEADER
#define OBSOLETE_COLORSYNC_API
#endif
#endif
#if USE(CFNETWORK)
/* Windows doesn't include CFNetwork.h via CoreServices.h, so we do
   it explicitly here to make Windows more consistent with Mac. */
#include <CFNetwork/CFNetwork.h>
// On Windows, dispatch.h needs to be included before certain CFNetwork headers.
#include <dispatch/dispatch.h>
#endif
#include <windows.h>
#else
#if !PLATFORM(IOS)
#include <CoreServices/CoreServices.h>
#endif // !PLATFORM(IOS)
#endif // OS(WINDOWS)

#endif

#ifdef __OBJC__
#if PLATFORM(IOS)
#import <Foundation/Foundation.h>
#else
#import <Cocoa/Cocoa.h>
#endif // PLATFORM(IOS)
#endif

#ifdef __cplusplus
#define new ("if you use new/delete make sure to include config.h at the top of the file"()) 
#define delete ("if you use new/delete make sure to include config.h at the top of the file"()) 
#endif

/* When C++ exceptions are disabled, the C++ library defines |try| and |catch|
 * to allow C++ code that expects exceptions to build. These definitions
 * interfere with Objective-C++ uses of Objective-C exception handlers, which
 * use |@try| and |@catch|. As a workaround, undefine these macros. */
#ifdef __OBJC__
#undef try
#undef catch
#endif

