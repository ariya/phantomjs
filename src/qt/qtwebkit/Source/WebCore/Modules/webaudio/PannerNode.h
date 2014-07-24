/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PannerNode_h
#define PannerNode_h

#include "AudioBus.h"
#include "AudioListener.h"
#include "AudioNode.h"
#include "AudioParam.h"
#include "Cone.h"
#include "Distance.h"
#include "FloatPoint3D.h"
#include "Panner.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

// PannerNode is an AudioNode with one input and one output.
// It positions a sound in 3D space, with the exact effect dependent on the panning model.
// It has a position and an orientation in 3D space which is relative to the position and orientation of the context's AudioListener.
// A distance effect will attenuate the gain as the position moves away from the listener.
// A cone effect will attenuate the gain as the orientation moves away from the listener.
// All of these effects follow the OpenAL specification very closely.

class PannerNode : public AudioNode {
public:
    // These must be defined as in the .idl file and must match those in the Panner class.
    enum {
        EQUALPOWER = 0,
        HRTF = 1,
        SOUNDFIELD = 2,
    };

    // These must be defined as in the .idl file and must match those
    // in the DistanceEffect class.
    enum {
        LINEAR_DISTANCE = 0,
        INVERSE_DISTANCE = 1,
        EXPONENTIAL_DISTANCE = 2,
    };

    static PassRefPtr<PannerNode> create(AudioContext* context, float sampleRate)
    {
        return adoptRef(new PannerNode(context, sampleRate));
    }

    virtual ~PannerNode();

    // AudioNode
    virtual void process(size_t framesToProcess);
    virtual void pullInputs(size_t framesToProcess);
    virtual void reset();
    virtual void initialize();
    virtual void uninitialize();

    // Listener
    AudioListener* listener();

    // Panning model
    String panningModel() const;
    bool setPanningModel(unsigned); // Returns true on success.
    void setPanningModel(const String&);

    // Position
    FloatPoint3D position() const { return m_position; }
    void setPosition(float x, float y, float z) { m_position = FloatPoint3D(x, y, z); }

    // Orientation
    FloatPoint3D orientation() const { return m_position; }
    void setOrientation(float x, float y, float z) { m_orientation = FloatPoint3D(x, y, z); }

    // Velocity
    FloatPoint3D velocity() const { return m_velocity; }
    void setVelocity(float x, float y, float z) { m_velocity = FloatPoint3D(x, y, z); }

    // Distance parameters
    String distanceModel() const;
    bool setDistanceModel(unsigned); // Returns true on success.
    void setDistanceModel(const String&);

    double refDistance() { return m_distanceEffect.refDistance(); }
    void setRefDistance(double refDistance) { m_distanceEffect.setRefDistance(refDistance); }

    double maxDistance() { return m_distanceEffect.maxDistance(); }
    void setMaxDistance(double maxDistance) { m_distanceEffect.setMaxDistance(maxDistance); }

    double rolloffFactor() { return m_distanceEffect.rolloffFactor(); }
    void setRolloffFactor(double rolloffFactor) { m_distanceEffect.setRolloffFactor(rolloffFactor); }

    // Sound cones - angles in degrees
    double coneInnerAngle() const { return m_coneEffect.innerAngle(); }
    void setConeInnerAngle(double angle) { m_coneEffect.setInnerAngle(angle); }

    double coneOuterAngle() const { return m_coneEffect.outerAngle(); }
    void setConeOuterAngle(double angle) { m_coneEffect.setOuterAngle(angle); }

    double coneOuterGain() const { return m_coneEffect.outerGain(); }
    void setConeOuterGain(double angle) { m_coneEffect.setOuterGain(angle); }

    void getAzimuthElevation(double* outAzimuth, double* outElevation);
    float dopplerRate();

    // Accessors for dynamically calculated gain values.
    AudioParam* distanceGain() { return m_distanceGain.get(); }
    AudioParam* coneGain() { return m_coneGain.get(); }

    virtual double tailTime() const OVERRIDE { return m_panner ? m_panner->tailTime() : 0; }
    virtual double latencyTime() const OVERRIDE { return m_panner ? m_panner->latencyTime() : 0; }

private:
    PannerNode(AudioContext*, float sampleRate);

    // Returns the combined distance and cone gain attenuation.
    float distanceConeGain();

    // Notifies any AudioBufferSourceNodes connected to us either directly or indirectly about our existence.
    // This is in order to handle the pitch change necessary for the doppler shift.
    void notifyAudioSourcesConnectedToNode(AudioNode*);

    OwnPtr<Panner> m_panner;
    unsigned m_panningModel;

    FloatPoint3D m_position;
    FloatPoint3D m_orientation;
    FloatPoint3D m_velocity;

    // Gain
    RefPtr<AudioParam> m_distanceGain;
    RefPtr<AudioParam> m_coneGain;
    DistanceEffect m_distanceEffect;
    ConeEffect m_coneEffect;
    float m_lastGain;

    unsigned m_connectionCount;

    // Synchronize process() and setPanningModel() which can change the panner.
    mutable Mutex m_pannerLock;
};

} // namespace WebCore

#endif // PannerNode_h
