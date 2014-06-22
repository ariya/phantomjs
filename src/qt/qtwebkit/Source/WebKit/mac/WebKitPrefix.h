/*
 * Copyright (C) 2005, 2006, 2007 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef __cplusplus
#define NULL __null
#else
#define NULL ((void *)0)
#endif

#import <stddef.h>
#import <stdio.h>
#import <fcntl.h>
#import <errno.h>
#import <unistd.h>
#import <signal.h>
#import <sys/types.h>
#import <sys/time.h>
#import <sys/resource.h>

#import <pthread.h>

#import <CoreGraphics/CoreGraphics.h>

#ifdef __cplusplus

#include <algorithm> // needed for exception_defines.h
#include <cstddef>
#include <new>

#endif

#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>

#ifndef CGFLOAT_DEFINED
#ifdef __LP64__
typedef double CGFloat;
#else
typedef float CGFloat;
#endif
#define CGFLOAT_DEFINED 1
#endif

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#endif

#include <wtf/Platform.h>
#include <wtf/ExportMacros.h>
#include <runtime/JSExportMacros.h>
#include <WebCore/PlatformExportMacros.h>
#include <WebCore/EmptyProtocolDefinitions.h>

/* Work around bug with C++ library that screws up Objective-C++ when exception support is disabled. */
#undef try
#undef catch

#ifdef __cplusplus
#include <wtf/FastMalloc.h>
#endif

#include <wtf/DisallowCType.h>
