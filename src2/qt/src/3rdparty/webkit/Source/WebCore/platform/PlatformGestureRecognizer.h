/*
 * Copyright (c) 2011, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PlatformGestureRecognizer_h
#define PlatformGestureRecognizer_h

#include <wtf/PassOwnPtr.h>

namespace WebCore {

class EventHandler;
class PlatformGestureRecognizer;
class PlatformTouchEvent;

// A GestureRecognizer detects gestures occurring in the touch event.
// In response to a given touch event, the GestureRecognizer, updates
// its internal state and optionally dispatches synthetic events to the
// invoking EventHandler instance.
class PlatformGestureRecognizer {
protected:
    PlatformGestureRecognizer();

public:
    static PassOwnPtr<PlatformGestureRecognizer> create();
    virtual ~PlatformGestureRecognizer();

    // Invoked for each touch event that could contribute to the current gesture.
    // Takes a PlatformTouchEvent and the EventHandler that originated it and which will also
    // be the target of any generated synthetic event. Finally, |handled|
    // specifies if the |event| was actually handled by |source| (by the JavaScript)
    // Returns true if the event resulted in firing a synthetic event.
    virtual bool processTouchEventForGesture(const PlatformTouchEvent&, EventHandler*, bool handled) = 0;
};

} // namespace WebCore

#endif // PlatformGestureRecognizer_h
