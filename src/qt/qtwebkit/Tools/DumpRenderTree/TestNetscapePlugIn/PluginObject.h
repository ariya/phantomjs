/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef PluginObject_h
#define PluginObject_h

#include <WebKit/npfunctions.h>
#include <stdarg.h>

class PluginTest;

extern NPNetscapeFuncs *browser;
extern NPPluginFuncs* pluginFunctions;

typedef struct {
    NPObject header;

    PluginTest* pluginTest;

    NPP npp;
    NPBool eventLogging;
    NPBool logSetWindow;
    NPBool logDestroy;
    NPBool returnNegativeOneFromWrite;
    NPBool returnErrorFromNewStream;
    NPObject* testObject;
    NPObject* rememberedObject;
    NPStream* stream;
    NPBool testGetURLOnDestroy;
    NPBool testWindowOpen;
    NPBool testKeyboardFocusForPlugins;
    NPBool mouseDownForEvaluateScript;
    char* onStreamLoad;
    char* onStreamDestroy;
    char* onDestroy;
    char* onURLNotify;
    char* onSetWindow;
    char* onPaintEvent;
    char* firstUrl;
    char* firstHeaders;
    char* lastUrl;
    char* lastHeaders;
    char* evaluateScriptOnMouseDownOrKeyDown;
#ifdef XP_MACOSX
    NPEventModel eventModel;
#endif
#ifdef XP_MACOSX
    void* coreAnimationLayer;
#endif
    NPWindow lastWindow;
} PluginObject;

extern NPClass *getPluginClass(void);
extern void handleCallback(PluginObject* object, const char *url, NPReason reason, void *notifyData);
extern void notifyStream(PluginObject* object, const char *url, const char *headers);
extern void testNPRuntime(NPP npp);
extern void pluginLog(NPP instance, const char* format, ...);
extern void pluginLogWithArguments(NPP instance, const char* format, va_list args);
extern bool testDocumentOpen(NPP npp);
extern bool testWindowOpen(NPP npp);

#ifdef XP_MACOSX
extern void* createCoreAnimationLayer();
#endif

#endif // PluginObject_h
