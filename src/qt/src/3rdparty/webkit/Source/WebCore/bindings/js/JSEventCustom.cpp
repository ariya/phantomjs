/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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

#include "config.h"
#include "JSEvent.h"

#include "Clipboard.h"
#include "CompositionEvent.h"
#include "CustomEvent.h"
#include "DeviceMotionEvent.h"
#include "DeviceOrientationEvent.h"
#include "Event.h"
#include "JSBeforeLoadEvent.h"
#include "JSClipboard.h"
#include "JSCustomEvent.h"
#include "JSCompositionEvent.h"
#include "JSDeviceMotionEvent.h"
#include "JSDeviceOrientationEvent.h"
#include "JSErrorEvent.h"
#include "JSHashChangeEvent.h"
#include "JSKeyboardEvent.h"
#include "JSMessageEvent.h"
#include "JSMouseEvent.h"
#include "JSMutationEvent.h"
#include "JSOverflowEvent.h"
#include "JSPageTransitionEvent.h"
#include "JSPopStateEvent.h"
#include "JSProgressEvent.h"
#include "JSSpeechInputEvent.h"
#include "JSTextEvent.h"
#include "JSUIEvent.h"
#include "JSWebKitAnimationEvent.h"
#include "JSWebKitTransitionEvent.h"
#include "JSWheelEvent.h"
#include "JSXMLHttpRequestProgressEvent.h"
#include "BeforeLoadEvent.h"
#include "ErrorEvent.h"
#include "HashChangeEvent.h"
#include "KeyboardEvent.h"
#include "MessageEvent.h"
#include "MouseEvent.h"
#include "MutationEvent.h"
#include "OverflowEvent.h"
#include "PageTransitionEvent.h"
#include "PopStateEvent.h"
#include "ProgressEvent.h"
#include "SpeechInputEvent.h"
#include "TextEvent.h"
#include "UIEvent.h"
#include "WebKitAnimationEvent.h"
#include "WebKitTransitionEvent.h"
#include "WheelEvent.h"
#include "XMLHttpRequestProgressEvent.h"
#include <runtime/JSLock.h>

#if ENABLE(DOM_STORAGE)
#include "JSStorageEvent.h"
#include "StorageEvent.h"
#endif

#if ENABLE(SVG)
#include "JSSVGZoomEvent.h"
#include "SVGZoomEvent.h"
#endif

#if ENABLE(TOUCH_EVENTS)
#include "JSTouchEvent.h"
#include "TouchEvent.h"
#endif

#if ENABLE(INDEXED_DATABASE)
#include "IDBVersionChangeEvent.h"
#include "JSIDBVersionChangeEvent.h"
#endif

#if ENABLE(WEB_AUDIO)
#include "AudioProcessingEvent.h"
#include "JSAudioProcessingEvent.h"
#include "JSOfflineAudioCompletionEvent.h"
#include "OfflineAudioCompletionEvent.h"
#endif

using namespace JSC;

namespace WebCore {

JSValue JSEvent::clipboardData(ExecState* exec) const
{
    return impl()->isClipboardEvent() ? toJS(exec, globalObject(), impl()->clipboardData()) : jsUndefined();
}

JSValue toJS(ExecState* exec, JSDOMGlobalObject* globalObject, Event* event)
{
    JSLock lock(SilenceAssertionsOnly);

    if (!event)
        return jsNull();

    JSDOMWrapper* wrapper = getCachedWrapper(currentWorld(exec), event);
    if (wrapper)
        return wrapper;

    if (event->isUIEvent()) {
        if (event->isKeyboardEvent())
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, KeyboardEvent, event);
        else if (event->isTextEvent())
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, TextEvent, event);
        else if (event->isMouseEvent())
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, MouseEvent, event);
        else if (event->isWheelEvent())
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, WheelEvent, event);
#if ENABLE(SVG)
        else if (event->isSVGZoomEvent())
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, SVGZoomEvent, event);
#endif
        else if (event->isCompositionEvent())
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, CompositionEvent, event);
#if ENABLE(TOUCH_EVENTS)
        else if (event->isTouchEvent())
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, TouchEvent, event);
#endif
        else
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, UIEvent, event);
    } else if (event->isMutationEvent())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, MutationEvent, event);
    else if (event->isOverflowEvent())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, OverflowEvent, event);
    else if (event->isMessageEvent())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, MessageEvent, event);
    else if (event->isPageTransitionEvent())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, PageTransitionEvent, event);
    else if (event->isProgressEvent()) {
        if (event->isXMLHttpRequestProgressEvent())
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, XMLHttpRequestProgressEvent, event);
        else
            wrapper = CREATE_DOM_WRAPPER(exec, globalObject, ProgressEvent, event);
    } else if (event->isBeforeLoadEvent())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, BeforeLoadEvent, event);
#if ENABLE(DOM_STORAGE)
    else if (event->isStorageEvent())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, StorageEvent, event);
#endif
#if ENABLE(INDEXED_DATABASE)
    else if (event->isIDBVersionChangeEvent())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, IDBVersionChangeEvent, event);
#endif
    else if (event->isWebKitAnimationEvent())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, WebKitAnimationEvent, event);
    else if (event->isWebKitTransitionEvent())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, WebKitTransitionEvent, event);
#if ENABLE(WORKERS)
    else if (event->isErrorEvent())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, ErrorEvent, event);
#endif
    else if (event->isHashChangeEvent())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, HashChangeEvent, event);
    else if (event->isPopStateEvent())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, PopStateEvent, event);
    else if (event->isCustomEvent())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, CustomEvent, event);
#if ENABLE(DEVICE_ORIENTATION)
    else if (event->isDeviceMotionEvent())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, DeviceMotionEvent, event);
    else if (event->isDeviceOrientationEvent())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, DeviceOrientationEvent, event);
#endif
#if ENABLE(WEB_AUDIO)
    else if (event->isAudioProcessingEvent())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, AudioProcessingEvent, event);
    else if (event->isOfflineAudioCompletionEvent())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, OfflineAudioCompletionEvent, event);
#endif
#if ENABLE(INPUT_SPEECH)
    else if (event->isSpeechInputEvent())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, SpeechInputEvent, event);
#endif
    else
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, Event, event);

    return wrapper;
}

} // namespace WebCore
