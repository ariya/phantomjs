/*
 * Copyright 2010, The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DeviceOrientationController_h
#define DeviceOrientationController_h

#include "DOMWindow.h"
#include "Timer.h"

#include <wtf/HashCountedSet.h>

namespace WebCore {

class DeviceOrientation;
class DeviceOrientationClient;
class Page;

class DeviceOrientationController {
public:
    DeviceOrientationController(Page*, DeviceOrientationClient*);
    ~DeviceOrientationController();

    void addListener(DOMWindow*);
    void removeListener(DOMWindow*);
    void removeAllListeners(DOMWindow*);

    void didChangeDeviceOrientation(DeviceOrientation*);

    bool isActive() { return !m_listeners.isEmpty(); }

private:
    void timerFired(Timer<DeviceOrientationController>*);

    Page* m_page;
    DeviceOrientationClient* m_client;
    typedef HashCountedSet<RefPtr<DOMWindow> > ListenersCountedSet;
    ListenersCountedSet m_listeners;
    typedef HashSet<RefPtr<DOMWindow> > ListenersSet;
    ListenersSet m_newListeners;
    Timer<DeviceOrientationController> m_timer;
};

} // namespace WebCore

#endif // DeviceOrientationController_h
