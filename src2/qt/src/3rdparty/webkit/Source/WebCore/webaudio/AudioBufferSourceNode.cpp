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

#include "AudioBufferSourceNode.h"

#include "AudioContext.h"
#include "AudioNodeOutput.h"
#include <algorithm>
#include <wtf/MathExtras.h>

using namespace std;

namespace WebCore {

const double DefaultGrainDuration = 0.020; // 20ms

PassRefPtr<AudioBufferSourceNode> AudioBufferSourceNode::create(AudioContext* context, double sampleRate)
{
    return adoptRef(new AudioBufferSourceNode(context, sampleRate));
}

AudioBufferSourceNode::AudioBufferSourceNode(AudioContext* context, double sampleRate)
    : AudioSourceNode(context, sampleRate)
    , m_buffer(0)
    , m_isPlaying(false)
    , m_isLooping(false)
    , m_hasFinished(false)
    , m_startTime(0.0)
    , m_schedulingFrameDelay(0)
    , m_readIndex(0)
    , m_isGrain(false)
    , m_grainOffset(0.0)
    , m_grainDuration(DefaultGrainDuration)
    , m_grainFrameCount(0)
    , m_lastGain(1.0)
    , m_pannerNode(0)
{
    setType(NodeTypeAudioBufferSource);

    m_gain = AudioGain::create("gain", 1.0, 0.0, 1.0);
    m_playbackRate = AudioParam::create("playbackRate", 1.0, 0.0, AudioResampler::MaxRate);

    // Default to mono.  A call to setBuffer() will set the number of output channels to that of the buffer.
    addOutput(adoptPtr(new AudioNodeOutput(this, 1)));

    initialize();
}

AudioBufferSourceNode::~AudioBufferSourceNode()
{
    uninitialize();
}

void AudioBufferSourceNode::process(size_t framesToProcess)
{
    AudioBus* outputBus = output(0)->bus();

    if (!isInitialized()) {
        outputBus->zero();
        return;
    }

    // The audio thread can't block on this lock, so we call tryLock() instead.
    // Careful - this is a tryLock() and not an autolocker, so we must unlock() before every return.
    if (m_processLock.tryLock()) {
        // Check if it's time to start playing.
        double sampleRate = this->sampleRate();
        double pitchRate = totalPitchRate();
        double quantumStartTime = context()->currentTime();
        double quantumEndTime = quantumStartTime + framesToProcess / sampleRate;

        if (!m_isPlaying || m_hasFinished || !buffer() || m_startTime >= quantumEndTime) {
            // FIXME: can optimize here by propagating silent hint instead of forcing the whole chain to process silence.
            outputBus->zero();
            m_processLock.unlock();
            return;
        }

        // Handle sample-accurate scheduling so that buffer playback will happen at a very precise time.
        m_schedulingFrameDelay = 0;
        if (m_startTime >= quantumStartTime) {
            // m_schedulingFrameDelay is set here only the very first render quantum (because of above check: m_startTime >= quantumEndTime)
            // So: quantumStartTime <= m_startTime < quantumEndTime
            ASSERT(m_startTime < quantumEndTime);
            
            double startTimeInQuantum = m_startTime - quantumStartTime;
            double startFrameInQuantum = startTimeInQuantum * sampleRate;
            
            // m_schedulingFrameDelay is used in provideInput(), so factor in the current playback pitch rate.
            m_schedulingFrameDelay = static_cast<int>(pitchRate * startFrameInQuantum);
        }

        // FIXME: optimization opportunity:
        // With a bit of work, it should be possible to avoid going through the resampler completely when the pitchRate == 1,
        // especially if the pitchRate has never deviated from 1 in the past.

        // Read the samples through the pitch resampler.  Our provideInput() method will be called by the resampler.
        m_resampler.setRate(pitchRate);
        m_resampler.process(this, outputBus, framesToProcess);

        // Apply the gain (in-place) to the output bus.
        double totalGain = gain()->value() * m_buffer->gain();
        outputBus->copyWithGainFrom(*outputBus, &m_lastGain, totalGain);

        m_processLock.unlock();
    } else {
        // Too bad - the tryLock() failed.  We must be in the middle of changing buffers and were already outputting silence anyway.
        outputBus->zero();
    }
}

// The resampler calls us back here to get the input samples from our buffer.
void AudioBufferSourceNode::provideInput(AudioBus* bus, size_t numberOfFrames)
{
    ASSERT(context()->isAudioThread());
    
    // Basic sanity checking
    ASSERT(bus);
    ASSERT(buffer());
    if (!bus || !buffer())
        return;

    unsigned numberOfChannels = this->numberOfChannels();
    unsigned busNumberOfChannels = bus->numberOfChannels();

    // FIXME: we can add support for sources with more than two channels, but this is not a common case.
    bool channelCountGood = numberOfChannels == busNumberOfChannels && (numberOfChannels == 1 || numberOfChannels == 2);
    ASSERT(channelCountGood);
    if (!channelCountGood)
        return;

    // Get the destination pointers.
    float* destinationL = bus->channel(0)->data();
    ASSERT(destinationL);
    if (!destinationL)
        return;
    float* destinationR = (numberOfChannels < 2) ? 0 : bus->channel(1)->data();

    size_t bufferLength = buffer()->length();
    double bufferSampleRate = buffer()->sampleRate();

    // Calculate the start and end frames in our buffer that we want to play.
    // If m_isGrain is true, then we will be playing a portion of the total buffer.
    unsigned startFrame = m_isGrain ? static_cast<unsigned>(m_grainOffset * bufferSampleRate) : 0;
    unsigned endFrame = m_isGrain ? static_cast<unsigned>(startFrame + m_grainDuration * bufferSampleRate) : bufferLength;

    // This is a HACK to allow for HRTF tail-time - avoids glitch at end.
    // FIXME: implement tailTime for each AudioNode for a more general solution to this problem.
    if (m_isGrain)
        endFrame += 512;

    // Do some sanity checking.
    if (startFrame >= bufferLength)
        startFrame = !bufferLength ? 0 : bufferLength - 1;
    if (endFrame > bufferLength)
        endFrame = bufferLength;
    if (m_readIndex >= endFrame)
        m_readIndex = startFrame; // reset to start
    
    int framesToProcess = numberOfFrames;

    // Handle sample-accurate scheduling so that we play the buffer at a very precise time.
    // m_schedulingFrameDelay will only be non-zero the very first time that provideInput() is called, which corresponds
    // with the very start of the buffer playback.
    if (m_schedulingFrameDelay > 0) {
        ASSERT(m_schedulingFrameDelay <= framesToProcess);
        if (m_schedulingFrameDelay <= framesToProcess) {
            // Generate silence for the initial portion of the destination.
            memset(destinationL, 0, sizeof(float) * m_schedulingFrameDelay);
            destinationL += m_schedulingFrameDelay;
            if (destinationR) {
                memset(destinationR, 0, sizeof(float) * m_schedulingFrameDelay);
                destinationR += m_schedulingFrameDelay;
            }

            // Since we just generated silence for the initial portion, we have fewer frames to provide.
            framesToProcess -= m_schedulingFrameDelay;
        }
    }
    
    // We have to generate a certain number of output sample-frames, but we need to handle the case where we wrap around
    // from the end of the buffer to the start if playing back with looping and also the case where we simply reach the
    // end of the sample data, but haven't yet rendered numberOfFrames worth of output.
    while (framesToProcess > 0) {
        ASSERT(m_readIndex <= endFrame);
        if (m_readIndex > endFrame)
            return;
            
        // Figure out how many frames we can process this time.
        int framesAvailable = endFrame - m_readIndex;
        int framesThisTime = min(framesToProcess, framesAvailable);
        
        // Create the destination bus for the part of the destination we're processing this time.
        AudioBus currentDestinationBus(busNumberOfChannels, framesThisTime, false);
        currentDestinationBus.setChannelMemory(0, destinationL, framesThisTime);
        if (busNumberOfChannels > 1)
            currentDestinationBus.setChannelMemory(1, destinationR, framesThisTime);

        // Generate output from the buffer.
        readFromBuffer(&currentDestinationBus, framesThisTime);

        // Update the destination pointers.
        destinationL += framesThisTime;
        if (busNumberOfChannels > 1)
            destinationR += framesThisTime;

        framesToProcess -= framesThisTime;

        // Handle the case where we reach the end of the part of the sample data we're supposed to play for the buffer.
        if (m_readIndex >= endFrame) {
            m_readIndex = startFrame;
            m_grainFrameCount = 0;
            
            if (!looping()) {
                // If we're not looping, then stop playing when we get to the end.
                m_isPlaying = false;

                if (framesToProcess > 0) {
                    // We're not looping and we've reached the end of the sample data, but we still need to provide more output,
                    // so generate silence for the remaining.
                    memset(destinationL, 0, sizeof(float) * framesToProcess);

                    if (destinationR)
                        memset(destinationR, 0, sizeof(float) * framesToProcess);
                }

                if (!m_hasFinished) {
                    // Let the context dereference this AudioNode.
                    context()->notifyNodeFinishedProcessing(this);
                    m_hasFinished = true;
                }
                return;
            }
        }
    }
}

void AudioBufferSourceNode::readFromBuffer(AudioBus* destinationBus, size_t framesToProcess)
{
    bool isBusGood = destinationBus && destinationBus->length() == framesToProcess && destinationBus->numberOfChannels() == numberOfChannels();
    ASSERT(isBusGood);
    if (!isBusGood)
        return;
    
    unsigned numberOfChannels = this->numberOfChannels();
    // FIXME: we can add support for sources with more than two channels, but this is not a common case.
    bool channelCountGood = numberOfChannels == 1 || numberOfChannels == 2;
    ASSERT(channelCountGood);
    if (!channelCountGood)
        return;
            
    // Get pointers to the start of the sample buffer.
    float* sourceL = m_buffer->getChannelData(0)->data();
    float* sourceR = m_buffer->numberOfChannels() == 2 ? m_buffer->getChannelData(1)->data() : 0;

    // Sanity check buffer access.
    bool isSourceGood = sourceL && (numberOfChannels == 1 || sourceR) && m_readIndex + framesToProcess <= m_buffer->length();
    ASSERT(isSourceGood);
    if (!isSourceGood)
        return;

    // Offset the pointers to the current read position in the sample buffer.
    sourceL += m_readIndex;
    sourceR += m_readIndex;

    // Get pointers to the destination.
    float* destinationL = destinationBus->channel(0)->data();
    float* destinationR = numberOfChannels == 2 ? destinationBus->channel(1)->data() : 0;
    bool isDestinationGood = destinationL && (numberOfChannels == 1 || destinationR);
    ASSERT(isDestinationGood);
    if (!isDestinationGood)
        return;

    if (m_isGrain)
        readFromBufferWithGrainEnvelope(sourceL, sourceR, destinationL, destinationR, framesToProcess);
    else {
        // Simply copy the data from the source buffer to the destination.
        memcpy(destinationL, sourceL, sizeof(float) * framesToProcess);
        if (numberOfChannels == 2)
            memcpy(destinationR, sourceR, sizeof(float) * framesToProcess);
    }

    // Advance the buffer's read index.
    m_readIndex += framesToProcess;
}

void AudioBufferSourceNode::readFromBufferWithGrainEnvelope(float* sourceL, float* sourceR, float* destinationL, float* destinationR, size_t framesToProcess)
{
    ASSERT(sourceL && destinationL);
    if (!sourceL || !destinationL)
        return;
        
    int grainFrameLength = static_cast<int>(m_grainDuration * m_buffer->sampleRate());
    bool isStereo = sourceR && destinationR;
    
    int n = framesToProcess;
    while (n--) {
        // Apply the grain envelope.
        float x = static_cast<float>(m_grainFrameCount) / static_cast<float>(grainFrameLength);
        m_grainFrameCount++;

        x = min(1.0f, x);
        float grainEnvelope = sinf(piFloat * x);
        
        *destinationL++ = grainEnvelope * *sourceL++;

        if (isStereo)
            *destinationR++ = grainEnvelope * *sourceR++;
    }
}

void AudioBufferSourceNode::reset()
{
    m_resampler.reset();
    m_readIndex = 0;
    m_grainFrameCount = 0;
    m_lastGain = gain()->value();
}

void AudioBufferSourceNode::setBuffer(AudioBuffer* buffer)
{
    ASSERT(isMainThread());
    
    // The context must be locked since changing the buffer can re-configure the number of channels that are output.
    AudioContext::AutoLocker contextLocker(context());
    
    // This synchronizes with process().
    MutexLocker processLocker(m_processLock);
    
    if (buffer) {
        // Do any necesssary re-configuration to the buffer's number of channels.
        unsigned numberOfChannels = buffer->numberOfChannels();
        m_resampler.configureChannels(numberOfChannels);
        output(0)->setNumberOfChannels(numberOfChannels);
    }

    m_readIndex = 0;
    m_buffer = buffer;
}

unsigned AudioBufferSourceNode::numberOfChannels()
{
    return output(0)->numberOfChannels();
}

void AudioBufferSourceNode::noteOn(double when)
{
    ASSERT(isMainThread());
    if (m_isPlaying)
        return;

    m_isGrain = false;
    m_startTime = when;
    m_readIndex = 0;
    m_isPlaying = true;
}

void AudioBufferSourceNode::noteGrainOn(double when, double grainOffset, double grainDuration)
{
    ASSERT(isMainThread());
    if (m_isPlaying)
        return;

    if (!buffer())
        return;
        
    // Do sanity checking of grain parameters versus buffer size.
    double bufferDuration = buffer()->duration();

    if (grainDuration > bufferDuration)
        return; // FIXME: maybe should throw exception - consider in specification.
    
    double maxGrainOffset = bufferDuration - grainDuration;
    maxGrainOffset = max(0.0, maxGrainOffset);

    grainOffset = max(0.0, grainOffset);
    grainOffset = min(maxGrainOffset, grainOffset);    
    m_grainOffset = grainOffset;

    m_grainDuration = grainDuration;
    m_grainFrameCount = 0;
    
    m_isGrain = true;
    m_startTime = when;
    m_readIndex = static_cast<int>(m_grainOffset * buffer()->sampleRate());
    m_isPlaying = true;
}

void AudioBufferSourceNode::noteOff(double)
{
    ASSERT(isMainThread());
    if (!m_isPlaying)
        return;
        
    // FIXME: the "when" argument to this method is ignored.
    m_isPlaying = false;
    m_readIndex = 0;
}

double AudioBufferSourceNode::totalPitchRate()
{
    double dopplerRate = 1.0;
    if (m_pannerNode.get())
        dopplerRate = m_pannerNode->dopplerRate();
    
    // Incorporate buffer's sample-rate versus AudioContext's sample-rate.
    // Normally it's not an issue because buffers are loaded at the AudioContext's sample-rate, but we can handle it in any case.
    double sampleRateFactor = 1.0;
    if (buffer())
        sampleRateFactor = buffer()->sampleRate() / sampleRate();
    
    double basePitchRate = playbackRate()->value();

    double totalRate = dopplerRate * sampleRateFactor * basePitchRate;

    // Sanity check the total rate.  It's very important that the resampler not get any bad rate values.
    totalRate = max(0.0, totalRate);
    totalRate = min(AudioResampler::MaxRate, totalRate);
    
    bool isTotalRateValid = !isnan(totalRate) && !isinf(totalRate);
    ASSERT(isTotalRateValid);
    if (!isTotalRateValid)
        totalRate = 1.0;
    
    return totalRate;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
