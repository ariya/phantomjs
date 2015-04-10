/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
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

#if defined (BUILDING_GTK__)
#include "autotoolsconfig.h"
#endif /* defined (BUILDING_GTK__) */

#include <wtf/Platform.h>

#if PLATFORM(MAC)

#define ENABLE_WEB_PROCESS_SANDBOX 1

#if ENABLE(NETSCAPE_PLUGIN_API)
#define ENABLE_PLUGIN_PROCESS 1
#endif

#define ENABLE_NETWORK_PROCESS 1

#define ENABLE_MEMORY_SAMPLER 1

#define ENABLE_CUSTOM_PROTOCOLS 1

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>

#ifdef __OBJC__
#import <Foundation/Foundation.h>
#import <WebCore/EmptyProtocolDefinitions.h>
#if USE(APPKIT)
#import <Cocoa/Cocoa.h>
#endif
#endif

#if ENABLE(PLUGIN_PROCESS)
#define ENABLE_SHARED_WORKER_PROCESS 1
#endif

#else
#define ENABLE_SHARED_WORKER_PROCESS 1
#endif

/* When C++ exceptions are disabled, the C++ library defines |try| and |catch|
* to allow C++ code that expects exceptions to build. These definitions
* interfere with Objective-C++ uses of Objective-C exception handlers, which
* use |@try| and |@catch|. As a workaround, undefine these macros. */

#ifdef __cplusplus
#include <algorithm> // needed for exception_defines.h
#endif

#ifdef __OBJC__
#undef try
#undef catch
#endif

#ifdef __cplusplus
#define new ("if you use new/delete make sure to include config.h at the top of the file"()) 
#define delete ("if you use new/delete make sure to include config.h at the top of the file"()) 
#endif
