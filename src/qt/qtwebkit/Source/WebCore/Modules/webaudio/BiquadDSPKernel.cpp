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

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "BiquadDSPKernel.h"

#include "BiquadProcessor.h"
#include "FloatConversion.h"
#include <limits.h>
#include <wtf/Vector.h>

namespace WebCore {

// FIXME: As a recursive linear filter, depending on its parameters, a biquad filter can have
// an infinite tailTime. In practice, Biquad filters do not usually (except for very high resonance values) 
// have a tailTime of longer than approx. 200ms. This value could possibly be calculated based on the
// settings of the Biquad.
static const double MaxBiquadDelayTime = 0.2;

void BiquadDSPKernel::updateCoefficientsIfNecessary(bool useSmoothing, bool forceUpdate)
{
    if (forceUpdate || biquadProcessor()->filterCoefficientsDirty()) {
        double value1;
        double value2;
        double gain;
        double detune; // in Cents

        if (biquadProcessor()->hasSampleAccurateValues()) {
            value1 = biquadProcessor()->parameter1()->finalValue();
            value2 = biquadProcessor()->parameter2()->finalValue();
            gain = biquadProcessor()->parameter3()->finalValue();
            detune = biquadProcessor()->parameter4()->finalValue();
        } else if (useSmoothing) {
            value1 = biquadProcessor()->parameter1()->smoothedValue();
            value2 = biquadProcessor()->parameter2()->smoothedValue();
            gain = biquadProcessor()->parameter3()->smoothedValue();
            detune = biquadProcessor()->parameter4()->smoothedValue();
        } else {
            value1 = biquadProcessor()->parameter1()->value();
            value2 = biquadProcessor()->parameter2()->value();
            gain = biquadProcessor()->parameter3()->value();
            detune = biquadProcessor()->parameter4()->value();
        }

        // Convert from Hertz to normalized frequency 0 -> 1.
        double nyquist = this->nyquist();
        double normalizedFrequency = value1 / nyquist;

        // Offset frequency by detune.
        if (detune)
            normalizedFrequency *= pow(2, detune / 1200);

        // Configure the biquad with the new filter parameters for the appropriate type of filter.
        switch (biquadProcessor()->type()) {
        case BiquadProcessor::LowPass:
            m_biquad.setLowpassParams(normalizedFrequency, value2);
            break;

        case BiquadProcessor::HighPass:
            m_biquad.setHighpassParams(normalizedFrequency, value2);
            break;

        case BiquadProcessor::BandPass:
            m_biquad.setBandpassParams(normalizedFrequency, value2);
            break;

        case BiquadProcessor::LowShelf:
            m_biquad.setLowShelfParams(normalizedFrequency, gain);
            break;

        case BiquadProcessor::HighShelf:
            m_biquad.setHighShelfParams(normalizedFrequency, gain);
            break;

        case BiquadProcessor::Peaking:
            m_biquad.setPeakingParams(normalizedFrequency, value2, gain);
            break;

        case BiquadProcessor::Notch:
            m_biquad.setNotchParams(normalizedFrequency, value2);
            break;

        case BiquadProcessor::Allpass:
            m_biquad.setAllpassParams(normalizedFrequency, value2);
            break;
        }
    }
}

void BiquadDSPKernel::process(const float* source, float* destination, size_t framesToProcess)
{
    ASSERT(source && destination && biquadProcessor());
    
    // Recompute filter coefficients if any of the parameters have changed.
    // FIXME: as an optimization, implement a way that a Biquad object can simply copy its internal filter coefficients from another Biquad object.
    // Then re-factor this code to only run for the first BiquadDSPKernel of each BiquadProcessor.

    updateCoefficientsIfNecessary(true, false);

    m_biquad.process(source, destination, framesToProcess);
}

void BiquadDSPKernel::getFrequencyResponse(int nFrequencies,
                                           const float* frequencyHz,
                                           float* magResponse,
                                           float* phaseResponse)
{
    bool isGood = nFrequencies > 0 && frequencyHz && magResponse && phaseResponse;
    ASSERT(isGood);
    if (!isGood)
        return;

    Vector<float> frequency(nFrequencies);

    double nyquist = this->nyquist();

    // Convert from frequency in Hz to normalized frequency (0 -> 1),
    // with 1 equal to the Nyquist frequency.
    for (int k = 0; k < nFrequencies; ++k)
        frequency[k] = narrowPrecisionToFloat(frequencyHz[k] / nyquist);

    // We want to get the final values of the coefficients and compute
    // the response from that instead of some intermediate smoothed
    // set. Forcefully update the coefficients even if they are not
    // dirty.

    updateCoefficientsIfNecessary(false, true);

    m_biquad.getFrequencyResponse(nFrequencies, frequency.data(), magResponse, phaseResponse);
}

double BiquadDSPKernel::tailTime() const
{
    return MaxBiquadDelayTime;
}

double BiquadDSPKernel::latencyTime() const
{
    return 0;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
