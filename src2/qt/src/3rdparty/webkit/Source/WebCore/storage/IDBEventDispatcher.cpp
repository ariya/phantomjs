/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "IDBEventDispatcher.h"

#if ENABLE(INDEXED_DATABASE)

#include "Event.h"
#include "EventTarget.h"

namespace WebCore {

bool IDBEventDispatcher::dispatch(Event* event, Vector<RefPtr<EventTarget> >& eventTargets)
{
    size_t size = eventTargets.size();
    ASSERT(size);

    event->setEventPhase(Event::CAPTURING_PHASE);
    for (size_t i = size - 1; i; --i) { // Don't do the first element.
        event->setCurrentTarget(eventTargets[i].get());
        eventTargets[i]->fireEventListeners(event);
        if (event->propagationStopped())
            goto doneDispatching;
    }

    event->setEventPhase(Event::AT_TARGET);
    event->setCurrentTarget(eventTargets[0].get());
    eventTargets[0]->fireEventListeners(event);
    if (event->propagationStopped() || !event->bubbles() || event->cancelBubble())
        goto doneDispatching;

    event->setEventPhase(Event::BUBBLING_PHASE);
    for (size_t i = 1; i < size; ++i) { // Don't do the first element.
        event->setCurrentTarget(eventTargets[i].get());
        eventTargets[i]->fireEventListeners(event);
        if (event->propagationStopped() || event->cancelBubble())
            goto doneDispatching;
    }

    // FIXME: "...However, we also wanted to integrate the window.onerror feature in
    //        HTML5. So after we've fired an "error" event, if .preventDefault() was
    //        never called on the event, we fire an error event on the window (can't
    //        remember if this happens before or after we abort the transaction).
    //        This is a separate event, which for example means that even if you
    //        attach a capturing "error" handler on window, you won't see any events
    //        unless an error really went unhandled. And you also can't call
    //        .preventDefault on the error event fired on the window in order to
    //        prevent the transaction from being aborted. It's purely there for
    //        error reporting and distinctly different from the event propagating to
    //        the window.
    //        
    //        This is similar to how "error" events are handled in workers.
    //        
    //        (I think that so far webkit hasn't implemented the window.onerror
    //        feature yet, so you probably don't want to fire the separate error
    //        event on the window until that has been implemented)." -- Jonas Sicking

doneDispatching:
    event->setCurrentTarget(0);
    event->setEventPhase(0);
    return !event->defaultPrevented();
}

} // namespace WebCore

#endif
