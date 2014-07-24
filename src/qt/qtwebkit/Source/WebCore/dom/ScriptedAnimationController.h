/*
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
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
 *  THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 *  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef ScriptedAnimationController_h
#define ScriptedAnimationController_h

#if ENABLE(REQUEST_ANIMATION_FRAME)
#include "DOMTimeStamp.h"
#if USE(REQUEST_ANIMATION_FRAME_TIMER)
#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
#include "DisplayRefreshMonitor.h"
#endif
#include "Timer.h"
#endif
#include "PlatformScreen.h"
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class Document;
class RequestAnimationFrameCallback;

class ScriptedAnimationController : public RefCounted<ScriptedAnimationController>
#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    , public DisplayRefreshMonitorClient
#endif
{
public:
    static PassRefPtr<ScriptedAnimationController> create(Document* document, PlatformDisplayID displayID)
    {
        return adoptRef(new ScriptedAnimationController(document, displayID));
    }
    ~ScriptedAnimationController();
    void clearDocumentPointer() { m_document = 0; }

    typedef int CallbackId;

    CallbackId registerCallback(PassRefPtr<RequestAnimationFrameCallback>);
    void cancelCallback(CallbackId);
    void serviceScriptedAnimations(double monotonicTimeNow);

    void suspend();
    void resume();
    void setThrottled(bool);

    void windowScreenDidChange(PlatformDisplayID);

private:
    ScriptedAnimationController(Document*, PlatformDisplayID);

    typedef Vector<RefPtr<RequestAnimationFrameCallback> > CallbackList;
    CallbackList m_callbacks;

    Document* m_document;
    CallbackId m_nextCallbackId;
    int m_suspendCount;

    void scheduleAnimation();

#if USE(REQUEST_ANIMATION_FRAME_TIMER)
    void animationTimerFired(Timer<ScriptedAnimationController>*);
    Timer<ScriptedAnimationController> m_animationTimer;
    double m_lastAnimationFrameTimeMonotonic;

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    // Override for DisplayRefreshMonitorClient
    virtual void displayRefreshFired(double timestamp);

    bool m_isUsingTimer;
    bool m_isThrottled;
#endif
#endif
};

}

#endif // ENABLE(REQUEST_ANIMATION_FRAME)

#endif // ScriptedAnimationController_h
