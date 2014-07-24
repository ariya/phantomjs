/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef DeviceMotionClientMock_h
#define DeviceMotionClientMock_h

#include "DeviceMotionClient.h"
#include "DeviceMotionData.h"
#include "Timer.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class DeviceMotionController;

// A mock implementation of DeviceMotionClient used to test the feature in
// DumpRenderTree. Embedders should should configure the Page object to use this
// client when running DumpRenderTree.
class DeviceMotionClientMock : public DeviceMotionClient {
public:
    DeviceMotionClientMock();

    // DeviceMotionClient
    virtual void setController(DeviceMotionController*) OVERRIDE;
    virtual void startUpdating() OVERRIDE;
    virtual void stopUpdating() OVERRIDE;
    virtual DeviceMotionData* lastMotion() const OVERRIDE { return m_motion.get(); }
    // mock is owned by the testing framework, which should handle deletion
    virtual void deviceMotionControllerDestroyed() OVERRIDE { }

    void setMotion(PassRefPtr<DeviceMotionData>);

private:
    void timerFired(Timer<DeviceMotionClientMock>*);

    RefPtr<DeviceMotionData> m_motion;
    DeviceMotionController* m_controller;
    Timer<DeviceMotionClientMock> m_timer;
    bool m_isUpdating;
};

} // namespace WebCore

#endif // DeviceMotionClientMock_h
