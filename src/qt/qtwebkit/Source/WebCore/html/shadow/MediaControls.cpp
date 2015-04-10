/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2011, 2012 Google Inc. All rights reserved.
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

#if ENABLE(VIDEO)
#include "MediaControls.h"

#include "ExceptionCodePlaceholder.h"
#include "Settings.h"

namespace WebCore {

MediaControls::MediaControls(Document* document)
    : HTMLDivElement(HTMLNames::divTag, document)
    , m_mediaController(0)
    , m_panel(0)
#if ENABLE(VIDEO_TRACK)
    , m_textDisplayContainer(0)
#endif
    , m_playButton(0)
    , m_currentTimeDisplay(0)
    , m_timeline(0)
    , m_panelMuteButton(0)
    , m_volumeSlider(0)
    , m_toggleClosedCaptionsButton(0)
    , m_fullScreenButton(0)
    , m_hideFullscreenControlsTimer(this, &MediaControls::hideFullscreenControlsTimerFired)
    , m_isFullscreen(false)
    , m_isMouseOverControls(false)
{
}

void MediaControls::setMediaController(MediaControllerInterface* controller)
{
    if (m_mediaController == controller)
        return;
    m_mediaController = controller;

    if (m_panel)
        m_panel->setMediaController(controller);
#if ENABLE(VIDEO_TRACK)
    if (m_textDisplayContainer)
        m_textDisplayContainer->setMediaController(controller);
#endif
    if (m_playButton)
        m_playButton->setMediaController(controller);
    if (m_currentTimeDisplay)
        m_currentTimeDisplay->setMediaController(controller);
    if (m_timeline)
        m_timeline->setMediaController(controller);
    if (m_panelMuteButton)
        m_panelMuteButton->setMediaController(controller);
    if (m_volumeSlider)
        m_volumeSlider->setMediaController(controller);
    if (m_toggleClosedCaptionsButton)
        m_toggleClosedCaptionsButton->setMediaController(controller);
    if (m_fullScreenButton)
        m_fullScreenButton->setMediaController(controller);
}

void MediaControls::reset()
{
    Page* page = document()->page();
    if (!page)
        return;

    m_playButton->updateDisplayType();

    updateCurrentTimeDisplay();

    double duration = m_mediaController->duration();
    if (std::isfinite(duration) || page->theme()->hasOwnDisabledStateHandlingFor(MediaSliderPart)) {
        m_timeline->setDuration(duration);
        m_timeline->setPosition(m_mediaController->currentTime());
    }

    if (m_mediaController->hasAudio() || page->theme()->hasOwnDisabledStateHandlingFor(MediaMuteButtonPart))
        m_panelMuteButton->show();
    else
        m_panelMuteButton->hide();

    if (m_volumeSlider) {
        if (!m_mediaController->hasAudio())
            m_volumeSlider->hide();
        else {
            m_volumeSlider->show();
            m_volumeSlider->setVolume(m_mediaController->volume());
        }
    }

    refreshClosedCaptionsButtonVisibility();

    if (m_fullScreenButton) {
        if (m_mediaController->supportsFullscreen() && m_mediaController->hasVideo())
            m_fullScreenButton->show();
        else
            m_fullScreenButton->hide();
    }

    makeOpaque();
}

void MediaControls::reportedError()
{
    Page* page = document()->page();
    if (!page)
        return;

    if (!page->theme()->hasOwnDisabledStateHandlingFor(MediaMuteButtonPart)) {
        m_panelMuteButton->hide();
        m_volumeSlider->hide();
    }

    if (m_toggleClosedCaptionsButton && !page->theme()->hasOwnDisabledStateHandlingFor(MediaToggleClosedCaptionsButtonPart))
        m_toggleClosedCaptionsButton->hide();

    if (m_fullScreenButton && !page->theme()->hasOwnDisabledStateHandlingFor(MediaEnterFullscreenButtonPart))
        m_fullScreenButton->hide();
}

void MediaControls::loadedMetadata()
{
    reset();
}

void MediaControls::show()
{
    makeOpaque();
    m_panel->setIsDisplayed(true);
    m_panel->show();
}

void MediaControls::hide()
{
    m_panel->setIsDisplayed(false);
    m_panel->hide();
}

void MediaControls::makeOpaque()
{
    m_panel->makeOpaque();
}

void MediaControls::makeTransparent()
{
    m_panel->makeTransparent();
}

bool MediaControls::shouldHideControls()
{
    return !m_panel->hovered();
}

void MediaControls::bufferingProgressed()
{
    // We only need to update buffering progress when paused, during normal
    // playback playbackProgressed() will take care of it.
    if (m_mediaController->paused())
        m_timeline->setPosition(m_mediaController->currentTime());
}

void MediaControls::playbackStarted()
{
    m_playButton->updateDisplayType();
    m_timeline->setPosition(m_mediaController->currentTime());
    updateCurrentTimeDisplay();

    if (m_isFullscreen)
        startHideFullscreenControlsTimer();
}

void MediaControls::playbackProgressed()
{
    m_timeline->setPosition(m_mediaController->currentTime());
    updateCurrentTimeDisplay();

    if (!m_isMouseOverControls && m_mediaController->hasVideo())
        makeTransparent();
}

void MediaControls::playbackStopped()
{
    m_playButton->updateDisplayType();
    m_timeline->setPosition(m_mediaController->currentTime());
    updateCurrentTimeDisplay();
    makeOpaque();

    stopHideFullscreenControlsTimer();
}

void MediaControls::updateCurrentTimeDisplay()
{
    double now = m_mediaController->currentTime();

    Page* page = document()->page();
    if (!page)
        return;

    m_currentTimeDisplay->setInnerText(page->theme()->formatMediaControlsTime(now), IGNORE_EXCEPTION);
    m_currentTimeDisplay->setCurrentValue(now);
}

void MediaControls::showVolumeSlider()
{
    if (!m_mediaController->hasAudio())
        return;

    m_volumeSlider->show();
}

void MediaControls::changedMute()
{
    m_panelMuteButton->changedMute();
}

void MediaControls::changedVolume()
{
    if (m_volumeSlider)
        m_volumeSlider->setVolume(m_mediaController->volume());
    if (m_panelMuteButton && m_panelMuteButton->renderer())
        m_panelMuteButton->renderer()->repaint();
}

void MediaControls::changedClosedCaptionsVisibility()
{
    if (m_toggleClosedCaptionsButton)
        m_toggleClosedCaptionsButton->updateDisplayType();
}

void MediaControls::refreshClosedCaptionsButtonVisibility()
{
    if (!m_toggleClosedCaptionsButton)
        return;

    if (m_mediaController->hasClosedCaptions())
        m_toggleClosedCaptionsButton->show();
    else
        m_toggleClosedCaptionsButton->hide();
}

void MediaControls::closedCaptionTracksChanged()
{
    refreshClosedCaptionsButtonVisibility();
}

void MediaControls::enteredFullscreen()
{
    m_isFullscreen = true;
    m_fullScreenButton->setIsFullscreen(true);

    if (Page* page = document()->page())
        page->chrome().setCursorHiddenUntilMouseMoves(true);

    startHideFullscreenControlsTimer();
#if ENABLE(VIDEO_TRACK)
    if (m_textDisplayContainer)
        m_textDisplayContainer->enteredFullscreen();
#endif
}

void MediaControls::exitedFullscreen()
{
    m_isFullscreen = false;
    m_fullScreenButton->setIsFullscreen(false);
    stopHideFullscreenControlsTimer();
#if ENABLE(VIDEO_TRACK)
    if (m_textDisplayContainer)
        m_textDisplayContainer->exitedFullscreen();
#endif
}

void MediaControls::defaultEventHandler(Event* event)
{
    HTMLDivElement::defaultEventHandler(event);

    if (event->type() == eventNames().mouseoverEvent) {
        if (!containsRelatedTarget(event)) {
            m_isMouseOverControls = true;
            if (!m_mediaController->canPlay()) {
                makeOpaque();
                if (shouldHideControls())
                    startHideFullscreenControlsTimer();
            }
        }
        return;
    }

    if (event->type() == eventNames().mouseoutEvent) {
        if (!containsRelatedTarget(event)) {
            m_isMouseOverControls = false;
            stopHideFullscreenControlsTimer();
        }
        return;
    }

    if (event->type() == eventNames().mousemoveEvent) {
        if (m_isFullscreen) {
            // When we get a mouse move in fullscreen mode, show the media controls, and start a timer
            // that will hide the media controls after a 3 seconds without a mouse move.
            makeOpaque();
            if (shouldHideControls())
                startHideFullscreenControlsTimer();
        }
        return;
    }
}

void MediaControls::hideFullscreenControlsTimerFired(Timer<MediaControls>*)
{
    if (m_mediaController->paused())
        return;

    if (!m_isFullscreen)
        return;

    if (!shouldHideControls())
        return;

    if (Page* page = document()->page())
        page->chrome().setCursorHiddenUntilMouseMoves(true);

    makeTransparent();
}

void MediaControls::startHideFullscreenControlsTimer()
{
    if (!m_isFullscreen)
        return;

    Page* page = document()->page();
    if (!page)
        return;

    m_hideFullscreenControlsTimer.startOneShot(page->settings()->timeWithoutMouseMovementBeforeHidingControls());
}

void MediaControls::stopHideFullscreenControlsTimer()
{
    m_hideFullscreenControlsTimer.stop();
}

const AtomicString& MediaControls::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls"));
    return id;
}

bool MediaControls::containsRelatedTarget(Event* event)
{
    if (!event->isMouseEvent())
        return false;
    EventTarget* relatedTarget = static_cast<MouseEvent*>(event)->relatedTarget();
    if (!relatedTarget)
        return false;
    return contains(relatedTarget->toNode());
}

#if ENABLE(VIDEO_TRACK)
void MediaControls::createTextTrackDisplay()
{
    if (m_textDisplayContainer)
        return;

    RefPtr<MediaControlTextTrackContainerElement> textDisplayContainer = MediaControlTextTrackContainerElement::create(document());
    m_textDisplayContainer = textDisplayContainer.get();

    if (m_mediaController)
        m_textDisplayContainer->setMediaController(m_mediaController);

    // Insert it before the first controller element so it always displays behind the controls.
    insertBefore(textDisplayContainer.release(), m_panel, IGNORE_EXCEPTION, AttachLazily);
}

void MediaControls::showTextTrackDisplay()
{
    if (!m_textDisplayContainer)
        createTextTrackDisplay();
    m_textDisplayContainer->show();
}

void MediaControls::hideTextTrackDisplay()
{
    if (!m_textDisplayContainer)
        createTextTrackDisplay();
    m_textDisplayContainer->hide();
}

void MediaControls::updateTextTrackDisplay()
{
    if (!m_textDisplayContainer)
        createTextTrackDisplay();

    m_textDisplayContainer->updateDisplay();
}
    
void MediaControls::textTrackPreferencesChanged()
{
    closedCaptionTracksChanged();
    if (m_textDisplayContainer)
        m_textDisplayContainer->updateSizes(true);
}
#endif

}

#endif
