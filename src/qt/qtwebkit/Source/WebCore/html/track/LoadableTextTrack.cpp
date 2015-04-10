/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(VIDEO_TRACK)

#include "LoadableTextTrack.h"

#include "Event.h"
#include "HTMLTrackElement.h"
#include "ScriptEventListener.h"
#include "ScriptExecutionContext.h"
#include "TextTrackCueList.h"

namespace WebCore {

LoadableTextTrack::LoadableTextTrack(HTMLTrackElement* track, const String& kind, const String& label, const String& language)
    : TextTrack(track->document(), track, kind, label, language, TrackElement)
    , m_trackElement(track)
    , m_loadTimer(this, &LoadableTextTrack::loadTimerFired)
    , m_isDefault(false)
{
}

LoadableTextTrack::~LoadableTextTrack()
{
}

void LoadableTextTrack::clearClient()
{
    m_trackElement = 0;
    TextTrack::clearClient();
}

void LoadableTextTrack::scheduleLoad(const KURL& url)
{
    if (url == m_url)
        return;

    // 4.8.10.12.3 Sourcing out-of-band text tracks (continued)

    // 2. Let URL be the track URL of the track element.
    m_url = url;
    
    // 3. Asynchronously run the remaining steps, while continuing with whatever task 
    // was responsible for creating the text track or changing the text track mode.
    if (!m_loadTimer.isActive())
        m_loadTimer.startOneShot(0);
}

Element* LoadableTextTrack::element()
{
    return m_trackElement;
}
    
void LoadableTextTrack::setTrackElement(HTMLTrackElement* element)
{
    ASSERT(!m_trackElement || m_trackElement == element);
    m_trackElement = element;
}

void LoadableTextTrack::loadTimerFired(Timer<LoadableTextTrack>*)
{
    if (m_loader)
        m_loader->cancelLoad();

    if (!m_trackElement)
        return;

    // 4.8.10.12.3 Sourcing out-of-band text tracks (continued)

    // 4. Download: If URL is not the empty string, perform a potentially CORS-enabled fetch of URL, with the
    // mode being the state of the media element's crossorigin content attribute, the origin being the
    // origin of the media element's Document, and the default origin behaviour set to fail.
    m_loader = TextTrackLoader::create(this, static_cast<ScriptExecutionContext*>(m_trackElement->document()));
    if (!m_loader->load(m_url, m_trackElement->mediaElementCrossOriginAttribute()))
        m_trackElement->didCompleteLoad(this, HTMLTrackElement::Failure);
}

void LoadableTextTrack::newCuesAvailable(TextTrackLoader* loader)
{
    ASSERT_UNUSED(loader, m_loader == loader);

    Vector<RefPtr<TextTrackCue> > newCues;
    m_loader->getNewCues(newCues);

    if (!m_cues)
        m_cues = TextTrackCueList::create();    

    for (size_t i = 0; i < newCues.size(); ++i) {
        newCues[i]->setTrack(this);
        m_cues->add(newCues[i]);
    }

    if (client())
        client()->textTrackAddCues(this, m_cues.get());
}

void LoadableTextTrack::cueLoadingStarted(TextTrackLoader* loader)
{
    ASSERT_UNUSED(loader, m_loader == loader);
}

void LoadableTextTrack::cueLoadingCompleted(TextTrackLoader* loader, bool loadingFailed)
{
    ASSERT_UNUSED(loader, m_loader == loader);

    if (!m_trackElement)
        return;

    m_trackElement->didCompleteLoad(this, loadingFailed ? HTMLTrackElement::Failure : HTMLTrackElement::Success);
}

#if ENABLE(WEBVTT_REGIONS)
void LoadableTextTrack::newRegionsAvailable(TextTrackLoader* loader)
{
    ASSERT_UNUSED(loader, m_loader == loader);

    Vector<RefPtr<TextTrackRegion> > newRegions;
    m_loader->getNewRegions(newRegions);

    for (size_t i = 0; i < newRegions.size(); ++i) {
        newRegions[i]->setTrack(this);
        regionList()->add(newRegions[i]);
    }
}
#endif

size_t LoadableTextTrack::trackElementIndex()
{
    ASSERT(m_trackElement);
    ASSERT(m_trackElement->parentNode());

    size_t index = 0;
    for (Node* node = m_trackElement->parentNode()->firstChild(); node; node = node->nextSibling()) {
        if (!node->hasTagName(trackTag) || !node->parentNode())
            continue;
        if (node == m_trackElement)
            return index;
        ++index;
    }
    ASSERT_NOT_REACHED();

    return 0;
}

} // namespace WebCore

#endif
