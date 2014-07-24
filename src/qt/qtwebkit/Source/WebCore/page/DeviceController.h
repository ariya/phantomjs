/*
 * Copyright 2010, The Android Open Source Project
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DeviceController_h
#define DeviceController_h

#include "DOMWindow.h"
#include "Event.h"
#include "Supplementable.h"
#include "Timer.h"
#include <wtf/HashCountedSet.h>

namespace WebCore {

class DeviceClient;
class Page;

class DeviceController : public Supplement<Page> {
public:
    explicit DeviceController(DeviceClient*);
    ~DeviceController() { }

    void addDeviceEventListener(DOMWindow*);
    void removeDeviceEventListener(DOMWindow*);
    void removeAllDeviceEventListeners(DOMWindow*);

    void dispatchDeviceEvent(PassRefPtr<Event>);
    bool isActive() { return !m_listeners.isEmpty(); }
    DeviceClient* client() { return m_client; }

    virtual bool hasLastData() { return false; }
    virtual PassRefPtr<Event> getLastEvent() { return 0; }

protected:
    void fireDeviceEvent(Timer<DeviceController>*);

    HashCountedSet<RefPtr<DOMWindow> > m_listeners;
    HashCountedSet<RefPtr<DOMWindow> > m_lastEventListeners;
    DeviceClient* m_client;
    Timer<DeviceController> m_timer;
};

} // namespace WebCore

#endif // DeviceController_h
