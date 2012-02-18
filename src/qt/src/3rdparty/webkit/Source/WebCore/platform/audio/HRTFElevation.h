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

#ifndef HRTFElevation_h
#define HRTFElevation_h

#include "HRTFKernel.h"
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

// HRTFElevation contains all of the HRTFKernels (one left ear and one right ear per azimuth angle) for a particular elevation.

class HRTFElevation {
    WTF_MAKE_NONCOPYABLE(HRTFElevation);
public:
    // Loads and returns an HRTFElevation with the given HRTF database subject name and elevation from browser (or WebKit.framework) resources.
    // Normally, there will only be a single HRTF database set, but this API supports the possibility of multiple ones with different names.
    // Interpolated azimuths will be generated based on InterpolationFactor.
    // Valid values for elevation are -45 -> +90 in 15 degree increments.
    static PassOwnPtr<HRTFElevation> createForSubject(const String& subjectName, int elevation, double sampleRate);

    // Given two HRTFElevations, and an interpolation factor x: 0 -> 1, returns an interpolated HRTFElevation.
    static PassOwnPtr<HRTFElevation> createByInterpolatingSlices(HRTFElevation* hrtfElevation1, HRTFElevation* hrtfElevation2, double x, double sampleRate);

    // Returns the list of left or right ear HRTFKernels for all the azimuths going from 0 to 360 degrees.
    HRTFKernelList* kernelListL() { return m_kernelListL.get(); }
    HRTFKernelList* kernelListR() { return m_kernelListR.get(); }

    double elevationAngle() const { return m_elevationAngle; }
    unsigned numberOfAzimuths() { return NumberOfTotalAzimuths; }
    double sampleRate() const { return m_sampleRate; }
    
    // Returns the left and right kernels for the given azimuth index.
    // The interpolated delays based on azimuthBlend: 0 -> 1 are returned in frameDelayL and frameDelayR.
    void getKernelsFromAzimuth(double azimuthBlend, unsigned azimuthIndex, HRTFKernel* &kernelL, HRTFKernel* &kernelR, double& frameDelayL, double& frameDelayR);
    
    // Spacing, in degrees, between every azimuth loaded from resource.
    static const unsigned AzimuthSpacing;
    
    // Number of azimuths loaded from resource.
    static const unsigned NumberOfRawAzimuths;

    // Interpolates by this factor to get the total number of azimuths from every azimuth loaded from resource.
    static const unsigned InterpolationFactor;
    
    // Total number of azimuths after interpolation.
    static const unsigned NumberOfTotalAzimuths;

    // Given a specific azimuth and elevation angle, returns the left and right HRTFKernel.
    // Valid values for azimuth are 0 -> 345 in 15 degree increments.
    // Valid values for elevation are -45 -> +90 in 15 degree increments.
    // Returns true on success.
    static bool calculateKernelsForAzimuthElevation(int azimuth, int elevation, double sampleRate, const String& subjectName,
                                                    RefPtr<HRTFKernel>& kernelL, RefPtr<HRTFKernel>& kernelR);

    // Given a specific azimuth and elevation angle, returns the left and right HRTFKernel in kernelL and kernelR.
    // This method averages the measured response using symmetry of azimuth (for example by averaging the -30.0 and +30.0 azimuth responses).
    // Returns true on success.
    static bool calculateSymmetricKernelsForAzimuthElevation(int azimuth, int elevation, double sampleRate, const String& subjectName,
                                                             RefPtr<HRTFKernel>& kernelL, RefPtr<HRTFKernel>& kernelR);
private:
    HRTFElevation(PassOwnPtr<HRTFKernelList> kernelListL, PassOwnPtr<HRTFKernelList> kernelListR, int elevation, double sampleRate)
        : m_kernelListL(kernelListL)
        , m_kernelListR(kernelListR)
        , m_elevationAngle(elevation)
        , m_sampleRate(sampleRate)
    {
    }

    OwnPtr<HRTFKernelList> m_kernelListL;
    OwnPtr<HRTFKernelList> m_kernelListR;
    double m_elevationAngle;
    double m_sampleRate;
};

} // namespace WebCore

#endif // HRTFElevation_h
