/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
 * Copyright (C) 2011, 2012, 2013 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(VIDEO_TRACK)

#include "AudioTrack.h"

#include "AudioTrackList.h"
#include "Event.h"
#include "ExceptionCode.h"
#include "HTMLMediaElement.h"
#include "TrackBase.h"

namespace WebCore {

const AtomicString& AudioTrack::alternativeKeyword()
{
    DEFINE_STATIC_LOCAL(const AtomicString, alternative, ("alternative", AtomicString::ConstructFromLiteral));
    return alternative;
}

const AtomicString& AudioTrack::descriptionKeyword()
{
    DEFINE_STATIC_LOCAL(const AtomicString, description, ("description", AtomicString::ConstructFromLiteral));
    return description;
}

const AtomicString& AudioTrack::mainKeyword()
{
    DEFINE_STATIC_LOCAL(const AtomicString, main, ("main", AtomicString::ConstructFromLiteral));
    return main;
}

const AtomicString& AudioTrack::mainDescKeyword()
{
    DEFINE_STATIC_LOCAL(const AtomicString, mainDesc, ("main-desc", AtomicString::ConstructFromLiteral));
    return mainDesc;
}

const AtomicString& AudioTrack::translationKeyword()
{
    DEFINE_STATIC_LOCAL(const AtomicString, translation, ("translation", AtomicString::ConstructFromLiteral));
    return translation;
}

const AtomicString& AudioTrack::commentaryKeyword()
{
    DEFINE_STATIC_LOCAL(const AtomicString, commentary, ("commentary", AtomicString::ConstructFromLiteral));
    return commentary;
}

AudioTrack::AudioTrack(AudioTrackClient* client, PassRefPtr<AudioTrackPrivate> trackPrivate)
    : TrackBase(TrackBase::AudioTrack, trackPrivate->label(), trackPrivate->language())
    , m_id(trackPrivate->id())
    , m_enabled(trackPrivate->enabled())
    , m_client(client)
    , m_private(trackPrivate)
{
    m_private->setClient(this);

    switch (m_private->kind()) {
    case AudioTrackPrivate::Alternative:
        setKind(AudioTrack::alternativeKeyword());
        break;
    case AudioTrackPrivate::Description:
        setKind(AudioTrack::descriptionKeyword());
        break;
    case AudioTrackPrivate::Main:
        setKind(AudioTrack::mainKeyword());
        break;
    case AudioTrackPrivate::MainDesc:
        setKind(AudioTrack::mainDescKeyword());
        break;
    case AudioTrackPrivate::Translation:
        setKind(AudioTrack::translationKeyword());
        break;
    case AudioTrackPrivate::Commentary:
        setKind(AudioTrack::commentaryKeyword());
        break;
    case AudioTrackPrivate::None:
        setKind(emptyString());
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

AudioTrack::~AudioTrack()
{
    m_private->setClient(0);
}

bool AudioTrack::isValidKind(const AtomicString& value) const
{
    if (value == alternativeKeyword())
        return true;
    if (value == descriptionKeyword())
        return true;
    if (value == mainKeyword())
        return true;
    if (value == mainDescKeyword())
        return true;
    if (value == translationKeyword())
        return true;
    if (value == commentaryKeyword())
        return true;

    return false;
}

void AudioTrack::setEnabled(const bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;

    if (m_client)
        m_client->audioTrackEnabledChanged(this);
}

size_t AudioTrack::inbandTrackIndex()
{
    ASSERT(m_private);
    return m_private->audioTrackIndex();
}

void AudioTrack::willRemoveAudioTrackPrivate(AudioTrackPrivate* trackPrivate)
{
    UNUSED_PARAM(trackPrivate);
    ASSERT(trackPrivate == m_private);
    mediaElement()->removeAudioTrack(this);
}

} // namespace WebCore

#endif
