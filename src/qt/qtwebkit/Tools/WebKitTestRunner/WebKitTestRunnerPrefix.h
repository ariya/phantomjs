/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifdef __OBJC__
#include <Cocoa/Cocoa.h>
#endif

#if defined(WIN32) || defined(_WIN32)
// If we don't define these, they get defined in windef.h. 
// We want to use std::min and std::max
#define max max
#define min min
#endif

#if defined(BUILDING_GTK__)
#include "autotoolsconfig.h"
#endif /* defined (BUILDING_GTK__) */

#include <wtf/Platform.h>
#include <WebKit2/WebKit2_C.h>


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

