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

#include "config.h"
#include "ScriptedAnimationController.h"

#if ENABLE(REQUEST_ANIMATION_FRAME)

#include "Document.h"
#include "DocumentLoader.h"
#include "FrameView.h"
#include "InspectorInstrumentation.h"
#include "RequestAnimationFrameCallback.h"
#include "Settings.h"

#if USE(REQUEST_ANIMATION_FRAME_TIMER)
#include <algorithm>
#include <wtf/CurrentTime.h>

using namespace std;

// Allow a little more than 60fps to make sure we can at least hit that frame rate.
#define MinimumAnimationInterval 0.015
#define MinimumThrottledAnimationInterval 10
#endif

namespace WebCore {

ScriptedAnimationController::ScriptedAnimationController(Document* document, PlatformDisplayID displayID)
    : m_document(document)
    , m_nextCallbackId(0)
    , m_suspendCount(0)
#if USE(REQUEST_ANIMATION_FRAME_TIMER)
    , m_animationTimer(this, &ScriptedAnimationController::animationTimerFired)
    , m_lastAnimationFrameTimeMonotonic(0)
#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    , m_isUsingTimer(false)
    , m_isThrottled(false)
#endif
#endif
{
    windowScreenDidChange(displayID);
}

ScriptedAnimationController::~ScriptedAnimationController()
{
}

void ScriptedAnimationController::suspend()
{
    ++m_suspendCount;
}

void ScriptedAnimationController::resume()
{
    // It would be nice to put an ASSERT(m_suspendCount > 0) here, but in WK1 resume() can be called
    // even when suspend hasn't (if a tab was created in the background).
    if (m_suspendCount > 0)
        --m_suspendCount;

    if (!m_suspendCount && m_callbacks.size())
        scheduleAnimation();
}

void ScriptedAnimationController::setThrottled(bool isThrottled)
{
#if USE(REQUEST_ANIMATION_FRAME_TIMER) && USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    if (m_isThrottled == isThrottled)
        return;

    m_isThrottled = isThrottled;
    if (m_animationTimer.isActive()) {
        m_animationTimer.stop();
        scheduleAnimation();
    }
#endif
}

ScriptedAnimationController::CallbackId ScriptedAnimationController::registerCallback(PassRefPtr<RequestAnimationFrameCallback> callback)
{
    ScriptedAnimationController::CallbackId id = ++m_nextCallbackId;
    callback->m_firedOrCancelled = false;
    callback->m_id = id;
    m_callbacks.append(callback);

    InspectorInstrumentation::didRequestAnimationFrame(m_document, id);

    if (!m_suspendCount)
        scheduleAnimation();
    return id;
}

void ScriptedAnimationController::cancelCallback(CallbackId id)
{
    for (size_t i = 0; i < m_callbacks.size(); ++i) {
        if (m_callbacks[i]->m_id == id) {
            m_callbacks[i]->m_firedOrCancelled = true;
            InspectorInstrumentation::didCancelAnimationFrame(m_document, id);
            m_callbacks.remove(i);
            return;
        }
    }
}

void ScriptedAnimationController::serviceScriptedAnimations(double monotonicTimeNow)
{
    if (!m_callbacks.size() || m_suspendCount || (m_document->settings() && !m_document->settings()->requestAnimationFrameEnabled()))
        return;

    double highResNowMs = 1000.0 * m_document->loader()->timing()->monotonicTimeToZeroBasedDocumentTime(monotonicTimeNow);
    double legacyHighResNowMs = 1000.0 * m_document->loader()->timing()->monotonicTimeToPseudoWallTime(monotonicTimeNow);

    // First, generate a list of callbacks to consider.  Callbacks registered from this point
    // on are considered only for the "next" frame, not this one.
    CallbackList callbacks(m_callbacks);

    // Invoking callbacks may detach elements from our document, which clears the document's
    // reference to us, so take a defensive reference.
    RefPtr<ScriptedAnimationController> protector(this);

    for (size_t i = 0; i < callbacks.size(); ++i) {
        RequestAnimationFrameCallback* callback = callbacks[i].get();
        if (!callback->m_firedOrCancelled) {
            callback->m_firedOrCancelled = true;
            InspectorInstrumentationCookie cookie = InspectorInstrumentation::willFireAnimationFrame(m_document, callback->m_id);
            if (callback->m_useLegacyTimeBase)
                callback->handleEvent(legacyHighResNowMs);
            else
                callback->handleEvent(highResNowMs);
            InspectorInstrumentation::didFireAnimationFrame(cookie);
        }
    }

    // Remove any callbacks we fired from the list of pending callbacks.
    for (size_t i = 0; i < m_callbacks.size();) {
        if (m_callbacks[i]->m_firedOrCancelled)
            m_callbacks.remove(i);
        else
            ++i;
    }

    if (m_callbacks.size())
        scheduleAnimation();
}

void ScriptedAnimationController::windowScreenDidChange(PlatformDisplayID displayID)
{
    if (m_document->settings() && !m_document->settings()->requestAnimationFrameEnabled())
        return;
#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    DisplayRefreshMonitorManager::sharedManager()->windowScreenDidChange(displayID, this);
#else
    UNUSED_PARAM(displayID);
#endif
}

void ScriptedAnimationController::scheduleAnimation()
{
    if (!m_document || (m_document->settings() && !m_document->settings()->requestAnimationFrameEnabled()))
        return;

#if USE(REQUEST_ANIMATION_FRAME_TIMER)
#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    if (!m_isUsingTimer && !m_isThrottled) {
        if (DisplayRefreshMonitorManager::sharedManager()->scheduleAnimation(this))
            return;

        m_isUsingTimer = true;
    }
#endif
    if (m_animationTimer.isActive())
        return;

    double animationInterval = MinimumAnimationInterval;
#if USE(REQUEST_ANIMATION_FRAME_TIMER) && USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    if (m_isThrottled)
        animationInterval = MinimumThrottledAnimationInterval;
#endif

    double scheduleDelay = max<double>(animationInterval - (monotonicallyIncreasingTime() - m_lastAnimationFrameTimeMonotonic), 0);
    m_animationTimer.startOneShot(scheduleDelay);
#else
    if (FrameView* frameView = m_document->view())
        frameView->scheduleAnimation();
#endif
}

#if USE(REQUEST_ANIMATION_FRAME_TIMER)
void ScriptedAnimationController::animationTimerFired(Timer<ScriptedAnimationController>*)
{
    m_lastAnimationFrameTimeMonotonic = monotonicallyIncreasingTime();
    serviceScriptedAnimations(m_lastAnimationFrameTimeMonotonic);
}
#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
void ScriptedAnimationController::displayRefreshFired(double monotonicTimeNow)
{
    serviceScriptedAnimations(monotonicTimeNow);
}
#endif
#endif



}

#endif
