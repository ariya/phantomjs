/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef PluginDebug_h
#define PluginDebug_h

#include "Logging.h"
#include "npruntime_internal.h"
#include <wtf/text/CString.h>

#define LOG_NPERROR(err) if (err != NPERR_NO_ERROR) LOG_VERBOSE(Plugins, "%s\n", prettyNameForNPError(err))
#define LOG_PLUGIN_NET_ERROR() LOG_VERBOSE(Plugins, "Stream failed due to problems with network, disk I/O, lack of memory, or other problems.\n")

#if !LOG_DISABLED

namespace WebCore {

const char* prettyNameForNPError(NPError error);

CString prettyNameForNPNVariable(NPNVariable variable);
CString prettyNameForNPPVariable(NPPVariable variable, void* value);
CString prettyNameForNPNURLVariable(NPNURLVariable variable);

#ifdef XP_MACOSX
const char* prettyNameForDrawingModel(NPDrawingModel drawingModel);
const char* prettyNameForEventModel(NPEventModel eventModel);
#endif

} // namespace WebCore

#endif // !LOG_DISABLED

#endif // PluginDebug_h
