/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
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

#include "CaptionUserPreferences.h"
#include "DOMWrapperWorld.h"
#include "Page.h"
#include "PageGroup.h"
#include "Settings.h"
#include "TextTrackList.h"
#include "UserStyleSheetTypes.h"
#include <wtf/NonCopyingSort.h>

namespace WebCore {

CaptionUserPreferences::CaptionUserPreferences(PageGroup* group)
    : m_pageGroup(group)
    , m_displayMode(ForcedOnly)
    , m_timer(this, &CaptionUserPreferences::timerFired)
    , m_testingMode(false)
    , m_havePreferences(false)
{
}

CaptionUserPreferences::~CaptionUserPreferences()
{
}

void CaptionUserPreferences::timerFired(Timer<CaptionUserPreferences>*)
{
    captionPreferencesChanged();
}

void CaptionUserPreferences::notify()
{
    m_havePreferences = true;
    if (!m_timer.isActive())
        m_timer.startOneShot(0);
}

CaptionUserPreferences::CaptionDisplayMode CaptionUserPreferences::captionDisplayMode() const
{
    return m_displayMode;
}

void CaptionUserPreferences::setCaptionDisplayMode(CaptionUserPreferences::CaptionDisplayMode mode)
{
    m_displayMode = mode;
    if (m_testingMode && mode != AlwaysOn) {
        setUserPrefersCaptions(false);
        setUserPrefersSubtitles(false);
    }
    notify();
}

bool CaptionUserPreferences::userPrefersCaptions() const
{
    Page* page = *(pageGroup()->pages().begin());
    if (!page)
        return false;

    return page->settings()->shouldDisplayCaptions();
}

void CaptionUserPreferences::setUserPrefersCaptions(bool preference)
{
    Page* page = *(pageGroup()->pages().begin());
    if (!page)
        return;

    page->settings()->setShouldDisplayCaptions(preference);
    notify();
}

bool CaptionUserPreferences::userPrefersSubtitles() const
{
    Page* page = *(pageGroup()->pages().begin());
    if (!page)
        return false;

    return page->settings()->shouldDisplaySubtitles();
}

void CaptionUserPreferences::setUserPrefersSubtitles(bool preference)
{
    Page* page = *(pageGroup()->pages().begin());
    if (!page)
        return;

    page->settings()->setShouldDisplaySubtitles(preference);
    notify();
}

bool CaptionUserPreferences::userPrefersTextDescriptions() const
{
    Page* page = *(pageGroup()->pages().begin());
    if (!page)
        return false;
    
    return page->settings()->shouldDisplayTextDescriptions();
}

void CaptionUserPreferences::setUserPrefersTextDescriptions(bool preference)
{
    Page* page = *(pageGroup()->pages().begin());
    if (!page)
        return;
    
    page->settings()->setShouldDisplayTextDescriptions(preference);
    notify();
}

void CaptionUserPreferences::captionPreferencesChanged()
{
    m_pageGroup->captionPreferencesChanged();
}

Vector<String> CaptionUserPreferences::preferredLanguages() const
{
    Vector<String> languages = userPreferredLanguages();
    if (m_testingMode && !m_userPreferredLanguage.isEmpty())
        languages.insert(0, m_userPreferredLanguage);

    return languages;
}

void CaptionUserPreferences::setPreferredLanguage(const String& language)
{
    m_userPreferredLanguage = language;
    notify();
}

static String trackDisplayName(TextTrack* track)
{
    if (track == TextTrack::captionMenuOffItem())
        return textTrackOffMenuItemText();
    if (track == TextTrack::captionMenuAutomaticItem())
        return textTrackAutomaticMenuItemText();

    if (track->label().isEmpty() && track->language().isEmpty())
        return textTrackNoLabelText();
    if (!track->label().isEmpty())
        return track->label();
    return track->language();
}

String CaptionUserPreferences::displayNameForTrack(TextTrack* track) const
{
    return trackDisplayName(track);
}
    
static bool textTrackCompare(const RefPtr<TextTrack>& a, const RefPtr<TextTrack>& b)
{
    return codePointCompare(trackDisplayName(a.get()), trackDisplayName(b.get())) < 0;
}

Vector<RefPtr<TextTrack> > CaptionUserPreferences::sortedTrackListForMenu(TextTrackList* trackList)
{
    ASSERT(trackList);

    Vector<RefPtr<TextTrack> > tracksForMenu;

    for (unsigned i = 0, length = trackList->length(); i < length; ++i)
        tracksForMenu.append(trackList->item(i));

    nonCopyingSort(tracksForMenu.begin(), tracksForMenu.end(), textTrackCompare);

    tracksForMenu.insert(0, TextTrack::captionMenuOffItem());
    tracksForMenu.insert(1, TextTrack::captionMenuAutomaticItem());

    return tracksForMenu;
}

int CaptionUserPreferences::textTrackSelectionScore(TextTrack* track, HTMLMediaElement*) const
{
    int trackScore = 0;

    if (track->kind() != TextTrack::captionsKeyword() && track->kind() != TextTrack::subtitlesKeyword())
        return trackScore;
    
    if (!userPrefersSubtitles() && !userPrefersCaptions())
        return trackScore;
    
    if (track->kind() == TextTrack::subtitlesKeyword() && userPrefersSubtitles())
        trackScore = 1;
    else if (track->kind() == TextTrack::captionsKeyword() && userPrefersCaptions())
        trackScore = 1;
    
    return trackScore + textTrackLanguageSelectionScore(track, preferredLanguages());
}

int CaptionUserPreferences::textTrackLanguageSelectionScore(TextTrack* track, const Vector<String>& preferredLanguages) const
{
    if (track->language().isEmpty())
        return 0;

    size_t languageMatchIndex = indexOfBestMatchingLanguageInList(track->language(), preferredLanguages);
    if (languageMatchIndex >= preferredLanguages.size())
        return 0;

    // Matching a track language is more important than matching track type, so this multiplier must be
    // greater than the maximum value returned by textTrackSelectionScore.
    return (preferredLanguages.size() - languageMatchIndex) * 10;
}

void CaptionUserPreferences::setCaptionsStyleSheetOverride(const String& override)
{
    m_captionsStyleSheetOverride = override;
    updateCaptionStyleSheetOveride();
}

void CaptionUserPreferences::updateCaptionStyleSheetOveride()
{
    // Identify our override style sheet with a unique URL - a new scheme and a UUID.
    DEFINE_STATIC_LOCAL(KURL, captionsStyleSheetURL, (ParsedURLString, "user-captions-override:01F6AF12-C3B0-4F70-AF5E-A3E00234DC23"));

    pageGroup()->removeUserStyleSheetFromWorld(mainThreadNormalWorld(), captionsStyleSheetURL);

    String captionsOverrideStyleSheet = captionsStyleSheetOverride();
    if (captionsOverrideStyleSheet.isEmpty())
        return;

    pageGroup()->addUserStyleSheetToWorld(mainThreadNormalWorld(), captionsOverrideStyleSheet, captionsStyleSheetURL, Vector<String>(),
        Vector<String>(), InjectInAllFrames, UserStyleAuthorLevel, InjectInExistingDocuments);
}

String CaptionUserPreferences::primaryAudioTrackLanguageOverride() const
{
    if (!m_primaryAudioTrackLanguageOverride.isEmpty())
        return m_primaryAudioTrackLanguageOverride;
    return defaultLanguage();
}
    
}

#endif // ENABLE(VIDEO_TRACK)
