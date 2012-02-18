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
#include "Element.h"
#include "FrameView.h"
#include "RequestAnimationFrameCallback.h"

namespace WebCore {

ScriptedAnimationController::ScriptedAnimationController(Document* document)
    : m_document(document)
    , m_nextCallbackId(0)
    , m_suspendCount(0)
{
}

void ScriptedAnimationController::suspend()
{
    ++m_suspendCount;
}

void ScriptedAnimationController::resume()
{
    --m_suspendCount;
    if (!m_suspendCount && m_callbacks.size())
        if (FrameView* fv = m_document->view())
            fv->scheduleAnimation();
}

ScriptedAnimationController::CallbackId ScriptedAnimationController::registerCallback(PassRefPtr<RequestAnimationFrameCallback> callback, Element* animationElement)
{
    ScriptedAnimationController::CallbackId id = m_nextCallbackId++;
    callback->m_firedOrCancelled = false;
    callback->m_id = id;
    callback->m_element = animationElement;
    m_callbacks.append(callback);
    if (!m_suspendCount)
        if (FrameView* view = m_document->view())
            view->scheduleAnimation();
    return id;
}

void ScriptedAnimationController::cancelCallback(CallbackId id)
{
    for (size_t i = 0; i < m_callbacks.size(); ++i) {
        if (m_callbacks[i]->m_id == id) {
            m_callbacks[i]->m_firedOrCancelled = true;
            m_callbacks.remove(i);
            return;
        }
    }
}

void ScriptedAnimationController::serviceScriptedAnimations(DOMTimeStamp time)
{
    if (!m_callbacks.size() || m_suspendCount)
        return;
    // We want to run the callback for all elements in the document that have registered
    // for a callback and that are visible.  Running the callbacks can cause new callbacks
    // to be registered, existing callbacks to be cancelled, and elements to gain or lose
    // visibility so this code has to iterate carefully.

    // FIXME: Currently, this code doesn't do any visibility tests beyond checking display:

    // First, generate a list of callbacks to consider.  Callbacks registered from this point
    // on are considered only for the "next" frame, not this one.
    CallbackList callbacks(m_callbacks);

    // Firing the callback may cause the visibility of other elements to change.  To avoid
    // missing any callbacks, we keep iterating through the list of candiate callbacks and firing
    // them until nothing new becomes visible.
    bool firedCallback;
    do {
        firedCallback = false;
        // A previous iteration may have invalidated style (or layout).  Update styles for each iteration
        // for now since all we check is the existence of a renderer.
        m_document->updateStyleIfNeeded();
        for (size_t i = 0; i < callbacks.size(); ++i) {
            RequestAnimationFrameCallback* callback = callbacks[i].get();
            if (!callback->m_firedOrCancelled && (!callback->m_element || callback->m_element->renderer())) {
                callback->m_firedOrCancelled = true;
                callback->handleEvent(time);
                firedCallback = true;
                callbacks.remove(i);
                break;
            }
        }
    } while (firedCallback);

    // Remove any callbacks we fired from the list of pending callbacks.
    for (size_t i = 0; i < m_callbacks.size();) {
        if (m_callbacks[i]->m_firedOrCancelled)
            m_callbacks.remove(i);
        else
            ++i;
    }

    if (m_callbacks.size())
        if (FrameView* view = m_document->view())
            view->scheduleAnimation();
}

}

#endif

