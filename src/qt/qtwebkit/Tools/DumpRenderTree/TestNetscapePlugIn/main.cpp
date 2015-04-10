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

#include "PluginObject.h"

#include "PluginTest.h"
#include <cstdlib>
#include <cstring>
#include <string>

#ifdef XP_UNIX
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#if !defined(NP_NO_CARBON) && defined(QD_HEADERS_ARE_PRIVATE) && QD_HEADERS_ARE_PRIVATE
extern "C" void GlobalToLocal(Point*);
#endif

using namespace std;

#define CRASH() do { \
    *(int *)(uintptr_t)0xbbadbeef = 0; \
    ((void(*)())0)(); /* More reliable, but doesn't say BBADBEEF */ \
} while(false)

static bool getEntryPointsWasCalled;
static bool initializeWasCalled;

#if defined(XP_WIN)
#define STDCALL __stdcall

static inline int strcasecmp(const char* s1, const char* s2)
{
    return _stricmp(s1, s2);
}

#else
#define STDCALL
#endif

extern "C" {
NPError STDCALL NP_GetEntryPoints(NPPluginFuncs *pluginFuncs);
}

// Entry points
extern "C"
NPError STDCALL NP_Initialize(NPNetscapeFuncs *browserFuncs
#ifdef XP_UNIX
                              , NPPluginFuncs *pluginFuncs
#endif
                              )
{
    initializeWasCalled = true;

#if defined(XP_WIN)
    // Simulate Flash and QuickTime's behavior of crashing when NP_Initialize is called before NP_GetEntryPoints.
    if (!getEntryPointsWasCalled)
        CRASH();
#endif

    browser = browserFuncs;

#ifdef XP_UNIX
    return NP_GetEntryPoints(pluginFuncs);
#else
    return NPERR_NO_ERROR;
#endif
}

extern "C"
NPError STDCALL NP_GetEntryPoints(NPPluginFuncs *pluginFuncs)
{
    getEntryPointsWasCalled = true;

#ifdef XP_MACOSX
    // Simulate Silverlight's behavior of crashing when NP_GetEntryPoints is called before NP_Initialize.
    if (!initializeWasCalled)
        CRASH();
#endif

    pluginFunctions = pluginFuncs;

    pluginFuncs->version = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
    pluginFuncs->size = sizeof(pluginFuncs);
    pluginFuncs->newp = NPP_New;
    pluginFuncs->destroy = NPP_Destroy;
    pluginFuncs->setwindow = NPP_SetWindow;
    pluginFuncs->newstream = NPP_NewStream;
    pluginFuncs->destroystream = NPP_DestroyStream;
    pluginFuncs->asfile = NPP_StreamAsFile;
    pluginFuncs->writeready = NPP_WriteReady;
    pluginFuncs->write = (NPP_WriteProcPtr)NPP_Write;
    pluginFuncs->print = NPP_Print;
    pluginFuncs->event = NPP_HandleEvent;
    pluginFuncs->urlnotify = NPP_URLNotify;
    pluginFuncs->getvalue = NPP_GetValue;
    pluginFuncs->setvalue = NPP_SetValue;
    
    return NPERR_NO_ERROR;
}

extern "C"
void STDCALL NP_Shutdown(void)
{
    PluginTest::NP_Shutdown();
}

static void executeScript(const PluginObject* obj, const char* script);

NPError NPP_New(NPMIMEType pluginType, NPP instance, uint16_t mode, int16_t argc, char *argn[], char *argv[], NPSavedData *saved)
{
#ifdef XP_MACOSX
    NPEventModel eventModel;
    
    // Always turn on the CG model
    NPBool supportsCoreGraphics;
    if (browser->getvalue(instance, NPNVsupportsCoreGraphicsBool, &supportsCoreGraphics) != NPERR_NO_ERROR)
        supportsCoreGraphics = false;
    
    if (!supportsCoreGraphics)
        return NPERR_INCOMPATIBLE_VERSION_ERROR;

    NPDrawingModel drawingModelToUse = NPDrawingModelCoreGraphics;

    NPBool supportsCoreAnimation;
    if (browser->getvalue(instance, NPNVsupportsCoreAnimationBool, &supportsCoreAnimation) != NPERR_NO_ERROR)
        supportsCoreAnimation = false;

#ifndef NP_NO_CARBON
    NPBool supportsCarbon = false;
#endif
    NPBool supportsCocoa = false;

#ifndef NP_NO_CARBON
    // A browser that doesn't know about NPNVsupportsCarbonBool is one that only supports Carbon event model.
    if (browser->getvalue(instance, NPNVsupportsCarbonBool, &supportsCarbon) != NPERR_NO_ERROR)
        supportsCarbon = true;
#endif

    if (browser->getvalue(instance, NPNVsupportsCocoaBool, &supportsCocoa) != NPERR_NO_ERROR)
        supportsCocoa = false;

    if (supportsCocoa) {
        eventModel = NPEventModelCocoa;
#ifndef NP_NO_CARBON
    } else if (supportsCarbon) {
        eventModel = NPEventModelCarbon;
#endif
    } else {
        return NPERR_INCOMPATIBLE_VERSION_ERROR;
    }

     browser->setvalue(instance, NPPVpluginEventModel, (void *)eventModel);
#endif // XP_MACOSX

    PluginObject* obj = (PluginObject*)browser->createobject(instance, getPluginClass());
    instance->pdata = obj;

#ifdef XP_MACOSX
    obj->eventModel = eventModel;
    obj->coreAnimationLayer = 0;
#endif // XP_MACOSX

    string testIdentifier;
    const char* onNewScript = 0;
    
    for (int i = 0; i < argc; i++) {
        if (strcasecmp(argn[i], "test") == 0)
            testIdentifier = argv[i];
        if (strcasecmp(argn[i], "onstreamload") == 0 && !obj->onStreamLoad)
            obj->onStreamLoad = strdup(argv[i]);
        else if (strcasecmp(argn[i], "onStreamDestroy") == 0 && !obj->onStreamDestroy)
            obj->onStreamDestroy = strdup(argv[i]);
        else if (strcasecmp(argn[i], "onURLNotify") == 0 && !obj->onURLNotify)
            obj->onURLNotify = strdup(argv[i]);
        else if (strcasecmp(argn[i], "src") == 0 &&
                 strcasecmp(argv[i], "data:application/x-webkit-test-netscape,returnerrorfromnewstream") == 0)
            obj->returnErrorFromNewStream = TRUE;
        else if (strcasecmp(argn[i], "src") == 0 &&
                 strcasecmp(argv[i], "data:application/x-webkit-test-netscape,alertwhenloaded") == 0)
            executeScript(obj, "alert('Plugin Loaded!')");
        else if (strcasecmp(argn[i], "src") == 0 &&
                 strcasecmp(argv[i], "data:application/x-webkit-test-netscape,logifloaded") == 0) {
            for (int j = 0; j < argc; j++) {
              if (strcasecmp(argn[j], "log") == 0) {
                int length = 26 + strlen(argv[j]) + 1;
                char* buffer = (char*) malloc(length);
                snprintf(buffer, length, "xWebkitTestNetscapeLog('%s')", argv[j]);
                executeScript(obj, buffer);
                free(buffer);
              }
            }
        } else if (strcasecmp(argn[i], "onSetWindow") == 0 && !obj->onSetWindow)
            obj->onSetWindow = strdup(argv[i]);
        else if (strcasecmp(argn[i], "onNew") == 0 && !onNewScript)
            onNewScript = argv[i];
        else if (strcasecmp(argn[i], "onPaintEvent") == 0 && !obj->onPaintEvent)
            obj->onPaintEvent = strdup(argv[i]);
        else if (strcasecmp(argn[i], "logfirstsetwindow") == 0)
            obj->logSetWindow = TRUE;
        else if (strcasecmp(argn[i], "testnpruntime") == 0)
            testNPRuntime(instance);
        else if (strcasecmp(argn[i], "logSrc") == 0) {
            for (int i = 0; i < argc; i++)
                if (strcasecmp(argn[i], "src") == 0)
                    pluginLog(instance, "src: %s", argv[i]);
        } else if (strcasecmp(argn[i], "cleardocumentduringnew") == 0)
            executeScript(obj, "document.body.innerHTML = ''");
        else if (!strcasecmp(argn[i], "ondestroy"))
            obj->onDestroy = strdup(argv[i]);
        else if (strcasecmp(argn[i], "testwindowopen") == 0)
            obj->testWindowOpen = TRUE;
        else if (strcasecmp(argn[i], "drawingmodel") == 0) {
#ifdef XP_MACOSX
            const char* value = argv[i];
            if (strcasecmp(value, "coreanimation") == 0) {
                if (supportsCoreAnimation)
                    drawingModelToUse = NPDrawingModelCoreAnimation;
                else
                    return NPERR_INCOMPATIBLE_VERSION_ERROR;
             } else if (strcasecmp(value, "coregraphics") == 0) {
                if (supportsCoreGraphics)
                    drawingModelToUse = NPDrawingModelCoreGraphics;
                else
                    return NPERR_INCOMPATIBLE_VERSION_ERROR;
             } else
                return NPERR_INCOMPATIBLE_VERSION_ERROR;
#endif
        } else if (strcasecmp(argn[i], "testGetURLOnDestroy") == 0) {
#if defined(XP_WIN)
            // FIXME: When https://bugs.webkit.org/show_bug.cgi?id=41831 is fixed, this #ifdef can be removed.
            obj->testGetURLOnDestroy = TRUE;
#endif
        } else if (!strcasecmp(argn[i], "src") && strstr(argv[i], "plugin-document-has-focus.pl"))
            obj->testKeyboardFocusForPlugins = TRUE;
        else if (!strcasecmp(argn[i], "evaluatescript")) {
            char* script = argv[i];
            if (script == strstr(script, "mouse::")) {
                obj->mouseDownForEvaluateScript = true;
                obj->evaluateScriptOnMouseDownOrKeyDown = strdup(script + sizeof("mouse::") - 1);
            } else if (script == strstr(script, "key::")) {
                obj->evaluateScriptOnMouseDownOrKeyDown = strdup(script + sizeof("key::") - 1);
            }
            // When testing evaluate script on mouse-down or key-down, allow event logging to handle events.
            if (obj->evaluateScriptOnMouseDownOrKeyDown)
                obj->eventLogging = true;
        } else if (!strcasecmp(argn[i], "windowedPlugin")) {
            void* windowed = 0;
            if (!strcasecmp(argv[i], "false") || !strcasecmp(argv[i], "0"))
                windowed = 0;
            else if (!strcasecmp(argv[i], "true") || !strcasecmp(argv[i], "1"))
                windowed = reinterpret_cast<void*>(1);
            else
                assert(false);
            browser->setvalue(instance, NPPVpluginWindowBool, windowed);
        }
    }

#ifdef XP_MACOSX
    browser->setvalue(instance, NPPVpluginDrawingModel, (void *)drawingModelToUse);
    if (drawingModelToUse == NPDrawingModelCoreAnimation)
        obj->coreAnimationLayer = createCoreAnimationLayer();
#endif

    obj->pluginTest = PluginTest::create(instance, testIdentifier);

    if (!obj->pluginTest) {
        pluginLog(instance, "NPP_New: Could not find a test named \"%s\", maybe its .cpp file wasn't added to the build system?", testIdentifier.c_str());
        return NPERR_GENERIC_ERROR;
    }

    if (onNewScript)
        executeScript(obj, onNewScript);

    return obj->pluginTest->NPP_New(pluginType, mode, argc, argn, argv, saved);
}

NPError NPP_Destroy(NPP instance, NPSavedData **save)
{
    PluginObject* obj = static_cast<PluginObject*>(instance->pdata);
    if (obj) {
        if (obj->testGetURLOnDestroy)
            browser->geturlnotify(obj->npp, "about:blank", "", 0);

        if (obj->onDestroy) {
            executeScript(obj, obj->onDestroy);
            free(obj->onDestroy);
        }

        if (obj->onStreamLoad)
            free(obj->onStreamLoad);

        if (obj->onStreamDestroy)
            free(obj->onStreamDestroy);

        if (obj->onURLNotify)
            free(obj->onURLNotify);

        if (obj->onSetWindow)
            free(obj->onSetWindow);

        if (obj->onPaintEvent)
            free(obj->onPaintEvent);
        
        if (obj->logDestroy)
            pluginLog(instance, "NPP_Destroy");

#ifdef XP_MACOSX
        if (obj->coreAnimationLayer)
            CFRelease(obj->coreAnimationLayer);
#endif

        if (obj->pluginTest)
            obj->pluginTest->NPP_Destroy(save);

        browser->releaseobject(&obj->header);
    }
    return NPERR_NO_ERROR;
}

NPError NPP_SetWindow(NPP instance, NPWindow *window)
{
    PluginObject* obj = static_cast<PluginObject*>(instance->pdata);

    if (obj) {
        obj->lastWindow = *window;

        if (obj->logSetWindow) {
            pluginLog(instance, "NPP_SetWindow: %d %d", (int)window->width, (int)window->height);
            obj->logSetWindow = FALSE;
            executeScript(obj, "testRunner.notifyDone();");
        }

        if (obj->onSetWindow)
            executeScript(obj, obj->onSetWindow);

        if (obj->testWindowOpen) {
            testWindowOpen(instance);
            obj->testWindowOpen = FALSE;
        }

        if (obj->testKeyboardFocusForPlugins) {
            obj->eventLogging = true;
            executeScript(obj, "eventSender.keyDown('A');");
        }
    }
    
    return obj->pluginTest->NPP_SetWindow(window);
}

static void executeScript(const PluginObject* obj, const char* script)
{
    NPObject *windowScriptObject;
    browser->getvalue(obj->npp, NPNVWindowNPObject, &windowScriptObject);

    NPString npScript;
    npScript.UTF8Characters = script;
    npScript.UTF8Length = strlen(script);

    NPVariant browserResult;
    browser->evaluate(obj->npp, windowScriptObject, &npScript, &browserResult);
    browser->releasevariantvalue(&browserResult);
}

NPError NPP_NewStream(NPP instance, NPMIMEType type, NPStream *stream, NPBool seekable, uint16_t *stype)
{
    PluginObject* obj = static_cast<PluginObject*>(instance->pdata);
    obj->stream = stream;
    *stype = NP_NORMAL;

    if (obj->returnErrorFromNewStream)
        return NPERR_GENERIC_ERROR;
    
    if (browser->version >= NPVERS_HAS_RESPONSE_HEADERS)
        notifyStream(obj, stream->url, stream->headers);

    if (obj->onStreamLoad)
        executeScript(obj, obj->onStreamLoad);

    return obj->pluginTest->NPP_NewStream(type, stream, seekable, stype);
}

NPError NPP_DestroyStream(NPP instance, NPStream *stream, NPReason reason)
{
    PluginObject* obj = (PluginObject*)instance->pdata;

    if (obj->onStreamDestroy) {
        NPObject* windowObject = 0;
        NPError error = browser->getvalue(instance, NPNVWindowNPObject, &windowObject);
        
        if (error == NPERR_NO_ERROR) {
            NPVariant onStreamDestroyVariant;
            if (browser->getproperty(instance, windowObject, browser->getstringidentifier(obj->onStreamDestroy), &onStreamDestroyVariant)) {
                if (NPVARIANT_IS_OBJECT(onStreamDestroyVariant)) {
                    NPObject* onStreamDestroyFunction = NPVARIANT_TO_OBJECT(onStreamDestroyVariant);

                    NPVariant reasonVariant;
                    INT32_TO_NPVARIANT(reason, reasonVariant);

                    NPVariant result;
                    browser->invokeDefault(instance, onStreamDestroyFunction, &reasonVariant, 1, &result);
                    browser->releasevariantvalue(&result);
                }
                browser->releasevariantvalue(&onStreamDestroyVariant);
            }
            browser->releaseobject(windowObject);
        }
    }

    return obj->pluginTest->NPP_DestroyStream(stream, reason);
}

int32_t NPP_WriteReady(NPP instance, NPStream *stream)
{
    PluginObject* obj = (PluginObject*)instance->pdata;
    return obj->pluginTest->NPP_WriteReady(stream);
}

int32_t NPP_Write(NPP instance, NPStream *stream, int32_t offset, int32_t len, void *buffer)
{
    PluginObject* obj = (PluginObject*)instance->pdata;

    if (obj->returnNegativeOneFromWrite)
        return -1;

    return obj->pluginTest->NPP_Write(stream, offset, len, buffer);
}

void NPP_StreamAsFile(NPP instance, NPStream *stream, const char *fname)
{
}

void NPP_Print(NPP instance, NPPrint *platformPrint)
{
}

#ifdef XP_MACOSX
#ifndef NP_NO_CARBON
static int16_t handleEventCarbon(NPP instance, PluginObject* obj, EventRecord* event)
{
    Point pt = { event->where.v, event->where.h };

    switch (event->what) {
        case nullEvent:
            // these are delivered non-deterministically, don't log.
            break;
        case mouseDown:
            if (obj->eventLogging) {
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
                GlobalToLocal(&pt);
#if __clang__
#pragma clang diagnostic pop
#endif
                pluginLog(instance, "mouseDown at (%d, %d)", pt.h, pt.v);
            }
            if (obj->evaluateScriptOnMouseDownOrKeyDown && obj->mouseDownForEvaluateScript)
                executeScript(obj, obj->evaluateScriptOnMouseDownOrKeyDown);
            break;
        case mouseUp:
            if (obj->eventLogging) {
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
                GlobalToLocal(&pt);
#if __clang__
#pragma clang diagnostic pop
#endif
                pluginLog(instance, "mouseUp at (%d, %d)", pt.h, pt.v);
            }
            break;
        case keyDown:
            if (obj->eventLogging)
                pluginLog(instance, "keyDown '%c'", (char)(event->message & 0xFF));
            if (obj->evaluateScriptOnMouseDownOrKeyDown && !obj->mouseDownForEvaluateScript)
                executeScript(obj, obj->evaluateScriptOnMouseDownOrKeyDown);
            break;
        case keyUp:
            if (obj->eventLogging)
                pluginLog(instance, "keyUp '%c'", (char)(event->message & 0xFF));
            if (obj->testKeyboardFocusForPlugins) {
                obj->eventLogging = false;
                obj->testKeyboardFocusForPlugins = FALSE;
                executeScript(obj, "testRunner.notifyDone();");
            }
            break;
        case autoKey:
            if (obj->eventLogging)
                pluginLog(instance, "autoKey '%c'", (char)(event->message & 0xFF));
            break;
        case updateEvt:
            if (obj->eventLogging)
                pluginLog(instance, "updateEvt");
            break;
        case diskEvt:
            if (obj->eventLogging)
                pluginLog(instance, "diskEvt");
            break;
        case activateEvt:
            if (obj->eventLogging)
                pluginLog(instance, "activateEvt");
            break;
        case osEvt:
            if (!obj->eventLogging)
                break;
            printf("PLUGIN: osEvt - ");
            switch ((event->message & 0xFF000000) >> 24) {
                case suspendResumeMessage:
                    printf("%s\n", (event->message & 0x1) ? "resume" : "suspend");
                    break;
                case mouseMovedMessage:
                    printf("mouseMoved\n");
                    break;
                default:
                    printf("%08lX\n", event->message);
            }
            break;
        case kHighLevelEvent:
            if (obj->eventLogging)
                pluginLog(instance, "kHighLevelEvent");
            break;
        // NPAPI events
        case NPEventType_GetFocusEvent:
            if (obj->eventLogging)
                pluginLog(instance, "getFocusEvent");
            break;
        case NPEventType_LoseFocusEvent:
            if (obj->eventLogging)
                pluginLog(instance, "loseFocusEvent");
            break;
        case NPEventType_AdjustCursorEvent:
            if (obj->eventLogging)
                pluginLog(instance, "adjustCursorEvent");
            break;
        default:
            if (obj->eventLogging)
                pluginLog(instance, "event %d", event->what);
    }
    
    return 0;
}
#endif

static int16_t handleEventCocoa(NPP instance, PluginObject* obj, NPCocoaEvent* event)
{
    switch (event->type) {
        case NPCocoaEventWindowFocusChanged:
            
        case NPCocoaEventFocusChanged:
            if (obj->eventLogging) {
                if (event->data.focus.hasFocus)
                    pluginLog(instance, "getFocusEvent");
                else
                    pluginLog(instance, "loseFocusEvent");
            }
            return 1;

        case NPCocoaEventDrawRect: {
            if (obj->onPaintEvent)
                executeScript(obj, obj->onPaintEvent);
            return 1;
        }

        case NPCocoaEventKeyDown:
            if (obj->eventLogging && event->data.key.characters)
                pluginLog(instance, "keyDown '%c'", CFStringGetCharacterAtIndex(reinterpret_cast<CFStringRef>(event->data.key.characters), 0));
            if (obj->evaluateScriptOnMouseDownOrKeyDown && !obj->mouseDownForEvaluateScript)
                executeScript(obj, obj->evaluateScriptOnMouseDownOrKeyDown);
            return 1;

        case NPCocoaEventKeyUp:
            if (obj->eventLogging && event->data.key.characters) {
                pluginLog(instance, "keyUp '%c'", CFStringGetCharacterAtIndex(reinterpret_cast<CFStringRef>(event->data.key.characters), 0));
                if (obj->testKeyboardFocusForPlugins) {
                    obj->eventLogging = false;
                    obj->testKeyboardFocusForPlugins = FALSE;
                    executeScript(obj, "testRunner.notifyDone();");
                }
            }
            return 1;

        case NPCocoaEventFlagsChanged:
            return 1;

        case NPCocoaEventMouseDown:
            if (obj->eventLogging) {
                pluginLog(instance, "mouseDown at (%d, %d)", 
                       (int)event->data.mouse.pluginX,
                       (int)event->data.mouse.pluginY);
            }
            if (obj->evaluateScriptOnMouseDownOrKeyDown && obj->mouseDownForEvaluateScript)
                executeScript(obj, obj->evaluateScriptOnMouseDownOrKeyDown);
            return 1;
        case NPCocoaEventMouseUp:
            if (obj->eventLogging) {
                pluginLog(instance, "mouseUp at (%d, %d)", 
                       (int)event->data.mouse.pluginX,
                       (int)event->data.mouse.pluginY);
            }
            return 1;
            
        case NPCocoaEventMouseMoved:
        case NPCocoaEventMouseEntered:
        case NPCocoaEventMouseExited:
        case NPCocoaEventMouseDragged:
        case NPCocoaEventScrollWheel:
        case NPCocoaEventTextInput:
            return 1;
    }
    
    return 0;
}

#endif // XP_MACOSX

#ifdef XP_UNIX

static char keyEventToChar(XKeyEvent* event)
{
    char c = ' ';
    XLookupString(event, &c, sizeof(c), 0, 0);
    return c;
}

static int16_t handleEventX11(NPP instance, PluginObject* obj, XEvent* event)
{
    switch (event->type) {
    case ButtonPress:
        if (obj->eventLogging)
            pluginLog(instance, "mouseDown at (%d, %d)", event->xbutton.x, event->xbutton.y);
        if (obj->evaluateScriptOnMouseDownOrKeyDown && obj->mouseDownForEvaluateScript)
            executeScript(obj, obj->evaluateScriptOnMouseDownOrKeyDown);
        break;
    case ButtonRelease:
        if (obj->eventLogging)
            pluginLog(instance, "mouseUp at (%d, %d)", event->xbutton.x, event->xbutton.y);
        break;
    case KeyPress:
        // FIXME: extract key code
        if (obj->eventLogging)
            pluginLog(instance, "keyDown '%c'", keyEventToChar(&event->xkey));
        if (obj->evaluateScriptOnMouseDownOrKeyDown && !obj->mouseDownForEvaluateScript)
            executeScript(obj, obj->evaluateScriptOnMouseDownOrKeyDown);
        break;
    case KeyRelease:
        // FIXME: extract key code
        if (obj->eventLogging)
            pluginLog(instance, "keyUp '%c'", keyEventToChar(&event->xkey));
        if (obj->testKeyboardFocusForPlugins) {
            obj->eventLogging = false;
            obj->testKeyboardFocusForPlugins = FALSE;
            executeScript(obj, "testRunner.notifyDone();");
        }
        break;
    case GraphicsExpose:
        if (obj->eventLogging)
            pluginLog(instance, "updateEvt");
        if (obj->onPaintEvent)
            executeScript(obj, obj->onPaintEvent);
        break;
    // NPAPI events
    case FocusIn:
        if (obj->eventLogging)
            pluginLog(instance, "getFocusEvent");
        break;
    case FocusOut:
        if (obj->eventLogging)
            pluginLog(instance, "loseFocusEvent");
        break;
    case EnterNotify:
    case LeaveNotify:
    case MotionNotify:
        break;
    default:
        if (obj->eventLogging)
            pluginLog(instance, "event %d", event->type);
    }

    fflush(stdout);
    return 0;
}
#endif // XP_UNIX

#ifdef XP_WIN
static int16_t handleEventWin(NPP instance, PluginObject* obj, NPEvent* event)
{
    switch (event->event) {
    case WM_PAINT:
        if (obj->onPaintEvent)
            executeScript(obj, obj->onPaintEvent);
        break;
    case WM_KEYDOWN:
        if (obj->eventLogging)
            pluginLog(instance, "keyDown '%c'", event->wParam);
        if (obj->evaluateScriptOnMouseDownOrKeyDown && !obj->mouseDownForEvaluateScript)
            executeScript(obj, obj->evaluateScriptOnMouseDownOrKeyDown);
        break;
    case WM_CHAR:
        break;
    case WM_KEYUP:
        if (obj->eventLogging)
            pluginLog(instance, "keyUp '%c'", event->wParam);
        if (obj->testKeyboardFocusForPlugins) {
            obj->eventLogging = false;
            obj->testKeyboardFocusForPlugins = FALSE;
            executeScript(obj, "testRunner.notifyDone();");
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        if (obj->eventLogging)
            pluginLog(instance, "mouseDown at (%d, %d)", LOWORD(event->lParam), HIWORD(event->lParam));
        if (obj->evaluateScriptOnMouseDownOrKeyDown && obj->mouseDownForEvaluateScript)
            executeScript(obj, obj->evaluateScriptOnMouseDownOrKeyDown);
        break;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        if (obj->eventLogging)
            pluginLog(instance, "mouseUp at (%d, %d)", LOWORD(event->lParam), HIWORD(event->lParam));
        break;
    case WM_SETFOCUS:
        if (obj->eventLogging)
            pluginLog(instance, "getFocusEvent");
        break;
    case WM_KILLFOCUS:
        if (obj->eventLogging)
            pluginLog(instance, "loseFocusEvent");
        break;
    }
    return 0;
}
#endif // XP_WIN

int16_t NPP_HandleEvent(NPP instance, void *event)
{
    PluginObject* obj = static_cast<PluginObject*>(instance->pdata);

    if (obj->pluginTest->NPP_HandleEvent(event) == 1)
        return 1;

#ifdef XP_MACOSX
#ifndef NP_NO_CARBON
    if (obj->eventModel == NPEventModelCarbon)
        return handleEventCarbon(instance, obj, static_cast<EventRecord*>(event));
#endif

    assert(obj->eventModel == NPEventModelCocoa);
    return handleEventCocoa(instance, obj, static_cast<NPCocoaEvent*>(event));
#elif defined(XP_UNIX)
    return handleEventX11(instance, obj, static_cast<XEvent*>(event));
#elif defined(XP_WIN)
    return handleEventWin(instance, obj, static_cast<NPEvent*>(event));
#else
    // FIXME: Implement for other platforms.
    return 0;
#endif // XP_MACOSX
}

void NPP_URLNotify(NPP instance, const char *url, NPReason reason, void *notifyData)
{
    PluginObject* obj = static_cast<PluginObject*>(instance->pdata);
    if (obj->pluginTest->NPP_URLNotify(url, reason, notifyData))
        return;

    if (obj->onURLNotify)
         executeScript(obj, obj->onURLNotify);

    handleCallback(obj, url, reason, notifyData);
}

NPError NPP_GetValue(NPP instance, NPPVariable variable, void *value)
{
#ifdef XP_UNIX
    if (variable == NPPVpluginNameString) {
        *((char **)value) = const_cast<char*>("WebKit Test PlugIn");
        return NPERR_NO_ERROR;
    }
    if (variable == NPPVpluginDescriptionString) {
        *((char **)value) = const_cast<char*>("Simple Netscape® plug-in that handles test content for WebKit");
        return NPERR_NO_ERROR;
    }
    if (variable == NPPVpluginNeedsXEmbed) {
        *((NPBool *)value) = TRUE;
        return NPERR_NO_ERROR;
    }
#endif

    if (!instance)
        return NPERR_GENERIC_ERROR;
    PluginObject* obj = static_cast<PluginObject*>(instance->pdata);

    // First, check if the PluginTest object supports getting this value.
    if (obj->pluginTest->NPP_GetValue(variable, value) == NPERR_NO_ERROR)
        return NPERR_NO_ERROR;

    if (variable == NPPVpluginScriptableNPObject) {
        void **v = (void **)value;
        // Return value is expected to be retained
        browser->retainobject((NPObject *)obj);
        *v = obj;
        return NPERR_NO_ERROR;
    }
    
#ifdef XP_MACOSX
    if (variable == NPPVpluginCoreAnimationLayer) {
        if (!obj->coreAnimationLayer)
            return NPERR_GENERIC_ERROR;
        
        void **v = (void **)value;
        *v = (void*)CFRetain(obj->coreAnimationLayer);
        return NPERR_NO_ERROR;
    }
#endif

    return NPERR_GENERIC_ERROR;
}

NPError NPP_SetValue(NPP instance, NPNVariable variable, void *value)
{
    PluginObject* obj = static_cast<PluginObject*>(instance->pdata);
    return obj->pluginTest->NPP_SetValue(variable, value);
}

#ifdef XP_UNIX
extern "C"
const char* NP_GetMIMEDescription(void)
{
    return "application/x-webkit-test-netscape:testnetscape:test netscape content;image/png:png:PNG image";
}

extern "C"
NPError NP_GetValue(NPP instance, NPPVariable variable, void* value)
{
    return NPP_GetValue(instance, variable, value);
}
#endif
