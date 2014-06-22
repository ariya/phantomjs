/*
 * Copyright (C) 2012 Apple Inc.  All rights reserved.
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

#include "InbandTextTrack.h"

#include "Document.h"
#include "Event.h"
#include "ExceptionCodePlaceholder.h"
#include "HTMLMediaElement.h"
#include "InbandTextTrackPrivate.h"
#include "Logging.h"
#include "TextTrackCueGeneric.h"
#include "TextTrackCueList.h"
#include "WebVTTParser.h"
#include <math.h>
#include <wtf/text/CString.h>

namespace WebCore {

TextTrackCueMap::TextTrackCueMap()
    : m_genericCueToDataMap(0)
    , m_genericDataToCueMap(0)
    , m_webVTTCueToDataMap(0)
    , m_webVTTDataToCueMap(0)
{
}

TextTrackCueMap::~TextTrackCueMap()
{
    if (m_genericCueToDataMap) {
        delete m_genericCueToDataMap;
        ASSERT(m_genericDataToCueMap);
        delete m_genericDataToCueMap;
    } else
        ASSERT(!m_genericDataToCueMap);

    if (m_webVTTCueToDataMap) {
        delete m_webVTTCueToDataMap;
        ASSERT(m_webVTTDataToCueMap);
        delete m_webVTTDataToCueMap;
    } else
        ASSERT(!m_webVTTDataToCueMap);
}

void TextTrackCueMap::add(GenericCueData* cueData, TextTrackCueGeneric* cue)
{
    if (!m_genericDataToCueMap) {
        m_genericDataToCueMap = new GenericCueDataToCueMap;
        ASSERT(!m_genericCueToDataMap);
        m_genericCueToDataMap = new GenericCueToDataMap;
    } else
        ASSERT(m_genericCueToDataMap);

    m_genericDataToCueMap->add(cueData, cue);
    m_genericCueToDataMap->add(cue, cueData);
}

void TextTrackCueMap::add(WebVTTCueData* cueData, TextTrackCue* cue)
{
    if (!m_webVTTDataToCueMap) {
        m_webVTTDataToCueMap = new WebVTTCueDataToCueMap;
        ASSERT(!m_webVTTCueToDataMap);
        m_webVTTCueToDataMap = new WebVTTCueToDataMap;
    } else
        ASSERT(m_webVTTCueToDataMap);

    m_webVTTDataToCueMap->add(cueData, cue);
    m_webVTTCueToDataMap->add(cue, cueData);
}

PassRefPtr<TextTrackCueGeneric> TextTrackCueMap::find(GenericCueData* cueData)
{
    if (!m_genericDataToCueMap)
        return 0;

    GenericCueDataToCueMap::iterator iter = m_genericDataToCueMap->find(cueData);
    if (iter == m_genericDataToCueMap->end())
        return 0;

    return iter->value;
}

PassRefPtr<TextTrackCue> TextTrackCueMap::find(WebVTTCueData* cueData)
{
    if (!m_webVTTDataToCueMap)
        return 0;

    WebVTTCueDataToCueMap::iterator iter = m_webVTTDataToCueMap->find(cueData);
    if (iter == m_webVTTDataToCueMap->end())
        return 0;

    return iter->value;
}

PassRefPtr<GenericCueData> TextTrackCueMap::findGenericData(TextTrackCue* cue)
{
    if (!m_genericCueToDataMap)
        return 0;

    GenericCueToDataMap::iterator iter = m_genericCueToDataMap->find(cue);
    if (iter == m_genericCueToDataMap->end())
        return 0;

    return iter->value;
}

PassRefPtr<WebVTTCueData> TextTrackCueMap::findWebVTTData(TextTrackCue* cue)
{
    if (!m_webVTTCueToDataMap)
        return 0;

    WebVTTCueToDataMap::iterator iter = m_webVTTCueToDataMap->find(cue);
    if (iter == m_webVTTCueToDataMap->end())
        return 0;
    
    return iter->value;
}

void TextTrackCueMap::remove(GenericCueData* cueData)
{
    if (!m_genericCueToDataMap)
        return;

    RefPtr<TextTrackCueGeneric> cue = find(cueData);

    if (cue)
        m_genericCueToDataMap->remove(cue);
    m_genericDataToCueMap->remove(cueData);
}

void TextTrackCueMap::remove(TextTrackCue* cue)
{
    if (m_genericCueToDataMap) {
        RefPtr<GenericCueData> genericData = findGenericData(cue);
        if (genericData) {
            m_genericDataToCueMap->remove(genericData);
            m_genericCueToDataMap->remove(cue);
            return;
        }
    }

    if (m_webVTTCueToDataMap) {
        RefPtr<WebVTTCueData> webVTTData = findWebVTTData(cue);
        if (webVTTData) {
            m_webVTTDataToCueMap->remove(webVTTData);
            m_webVTTCueToDataMap->remove(cue);
        }
    }
}

void TextTrackCueMap::remove(WebVTTCueData* cueData)
{
    if (!m_webVTTCueToDataMap)
        return;

    RefPtr<TextTrackCue> cue = find(cueData);

    if (cue)
        m_webVTTCueToDataMap->remove(cue);
    m_webVTTDataToCueMap->remove(cueData);
}


PassRefPtr<InbandTextTrack> InbandTextTrack::create(ScriptExecutionContext* context, TextTrackClient* client, PassRefPtr<InbandTextTrackPrivate> playerPrivate)
{
    return adoptRef(new InbandTextTrack(context, client, playerPrivate));
}

InbandTextTrack::InbandTextTrack(ScriptExecutionContext* context, TextTrackClient* client, PassRefPtr<InbandTextTrackPrivate> tracksPrivate)
    : TextTrack(context, client, emptyString(), tracksPrivate->label(), tracksPrivate->language(), InBand)
    , m_private(tracksPrivate)
{
    m_private->setClient(this);
    
    switch (m_private->kind()) {
    case InbandTextTrackPrivate::Subtitles:
        setKind(TextTrack::subtitlesKeyword());
        break;
    case InbandTextTrackPrivate::Captions:
        setKind(TextTrack::captionsKeyword());
        break;
    case InbandTextTrackPrivate::Descriptions:
        setKind(TextTrack::descriptionsKeyword());
        break;
    case InbandTextTrackPrivate::Chapters:
        setKind(TextTrack::chaptersKeyword());
        break;
    case InbandTextTrackPrivate::Metadata:
        setKind(TextTrack::metadataKeyword());
        break;
    case InbandTextTrackPrivate::Forced:
        setKind(TextTrack::forcedKeyword());
        break;
    case InbandTextTrackPrivate::None:
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

InbandTextTrack::~InbandTextTrack()
{
    m_private->setClient(0);
}

void InbandTextTrack::setMode(const AtomicString& mode)
{
    TextTrack::setMode(mode);

    if (mode == TextTrack::disabledKeyword())
        m_private->setMode(InbandTextTrackPrivate::Disabled);
    else if (mode == TextTrack::hiddenKeyword())
        m_private->setMode(InbandTextTrackPrivate::Hidden);
    else if (mode == TextTrack::showingKeyword())
        m_private->setMode(InbandTextTrackPrivate::Showing);
    else
        ASSERT_NOT_REACHED();
}

bool InbandTextTrack::isClosedCaptions() const
{
    if (!m_private)
        return false;

    return m_private->isClosedCaptions();
}

bool InbandTextTrack::isSDH() const
{
    if (!m_private)
        return false;
    
    return m_private->isSDH();
}

bool InbandTextTrack::containsOnlyForcedSubtitles() const
{
    if (!m_private)
        return false;
    
    return m_private->containsOnlyForcedSubtitles();
}

bool InbandTextTrack::isMainProgramContent() const
{
    if (!m_private)
        return false;
    
    return m_private->isMainProgramContent();
}

bool InbandTextTrack::isEasyToRead() const
{
    if (!m_private)
        return false;
    
    return m_private->isEasyToRead();
}
    
size_t InbandTextTrack::inbandTrackIndex()
{
    ASSERT(m_private);
    return m_private->textTrackIndex();
}

void InbandTextTrack::updateCueFromCueData(TextTrackCueGeneric* cue, GenericCueData* cueData)
{
    cue->willChange();

    cue->setStartTime(cueData->startTime(), IGNORE_EXCEPTION);
    double endTime = cueData->endTime();
    if (std::isinf(endTime) && mediaElement())
        endTime = mediaElement()->duration();
    cue->setEndTime(endTime, IGNORE_EXCEPTION);
    cue->setText(cueData->content());
    cue->setId(cueData->id());
    cue->setBaseFontSizeRelativeToVideoHeight(cueData->baseFontSize());
    cue->setFontSizeMultiplier(cueData->relativeFontSize());
    cue->setFontName(cueData->fontName());

    if (cueData->position() > 0)
        cue->setPosition(lround(cueData->position()), IGNORE_EXCEPTION);
    if (cueData->line() > 0)
        cue->setLine(lround(cueData->line()), IGNORE_EXCEPTION);
    if (cueData->size() > 0)
        cue->setSize(lround(cueData->size()), IGNORE_EXCEPTION);
    if (cueData->backgroundColor().isValid())
        cue->setBackgroundColor(cueData->backgroundColor().rgb());
    if (cueData->foregroundColor().isValid())
        cue->setForegroundColor(cueData->foregroundColor().rgb());
    if (cueData->highlightColor().isValid())
        cue->setHighlightColor(cueData->highlightColor().rgb());

    if (cueData->align() == GenericCueData::Start)
        cue->setAlign(ASCIILiteral("start"), IGNORE_EXCEPTION);
    else if (cueData->align() == GenericCueData::Middle)
        cue->setAlign(ASCIILiteral("middle"), IGNORE_EXCEPTION);
    else if (cueData->align() == GenericCueData::End)
        cue->setAlign(ASCIILiteral("end"), IGNORE_EXCEPTION);
    cue->setSnapToLines(false);

    cue->didChange();
}
    
void InbandTextTrack::addGenericCue(InbandTextTrackPrivate* trackPrivate, PassRefPtr<GenericCueData> prpCueData)
{
    ASSERT_UNUSED(trackPrivate, trackPrivate == m_private);

    RefPtr<GenericCueData> cueData = prpCueData;
    if (m_cueMap.find(cueData.get()))
        return;

    RefPtr<TextTrackCueGeneric> cue = TextTrackCueGeneric::create(scriptExecutionContext(), cueData->startTime(), cueData->endTime(), cueData->content());
    updateCueFromCueData(cue.get(), cueData.get());
    if (hasCue(cue.get(), TextTrackCue::IgnoreDuration)) {
        LOG(Media, "InbandTextTrack::addGenericCue ignoring already added cue: start=%.2f, end=%.2f, content=\"%s\"\n", cueData->startTime(), cueData->endTime(), cueData->content().utf8().data());
        return;
    }

    if (cueData->status() != GenericCueData::Complete)
        m_cueMap.add(cueData.get(), cue.get());

    addCue(cue);
}

void InbandTextTrack::updateGenericCue(InbandTextTrackPrivate*, GenericCueData* cueData)
{
    RefPtr<TextTrackCueGeneric> cue = m_cueMap.find(cueData);
    if (!cue)
        return;

    updateCueFromCueData(cue.get(), cueData);
    
    if (cueData->status() == GenericCueData::Complete)
        m_cueMap.remove(cueData);
}

void InbandTextTrack::removeGenericCue(InbandTextTrackPrivate*, GenericCueData* cueData)
{
    RefPtr<TextTrackCueGeneric> cue = m_cueMap.find(cueData);
    if (cue) {
        LOG(Media, "InbandTextTrack::removeGenericCue removing cue: start=%.2f, end=%.2f, content=\"%s\"\n", cueData->startTime(), cueData->endTime(), cueData->content().utf8().data());
        removeCue(cue.get(), IGNORE_EXCEPTION);
    } else
        m_cueMap.remove(cueData);
}

void InbandTextTrack::addWebVTTCue(InbandTextTrackPrivate* trackPrivate, PassRefPtr<WebVTTCueData> prpCueData)
{
    ASSERT_UNUSED(trackPrivate, trackPrivate == m_private);

    RefPtr<WebVTTCueData> cueData = prpCueData;
    if (m_cueMap.find(cueData.get()))
        return;

    RefPtr<TextTrackCue> cue = TextTrackCue::create(scriptExecutionContext(), cueData->startTime(), cueData->endTime(), cueData->content());
    cue->setId(cueData->id());
    cue->setCueSettings(cueData->settings());

    m_cueMap.add(cueData.get(), cue.get());
    addCue(cue.release());
}

void InbandTextTrack::removeWebVTTCue(InbandTextTrackPrivate*, WebVTTCueData* cueData)
{
    RefPtr<TextTrackCue> cue = m_cueMap.find(cueData);
    if (cue) {
        LOG(Media, "InbandTextTrack::removeWebVTTCue removing cue: start=%.2f, end=%.2f, content=\"%s\"\n", cueData->startTime(), cueData->endTime(), cueData->content().utf8().data());
        removeCue(cue.get(), IGNORE_EXCEPTION);
    } else
        m_cueMap.remove(cueData);
}

void InbandTextTrack::removeCue(TextTrackCue* cue, ExceptionCode& ec)
{
    m_cueMap.remove(cue);
    TextTrack::removeCue(cue, ec);
}

void InbandTextTrack::willRemoveTextTrackPrivate(InbandTextTrackPrivate* trackPrivate)
{
    if (!mediaElement())
        return;
    ASSERT_UNUSED(trackPrivate, trackPrivate == m_private);
    mediaElement()->removeTextTrack(this);
}

} // namespace WebCore

#endif
