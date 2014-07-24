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

#if defined(BUILDING_WITH_CMAKE)
#include "cmakeconfig.h"
#elif defined(BUILDING_GTK__)
#include "autotoolsconfig.h"
#endif

#include <runtime/JSExportMacros.h>
#include <wtf/DisallowCType.h>
#include <wtf/Platform.h>
#include <wtf/ExportMacros.h>

#ifdef __cplusplus
#ifndef EXTERN_C_BEGIN
#define EXTERN_C_BEGIN extern "C" {
#endif
#ifndef EXTERN_C_END
#define EXTERN_C_END }
#endif
#else
#define EXTERN_C_BEGIN
#define EXTERN_C_END
#endif

// For defining getters to a static value, where the getters have internal linkage
#define DEFINE_STATIC_GETTER(type, name, arguments) \
static const type& name() \
{ \
    DEFINE_STATIC_LOCAL(type, name##Value, arguments); \
    return name##Value; \
}

#if OS(WINDOWS)
/* If we don't define these, they get defined in windef.h. */
/* We want to use std::min and std::max. */
#ifndef max
#define max max
#endif
#ifndef min
#define min min
#endif

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_ /* Prevent inclusion of winsock.h in windows.h */
#endif

#if !PLATFORM(QT)
#include <WebCore/config.h>
#endif
#include <windows.h>

#if USE(CG)
#include <CoreGraphics/CoreGraphics.h>
#endif

#endif /* OS(WINDOWS) */

#ifdef __cplusplus

// These undefs match up with defines in WebKit2Prefix.h for Mac OS X.
// Helps us catch if anyone uses new or delete by accident in code and doesn't include "config.h".
#undef new
#undef delete
#include <wtf/FastMalloc.h>

#endif

#ifndef PLUGIN_ARCHITECTURE_UNSUPPORTED
#if PLATFORM(MAC)
#define PLUGIN_ARCHITECTURE_MAC 1
#elif (PLATFORM(GTK) || PLATFORM(EFL)) && (OS(UNIX) && !OS(MAC_OS_X))
#define PLUGIN_ARCHITECTURE_X11 1
#elif PLATFORM(QT)
// Qt handles this features.prf
#else
#define PLUGIN_ARCHITECTURE_UNSUPPORTED 1
#endif
#endif

#define PLUGIN_ARCHITECTURE(ARCH) (defined PLUGIN_ARCHITECTURE_##ARCH && PLUGIN_ARCHITECTURE_##ARCH)

#ifndef ENABLE_INSPECTOR_SERVER
#if ENABLE(INSPECTOR) && (PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL))
#define ENABLE_INSPECTOR_SERVER 1
#endif
#endif
