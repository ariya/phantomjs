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

#ifndef AudioListener_h
#define AudioListener_h

#include "FloatPoint3D.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

// AudioListener maintains the state of the listener in the audio scene as defined in the OpenAL specification.

class AudioListener : public RefCounted<AudioListener> {
public:
    static PassRefPtr<AudioListener> create()
    {
        return adoptRef(new AudioListener());
    }

    // Position
    void setPosition(float x, float y, float z) { setPosition(FloatPoint3D(x, y, z)); }
    void setPosition(const FloatPoint3D &position) { m_position = position; }
    const FloatPoint3D& position() const { return m_position; }

    // Orientation
    void setOrientation(float x, float y, float z, float upX, float upY, float upZ)
    {
        setOrientation(FloatPoint3D(x, y, z));
        setUpVector(FloatPoint3D(upX, upY, upZ));
    }
    void setOrientation(const FloatPoint3D &orientation) { m_orientation = orientation; }
    const FloatPoint3D& orientation() const { return m_orientation; }

    // Up-vector
    void setUpVector(const FloatPoint3D &upVector) { m_upVector = upVector; }
    const FloatPoint3D& upVector() const { return m_upVector; }

    // Velocity
    void setVelocity(float x, float y, float z) { setVelocity(FloatPoint3D(x, y, z)); }
    void setVelocity(const FloatPoint3D &velocity) { m_velocity = velocity; }
    const FloatPoint3D& velocity() const { return m_velocity; }

    // Doppler factor
    void setDopplerFactor(double dopplerFactor) { m_dopplerFactor = dopplerFactor; }
    double dopplerFactor() const { return m_dopplerFactor; }

    // Speed of sound
    void setSpeedOfSound(double speedOfSound) { m_speedOfSound = speedOfSound; }
    double speedOfSound() const { return m_speedOfSound; }

private:
    AudioListener();

    // Position / Orientation
    FloatPoint3D m_position;
    FloatPoint3D m_orientation;
    FloatPoint3D m_upVector;

    FloatPoint3D m_velocity;

    double m_dopplerFactor;
    double m_speedOfSound;
};

} // WebCore

#endif // AudioListener_h
