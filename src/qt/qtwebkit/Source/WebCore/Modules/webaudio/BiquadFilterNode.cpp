/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
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

#include "BiquadFilterNode.h"

#include "ExceptionCode.h"

namespace WebCore {

BiquadFilterNode::BiquadFilterNode(AudioContext* context, float sampleRate)
    : AudioBasicProcessorNode(context, sampleRate)
{
    // Initially setup as lowpass filter.
    m_processor = adoptPtr(new BiquadProcessor(context, sampleRate, 1, false));
    setNodeType(NodeTypeBiquadFilter);
}

String BiquadFilterNode::type() const
{
    switch (const_cast<BiquadFilterNode*>(this)->biquadProcessor()->type()) {
    case BiquadProcessor::LowPass:
        return "lowpass";
    case BiquadProcessor::HighPass:
        return "highpass";
    case BiquadProcessor::BandPass:
        return "bandpass";
    case BiquadProcessor::LowShelf:
        return "lowshelf";
    case BiquadProcessor::HighShelf:
        return "highshelf";
    case BiquadProcessor::Peaking:
        return "peaking";
    case BiquadProcessor::Notch:
        return "notch";
    case BiquadProcessor::Allpass:
        return "allpass";
    default:
        ASSERT_NOT_REACHED();
        return "lowpass";
    }
}

void BiquadFilterNode::setType(const String& type)
{
    if (type == "lowpass")
        setType(BiquadProcessor::LowPass);
    else if (type == "highpass")
        setType(BiquadProcessor::HighPass);
    else if (type == "bandpass")
        setType(BiquadProcessor::BandPass);
    else if (type == "lowshelf")
        setType(BiquadProcessor::LowShelf);
    else if (type == "highshelf")
        setType(BiquadProcessor::HighShelf);
    else if (type == "peaking")
        setType(BiquadProcessor::Peaking);
    else if (type == "notch")
        setType(BiquadProcessor::Notch);
    else if (type == "allpass")
        setType(BiquadProcessor::Allpass);
    else
        ASSERT_NOT_REACHED();
}

bool BiquadFilterNode::setType(unsigned type)
{
    if (type > BiquadProcessor::Allpass)
        return false;
    
    biquadProcessor()->setType(static_cast<BiquadProcessor::FilterType>(type));
    return true;
}

void BiquadFilterNode::getFrequencyResponse(const Float32Array* frequencyHz,
                                            Float32Array* magResponse,
                                            Float32Array* phaseResponse)
{
    if (!frequencyHz || !magResponse || !phaseResponse)
        return;
    
    int n = std::min(frequencyHz->length(),
                     std::min(magResponse->length(), phaseResponse->length()));

    if (n) {
        biquadProcessor()->getFrequencyResponse(n,
                                                frequencyHz->data(),
                                                magResponse->data(),
                                                phaseResponse->data());
    }
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
