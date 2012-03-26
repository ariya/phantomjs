/*
 * Copyright (C) 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "MediaControlRootElement.h"

#include "MediaControlElements.h"
#include "MouseEvent.h"
#include "Page.h"
#include "RenderTheme.h"

using namespace std;

namespace WebCore {

static const double timeWithoutMouseMovementBeforeHidingControls = 3;

MediaControlRootElement::MediaControlRootElement(HTMLMediaElement* mediaElement)
    : MediaControls(mediaElement)
    , m_mediaElement(mediaElement)
    , m_rewindButton(0)
    , m_playButton(0)
    , m_returnToRealTimeButton(0)
    , m_statusDisplay(0)
    , m_currentTimeDisplay(0)
    , m_timeline(0)
    , m_timeRemainingDisplay(0)
    , m_timelineContainer(0)
    , m_seekBackButton(0)
    , m_seekForwardButton(0)
    , m_toggleClosedCaptionsButton(0)
    , m_panelMuteButton(0)
    , m_volumeSlider(0)
    , m_volumeSliderMuteButton(0)
    , m_volumeSliderContainer(0)
    , m_fullScreenButton(0)
    , m_fullScreenMinVolumeButton(0)
    , m_fullScreenVolumeSlider(0)
    , m_fullScreenMaxVolumeButton(0)
    , m_panel(0)
    , m_opaque(true)
    , m_isMouseOverControls(false)
    , m_hideFullscreenControlsTimer(this, &MediaControlRootElement::hideFullscreenControlsTimerFired)
{
}

PassRefPtr<MediaControls> MediaControls::create(HTMLMediaElement* mediaElement)
{
    return MediaControlRootElement::create(mediaElement);
}

PassRefPtr<MediaControlRootElement> MediaControlRootElement::create(HTMLMediaElement* mediaElement)
{
    if (!mediaElement->document()->page())
        return 0;

    RefPtr<MediaControlRootElement> controls = adoptRef(new MediaControlRootElement(mediaElement));

    RefPtr<MediaControlPanelElement> panel = MediaControlPanelElement::create(mediaElement);

    ExceptionCode ec;

    RefPtr<MediaControlRewindButtonElement> rewindButton = MediaControlRewindButtonElement::create(mediaElement);
    controls->m_rewindButton = rewindButton.get();
    panel->appendChild(rewindButton.release(), ec, true);
    if (ec)
        return 0;

    RefPtr<MediaControlPlayButtonElement> playButton = MediaControlPlayButtonElement::create(mediaElement);
    controls->m_playButton = playButton.get();
    panel->appendChild(playButton.release(), ec, true);
    if (ec)
        return 0;

    RefPtr<MediaControlReturnToRealtimeButtonElement> returnToRealtimeButton = MediaControlReturnToRealtimeButtonElement::create(mediaElement);
    controls->m_returnToRealTimeButton = returnToRealtimeButton.get();
    panel->appendChild(returnToRealtimeButton.release(), ec, true);
    if (ec)
        return 0;

    if (mediaElement->document()->page()->theme()->usesMediaControlStatusDisplay()) {
        RefPtr<MediaControlStatusDisplayElement> statusDisplay = MediaControlStatusDisplayElement::create(mediaElement);
        controls->m_statusDisplay = statusDisplay.get();
        panel->appendChild(statusDisplay.release(), ec, true);
        if (ec)
            return 0;
    }

    RefPtr<MediaControlTimelineContainerElement> timelineContainer = MediaControlTimelineContainerElement::create(mediaElement);

    RefPtr<MediaControlCurrentTimeDisplayElement> currentTimeDisplay = MediaControlCurrentTimeDisplayElement::create(mediaElement);
    controls->m_currentTimeDisplay = currentTimeDisplay.get();
    timelineContainer->appendChild(currentTimeDisplay.release(), ec, true);
    if (ec)
        return 0;

    RefPtr<MediaControlTimelineElement> timeline = MediaControlTimelineElement::create(mediaElement, controls.get());
    controls->m_timeline = timeline.get();
    timelineContainer->appendChild(timeline.release(), ec, true);
    if (ec)
        return 0;

    RefPtr<MediaControlTimeRemainingDisplayElement> timeRemainingDisplay = MediaControlTimeRemainingDisplayElement::create(mediaElement);
    controls->m_timeRemainingDisplay = timeRemainingDisplay.get();
    timelineContainer->appendChild(timeRemainingDisplay.release(), ec, true);
    if (ec)
        return 0;

    controls->m_timelineContainer = timelineContainer.get();
    panel->appendChild(timelineContainer.release(), ec, true);
    if (ec)
        return 0;

    // FIXME: Only create when needed <http://webkit.org/b/57163>
    RefPtr<MediaControlSeekBackButtonElement> seekBackButton = MediaControlSeekBackButtonElement::create(mediaElement);
    controls->m_seekBackButton = seekBackButton.get();
    panel->appendChild(seekBackButton.release(), ec, true);
    if (ec)
        return 0;

    // FIXME: Only create when needed <http://webkit.org/b/57163>
    RefPtr<MediaControlSeekForwardButtonElement> seekForwardButton = MediaControlSeekForwardButtonElement::create(mediaElement);
    controls->m_seekForwardButton = seekForwardButton.get();
    panel->appendChild(seekForwardButton.release(), ec, true);
    if (ec)
        return 0;

    if (mediaElement->document()->page()->theme()->supportsClosedCaptioning()) {
        RefPtr<MediaControlToggleClosedCaptionsButtonElement> toggleClosedCaptionsButton = MediaControlToggleClosedCaptionsButtonElement::create(mediaElement);
        controls->m_toggleClosedCaptionsButton = toggleClosedCaptionsButton.get();
        panel->appendChild(toggleClosedCaptionsButton.release(), ec, true);
        if (ec)
            return 0;
    }

    // FIXME: Only create when needed <http://webkit.org/b/57163>
    RefPtr<MediaControlFullscreenButtonElement> fullScreenButton = MediaControlFullscreenButtonElement::create(mediaElement, controls.get());
    controls->m_fullScreenButton = fullScreenButton.get();
    panel->appendChild(fullScreenButton.release(), ec, true);

    RefPtr<MediaControlPanelMuteButtonElement> panelMuteButton = MediaControlPanelMuteButtonElement::create(mediaElement, controls.get());
    controls->m_panelMuteButton = panelMuteButton.get();
    panel->appendChild(panelMuteButton.release(), ec, true);
    if (ec)
        return 0;

    if (mediaElement->document()->page()->theme()->usesMediaControlVolumeSlider()) {
        RefPtr<MediaControlVolumeSliderContainerElement> volumeSliderContainer = MediaControlVolumeSliderContainerElement::create(mediaElement);

        RefPtr<MediaControlVolumeSliderElement> slider = MediaControlVolumeSliderElement::create(mediaElement);
        controls->m_volumeSlider = slider.get();
        volumeSliderContainer->appendChild(slider.release(), ec, true);
        if (ec)
            return 0;

        RefPtr<MediaControlVolumeSliderMuteButtonElement> volumeSliderMuteButton = MediaControlVolumeSliderMuteButtonElement::create(mediaElement);
        controls->m_volumeSliderMuteButton = volumeSliderMuteButton.get();
        volumeSliderContainer->appendChild(volumeSliderMuteButton.release(), ec, true);
        if (ec)
            return 0;

        controls->m_volumeSliderContainer = volumeSliderContainer.get();
        panel->appendChild(volumeSliderContainer.release(), ec, true);
        if (ec)
            return 0;
    }

    // FIXME: Only create when needed <http://webkit.org/b/57163>
    RefPtr<MediaControlFullscreenVolumeMinButtonElement> fullScreenMinVolumeButton = MediaControlFullscreenVolumeMinButtonElement::create(mediaElement);
    controls->m_fullScreenMinVolumeButton = fullScreenMinVolumeButton.get();
    panel->appendChild(fullScreenMinVolumeButton.release(), ec, true);
    if (ec)
        return 0;

    RefPtr<MediaControlFullscreenVolumeSliderElement> fullScreenVolumeSlider = MediaControlFullscreenVolumeSliderElement::create(mediaElement);
    controls->m_fullScreenVolumeSlider = fullScreenVolumeSlider.get();
    panel->appendChild(fullScreenVolumeSlider.release(), ec, true);
    if (ec)
        return 0;

    RefPtr<MediaControlFullscreenVolumeMaxButtonElement> fullScreenMaxVolumeButton = MediaControlFullscreenVolumeMaxButtonElement::create(mediaElement);
    controls->m_fullScreenMaxVolumeButton = fullScreenMaxVolumeButton.get();
    panel->appendChild(fullScreenMaxVolumeButton.release(), ec, true);
    if (ec)
        return 0;

    controls->m_panel = panel.get();
    controls->appendChild(panel.release(), ec, true);
    if (ec)
        return 0;

    return controls.release();
}

void MediaControlRootElement::show()
{
    m_panel->show();
}

void MediaControlRootElement::hide()
{
    m_panel->hide();
}

static const String& webkitTransitionString()
{
    DEFINE_STATIC_LOCAL(String, s, ("-webkit-transition"));
    return s;
}

static const String& opacityString()
{
    DEFINE_STATIC_LOCAL(String, s, ("opacity"));
    return s;
}

void MediaControlRootElement::makeOpaque()
{
    if (m_opaque)
        return;

    DEFINE_STATIC_LOCAL(String, transitionValue, ());
    if (transitionValue.isNull())
        transitionValue = String::format("opacity %.1gs", document()->page()->theme()->mediaControlsFadeInDuration());
    DEFINE_STATIC_LOCAL(String, opacityValue, ("1"));

    ExceptionCode ec;
    // FIXME: Make more efficient <http://webkit.org/b/58157>
    m_panel->style()->setProperty(webkitTransitionString(), transitionValue, ec);
    m_panel->style()->setProperty(opacityString(), opacityValue, ec);
    m_opaque = true;
}

void MediaControlRootElement::makeTransparent()
{
    if (!m_opaque)
        return;

    DEFINE_STATIC_LOCAL(String, transitionValue, ());
    if (transitionValue.isNull())
        transitionValue = String::format("opacity %.1gs", document()->page()->theme()->mediaControlsFadeOutDuration());
    DEFINE_STATIC_LOCAL(String, opacityValue, ("0"));

    ExceptionCode ec;
    // FIXME: Make more efficient <http://webkit.org/b/58157>
    m_panel->style()->setProperty(webkitTransitionString(), transitionValue, ec);
    m_panel->style()->setProperty(opacityString(), opacityValue, ec);
    m_opaque = false;
}

void MediaControlRootElement::reset()
{
    Page* page = document()->page();
    if (!page)
        return;

    updateStatusDisplay();

    if (m_mediaElement->supportsFullscreen())
        m_fullScreenButton->show();
    else
        m_fullScreenButton->hide();

    float duration = m_mediaElement->duration();
    if (isfinite(duration) || page->theme()->hasOwnDisabledStateHandlingFor(MediaSliderPart)) {
        m_timeline->setDuration(duration);
        m_timelineContainer->show();
        m_timeline->setPosition(m_mediaElement->currentTime());
        updateTimeDisplay();
    } else
        m_timelineContainer->hide();

    if (m_mediaElement->hasAudio() || page->theme()->hasOwnDisabledStateHandlingFor(MediaMuteButtonPart))
        m_panelMuteButton->show();
    else
        m_panelMuteButton->hide();

    if (m_volumeSlider)
        m_volumeSlider->setVolume(m_mediaElement->volume());

    if (m_toggleClosedCaptionsButton) {
        if (m_mediaElement->hasClosedCaptions())
            m_toggleClosedCaptionsButton->show();
        else
            m_toggleClosedCaptionsButton->hide();
    }

    if (m_mediaElement->movieLoadType() != MediaPlayer::LiveStream) {
        m_returnToRealTimeButton->hide();
        m_rewindButton->show();
    } else {
        m_returnToRealTimeButton->show();
        m_rewindButton->hide();
    }

    makeOpaque();
}

void MediaControlRootElement::playbackStarted()
{
    m_playButton->updateDisplayType();
    m_timeline->setPosition(m_mediaElement->currentTime());
    updateTimeDisplay();

    if (m_mediaElement->isFullscreen())
        startHideFullscreenControlsTimer();
}

void MediaControlRootElement::playbackProgressed()
{
    m_timeline->setPosition(m_mediaElement->currentTime());
    updateTimeDisplay();
    
    if (!m_isMouseOverControls && m_mediaElement->hasVideo())
        makeTransparent();
}

void MediaControlRootElement::playbackStopped()
{
    m_playButton->updateDisplayType();
    m_timeline->setPosition(m_mediaElement->currentTime());
    updateTimeDisplay();
    makeOpaque();
    
    stopHideFullscreenControlsTimer();
}

void MediaControlRootElement::updateTimeDisplay()
{
    float now = m_mediaElement->currentTime();
    float duration = m_mediaElement->duration();

    Page* page = document()->page();
    if (!page)
        return;

    // Allow the theme to format the time.
    ExceptionCode ec;
    m_currentTimeDisplay->setInnerText(page->theme()->formatMediaControlsCurrentTime(now, duration), ec);
    m_currentTimeDisplay->setCurrentValue(now);
    m_timeRemainingDisplay->setInnerText(page->theme()->formatMediaControlsRemainingTime(now, duration), ec);
    m_timeRemainingDisplay->setCurrentValue(now - duration);
}

void MediaControlRootElement::reportedError()
{
    Page* page = document()->page();
    if (!page)
        return;

    if (!page->theme()->hasOwnDisabledStateHandlingFor(MediaSliderPart))
        m_timelineContainer->hide();

    if (!page->theme()->hasOwnDisabledStateHandlingFor(MediaMuteButtonPart))
        m_panelMuteButton->hide();

     m_fullScreenButton->hide();

    if (m_volumeSliderContainer)
        m_volumeSliderContainer->hide();
    if (m_toggleClosedCaptionsButton && !page->theme()->hasOwnDisabledStateHandlingFor(MediaToggleClosedCaptionsButtonPart))
        m_toggleClosedCaptionsButton->hide();
}

void MediaControlRootElement::updateStatusDisplay()
{
    if (m_statusDisplay)
        m_statusDisplay->update();
}

void MediaControlRootElement::loadedMetadata()
{
    if (m_statusDisplay && m_mediaElement->movieLoadType() != MediaPlayer::LiveStream)
        m_statusDisplay->hide();

    reset();
}

void MediaControlRootElement::changedClosedCaptionsVisibility()
{
    if (m_toggleClosedCaptionsButton)
        m_toggleClosedCaptionsButton->updateDisplayType();
}

void MediaControlRootElement::changedMute()
{
    m_panelMuteButton->changedMute();
    if (m_volumeSliderMuteButton)
        m_volumeSliderMuteButton->changedMute();
}

void MediaControlRootElement::changedVolume()
{
    if (m_volumeSlider)
        m_volumeSlider->setVolume(m_mediaElement->volume());
}

void MediaControlRootElement::enteredFullscreen()
{
    if (m_mediaElement->movieLoadType() == MediaPlayer::LiveStream || m_mediaElement->movieLoadType() == MediaPlayer::StoredStream) {
        m_seekBackButton->hide();
        m_seekForwardButton->hide();
    } else
        m_rewindButton->hide();

    m_panel->setCanBeDragged(true);

    startHideFullscreenControlsTimer();
}

void MediaControlRootElement::exitedFullscreen()
{
    // "show" actually means removal of display:none style, so we are just clearing styles
    // when exiting fullscreen.
    // FIXME: Clarify naming of show/hide <http://webkit.org/b/58157>
    m_rewindButton->show();
    m_seekBackButton->show();
    m_seekForwardButton->show();

    m_panel->setCanBeDragged(false);

    // We will keep using the panel, but we want it to go back to the standard position.
    // This will matter right away because we use the panel even when not fullscreen.
    // And if we reenter fullscreen we also want the panel in the standard position.
    m_panel->resetPosition();

    stopHideFullscreenControlsTimer();    
}

void MediaControlRootElement::showVolumeSlider()
{
    if (!m_mediaElement->hasAudio())
        return;

    if (m_volumeSliderContainer)
        m_volumeSliderContainer->show();
}

bool MediaControlRootElement::shouldHideControls()
{
    return !m_panel->hovered();
}

bool MediaControlRootElement::containsRelatedTarget(Event* event)
{
    if (!event->isMouseEvent())
        return false;
    EventTarget* relatedTarget = static_cast<MouseEvent*>(event)->relatedTarget();
    if (!relatedTarget)
        return false;
    return contains(relatedTarget->toNode());
}

void MediaControlRootElement::defaultEventHandler(Event* event)
{
    MediaControls::defaultEventHandler(event);

    if (event->type() == eventNames().mouseoverEvent) {
        if (!containsRelatedTarget(event)) {
            m_isMouseOverControls = true;
            if (!m_mediaElement->canPlay()) {
                makeOpaque();
                if (shouldHideControls())
                    startHideFullscreenControlsTimer();
            }
        }
    } else if (event->type() == eventNames().mouseoutEvent) {
        if (!containsRelatedTarget(event)) {
            m_isMouseOverControls = false;
            stopHideFullscreenControlsTimer();
        }
    } else if (event->type() == eventNames().mousemoveEvent) {
        if (m_mediaElement->isFullscreen()) {
            // When we get a mouse move in fullscreen mode, show the media controls, and start a timer
            // that will hide the media controls after a 3 seconds without a mouse move.
            makeOpaque();
            if (shouldHideControls())
                startHideFullscreenControlsTimer();
        }
    }
}

void MediaControlRootElement::startHideFullscreenControlsTimer()
{
    if (!m_mediaElement->isFullscreen())
        return;
    
    m_hideFullscreenControlsTimer.startOneShot(timeWithoutMouseMovementBeforeHidingControls);
}

void MediaControlRootElement::hideFullscreenControlsTimerFired(Timer<MediaControlRootElement>*)
{
    if (!m_mediaElement->isPlaying())
        return;
    
    if (!m_mediaElement->isFullscreen())
        return;
    
    if (!shouldHideControls())
        return;
    
    makeTransparent();
}

void MediaControlRootElement::stopHideFullscreenControlsTimer()
{
    m_hideFullscreenControlsTimer.stop();
}

const AtomicString& MediaControlRootElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls"));
    return id;
}

}

#endif
