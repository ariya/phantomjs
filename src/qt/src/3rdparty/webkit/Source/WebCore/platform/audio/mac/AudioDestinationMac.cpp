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

#include "AudioDestinationMac.h"

#include "AudioSourceProvider.h"
#include <CoreAudio/AudioHardware.h>

namespace WebCore {

const int kBufferSize = 128;

// Factory method: Mac-implementation
PassOwnPtr<AudioDestination> AudioDestination::create(AudioSourceProvider& provider, double sampleRate)
{
    return adoptPtr(new AudioDestinationMac(provider, sampleRate));
}

double AudioDestination::hardwareSampleRate()
{
    // Determine the default output device's sample-rate.
    AudioDeviceID deviceID = kAudioDeviceUnknown;
    UInt32 infoSize = sizeof(deviceID);

    AudioObjectPropertyAddress defaultOutputDeviceAddress = { kAudioHardwarePropertyDefaultOutputDevice, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
    OSStatus result = AudioObjectGetPropertyData(kAudioObjectSystemObject, &defaultOutputDeviceAddress, 0, 0, &infoSize, (void*)&deviceID);
    if (result)
        return 0.0; // error

    Float64 nominalSampleRate;
    infoSize = sizeof(Float64);

    AudioObjectPropertyAddress nominalSampleRateAddress = { kAudioDevicePropertyNominalSampleRate, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
    result = AudioObjectGetPropertyData(deviceID, &nominalSampleRateAddress, 0, 0, &infoSize, (void*)&nominalSampleRate);
    if (result)
        return 0.0; // error

    return nominalSampleRate;
}

AudioDestinationMac::AudioDestinationMac(AudioSourceProvider& provider, double sampleRate)
    : m_outputUnit(0)
    , m_provider(provider)
    , m_renderBus(2, kBufferSize, false)
    , m_sampleRate(sampleRate)
    , m_isPlaying(false)
{
    // Open and initialize DefaultOutputUnit
    Component comp;
    ComponentDescription desc;

    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    comp = FindNextComponent(0, &desc);

    ASSERT(comp);

    OSStatus result = OpenAComponent(comp, &m_outputUnit);
    ASSERT(!result);

    result = AudioUnitInitialize(m_outputUnit);
    ASSERT(!result);

    configure();
}

AudioDestinationMac::~AudioDestinationMac()
{
    if (m_outputUnit)
        CloseComponent(m_outputUnit);
}

void AudioDestinationMac::configure()
{
    // Set render callback
    AURenderCallbackStruct input;
    input.inputProc = inputProc;
    input.inputProcRefCon = this;
    OSStatus result = AudioUnitSetProperty(m_outputUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Global, 0, &input, sizeof(input));
    ASSERT(!result);

    // Set stream format
    AudioStreamBasicDescription streamFormat;
    streamFormat.mSampleRate = m_sampleRate;
    streamFormat.mFormatID = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags = kAudioFormatFlagsCanonical | kAudioFormatFlagIsNonInterleaved;
    streamFormat.mBitsPerChannel = 8 * sizeof(AudioSampleType);
    streamFormat.mChannelsPerFrame = 2;
    streamFormat.mFramesPerPacket = 1;
    streamFormat.mBytesPerPacket = sizeof(AudioSampleType);
    streamFormat.mBytesPerFrame = sizeof(AudioSampleType);

    result = AudioUnitSetProperty(m_outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, (void*)&streamFormat, sizeof(AudioStreamBasicDescription));
    ASSERT(!result);

    // Set the buffer frame size.
    UInt32 bufferSize = kBufferSize;
    result = AudioUnitSetProperty(m_outputUnit, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Output, 0, (void*)&bufferSize, sizeof(bufferSize));
    ASSERT(!result);
}

void AudioDestinationMac::start()
{
    OSStatus result = AudioOutputUnitStart(m_outputUnit);

    if (!result)
        m_isPlaying = true;
}

void AudioDestinationMac::stop()
{
    OSStatus result = AudioOutputUnitStop(m_outputUnit);

    if (!result)
        m_isPlaying = false;
}

// Pulls on our provider to get rendered audio stream.
OSStatus AudioDestinationMac::render(UInt32 numberOfFrames, AudioBufferList* ioData)
{
    AudioBuffer* buffers = ioData->mBuffers;
    m_renderBus.setChannelMemory(0, (float*)buffers[0].mData, numberOfFrames);
    m_renderBus.setChannelMemory(1, (float*)buffers[1].mData, numberOfFrames);

    m_provider.provideInput(&m_renderBus, numberOfFrames);

    return noErr;
}

// DefaultOutputUnit callback
OSStatus AudioDestinationMac::inputProc(void* userData, AudioUnitRenderActionFlags*, const AudioTimeStamp*, UInt32 /*busNumber*/, UInt32 numberOfFrames, AudioBufferList* ioData)
{
    AudioDestinationMac* audioOutput = static_cast<AudioDestinationMac*>(userData);
    return audioOutput->render(numberOfFrames, ioData);
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
