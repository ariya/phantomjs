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

namespace WebCore {

void BiquadDSPKernel::process(const float* source, float* destination, size_t framesToProcess)
{
    ASSERT(source && destination && biquadProcessor());
    
    // Recompute filter coefficients if any of the parameters have changed.
    // FIXME: as an optimization, implement a way that a Biquad object can simply copy its internal filter coefficients from another Biquad object.
    // Then re-factor this code to only run for the first BiquadDSPKernel of each BiquadProcessor.
    if (biquadProcessor()->filterCoefficientsDirty()) {
        double value1 = biquadProcessor()->parameter1()->smoothedValue();
        double value2 = biquadProcessor()->parameter2()->smoothedValue();
        
        // Convert from Hertz to normalized frequency 0 -> 1.
        double nyquist = this->nyquist();
        double normalizedValue1 = value1 / nyquist;

        // Configure the biquad with the new filter parameters for the appropriate type of filter.
        switch (biquadProcessor()->type()) {
        case BiquadProcessor::LowPass2:
            m_biquad.setLowpassParams(normalizedValue1, value2);
            break;

        case BiquadProcessor::HighPass2:
            m_biquad.setHighpassParams(normalizedValue1, value2);
            break;

        case BiquadProcessor::LowShelf:
            m_biquad.setLowShelfParams(normalizedValue1, value2);
            break;

        // FIXME: add other biquad filter types...
        case BiquadProcessor::Peaking:
        case BiquadProcessor::Allpass:
        case BiquadProcessor::HighShelf:
            break;
        }
    }

    m_biquad.process(source, destination, framesToProcess);
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
