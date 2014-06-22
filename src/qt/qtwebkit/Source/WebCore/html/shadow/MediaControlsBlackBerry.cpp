/*
 * Copyright (C) 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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
#include "MediaControlsBlackBerry.h"

#include "Chrome.h"
#include "DOMTokenList.h"
#include "ExceptionCodePlaceholder.h"
#include "Frame.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "MediaControlElements.h"
#include "MouseEvent.h"
#include "Page.h"
#include "RenderDeprecatedFlexibleBox.h"
#include "RenderSlider.h"
#include "RenderTheme.h"
#include "Settings.h"
#include "Text.h"

#if ENABLE(VIDEO_TRACK)
#include "TextTrackCue.h"
#endif

using namespace std;

namespace WebCore {

using namespace HTMLNames;

static const double timeWithoutMouseMovementBeforeHidingControls = 3;

inline MediaControlButtonGroupContainerElement::MediaControlButtonGroupContainerElement(Document* document)
    : MediaControlDivElement(document, MediaControlsPanel)
{
}

PassRefPtr<MediaControlButtonGroupContainerElement> MediaControlButtonGroupContainerElement::create(Document* document)
{
    RefPtr<MediaControlButtonGroupContainerElement> element = adoptRef(new MediaControlButtonGroupContainerElement(document));
    return element.release();
}

const AtomicString& MediaControlButtonGroupContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-button-group-container", AtomicString::ConstructFromLiteral));
    return id;
}

inline MediaControlTimeDisplayContainerElement::MediaControlTimeDisplayContainerElement(Document* document)
    : MediaControlDivElement(document, MediaControlsPanel)
{
}

PassRefPtr<MediaControlTimeDisplayContainerElement> MediaControlTimeDisplayContainerElement::create(Document* document)
{
    RefPtr<MediaControlTimeDisplayContainerElement> element = adoptRef(new MediaControlTimeDisplayContainerElement(document));
    return element.release();
}

const AtomicString& MediaControlTimeDisplayContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-time-display-container", AtomicString::ConstructFromLiteral));
    return id;
}

MediaControlEmbeddedPanelElement::MediaControlEmbeddedPanelElement(Document* document)
    : MediaControlDivElement(document, MediaControlsPanel)
    , m_canBeDragged(false)
    , m_isBeingDragged(false)
    , m_isDisplayed(false)
    , m_opaque(true)
    , m_transitionTimer(this, &MediaControlEmbeddedPanelElement::transitionTimerFired)
{
}

PassRefPtr<MediaControlEmbeddedPanelElement> MediaControlEmbeddedPanelElement::create(Document* document)
{
    return adoptRef(new MediaControlEmbeddedPanelElement(document));
}

const AtomicString& MediaControlEmbeddedPanelElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-embedded-panel", AtomicString::ConstructFromLiteral));
    return id;
}

void MediaControlEmbeddedPanelElement::startDrag(const LayoutPoint& eventLocation)
{
    if (!m_canBeDragged)
        return;

    if (m_isBeingDragged)
        return;

    RenderObject* renderer = this->renderer();
    if (!renderer || !renderer->isBox())
        return;

    Frame* frame = document()->frame();
    if (!frame)
        return;

    m_lastDragEventLocation = eventLocation;

    frame->eventHandler()->setCapturingMouseEventsNode(this);

    m_isBeingDragged = true;
}

void MediaControlEmbeddedPanelElement::continueDrag(const LayoutPoint& eventLocation)
{
    if (!m_isBeingDragged)
        return;

    LayoutSize distanceDragged = eventLocation - m_lastDragEventLocation;
    m_cumulativeDragOffset.move(distanceDragged);
    m_lastDragEventLocation = eventLocation;
    setPosition(m_cumulativeDragOffset);
}

void MediaControlEmbeddedPanelElement::endDrag()
{
    if (!m_isBeingDragged)
        return;

    m_isBeingDragged = false;

    Frame* frame = document()->frame();
    if (!frame)
        return;

    frame->eventHandler()->setCapturingMouseEventsNode(0);
}

void MediaControlEmbeddedPanelElement::startTimer()
{
    stopTimer();

    // The timer is required to set the property display:'none' on the panel,
    // such that captions are correctly displayed at the bottom of the video
    // at the end of the fadeout transition.
    double duration = document()->page() ? document()->page()->theme()->mediaControlsFadeOutDuration() : 0;
    m_transitionTimer.startOneShot(duration);
}

void MediaControlEmbeddedPanelElement::stopTimer()
{
    if (m_transitionTimer.isActive())
        m_transitionTimer.stop();
}

void MediaControlEmbeddedPanelElement::transitionTimerFired(Timer<MediaControlEmbeddedPanelElement>*)
{
    if (!m_opaque)
        hide();

    stopTimer();
}

void MediaControlEmbeddedPanelElement::setPosition(const LayoutPoint& position)
{
    double left = position.x();
    double top = position.y();

    // Set the left and top to control the panel's position; this depends on it being absolute positioned.
    // Set the margin to zero since the position passed in will already include the effect of the margin.
    setInlineStyleProperty(CSSPropertyLeft, left, CSSPrimitiveValue::CSS_PX);
    setInlineStyleProperty(CSSPropertyTop, top, CSSPrimitiveValue::CSS_PX);
    setInlineStyleProperty(CSSPropertyMarginLeft, 0.0, CSSPrimitiveValue::CSS_PX);
    setInlineStyleProperty(CSSPropertyMarginTop, 0.0, CSSPrimitiveValue::CSS_PX);

    classList()->add("dragged", IGNORE_EXCEPTION);
}

void MediaControlEmbeddedPanelElement::resetPosition()
{
    removeInlineStyleProperty(CSSPropertyLeft);
    removeInlineStyleProperty(CSSPropertyTop);
    removeInlineStyleProperty(CSSPropertyMarginLeft);
    removeInlineStyleProperty(CSSPropertyMarginTop);

    classList()->remove("dragged", IGNORE_EXCEPTION);

    m_cumulativeDragOffset.setX(0);
    m_cumulativeDragOffset.setY(0);
}

void MediaControlEmbeddedPanelElement::makeOpaque()
{
    if (m_opaque)
        return;

    double duration = document()->page() ? document()->page()->theme()->mediaControlsFadeInDuration() : 0;

    setInlineStyleProperty(CSSPropertyWebkitTransitionProperty, CSSPropertyOpacity);
    setInlineStyleProperty(CSSPropertyWebkitTransitionDuration, duration, CSSPrimitiveValue::CSS_S);
    setInlineStyleProperty(CSSPropertyOpacity, 1.0, CSSPrimitiveValue::CSS_NUMBER);

    m_opaque = true;

    if (m_isDisplayed)
        show();
}

void MediaControlEmbeddedPanelElement::makeTransparent()
{
    if (!m_opaque)
        return;

    double duration = document()->page() ? document()->page()->theme()->mediaControlsFadeOutDuration() : 0;

    setInlineStyleProperty(CSSPropertyWebkitTransitionProperty, CSSPropertyOpacity);
    setInlineStyleProperty(CSSPropertyWebkitTransitionDuration, duration, CSSPrimitiveValue::CSS_S);
    setInlineStyleProperty(CSSPropertyOpacity, 0.0, CSSPrimitiveValue::CSS_NUMBER);

    m_opaque = false;
    startTimer();
}

void MediaControlEmbeddedPanelElement::defaultEventHandler(Event* event)
{
    MediaControlDivElement::defaultEventHandler(event);

    if (event->isMouseEvent()) {
        LayoutPoint location = static_cast<MouseEvent*>(event)->absoluteLocation();
        if (event->type() == eventNames().mousedownEvent && event->target() == this) {
            startDrag(location);
            event->setDefaultHandled();
        } else if (event->type() == eventNames().mousemoveEvent && m_isBeingDragged)
            continueDrag(location);
        else if (event->type() == eventNames().mouseupEvent && m_isBeingDragged) {
            continueDrag(location);
            endDrag();
            event->setDefaultHandled();
        }
    }
}

void MediaControlEmbeddedPanelElement::setCanBeDragged(bool canBeDragged)
{
    if (m_canBeDragged == canBeDragged)
        return;

    m_canBeDragged = canBeDragged;

    if (!canBeDragged)
        endDrag();
}

void MediaControlEmbeddedPanelElement::setIsDisplayed(bool isDisplayed)
{
    m_isDisplayed = isDisplayed;
}

inline MediaControlFullscreenTimeDisplayContainerElement::MediaControlFullscreenTimeDisplayContainerElement(Document* document)
    : MediaControlDivElement(document, MediaControlsPanel)
{
}

PassRefPtr<MediaControlFullscreenTimeDisplayContainerElement> MediaControlFullscreenTimeDisplayContainerElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenTimeDisplayContainerElement> element = adoptRef(new MediaControlFullscreenTimeDisplayContainerElement(document));
    return element.release();
}

const AtomicString& MediaControlFullscreenTimeDisplayContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-time-display-container", AtomicString::ConstructFromLiteral));
    return id;
}

inline MediaControlFullscreenButtonContainerElement::MediaControlFullscreenButtonContainerElement(Document* document)
    : MediaControlDivElement(document, MediaControlsPanel)
{
}

PassRefPtr<MediaControlFullscreenButtonContainerElement> MediaControlFullscreenButtonContainerElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenButtonContainerElement> element = adoptRef(new MediaControlFullscreenButtonContainerElement(document));
    return element.release();
}

const AtomicString& MediaControlFullscreenButtonContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-button-container", AtomicString::ConstructFromLiteral));
    return id;
}

inline MediaControlFullscreenButtonDividerElement::MediaControlFullscreenButtonDividerElement(Document* document)
    : MediaControlDivElement(document, MediaRewindButton)
{
}

PassRefPtr<MediaControlFullscreenButtonDividerElement> MediaControlFullscreenButtonDividerElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenButtonDividerElement> element = adoptRef(new MediaControlFullscreenButtonDividerElement(document));
    return element.release();
}

const AtomicString& MediaControlFullscreenButtonDividerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-button-divider", AtomicString::ConstructFromLiteral));
    return id;
}

inline MediaControlPlayButtonContainerElement::MediaControlPlayButtonContainerElement(Document* document)
    : MediaControlDivElement(document, MediaControlsPanel)
{
}

PassRefPtr<MediaControlPlayButtonContainerElement> MediaControlPlayButtonContainerElement::create(Document* document)
{
    RefPtr<MediaControlPlayButtonContainerElement> element = adoptRef(new MediaControlPlayButtonContainerElement(document));
    return element.release();
}

const AtomicString& MediaControlPlayButtonContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-play-button-container", AtomicString::ConstructFromLiteral));
    return id;
}

inline MediaControlPlaceholderElement::MediaControlPlaceholderElement(Document* document)
    : MediaControlDivElement(document, MediaControlsPanel)
{
}

PassRefPtr<MediaControlPlaceholderElement> MediaControlPlaceholderElement::create(Document* document)
{
    RefPtr<MediaControlPlaceholderElement> element = adoptRef(new MediaControlPlaceholderElement(document));
    return element.release();
}

const AtomicString& MediaControlPlaceholderElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-placeholder", AtomicString::ConstructFromLiteral));
    return id;
}

inline MediaControlFullscreenPlayButtonElement::MediaControlFullscreenPlayButtonElement(Document* document)
    : MediaControlInputElement(document, MediaPlayButton)
{
}

PassRefPtr<MediaControlFullscreenPlayButtonElement> MediaControlFullscreenPlayButtonElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenPlayButtonElement> button = adoptRef(new MediaControlFullscreenPlayButtonElement(document));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    return button.release();
}

void MediaControlFullscreenPlayButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        if (mediaController()->canPlay())
            mediaController()->play();
        else
            mediaController()->pause();
        updateDisplayType();
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

void MediaControlFullscreenPlayButtonElement::updateDisplayType()
{
    setDisplayType(mediaController()->canPlay() ? MediaPlayButton : MediaPauseButton);
}

const AtomicString& MediaControlFullscreenPlayButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-play-button", AtomicString::ConstructFromLiteral));
    return id;
}

inline MediaControlFullscreenFullscreenButtonElement::MediaControlFullscreenFullscreenButtonElement(Document* document)
    : MediaControlInputElement(document, MediaExitFullscreenButton)
{
}

PassRefPtr<MediaControlFullscreenFullscreenButtonElement> MediaControlFullscreenFullscreenButtonElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenFullscreenButtonElement> button = adoptRef(new MediaControlFullscreenFullscreenButtonElement(document));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    button->hide();
    return button.release();
}

void MediaControlFullscreenFullscreenButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
#if ENABLE(FULLSCREEN_API)
        // Only use the new full screen API if the fullScreenEnabled setting has 
        // been explicitly enabled. Otherwise, use the old fullscreen API. This
        // allows apps which embed a WebView to retain the existing full screen
        // video implementation without requiring them to implement their own full 
        // screen behavior.
        if (document()->settings() && document()->settings()->fullScreenEnabled()) {
            if (document()->webkitIsFullScreen() && document()->webkitCurrentFullScreenElement() == toParentMediaElement(this))
                document()->webkitCancelFullScreen();
            else
                document()->requestFullScreenForElement(toParentMediaElement(this), 0, Document::ExemptIFrameAllowFullScreenRequirement);
        } else
#endif
            mediaController()->enterFullscreen();
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlFullscreenFullscreenButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-fullscreen-button", AtomicString::ConstructFromLiteral));
    return id;
}

void MediaControlFullscreenFullscreenButtonElement::setIsFullscreen(bool)
{
    setDisplayType(MediaExitFullscreenButton);
}

inline MediaControlFullscreenTimelineContainerElement::MediaControlFullscreenTimelineContainerElement(Document* document)
    : MediaControlDivElement(document, MediaTimelineContainer)
{
}

PassRefPtr<MediaControlFullscreenTimelineContainerElement> MediaControlFullscreenTimelineContainerElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenTimelineContainerElement> element = adoptRef(new MediaControlFullscreenTimelineContainerElement(document));
    element->hide();
    return element.release();
}

const AtomicString& MediaControlFullscreenTimelineContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-timeline-container", AtomicString::ConstructFromLiteral));
    return id;
}

MediaControlFullscreenTimelineElement::MediaControlFullscreenTimelineElement(Document* document, MediaControls* controls)
    : MediaControlInputElement(document, MediaSlider)
    , m_controls(controls)
{
}

PassRefPtr<MediaControlFullscreenTimelineElement> MediaControlFullscreenTimelineElement::create(Document* document, MediaControls* controls)
{
    ASSERT(controls);

    RefPtr<MediaControlFullscreenTimelineElement> timeline = adoptRef(new MediaControlFullscreenTimelineElement(document, controls));
    timeline->ensureUserAgentShadowRoot();
    timeline->setType("range");
    timeline->setAttribute(precisionAttr, "double");
    return timeline.release();
}

void MediaControlFullscreenTimelineElement::defaultEventHandler(Event* event)
{
    // Left button is 0. Rejects mouse events not from left button.
    if (event->isMouseEvent() && static_cast<MouseEvent*>(event)->button())
        return;

    if (!attached())
        return;

    if (event->type() == eventNames().mousedownEvent)
        mediaController()->beginScrubbing();

    if (event->type() == eventNames().mouseupEvent)
        mediaController()->endScrubbing();

    MediaControlInputElement::defaultEventHandler(event);

    if (event->type() == eventNames().mouseoverEvent || event->type() == eventNames().mouseoutEvent || event->type() == eventNames().mousemoveEvent)
        return;

    double time = value().toDouble();
    if (event->type() == eventNames().inputEvent && time != mediaController()->currentTime())
        mediaController()->setCurrentTime(time, IGNORE_EXCEPTION);

    RenderSlider* slider = toRenderSlider(renderer());
    if (slider && slider->inDragMode())
        m_controls->updateCurrentTimeDisplay();
}

bool MediaControlFullscreenTimelineElement::willRespondToMouseClickEvents()
{
    if (!attached())
        return false;

    return true;
}

void MediaControlFullscreenTimelineElement::setPosition(double currentTime)
{
    setValue(String::number(currentTime));
}

void MediaControlFullscreenTimelineElement::setDuration(double duration)
{
    setAttribute(maxAttr, String::number(std::isfinite(duration) ? duration : 0));
}

const AtomicString& MediaControlFullscreenTimelineElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-timeline", AtomicString::ConstructFromLiteral));
    return id;
}

PassRefPtr<MediaControlFullscreenTimeRemainingDisplayElement> MediaControlFullscreenTimeRemainingDisplayElement::create(Document* document)
{
    return adoptRef(new MediaControlFullscreenTimeRemainingDisplayElement(document));
}

MediaControlFullscreenTimeRemainingDisplayElement::MediaControlFullscreenTimeRemainingDisplayElement(Document* document)
    : MediaControlTimeDisplayElement(document, MediaTimeRemainingDisplay)
{
}

const AtomicString& MediaControlFullscreenTimeRemainingDisplayElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-time-remaining-display", AtomicString::ConstructFromLiteral));
    return id;
}

PassRefPtr<MediaControlFullscreenCurrentTimeDisplayElement> MediaControlFullscreenCurrentTimeDisplayElement::create(Document* document)
{
    return adoptRef(new MediaControlFullscreenCurrentTimeDisplayElement(document));
}

MediaControlFullscreenCurrentTimeDisplayElement::MediaControlFullscreenCurrentTimeDisplayElement(Document* document)
    : MediaControlTimeDisplayElement(document, MediaCurrentTimeDisplay)
{
}

const AtomicString& MediaControlFullscreenCurrentTimeDisplayElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-current-time-display", AtomicString::ConstructFromLiteral));
    return id;
}

MediaControlAudioMuteButtonElement::MediaControlAudioMuteButtonElement(Document* document, MediaControls* controls)
    : MediaControlMuteButtonElement(document, MediaMuteButton)
    , m_controls(controls)
{
}

PassRefPtr<MediaControlAudioMuteButtonElement> MediaControlAudioMuteButtonElement::create(Document* document, MediaControls* controls)
{
    ASSERT(controls);

    RefPtr<MediaControlAudioMuteButtonElement> button = adoptRef(new MediaControlAudioMuteButtonElement(document, controls));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    return button.release();
}

void MediaControlAudioMuteButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().mousedownEvent) {
        // We do not mute when the media player volume/mute control is touched.
        // Instead we show/hide the volume slider.
        static_cast<MediaControlsBlackBerry*>(m_controls)->toggleVolumeSlider();
        event->setDefaultHandled();
        return;
    }
    if (event->type() == eventNames().mouseoverEvent)
        m_controls->showVolumeSlider();

    MediaControlMuteButtonElement::defaultEventHandler(event);
}

const AtomicString& MediaControlAudioMuteButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-audio-mute-button", AtomicString::ConstructFromLiteral));
    return id;
}

MediaControlsBlackBerry::MediaControlsBlackBerry(Document* document)
    : MediaControls(document)
    , m_buttonContainer(0)
    , m_timeDisplayContainer(0)
    , m_fullscreenTimeDisplayContainer(0)
    , m_fullscreenPlayButton(0)
    , m_fullscreenCurrentTimeDisplay(0)
    , m_fullscreenTimeline(0)
    , m_timeRemainingDisplay(0)
    , m_fullscreenTimeRemainingDisplay(0)
    , m_timelineContainer(0)
    , m_fullscreenTimelineContainer(0)
    , m_fullScreenDivider(0)
    , m_fullscreenFullScreenButton(0)
    , m_muteButton(0)
    , m_volumeSliderContainer(0)
    , m_embeddedPanel(0)
    , m_fullScreenButtonContainer(0)
    , m_playButtonContainer(0)
    , m_placeholder(0)
{
}

PassRefPtr<MediaControls> MediaControls::create(Document* document)
{
    return MediaControlsBlackBerry::createControls(document);
}

PassRefPtr<MediaControlsBlackBerry> MediaControlsBlackBerry::createControls(Document* document)
{
    if (!document->page())
        return 0;

    RefPtr<MediaControlsBlackBerry> controls = adoptRef(new MediaControlsBlackBerry(document));

    RefPtr<MediaControlPanelElement> panel = MediaControlPanelElement::create(document);
    RefPtr<MediaControlEmbeddedPanelElement> embedPanel = MediaControlEmbeddedPanelElement::create(document);

    ExceptionCode ec;

    RefPtr<MediaControlPlayButtonElement> playButton = MediaControlPlayButtonElement::create(document);
    controls->m_playButton = playButton.get();
    embedPanel->appendChild(playButton.release(), ec, AttachLazily);
    if (ec)
        return 0;

    RefPtr<MediaControlTimelineContainerElement> timelineContainer = MediaControlTimelineContainerElement::create(document);

    RefPtr<MediaControlTimelineElement> timeline = MediaControlTimelineElement::create(document, controls.get());
    controls->m_timeline = timeline.get();
    timelineContainer->appendChild(timeline.release(), ec, AttachLazily);
    if (ec)
        return 0;

    RefPtr<MediaControlTimeDisplayContainerElement> timeDisplayContainer = MediaControlTimeDisplayContainerElement::create(document);

    RefPtr<MediaControlCurrentTimeDisplayElement> currentTimeDisplay = MediaControlCurrentTimeDisplayElement::create(document);
    controls->m_currentTimeDisplay = currentTimeDisplay.get();
    timeDisplayContainer->appendChild(currentTimeDisplay.release(), ec, AttachLazily);
    if (ec)
        return 0;

    RefPtr<MediaControlTimeRemainingDisplayElement> timeRemainingDisplay = MediaControlTimeRemainingDisplayElement::create(document);
    controls->m_timeRemainingDisplay = timeRemainingDisplay.get();
    timeDisplayContainer->appendChild(timeRemainingDisplay.release(), ec, AttachLazily);
    if (ec)
        return 0;

    controls->m_timeDisplayContainer = timeDisplayContainer.get();
    timelineContainer->appendChild(timeDisplayContainer.release(), ec, AttachLazily);
    if (ec)
        return 0;

    controls->m_timelineContainer = timelineContainer.get();
    embedPanel->appendChild(timelineContainer.release(), ec, AttachLazily);
    if (ec)
        return 0;

    RefPtr<MediaControlFullscreenButtonElement> fullScreenButton = MediaControlFullscreenButtonElement::create(document);
    controls->m_fullScreenButton = fullScreenButton.get();
    embedPanel->appendChild(fullScreenButton.release(), ec, AttachLazily);
    if (ec)
        return 0;

    if (document->page()->theme()->usesMediaControlVolumeSlider()) {
        // The mute button and the slider element should be in the same div.
        RefPtr<HTMLDivElement> volumeControlContainer = HTMLDivElement::create(document);

        RefPtr<MediaControlVolumeSliderContainerElement> volumeSliderContainer = MediaControlVolumeSliderContainerElement::create(document);

        RefPtr<MediaControlPanelVolumeSliderElement> slider = MediaControlPanelVolumeSliderElement::create(document);
        controls->m_volumeSlider = slider.get();
        volumeSliderContainer->appendChild(slider.release(), ec, AttachLazily);
        if (ec)
            return 0;

        controls->m_volumeSliderContainer = volumeSliderContainer.get();
        volumeControlContainer->appendChild(volumeSliderContainer.release(), ec, AttachLazily);
        if (ec)
            return 0;
        RefPtr<MediaControlAudioMuteButtonElement> muteButton = MediaControlAudioMuteButtonElement::create(document, controls.get());
        controls->m_muteButton = muteButton.get();
        volumeControlContainer->appendChild(muteButton.release(), ec, AttachLazily);
        if (ec)
            return 0;

        embedPanel->appendChild(volumeControlContainer.release(), ec, AttachLazily);
        if (ec)
            return 0;
    }

    RefPtr<MediaControlFullscreenTimelineContainerElement> fullscreenTimelineContainer = MediaControlFullscreenTimelineContainerElement::create(document);

    RefPtr<MediaControlFullscreenTimelineElement> fullscreenTimeline = MediaControlFullscreenTimelineElement::create(document, controls.get());
    controls->m_fullscreenTimeline = fullscreenTimeline.get();
    fullscreenTimelineContainer->appendChild(fullscreenTimeline.release(), ec, AttachLazily);
    if (ec)
        return 0;

    RefPtr<MediaControlFullscreenTimeDisplayContainerElement> fullscreenTimeDisplayContainer = MediaControlFullscreenTimeDisplayContainerElement::create(document);

    RefPtr<MediaControlFullscreenCurrentTimeDisplayElement> fullscreenCurrentTimeDisplay = MediaControlFullscreenCurrentTimeDisplayElement::create(document);
    controls->m_fullscreenCurrentTimeDisplay = fullscreenCurrentTimeDisplay.get();
    fullscreenTimeDisplayContainer->appendChild(fullscreenCurrentTimeDisplay.release(), ec, AttachLazily);
    if (ec)
        return 0;

    RefPtr<MediaControlFullscreenTimeRemainingDisplayElement> fullscreenTimeRemainingDisplay = MediaControlFullscreenTimeRemainingDisplayElement::create(document);
    controls->m_fullscreenTimeRemainingDisplay = fullscreenTimeRemainingDisplay.get();
    fullscreenTimeDisplayContainer->appendChild(fullscreenTimeRemainingDisplay.release(), ec, AttachLazily);
    if (ec)
        return 0;

    controls->m_fullscreenTimeDisplayContainer = fullscreenTimeDisplayContainer.get();
    fullscreenTimelineContainer->appendChild(fullscreenTimeDisplayContainer.release(), ec, AttachLazily);
    if (ec)
        return 0;

    controls->m_fullscreenTimelineContainer = fullscreenTimelineContainer.get();
    panel->appendChild(fullscreenTimelineContainer.release(), ec, AttachLazily);
    if (ec)
        return 0;

    RefPtr<MediaControlButtonGroupContainerElement> buttonGroupContainer = MediaControlButtonGroupContainerElement::create(document);

    // FIXME: Only create when needed <http://webkit.org/b/57163>
    RefPtr<MediaControlFullscreenButtonContainerElement> fullScreenButtonContainer = MediaControlFullscreenButtonContainerElement::create(document);
    controls->m_fullScreenButtonContainer = fullScreenButtonContainer.get();
    RefPtr<MediaControlFullscreenFullscreenButtonElement> fullscreenFullScreenButton = MediaControlFullscreenFullscreenButtonElement::create(document);
    controls->m_fullscreenFullScreenButton = fullscreenFullScreenButton.get();
    fullScreenButtonContainer->appendChild(fullscreenFullScreenButton.release(), ec, AttachLazily);
    if (ec)
        return 0;
    RefPtr<MediaControlFullscreenButtonDividerElement> fullScreenDivider = MediaControlFullscreenButtonDividerElement::create(document);
    controls->m_fullScreenDivider = fullScreenDivider.get();
    fullScreenButtonContainer->appendChild(fullScreenDivider.release(), ec, AttachLazily);
    if (ec)
        return 0;
    buttonGroupContainer->appendChild(fullScreenButtonContainer.release(), ec, AttachLazily);
    if (ec)
        return 0;

    RefPtr<MediaControlPlayButtonContainerElement> playButtonContainer = MediaControlPlayButtonContainerElement::create(document);
    controls->m_playButtonContainer = playButtonContainer.get();
    RefPtr<MediaControlFullscreenPlayButtonElement> fullscreenPlayButton = MediaControlFullscreenPlayButtonElement::create(document);
    controls->m_fullscreenPlayButton = fullscreenPlayButton.get();
    playButtonContainer->appendChild(fullscreenPlayButton.release(), ec, AttachLazily);
    if (ec)
        return 0;
    buttonGroupContainer->appendChild(playButtonContainer.release(), ec, AttachLazily);
    if (ec)
        return 0;

    RefPtr<MediaControlPlaceholderElement> placeholder = MediaControlPlaceholderElement::create(document);
    controls->m_placeholder = placeholder.get();
    buttonGroupContainer->appendChild(placeholder.release(), ec, AttachLazily);
    if (ec)
        return 0;

    controls->m_buttonContainer = buttonGroupContainer.get();
    panel->appendChild(buttonGroupContainer.release(), ec, AttachLazily);
    if (ec)
        return 0;

    if (document->page()->theme()->supportsClosedCaptioning()) {
        RefPtr<MediaControlToggleClosedCaptionsButtonElement> toggleClosedCaptionsButton = MediaControlToggleClosedCaptionsButtonElement::create(document, controls.get());
        controls->m_toggleClosedCaptionsButton = toggleClosedCaptionsButton.get();
        panel->appendChild(toggleClosedCaptionsButton.release(), ec, AttachLazily);
        if (ec)
            return 0;
    }

    controls->m_panel = panel.get();
    controls->appendChild(panel.release(), ec, AttachLazily);
    if (ec)
        return 0;

    controls->m_embeddedPanel = embedPanel.get();
    controls->appendChild(embedPanel.release(), ec, AttachLazily);
    if (ec)
        return 0;

    return controls.release();
}

void MediaControlsBlackBerry::setMediaController(MediaControllerInterface* controller)
{
    if (m_mediaController == controller)
        return;

    MediaControls::setMediaController(controller);

    if (m_buttonContainer)
        m_buttonContainer->setMediaController(controller);
    if (m_timeDisplayContainer)
        m_timeDisplayContainer->setMediaController(controller);
    if (m_fullscreenTimeDisplayContainer)
        m_fullscreenTimeDisplayContainer->setMediaController(controller);
    if (m_fullscreenPlayButton)
        m_fullscreenPlayButton->setMediaController(controller);
    if (m_fullscreenCurrentTimeDisplay)
        m_fullscreenCurrentTimeDisplay->setMediaController(controller);
    if (m_fullscreenTimeline)
        m_fullscreenTimeline->setMediaController(controller);
    if (m_timeRemainingDisplay)
        m_timeRemainingDisplay->setMediaController(controller);
    if (m_fullscreenTimeRemainingDisplay)
        m_fullscreenTimeRemainingDisplay->setMediaController(controller);
    if (m_timelineContainer)
        m_timelineContainer->setMediaController(controller);
    if (m_fullscreenTimelineContainer)
        m_fullscreenTimelineContainer->setMediaController(controller);
    if (m_fullscreenFullScreenButton)
        m_fullscreenFullScreenButton->setMediaController(controller);
    if (m_muteButton)
        m_muteButton->setMediaController(controller);
    if (m_volumeSliderContainer)
        m_volumeSliderContainer->setMediaController(controller);
    if (m_embeddedPanel)
        m_embeddedPanel->setMediaController(controller);
    reset();
}

void MediaControlsBlackBerry::show()
{
    if (m_isFullscreen) {
        m_panel->setIsDisplayed(true);
        m_panel->show();
    } else {
        m_embeddedPanel->setIsDisplayed(true);
        m_embeddedPanel->show();
    }
}

void MediaControlsBlackBerry::hide()
{
    if (m_isFullscreen) {
        m_panel->setIsDisplayed(false);
        m_panel->hide();
    } else {
        m_embeddedPanel->setIsDisplayed(false);
        m_embeddedPanel->hide();
        m_volumeSliderContainer->hide();
    }
}

void MediaControlsBlackBerry::makeOpaque()
{
    if (m_isFullscreen)
        m_panel->makeOpaque();
    else
        m_embeddedPanel->makeOpaque();
}

void MediaControlsBlackBerry::makeTransparent()
{
    if (m_isFullscreen)
        m_panel->makeTransparent();
    else {
        m_embeddedPanel->makeTransparent();
        m_volumeSliderContainer->hide();
    }
}

void MediaControlsBlackBerry::reset()
{
    Page* page = document()->page();
    if (!page)
        return;

    updateStatusDisplay();

    if (m_fullScreenButton) {
        if (m_mediaController->supportsFullscreen())
            m_fullScreenButton->show();
        else
            m_fullScreenButton->hide();
    }
    if (m_fullscreenFullScreenButton) {
        if (m_mediaController->supportsFullscreen())
            m_fullscreenFullScreenButton->show();
        else
            m_fullscreenFullScreenButton->hide();
    }
    double duration = m_mediaController->duration();
    if (std::isfinite(duration) || page->theme()->hasOwnDisabledStateHandlingFor(MediaSliderPart)) {
        double now = m_mediaController->currentTime();
        m_timeline->setDuration(duration);
        m_fullscreenTimeline->setDuration(duration);
        m_timelineContainer->show();
        m_fullscreenTimelineContainer->show();
        m_timeline->setPosition(now);
        m_fullscreenTimeline->setPosition(now);
        updateCurrentTimeDisplay();
    } else {
        m_timelineContainer->hide();
        m_fullscreenTimelineContainer->hide();
    }

    if (m_mediaController->hasAudio() || page->theme()->hasOwnDisabledStateHandlingFor(MediaMuteButtonPart))
        m_muteButton->show();
    else
        m_muteButton->hide();

    if (m_volumeSlider)
        m_volumeSlider->setVolume(m_mediaController->volume());

    if (m_toggleClosedCaptionsButton) {
        if (m_mediaController->hasClosedCaptions())
            m_toggleClosedCaptionsButton->show();
        else
            m_toggleClosedCaptionsButton->hide();
    }

    if (m_playButton)
        m_playButton->updateDisplayType();

    if (m_fullscreenPlayButton)
        m_fullscreenPlayButton->updateDisplayType();

    makeOpaque();
}

void MediaControlsBlackBerry::bufferingProgressed()
{
    // We only need to update buffering progress when paused, during normal
    // playback playbackProgressed() will take care of it.
    if (m_mediaController->paused()) {
        double now = m_mediaController->currentTime();
        m_timeline->setPosition(now);
        m_fullscreenTimeline->setPosition(now);
    }
}

void MediaControlsBlackBerry::playbackStarted()
{
    double now = m_mediaController->currentTime();
    m_playButton->updateDisplayType();
    m_fullscreenPlayButton->updateDisplayType();
    m_timeline->setPosition(now);
    m_fullscreenTimeline->setPosition(now);
    updateCurrentTimeDisplay();

    if (m_isFullscreen)
        startHideFullscreenControlsTimer();
}

void MediaControlsBlackBerry::playbackProgressed()
{
    double now = m_mediaController->currentTime();
    m_timeline->setPosition(now);
    m_fullscreenTimeline->setPosition(now);
    updateCurrentTimeDisplay();
    
    if (!m_isMouseOverControls && m_mediaController->hasVideo())
        makeTransparent();
}

void MediaControlsBlackBerry::playbackStopped()
{
    double now = m_mediaController->currentTime();
    m_playButton->updateDisplayType();
    m_fullscreenPlayButton->updateDisplayType();
    m_timeline->setPosition(now);
    m_fullscreenTimeline->setPosition(now);
    updateCurrentTimeDisplay();
    makeOpaque();
    
    stopHideFullscreenControlsTimer();
}

void MediaControlsBlackBerry::updateCurrentTimeDisplay()
{
    double now = m_mediaController->currentTime();
    double duration = m_mediaController->duration();

    Page* page = document()->page();
    if (!page)
        return;

    // Allow the theme to format the time.
    m_currentTimeDisplay->setInnerText(page->theme()->formatMediaControlsCurrentTime(now, duration), IGNORE_EXCEPTION);
    m_currentTimeDisplay->setCurrentValue(now);
    m_fullscreenCurrentTimeDisplay->setInnerText(page->theme()->formatMediaControlsCurrentTime(now, duration), IGNORE_EXCEPTION);
    m_fullscreenCurrentTimeDisplay->setCurrentValue(now);
    m_timeRemainingDisplay->setInnerText(page->theme()->formatMediaControlsRemainingTime(now, duration), IGNORE_EXCEPTION);
    m_timeRemainingDisplay->setCurrentValue(now - duration);
    m_fullscreenTimeRemainingDisplay->setInnerText(page->theme()->formatMediaControlsRemainingTime(now, duration), IGNORE_EXCEPTION);
    m_fullscreenTimeRemainingDisplay->setCurrentValue(now - duration);
}

void MediaControlsBlackBerry::reportedError()
{
    Page* page = document()->page();
    if (!page)
        return;

    if (!page->theme()->hasOwnDisabledStateHandlingFor(MediaSliderPart)) {
        m_timelineContainer->hide();
        m_fullscreenTimelineContainer->hide();
    }

    if (!page->theme()->hasOwnDisabledStateHandlingFor(MediaMuteButtonPart))
        m_muteButton->hide();

    if (m_fullScreenButton)
        m_fullScreenButton->hide();

    if (m_fullScreenDivider)
        m_fullScreenDivider->hide();

    if (m_fullscreenFullScreenButton)
        m_fullscreenFullScreenButton->hide();

    if (m_volumeSliderContainer)
        m_volumeSliderContainer->hide();

    if (m_toggleClosedCaptionsButton && !page->theme()->hasOwnDisabledStateHandlingFor(MediaToggleClosedCaptionsButtonPart))
        m_toggleClosedCaptionsButton->hide();
}

void MediaControlsBlackBerry::changedMute()
{
    m_muteButton->changedMute();
}

void MediaControlsBlackBerry::enteredFullscreen()
{
    MediaControls::enteredFullscreen();

    m_panel->setCanBeDragged(true);
    m_embeddedPanel->setCanBeDragged(true);

    if (m_fullscreenFullScreenButton)
        m_fullscreenFullScreenButton->setIsFullscreen(true);
}

void MediaControlsBlackBerry::exitedFullscreen()
{
    m_panel->setCanBeDragged(false);
    m_embeddedPanel->setCanBeDragged(false);

    if (m_fullscreenFullScreenButton)
        m_fullscreenFullScreenButton->setIsFullscreen(false);

    // We will keep using the panel, but we want it to go back to the standard position.
    // This will matter right away because we use the panel even when not fullscreen.
    // And if we reenter fullscreen we also want the panel in the standard position.
    m_panel->resetPosition();
    m_embeddedPanel->resetPosition();

    MediaControls::exitedFullscreen();
}

void MediaControlsBlackBerry::showVolumeSlider()
{
    if (!m_mediaController->hasAudio())
        return;

    if (m_volumeSliderContainer)
        m_volumeSliderContainer->show();
}

void MediaControlsBlackBerry::toggleVolumeSlider()
{
    if (!m_mediaController->hasAudio())
        return;

    if (m_volumeSliderContainer) {
        if (m_volumeSliderContainer->renderer() && m_volumeSliderContainer->renderer()->visibleToHitTesting())
            m_volumeSliderContainer->hide();
        else
            m_volumeSliderContainer->show();
    }
}

bool MediaControlsBlackBerry::shouldHideControls()
{
    if (m_isFullscreen)
        return !m_panel->hovered();
    return !m_embeddedPanel->hovered();
}
}

#endif
