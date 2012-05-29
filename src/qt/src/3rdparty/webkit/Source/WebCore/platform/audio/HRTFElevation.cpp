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

#if ENABLE(WEB_AUDIO)

#include "HRTFElevation.h"

#include "AudioBus.h"
#include "AudioFileReader.h"
#include "Biquad.h"
#include "FFTFrame.h"
#include "HRTFPanner.h"
#include <algorithm>
#include <math.h>
#include <wtf/OwnPtr.h>

using namespace std;
 
namespace WebCore {

const unsigned HRTFElevation::AzimuthSpacing = 15;
const unsigned HRTFElevation::NumberOfRawAzimuths = 360 / AzimuthSpacing;
const unsigned HRTFElevation::InterpolationFactor = 8;
const unsigned HRTFElevation::NumberOfTotalAzimuths = NumberOfRawAzimuths * InterpolationFactor;

// Takes advantage of the symmetry and creates a composite version of the two measured versions.  For example, we have both azimuth 30 and -30 degrees
// where the roles of left and right ears are reversed with respect to each other.
bool HRTFElevation::calculateSymmetricKernelsForAzimuthElevation(int azimuth, int elevation, double sampleRate, const String& subjectName,
                                                                 RefPtr<HRTFKernel>& kernelL, RefPtr<HRTFKernel>& kernelR)
{
    RefPtr<HRTFKernel> kernelL1;
    RefPtr<HRTFKernel> kernelR1;
    bool success = calculateKernelsForAzimuthElevation(azimuth, elevation, sampleRate, subjectName, kernelL1, kernelR1);
    if (!success)
        return false;
        
    // And symmetric version
    int symmetricAzimuth = !azimuth ? 0 : 360 - azimuth;
                                                              
    RefPtr<HRTFKernel> kernelL2;
    RefPtr<HRTFKernel> kernelR2;
    success = calculateKernelsForAzimuthElevation(symmetricAzimuth, elevation, sampleRate, subjectName, kernelL2, kernelR2);
    if (!success)
        return false;
        
    // Notice L/R reversal in symmetric version.
    kernelL = HRTFKernel::createInterpolatedKernel(kernelL1.get(), kernelR2.get(), 0.5);
    kernelR = HRTFKernel::createInterpolatedKernel(kernelR1.get(), kernelL2.get(), 0.5);
    
    return true;
}

bool HRTFElevation::calculateKernelsForAzimuthElevation(int azimuth, int elevation, double sampleRate, const String& subjectName,
                                                        RefPtr<HRTFKernel>& kernelL, RefPtr<HRTFKernel>& kernelR)
{
    // Valid values for azimuth are 0 -> 345 in 15 degree increments.
    // Valid values for elevation are -45 -> +90 in 15 degree increments.

    bool isAzimuthGood = azimuth >= 0 && azimuth <= 345 && (azimuth / 15) * 15 == azimuth;
    ASSERT(isAzimuthGood);
    if (!isAzimuthGood)
        return false;

    bool isElevationGood = elevation >= -45 && elevation <= 90 && (elevation / 15) * 15 == elevation;
    ASSERT(isElevationGood);
    if (!isElevationGood)
        return false;
    
    // Construct the resource name from the subject name, azimuth, and elevation, for example:
    // "IRC_Composite_C_R0195_T015_P000"
    // Note: the passed in subjectName is not a string passed in via JavaScript or the web.
    // It's passed in as an internal ASCII identifier and is an implementation detail.
    int positiveElevation = elevation < 0 ? elevation + 360 : elevation;
    String resourceName = String::format("IRC_%s_C_R0195_T%03d_P%03d", subjectName.utf8().data(), azimuth, positiveElevation);

    OwnPtr<AudioBus> impulseResponse(AudioBus::loadPlatformResource(resourceName.utf8().data(), sampleRate));

    ASSERT(impulseResponse.get());
    if (!impulseResponse.get())
        return false;
    
    size_t responseLength = impulseResponse->length();
    size_t expectedLength = static_cast<size_t>(256 * (sampleRate / 44100.0));

    // Check number of channels and length.  For now these are fixed and known.
    bool isBusGood = responseLength == expectedLength && impulseResponse->numberOfChannels() == 2;
    ASSERT(isBusGood);
    if (!isBusGood)
        return false;
    
    AudioChannel* leftEarImpulseResponse = impulseResponse->channelByType(AudioBus::ChannelLeft);
    AudioChannel* rightEarImpulseResponse = impulseResponse->channelByType(AudioBus::ChannelRight);

    // Note that depending on the fftSize returned by the panner, we may be truncating the impulse response we just loaded in.
    const size_t fftSize = HRTFPanner::fftSizeForSampleRate(sampleRate);
    kernelL = HRTFKernel::create(leftEarImpulseResponse, fftSize, sampleRate, true);
    kernelR = HRTFKernel::create(rightEarImpulseResponse, fftSize, sampleRate, true);
    
    return true;
}

// The range of elevations for the IRCAM impulse responses varies depending on azimuth, but the minimum elevation appears to always be -45.
//
// Here's how it goes:
static int maxElevations[] = {
        //  Azimuth
        //
    90, // 0  
    45, // 15 
    60, // 30 
    45, // 45 
    75, // 60 
    45, // 75 
    60, // 90 
    45, // 105 
    75, // 120 
    45, // 135 
    60, // 150 
    45, // 165 
    75, // 180 
    45, // 195 
    60, // 210 
    45, // 225 
    75, // 240 
    45, // 255 
    60, // 270 
    45, // 285 
    75, // 300 
    45, // 315 
    60, // 330 
    45 //  345 
};

PassOwnPtr<HRTFElevation> HRTFElevation::createForSubject(const String& subjectName, int elevation, double sampleRate)
{
    bool isElevationGood = elevation >= -45 && elevation <= 90 && (elevation / 15) * 15 == elevation;
    ASSERT(isElevationGood);
    if (!isElevationGood)
        return 0;
        
    OwnPtr<HRTFKernelList> kernelListL = adoptPtr(new HRTFKernelList(NumberOfTotalAzimuths));
    OwnPtr<HRTFKernelList> kernelListR = adoptPtr(new HRTFKernelList(NumberOfTotalAzimuths));

    // Load convolution kernels from HRTF files.
    int interpolatedIndex = 0;
    for (unsigned rawIndex = 0; rawIndex < NumberOfRawAzimuths; ++rawIndex) {
        // Don't let elevation exceed maximum for this azimuth.
        int maxElevation = maxElevations[rawIndex];
        int actualElevation = min(elevation, maxElevation);

        bool success = calculateKernelsForAzimuthElevation(rawIndex * AzimuthSpacing, actualElevation, sampleRate, subjectName, kernelListL->at(interpolatedIndex), kernelListR->at(interpolatedIndex));
        if (!success)
            return 0;
            
        interpolatedIndex += InterpolationFactor;
    }

    // Now go back and interpolate intermediate azimuth values.
    for (unsigned i = 0; i < NumberOfTotalAzimuths; i += InterpolationFactor) {
        int j = (i + InterpolationFactor) % NumberOfTotalAzimuths;

        // Create the interpolated convolution kernels and delays.
        for (unsigned jj = 1; jj < InterpolationFactor; ++jj) {
            double x = double(jj) / double(InterpolationFactor); // interpolate from 0 -> 1

            (*kernelListL)[i + jj] = HRTFKernel::createInterpolatedKernel(kernelListL->at(i).get(), kernelListL->at(j).get(), x);
            (*kernelListR)[i + jj] = HRTFKernel::createInterpolatedKernel(kernelListR->at(i).get(), kernelListR->at(j).get(), x);
        }
    }
    
    OwnPtr<HRTFElevation> hrtfElevation = adoptPtr(new HRTFElevation(kernelListL.release(), kernelListR.release(), elevation, sampleRate));
    return hrtfElevation.release();
}

PassOwnPtr<HRTFElevation> HRTFElevation::createByInterpolatingSlices(HRTFElevation* hrtfElevation1, HRTFElevation* hrtfElevation2, double x, double sampleRate)
{
    ASSERT(hrtfElevation1 && hrtfElevation2);
    if (!hrtfElevation1 || !hrtfElevation2)
        return 0;
        
    ASSERT(x >= 0.0 && x < 1.0);
    
    OwnPtr<HRTFKernelList> kernelListL = adoptPtr(new HRTFKernelList(NumberOfTotalAzimuths));
    OwnPtr<HRTFKernelList> kernelListR = adoptPtr(new HRTFKernelList(NumberOfTotalAzimuths));

    HRTFKernelList* kernelListL1 = hrtfElevation1->kernelListL();
    HRTFKernelList* kernelListR1 = hrtfElevation1->kernelListR();
    HRTFKernelList* kernelListL2 = hrtfElevation2->kernelListL();
    HRTFKernelList* kernelListR2 = hrtfElevation2->kernelListR();
    
    // Interpolate kernels of corresponding azimuths of the two elevations.
    for (unsigned i = 0; i < NumberOfTotalAzimuths; ++i) {
        (*kernelListL)[i] = HRTFKernel::createInterpolatedKernel(kernelListL1->at(i).get(), kernelListL2->at(i).get(), x);
        (*kernelListR)[i] = HRTFKernel::createInterpolatedKernel(kernelListR1->at(i).get(), kernelListR2->at(i).get(), x);
    }

    // Interpolate elevation angle.
    double angle = (1.0 - x) * hrtfElevation1->elevationAngle() + x * hrtfElevation2->elevationAngle();
    
    OwnPtr<HRTFElevation> hrtfElevation = adoptPtr(new HRTFElevation(kernelListL.release(), kernelListR.release(), static_cast<int>(angle), sampleRate));
    return hrtfElevation.release();  
}

void HRTFElevation::getKernelsFromAzimuth(double azimuthBlend, unsigned azimuthIndex, HRTFKernel* &kernelL, HRTFKernel* &kernelR, double& frameDelayL, double& frameDelayR)
{
    bool checkAzimuthBlend = azimuthBlend >= 0.0 && azimuthBlend < 1.0;
    ASSERT(checkAzimuthBlend);
    if (!checkAzimuthBlend)
        azimuthBlend = 0.0;
    
    unsigned numKernels = m_kernelListL->size();

    bool isIndexGood = azimuthIndex < numKernels;
    ASSERT(isIndexGood);
    if (!isIndexGood) {
        kernelL = 0;
        kernelR = 0;
        return;
    }
    
    // Return the left and right kernels.
    kernelL = m_kernelListL->at(azimuthIndex).get();
    kernelR = m_kernelListR->at(azimuthIndex).get();

    frameDelayL = m_kernelListL->at(azimuthIndex)->frameDelay();
    frameDelayR = m_kernelListR->at(azimuthIndex)->frameDelay();

    int azimuthIndex2 = (azimuthIndex + 1) % numKernels;
    double frameDelay2L = m_kernelListL->at(azimuthIndex2)->frameDelay();
    double frameDelay2R = m_kernelListR->at(azimuthIndex2)->frameDelay();

    // Linearly interpolate delays.
    frameDelayL = (1.0 - azimuthBlend) * frameDelayL + azimuthBlend * frameDelay2L;
    frameDelayR = (1.0 - azimuthBlend) * frameDelayR + azimuthBlend * frameDelay2R;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
