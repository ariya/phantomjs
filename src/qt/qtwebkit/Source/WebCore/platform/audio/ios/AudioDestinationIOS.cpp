/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#if PLATFORM(IOS)

#include "AudioDestinationIOS.h"

#include "AudioIOCallback.h"
#include "AudioSession.h"
#include "FloatConversion.h"
#include "Logging.h"
#include "Page.h"
#include "SoftLinking.h"
#include <AudioToolbox/AudioServices.h>
#include <WebCore/RuntimeApplicationChecksIOS.h>
#include <wtf/HashSet.h>

SOFT_LINK_FRAMEWORK(AudioToolbox)
SOFT_LINK(AudioToolbox, AudioComponentFindNext, AudioComponent, (AudioComponent inComponent, const AudioComponentDescription *inDesc), (inComponent, inDesc))
SOFT_LINK(AudioToolbox, AudioComponentInstanceDispose, OSStatus, (AudioComponentInstance inInstance), (inInstance))
SOFT_LINK(AudioToolbox, AudioComponentInstanceNew, OSStatus, (AudioComponent inComponent, AudioComponentInstance *outInstance), (inComponent, outInstance))
SOFT_LINK(AudioToolbox, AudioOutputUnitStart, OSStatus, (AudioUnit ci), (ci))
SOFT_LINK(AudioToolbox, AudioOutputUnitStop, OSStatus, (AudioUnit ci), (ci))
SOFT_LINK(AudioToolbox, AudioUnitAddPropertyListener, OSStatus, (AudioUnit inUnit, AudioUnitPropertyID inID, AudioUnitPropertyListenerProc inProc, void *inProcUserData), (inUnit, inID, inProc, inProcUserData))
SOFT_LINK(AudioToolbox, AudioUnitGetProperty, OSStatus, (AudioUnit inUnit, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, void *outData, UInt32 *ioDataSize), (inUnit, inID, inScope, inElement, outData, ioDataSize))
SOFT_LINK(AudioToolbox, AudioUnitInitialize, OSStatus, (AudioUnit inUnit), (inUnit))
SOFT_LINK(AudioToolbox, AudioUnitSetProperty, OSStatus, (AudioUnit inUnit, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, const void *inData, UInt32 inDataSize), (inUnit, inID, inScope, inElement, inData, inDataSize))

namespace WebCore {

const int kRenderBufferSize = 128;
const int kPreferredBufferSize = 256;

typedef HashSet<AudioDestinationIOS*> AudioDestinationSet;
static AudioDestinationSet& audioDestinations()
{
    DEFINE_STATIC_LOCAL(AudioDestinationSet, audioDestinationSet, ());
    return audioDestinationSet;
}

// Factory method: iOS-implementation
PassOwnPtr<AudioDestination> AudioDestination::create(AudioIOCallback& callback, const String&, unsigned numberOfInputChannels, unsigned numberOfOutputChannels, float sampleRate)
{
    // FIXME: make use of inputDeviceId as appropriate.

    // FIXME: Add support for local/live audio input.
    if (numberOfInputChannels)
        LOG(Media, "AudioDestination::create(%u, %u, %f) - unhandled input channels", numberOfInputChannels, numberOfOutputChannels, sampleRate);

    // FIXME: Add support for multi-channel (> stereo) output.
    if (numberOfOutputChannels != 2)
        LOG(Media, "AudioDestination::create(%u, %u, %f) - unhandled output channels", numberOfInputChannels, numberOfOutputChannels, sampleRate);

    return adoptPtr(new AudioDestinationIOS(callback, sampleRate));
}

float AudioDestination::hardwareSampleRate()
{
    return AudioSession::sharedSession().sampleRate();
}

unsigned long AudioDestination::maxChannelCount()
{
    // FIXME: query the default audio hardware device to return the actual number
    // of channels of the device. Also see corresponding FIXME in create().
    // There is a small amount of code which assumes stereo in AudioDestinationIOS which
    // can be upgraded.
    return 0;
}

AudioDestinationIOS::AudioDestinationIOS(AudioIOCallback& callback, double sampleRate)
    : m_outputUnit(0)
    , m_callback(callback)
    , m_renderBus(AudioBus::create(2, kRenderBufferSize, false))
    , m_sampleRate(sampleRate)
    , m_isPlaying(false)
    , m_interruptedOnPlayback(false)
{
    AudioSession& session = AudioSession::sharedSession();
    session.addListener(this);
    session.setCategory(AudioSession::AmbientSound);

    audioDestinations().add(this);
    if (audioDestinations().size() == 1)
        session.setActive(1);

    // Open and initialize DefaultOutputUnit
    AudioComponent comp;
    AudioComponentDescription desc;

    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_RemoteIO;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    comp = AudioComponentFindNext(0, &desc);

    ASSERT(comp);

    OSStatus result = AudioComponentInstanceNew(comp, &m_outputUnit);
    ASSERT(!result);

    UInt32 flag = 1;
    result = AudioUnitSetProperty(m_outputUnit, 
        kAudioOutputUnitProperty_EnableIO, 
        kAudioUnitScope_Output, 
        0,
        &flag, 
        sizeof(flag));
    ASSERT(!result);

    result = AudioUnitAddPropertyListener(m_outputUnit, kAudioUnitProperty_MaximumFramesPerSlice, frameSizeChangedProc, this);
    ASSERT(!result);

    result = AudioUnitInitialize(m_outputUnit);
    ASSERT(!result);

    configure();
}

AudioDestinationIOS::~AudioDestinationIOS()
{
    audioDestinations().remove(this);
    if (!audioDestinations().size())
        AudioSession::sharedSession().setActive(0);

    if (m_outputUnit)
        AudioComponentInstanceDispose(m_outputUnit);
}

void AudioDestinationIOS::configure()
{
    // Set render callback
    AURenderCallbackStruct input;
    input.inputProc = inputProc;
    input.inputProcRefCon = this;
    OSStatus result = AudioUnitSetProperty(m_outputUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &input, sizeof(input));
    ASSERT(!result);

    // Set stream format
    AudioStreamBasicDescription streamFormat;

    UInt32 size = sizeof(AudioStreamBasicDescription);
    result = AudioUnitGetProperty(m_outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, (void*)&streamFormat, &size);
    ASSERT(!result);

    const int bytesPerFloat = sizeof(Float32);
    const int bitsPerByte = 8;
    streamFormat.mSampleRate = m_sampleRate;
    streamFormat.mFormatID = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
    streamFormat.mBytesPerPacket = bytesPerFloat;
    streamFormat.mFramesPerPacket = 1;
    streamFormat.mBytesPerFrame = bytesPerFloat;
    streamFormat.mChannelsPerFrame = 2;
    streamFormat.mBitsPerChannel = bitsPerByte * bytesPerFloat;

    result = AudioUnitSetProperty(m_outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, (void*)&streamFormat, sizeof(AudioStreamBasicDescription));
    ASSERT(!result);

    AudioSession::sharedSession().setPreferredBufferSize(kPreferredBufferSize);
}

void AudioDestinationIOS::start()
{
    OSStatus result = AudioOutputUnitStart(m_outputUnit);

    if (!result)
        m_isPlaying = true;
}

void AudioDestinationIOS::stop()
{
    OSStatus result = AudioOutputUnitStop(m_outputUnit);

    if (!result)
        m_isPlaying = false;
}

void AudioDestinationIOS::beganAudioInterruption()
{
    if (!m_isPlaying)
        return;

    stop();
    m_interruptedOnPlayback = true;
}

void AudioDestinationIOS::endedAudioInterruption()
{
    if (!m_interruptedOnPlayback)
        return;

    m_interruptedOnPlayback = false;
    start();
}

// Pulls on our provider to get rendered audio stream.
OSStatus AudioDestinationIOS::render(UInt32 numberOfFrames, AudioBufferList* ioData)
{
    AudioBuffer* buffers = ioData->mBuffers;
    for (UInt32 frameOffset = 0; frameOffset + kRenderBufferSize <= numberOfFrames; frameOffset += kRenderBufferSize) {
        UInt32 remainingFrames = std::min<UInt32>(kRenderBufferSize, numberOfFrames - frameOffset);
        for (UInt32 i = 0; i < ioData->mNumberBuffers; ++i) {
            UInt32 bytesPerFrame = buffers[i].mDataByteSize / numberOfFrames;
            UInt32 byteOffset = frameOffset * bytesPerFrame;
            float* memory = (float*)((char*)buffers[i].mData + byteOffset);
            m_renderBus->setChannelMemory(i, memory, remainingFrames);
        }
        m_callback.render(0, m_renderBus.get(), remainingFrames);
    }

    return noErr;
}

// DefaultOutputUnit callback
OSStatus AudioDestinationIOS::inputProc(void* userData, AudioUnitRenderActionFlags*, const AudioTimeStamp*, UInt32 /*busNumber*/, UInt32 numberOfFrames, AudioBufferList* ioData)
{
    AudioDestinationIOS* audioOutput = static_cast<AudioDestinationIOS*>(userData);
    return audioOutput->render(numberOfFrames, ioData);
}

void AudioDestinationIOS::frameSizeChangedProc(void *inRefCon, AudioUnit, AudioUnitPropertyID, AudioUnitScope, AudioUnitElement)
{
    AudioDestinationIOS* audioOutput = static_cast<AudioDestinationIOS*>(inRefCon);
    UInt32 bufferSize = 0;
    UInt32 dataSize = sizeof(bufferSize);
    AudioUnitGetProperty(audioOutput->m_outputUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, (void*)&bufferSize, &dataSize);
    fprintf(stderr, ">>>> frameSizeChanged = %lu\n", static_cast<unsigned long>(bufferSize));
}

} // namespace WebCore

#endif // PLATFORM(IOS)

#endif // ENABLE(WEB_AUDIO)
