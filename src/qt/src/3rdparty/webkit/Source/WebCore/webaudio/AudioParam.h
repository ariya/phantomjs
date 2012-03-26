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

#ifndef AudioParam_h
#define AudioParam_h

#include "PlatformString.h"
#include <sys/types.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class AudioParam : public RefCounted<AudioParam> {
public:
    static const double DefaultSmoothingConstant;
    static const double SnapThreshold;

    static PassRefPtr<AudioParam> create(const String& name, double defaultValue, double minValue, double maxValue, unsigned units = 0)
    {
        return adoptRef(new AudioParam(name, defaultValue, minValue, maxValue, units));
    }

    AudioParam(const String& name, double defaultValue, double minValue, double maxValue, unsigned units = 0)
        : m_name(name)
        , m_value(defaultValue)
        , m_defaultValue(defaultValue)
        , m_minValue(minValue)
        , m_maxValue(maxValue)
        , m_units(units)
        , m_smoothedValue(defaultValue)
        , m_smoothingConstant(DefaultSmoothingConstant)
    {
    }

    float value() const { return static_cast<float>(m_value); }
    
    void setValue(float);

    String name() const { return m_name; }

    float minValue() const { return static_cast<float>(m_minValue); }
    float maxValue() const { return static_cast<float>(m_maxValue); }
    float defaultValue() const { return static_cast<float>(m_defaultValue); }
    unsigned units() const { return m_units; }

    // Value smoothing:

    // When a new value is set with setValue(), in our internal use of the parameter we don't immediately jump to it.
    // Instead we smoothly approach this value to avoid glitching.
    float smoothedValue() const { return static_cast<float>(m_smoothedValue); }

    // Smoothly exponentially approaches to (de-zippers) the desired value.
    // Returns true if smoothed value has already snapped exactly to value.
    bool smooth();

    void resetSmoothedValue() { m_smoothedValue = m_value; }
    void setSmoothingConstant(double k) { m_smoothingConstant = k; }

private:
    String m_name;
    double m_value;
    double m_defaultValue;
    double m_minValue;
    double m_maxValue;
    unsigned m_units;

    // Smoothing (de-zippering)
    double m_smoothedValue;
    double m_smoothingConstant;
};

} // namespace WebCore

#endif // AudioParam_h
