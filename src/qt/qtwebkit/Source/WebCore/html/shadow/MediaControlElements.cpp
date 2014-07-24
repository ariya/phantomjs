/*
 * Copyright (C) 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#if ENABLE(VIDEO)
#include "MediaControlElements.h"

#include "CaptionUserPreferences.h"
#include "DOMTokenList.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "ExceptionCodePlaceholder.h"
#include "Frame.h"
#include "GraphicsContext.h"
#include "HTMLVideoElement.h"
#include "ImageBuffer.h"
#include "Language.h"
#include "LocalizedStrings.h"
#include "MediaControls.h"
#include "MouseEvent.h"
#include "Page.h"
#include "PageGroup.h"
#include "RenderLayer.h"
#include "RenderMediaControlElements.h"
#include "RenderSlider.h"
#include "RenderTheme.h"
#include "RenderVideo.h"
#include "RenderView.h"
#include "Settings.h"
#if ENABLE(VIDEO_TRACK)
#include "TextTrack.h"
#include "TextTrackList.h"
#endif

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
#include "RenderPart.h"
#endif

namespace WebCore {

using namespace HTMLNames;

static const AtomicString& getMediaControlCurrentTimeDisplayElementShadowPseudoId();
static const AtomicString& getMediaControlTimeRemainingDisplayElementShadowPseudoId();

MediaControlPanelElement::MediaControlPanelElement(Document* document)
    : MediaControlDivElement(document, MediaControlsPanel)
    , m_canBeDragged(false)
    , m_isBeingDragged(false)
    , m_isDisplayed(false)
    , m_opaque(true)
    , m_transitionTimer(this, &MediaControlPanelElement::transitionTimerFired)
{
}

PassRefPtr<MediaControlPanelElement> MediaControlPanelElement::create(Document* document)
{
    return adoptRef(new MediaControlPanelElement(document));
}

const AtomicString& MediaControlPanelElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-panel", AtomicString::ConstructFromLiteral));
    return id;
}

void MediaControlPanelElement::startDrag(const LayoutPoint& eventLocation)
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

void MediaControlPanelElement::continueDrag(const LayoutPoint& eventLocation)
{
    if (!m_isBeingDragged)
        return;

    LayoutSize distanceDragged = eventLocation - m_lastDragEventLocation;
    m_cumulativeDragOffset.move(distanceDragged);
    m_lastDragEventLocation = eventLocation;
    setPosition(m_cumulativeDragOffset);
}

void MediaControlPanelElement::endDrag()
{
    if (!m_isBeingDragged)
        return;

    m_isBeingDragged = false;

    Frame* frame = document()->frame();
    if (!frame)
        return;

    frame->eventHandler()->setCapturingMouseEventsNode(0);
}

void MediaControlPanelElement::startTimer()
{
    stopTimer();

    // The timer is required to set the property display:'none' on the panel,
    // such that captions are correctly displayed at the bottom of the video
    // at the end of the fadeout transition.
    double duration = document()->page() ? document()->page()->theme()->mediaControlsFadeOutDuration() : 0;
    m_transitionTimer.startOneShot(duration);
}

void MediaControlPanelElement::stopTimer()
{
    if (m_transitionTimer.isActive())
        m_transitionTimer.stop();
}

void MediaControlPanelElement::transitionTimerFired(Timer<MediaControlPanelElement>*)
{
    if (!m_opaque)
        hide();

    stopTimer();
}

void MediaControlPanelElement::setPosition(const LayoutPoint& position)
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

void MediaControlPanelElement::resetPosition()
{
    removeInlineStyleProperty(CSSPropertyLeft);
    removeInlineStyleProperty(CSSPropertyTop);
    removeInlineStyleProperty(CSSPropertyMarginLeft);
    removeInlineStyleProperty(CSSPropertyMarginTop);

    classList()->remove("dragged", IGNORE_EXCEPTION);

    m_cumulativeDragOffset.setX(0);
    m_cumulativeDragOffset.setY(0);
}

void MediaControlPanelElement::makeOpaque()
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

void MediaControlPanelElement::makeTransparent()
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

void MediaControlPanelElement::defaultEventHandler(Event* event)
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

void MediaControlPanelElement::setCanBeDragged(bool canBeDragged)
{
    if (m_canBeDragged == canBeDragged)
        return;

    m_canBeDragged = canBeDragged;

    if (!canBeDragged)
        endDrag();
}

void MediaControlPanelElement::setIsDisplayed(bool isDisplayed)
{
    m_isDisplayed = isDisplayed;
}

// ----------------------------

MediaControlPanelEnclosureElement::MediaControlPanelEnclosureElement(Document* document)
    // Mapping onto same MediaControlElementType as panel element, since it has similar properties.
    : MediaControlDivElement(document, MediaControlsPanel)
{
}

PassRefPtr<MediaControlPanelEnclosureElement> MediaControlPanelEnclosureElement::create(Document* document)
{
    return adoptRef(new MediaControlPanelEnclosureElement(document));
}

const AtomicString& MediaControlPanelEnclosureElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-enclosure", AtomicString::ConstructFromLiteral));
    return id;
}

// ----------------------------

MediaControlOverlayEnclosureElement::MediaControlOverlayEnclosureElement(Document* document)
    // Mapping onto same MediaControlElementType as panel element, since it has similar properties.
    : MediaControlDivElement(document, MediaControlsPanel)
{
}

PassRefPtr<MediaControlOverlayEnclosureElement> MediaControlOverlayEnclosureElement::create(Document* document)
{
    return adoptRef(new MediaControlOverlayEnclosureElement(document));
}

const AtomicString& MediaControlOverlayEnclosureElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-overlay-enclosure", AtomicString::ConstructFromLiteral));
    return id;
}

// ----------------------------

MediaControlTimelineContainerElement::MediaControlTimelineContainerElement(Document* document)
    : MediaControlDivElement(document, MediaTimelineContainer)
{
}

PassRefPtr<MediaControlTimelineContainerElement> MediaControlTimelineContainerElement::create(Document* document)
{
    RefPtr<MediaControlTimelineContainerElement> element = adoptRef(new MediaControlTimelineContainerElement(document));
    element->hide();
    return element.release();
}

const AtomicString& MediaControlTimelineContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-timeline-container", AtomicString::ConstructFromLiteral));
    return id;
}

void MediaControlTimelineContainerElement::setTimeDisplaysHidden(bool hidden)
{
    for (unsigned i = 0; i < childNodeCount(); ++i) {
        Node* child = childNode(i);
        if (!child || !child->isElementNode())
            continue;
        Element* element = static_cast<Element*>(child);
        if (element->shadowPseudoId() != getMediaControlTimeRemainingDisplayElementShadowPseudoId()
            && element->shadowPseudoId() != getMediaControlCurrentTimeDisplayElementShadowPseudoId())
            continue;

        MediaControlTimeDisplayElement* timeDisplay = static_cast<MediaControlTimeDisplayElement*>(element);
        if (hidden)
            timeDisplay->hide();
        else
            timeDisplay->show();
    }
}

RenderObject* MediaControlTimelineContainerElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderMediaControlTimelineContainer(this);
}

// ----------------------------

MediaControlVolumeSliderContainerElement::MediaControlVolumeSliderContainerElement(Document* document)
    : MediaControlDivElement(document, MediaVolumeSliderContainer)
{
}

PassRefPtr<MediaControlVolumeSliderContainerElement> MediaControlVolumeSliderContainerElement::create(Document* document)
{
    RefPtr<MediaControlVolumeSliderContainerElement> element = adoptRef(new MediaControlVolumeSliderContainerElement(document));
    element->hide();
    return element.release();
}

RenderObject* MediaControlVolumeSliderContainerElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderMediaVolumeSliderContainer(this);
}

void MediaControlVolumeSliderContainerElement::defaultEventHandler(Event* event)
{
    if (!event->isMouseEvent() || event->type() != eventNames().mouseoutEvent)
        return;

    // Poor man's mouseleave event detection.
    MouseEvent* mouseEvent = static_cast<MouseEvent*>(event);
    if (!mouseEvent->relatedTarget() || !mouseEvent->relatedTarget()->toNode())
        return;

    if (this->containsIncludingShadowDOM(mouseEvent->relatedTarget()->toNode()))
        return;

    hide();
}

const AtomicString& MediaControlVolumeSliderContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-volume-slider-container", AtomicString::ConstructFromLiteral));
    return id;
}

// ----------------------------

MediaControlStatusDisplayElement::MediaControlStatusDisplayElement(Document* document)
    : MediaControlDivElement(document, MediaStatusDisplay)
    , m_stateBeingDisplayed(Nothing)
{
}

PassRefPtr<MediaControlStatusDisplayElement> MediaControlStatusDisplayElement::create(Document* document)
{
    RefPtr<MediaControlStatusDisplayElement> element = adoptRef(new MediaControlStatusDisplayElement(document));
    element->hide();
    return element.release();
}

void MediaControlStatusDisplayElement::update()
{
    // Get the new state that we'll have to display.
    StateBeingDisplayed newStateToDisplay = Nothing;

    if (mediaController()->readyState() <= MediaControllerInterface::HAVE_METADATA && mediaController()->hasCurrentSrc())
        newStateToDisplay = Loading;
    else if (mediaController()->isLiveStream())
        newStateToDisplay = LiveBroadcast;

    if (newStateToDisplay == m_stateBeingDisplayed)
        return;

    if (m_stateBeingDisplayed == Nothing)
        show();
    else if (newStateToDisplay == Nothing)
        hide();

    m_stateBeingDisplayed = newStateToDisplay;

    switch (m_stateBeingDisplayed) {
    case Nothing:
        setInnerText("", IGNORE_EXCEPTION);
        break;
    case Loading:
        setInnerText(mediaElementLoadingStateText(), IGNORE_EXCEPTION);
        break;
    case LiveBroadcast:
        setInnerText(mediaElementLiveBroadcastStateText(), IGNORE_EXCEPTION);
        break;
    }
}

const AtomicString& MediaControlStatusDisplayElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-status-display", AtomicString::ConstructFromLiteral));
    return id;
}


// ----------------------------

MediaControlPanelMuteButtonElement::MediaControlPanelMuteButtonElement(Document* document, MediaControls* controls)
    : MediaControlMuteButtonElement(document, MediaMuteButton)
    , m_controls(controls)
{
}

PassRefPtr<MediaControlPanelMuteButtonElement> MediaControlPanelMuteButtonElement::create(Document* document, MediaControls* controls)
{
    ASSERT(controls);

    RefPtr<MediaControlPanelMuteButtonElement> button = adoptRef(new MediaControlPanelMuteButtonElement(document, controls));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    return button.release();
}

void MediaControlPanelMuteButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().mouseoverEvent)
        m_controls->showVolumeSlider();

    MediaControlMuteButtonElement::defaultEventHandler(event);
}

const AtomicString& MediaControlPanelMuteButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-mute-button", AtomicString::ConstructFromLiteral));
    return id;
}

// ----------------------------

MediaControlVolumeSliderMuteButtonElement::MediaControlVolumeSliderMuteButtonElement(Document* document)
    : MediaControlMuteButtonElement(document, MediaMuteButton)
{
}

PassRefPtr<MediaControlVolumeSliderMuteButtonElement> MediaControlVolumeSliderMuteButtonElement::create(Document* document)
{
    RefPtr<MediaControlVolumeSliderMuteButtonElement> button = adoptRef(new MediaControlVolumeSliderMuteButtonElement(document));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    return button.release();
}

const AtomicString& MediaControlVolumeSliderMuteButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-volume-slider-mute-button", AtomicString::ConstructFromLiteral));
    return id;
}

// ----------------------------

MediaControlPlayButtonElement::MediaControlPlayButtonElement(Document* document)
    : MediaControlInputElement(document, MediaPlayButton)
{
}

PassRefPtr<MediaControlPlayButtonElement> MediaControlPlayButtonElement::create(Document* document)
{
    RefPtr<MediaControlPlayButtonElement> button = adoptRef(new MediaControlPlayButtonElement(document));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    return button.release();
}

void MediaControlPlayButtonElement::defaultEventHandler(Event* event)
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

void MediaControlPlayButtonElement::updateDisplayType()
{
    setDisplayType(mediaController()->canPlay() ? MediaPlayButton : MediaPauseButton);
}

const AtomicString& MediaControlPlayButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-play-button", AtomicString::ConstructFromLiteral));
    return id;
}

// ----------------------------

MediaControlOverlayPlayButtonElement::MediaControlOverlayPlayButtonElement(Document* document)
    : MediaControlInputElement(document, MediaOverlayPlayButton)
{
}

PassRefPtr<MediaControlOverlayPlayButtonElement> MediaControlOverlayPlayButtonElement::create(Document* document)
{
    RefPtr<MediaControlOverlayPlayButtonElement> button = adoptRef(new MediaControlOverlayPlayButtonElement(document));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    return button.release();
}

void MediaControlOverlayPlayButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent && mediaController()->canPlay()) {
        mediaController()->play();
        updateDisplayType();
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

void MediaControlOverlayPlayButtonElement::updateDisplayType()
{
    if (mediaController()->canPlay()) {
        show();
    } else
        hide();
}

const AtomicString& MediaControlOverlayPlayButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-overlay-play-button", AtomicString::ConstructFromLiteral));
    return id;
}


// ----------------------------

MediaControlSeekForwardButtonElement::MediaControlSeekForwardButtonElement(Document* document)
    : MediaControlSeekButtonElement(document, MediaSeekForwardButton)
{
}

PassRefPtr<MediaControlSeekForwardButtonElement> MediaControlSeekForwardButtonElement::create(Document* document)
{
    RefPtr<MediaControlSeekForwardButtonElement> button = adoptRef(new MediaControlSeekForwardButtonElement(document));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    return button.release();
}

const AtomicString& MediaControlSeekForwardButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-seek-forward-button", AtomicString::ConstructFromLiteral));
    return id;
}

// ----------------------------

MediaControlSeekBackButtonElement::MediaControlSeekBackButtonElement(Document* document)
    : MediaControlSeekButtonElement(document, MediaSeekBackButton)
{
}

PassRefPtr<MediaControlSeekBackButtonElement> MediaControlSeekBackButtonElement::create(Document* document)
{
    RefPtr<MediaControlSeekBackButtonElement> button = adoptRef(new MediaControlSeekBackButtonElement(document));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    return button.release();
}

const AtomicString& MediaControlSeekBackButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-seek-back-button", AtomicString::ConstructFromLiteral));
    return id;
}

// ----------------------------

MediaControlRewindButtonElement::MediaControlRewindButtonElement(Document* document)
    : MediaControlInputElement(document, MediaRewindButton)
{
}

PassRefPtr<MediaControlRewindButtonElement> MediaControlRewindButtonElement::create(Document* document)
{
    RefPtr<MediaControlRewindButtonElement> button = adoptRef(new MediaControlRewindButtonElement(document));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    return button.release();
}

void MediaControlRewindButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        mediaController()->setCurrentTime(max(0.0, mediaController()->currentTime() - 30), IGNORE_EXCEPTION);
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlRewindButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-rewind-button", AtomicString::ConstructFromLiteral));
    return id;
}

// ----------------------------

MediaControlReturnToRealtimeButtonElement::MediaControlReturnToRealtimeButtonElement(Document* document)
    : MediaControlInputElement(document, MediaReturnToRealtimeButton)
{
}

PassRefPtr<MediaControlReturnToRealtimeButtonElement> MediaControlReturnToRealtimeButtonElement::create(Document* document)
{
    RefPtr<MediaControlReturnToRealtimeButtonElement> button = adoptRef(new MediaControlReturnToRealtimeButtonElement(document));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    button->hide();
    return button.release();
}

void MediaControlReturnToRealtimeButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        mediaController()->returnToRealtime();
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlReturnToRealtimeButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-return-to-realtime-button", AtomicString::ConstructFromLiteral));
    return id;
}

// ----------------------------

MediaControlToggleClosedCaptionsButtonElement::MediaControlToggleClosedCaptionsButtonElement(Document* document, MediaControls* controls)
    : MediaControlInputElement(document, MediaShowClosedCaptionsButton)
#if PLATFORM(MAC) || PLATFORM(WIN) || PLATFORM(QT)
    , m_controls(controls)
#endif
{
#if !PLATFORM(MAC) && !PLATFORM(WIN) && !PLATFORM(QT)
    UNUSED_PARAM(controls);
#endif
}

PassRefPtr<MediaControlToggleClosedCaptionsButtonElement> MediaControlToggleClosedCaptionsButtonElement::create(Document* document, MediaControls* controls)
{
    ASSERT(controls);

    RefPtr<MediaControlToggleClosedCaptionsButtonElement> button = adoptRef(new MediaControlToggleClosedCaptionsButtonElement(document, controls));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    button->hide();
    return button.release();
}

void MediaControlToggleClosedCaptionsButtonElement::updateDisplayType()
{
    bool captionsVisible = mediaController()->closedCaptionsVisible();
    setDisplayType(captionsVisible ? MediaHideClosedCaptionsButton : MediaShowClosedCaptionsButton);
    setChecked(captionsVisible);
}

void MediaControlToggleClosedCaptionsButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        // FIXME: It's not great that the shared code is dictating behavior of platform-specific
        // UI. Not all ports may want the closed captions button to toggle a list of tracks, so
        // we have to use #if.
        // https://bugs.webkit.org/show_bug.cgi?id=101877
#if !PLATFORM(MAC) && !PLATFORM(WIN) && !PLATFORM(QT)
        mediaController()->setClosedCaptionsVisible(!mediaController()->closedCaptionsVisible());
        setChecked(mediaController()->closedCaptionsVisible());
        updateDisplayType();
#else
        m_controls->toggleClosedCaptionTrackList();
#endif
        event->setDefaultHandled();
    }

    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlToggleClosedCaptionsButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-toggle-closed-captions-button", AtomicString::ConstructFromLiteral));
    return id;
}

// ----------------------------

MediaControlClosedCaptionsContainerElement::MediaControlClosedCaptionsContainerElement(Document* document)
    : MediaControlDivElement(document, MediaClosedCaptionsContainer)
{
}

PassRefPtr<MediaControlClosedCaptionsContainerElement> MediaControlClosedCaptionsContainerElement::create(Document* document)
{
    RefPtr<MediaControlClosedCaptionsContainerElement> element = adoptRef(new MediaControlClosedCaptionsContainerElement(document));
    element->setAttribute(dirAttr, "auto");
    element->hide();
    return element.release();
}

const AtomicString& MediaControlClosedCaptionsContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-closed-captions-container", AtomicString::ConstructFromLiteral));
    return id;
}

// ----------------------------

MediaControlClosedCaptionsTrackListElement::MediaControlClosedCaptionsTrackListElement(Document* document, MediaControls* controls)
    : MediaControlDivElement(document, MediaClosedCaptionsTrackList)
    , m_controls(controls)
{
}

PassRefPtr<MediaControlClosedCaptionsTrackListElement> MediaControlClosedCaptionsTrackListElement::create(Document* document, MediaControls* controls)
{
    ASSERT(controls);
    RefPtr<MediaControlClosedCaptionsTrackListElement> element = adoptRef(new MediaControlClosedCaptionsTrackListElement(document, controls));
    return element.release();
}

void MediaControlClosedCaptionsTrackListElement::defaultEventHandler(Event* event)
{
#if ENABLE(VIDEO_TRACK)
    if (event->type() == eventNames().clickEvent) {
        Node* target = event->target()->toNode();
        if (!target || !target->isElementNode())
            return;

        // When we created the elements in the track list, we gave them a custom
        // attribute representing the index in the HTMLMediaElement's list of tracks.
        // Check if the event target has such a custom element and, if so,
        // tell the HTMLMediaElement to enable that track.

        RefPtr<TextTrack> textTrack;
        MenuItemToTrackMap::iterator iter = m_menuToTrackMap.find(toElement(target));
        if (iter != m_menuToTrackMap.end())
            textTrack = iter->value;
        m_menuToTrackMap.clear();
        m_controls->toggleClosedCaptionTrackList();
        if (!textTrack)
            return;

        HTMLMediaElement* mediaElement = toParentMediaElement(this);
        if (!mediaElement)
            return;

        mediaElement->setSelectedTextTrack(textTrack.get());

        updateDisplay();
    }

    MediaControlDivElement::defaultEventHandler(event);
#endif
}

const AtomicString& MediaControlClosedCaptionsTrackListElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-closed-captions-track-list", AtomicString::ConstructFromLiteral));
    return id;
}

void MediaControlClosedCaptionsTrackListElement::updateDisplay()
{
#if ENABLE(VIDEO_TRACK)
    DEFINE_STATIC_LOCAL(AtomicString, selectedClassValue, ("selected", AtomicString::ConstructFromLiteral));

    if (!mediaController()->hasClosedCaptions())
        return;

    if (!document()->page())
        return;
    CaptionUserPreferences::CaptionDisplayMode displayMode = document()->page()->group().captionPreferences()->captionDisplayMode();

    HTMLMediaElement* mediaElement = toParentMediaElement(this);
    if (!mediaElement)
        return;

    TextTrackList* trackList = mediaElement->textTracks();
    if (!trackList || !trackList->length())
        return;

    rebuildTrackListMenu();

    RefPtr<Element> offMenuItem;
    bool trackMenuItemSelected = false;

    for (unsigned i = 0, length = m_menuItems.size(); i < length; ++i) {
        RefPtr<Element> trackItem = m_menuItems[i];

        RefPtr<TextTrack> textTrack;
        MenuItemToTrackMap::iterator iter = m_menuToTrackMap.find(trackItem.get());
        if (iter == m_menuToTrackMap.end())
            continue;
        textTrack = iter->value;
        if (!textTrack)
            continue;

        if (textTrack == TextTrack::captionMenuOffItem()) {
            offMenuItem = trackItem;
            continue;
        }

        if (textTrack == TextTrack::captionMenuAutomaticItem()) {
            if (displayMode == CaptionUserPreferences::Automatic)
                trackItem->classList()->add(selectedClassValue, ASSERT_NO_EXCEPTION);
            else
                trackItem->classList()->remove(selectedClassValue, ASSERT_NO_EXCEPTION);
            continue;
        }

        if (displayMode != CaptionUserPreferences::Automatic && textTrack->mode() == TextTrack::showingKeyword()) {
            trackMenuItemSelected = true;
            trackItem->classList()->add(selectedClassValue, ASSERT_NO_EXCEPTION);
        } else
            trackItem->classList()->remove(selectedClassValue, ASSERT_NO_EXCEPTION);
    }

    if (offMenuItem) {
        if (displayMode == CaptionUserPreferences::ForcedOnly && !trackMenuItemSelected)
            offMenuItem->classList()->add(selectedClassValue, ASSERT_NO_EXCEPTION);
        else
            offMenuItem->classList()->remove(selectedClassValue, ASSERT_NO_EXCEPTION);
    }
#endif
}

void MediaControlClosedCaptionsTrackListElement::rebuildTrackListMenu()
{
#if ENABLE(VIDEO_TRACK)
    // Remove any existing content.
    removeChildren();
    m_menuItems.clear();
    m_menuToTrackMap.clear();

    if (!mediaController()->hasClosedCaptions())
        return;

    HTMLMediaElement* mediaElement = toParentMediaElement(this);
    if (!mediaElement)
        return;

    TextTrackList* trackList = mediaElement->textTracks();
    if (!trackList || !trackList->length())
        return;

    Document* doc = document();
    if (!document()->page())
        return;
    CaptionUserPreferences* captionPreferences = document()->page()->group().captionPreferences();
    Vector<RefPtr<TextTrack> > tracksForMenu = captionPreferences->sortedTrackListForMenu(trackList);

    RefPtr<Element> captionsHeader = doc->createElement(h3Tag, ASSERT_NO_EXCEPTION);
    captionsHeader->appendChild(doc->createTextNode(textTrackSubtitlesText()));
    appendChild(captionsHeader);
    RefPtr<Element> captionsMenuList = doc->createElement(ulTag, ASSERT_NO_EXCEPTION);

    for (unsigned i = 0, length = tracksForMenu.size(); i < length; ++i) {
        RefPtr<TextTrack> textTrack = tracksForMenu[i];
        RefPtr<Element> menuItem = doc->createElement(liTag, ASSERT_NO_EXCEPTION);
        menuItem->appendChild(doc->createTextNode(captionPreferences->displayNameForTrack(textTrack.get())));
        captionsMenuList->appendChild(menuItem);
        m_menuItems.append(menuItem);
        m_menuToTrackMap.add(menuItem, textTrack);
    }

    appendChild(captionsMenuList);
#endif
}

// ----------------------------

MediaControlTimelineElement::MediaControlTimelineElement(Document* document, MediaControls* controls)
    : MediaControlInputElement(document, MediaSlider)
    , m_controls(controls)
{
}

PassRefPtr<MediaControlTimelineElement> MediaControlTimelineElement::create(Document* document, MediaControls* controls)
{
    ASSERT(controls);

    RefPtr<MediaControlTimelineElement> timeline = adoptRef(new MediaControlTimelineElement(document, controls));
    timeline->ensureUserAgentShadowRoot();
    timeline->setType("range");
    timeline->setAttribute(precisionAttr, "float");
    return timeline.release();
}

void MediaControlTimelineElement::defaultEventHandler(Event* event)
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

bool MediaControlTimelineElement::willRespondToMouseClickEvents()
{
    if (!attached())
        return false;

    return true;
}

void MediaControlTimelineElement::setPosition(double currentTime)
{
    setValue(String::number(currentTime));
}

void MediaControlTimelineElement::setDuration(double duration)
{
    setAttribute(maxAttr, String::number(std::isfinite(duration) ? duration : 0));
}


const AtomicString& MediaControlTimelineElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-timeline", AtomicString::ConstructFromLiteral));
    return id;
}

// ----------------------------

MediaControlPanelVolumeSliderElement::MediaControlPanelVolumeSliderElement(Document* document)
    : MediaControlVolumeSliderElement(document)
{
}

PassRefPtr<MediaControlPanelVolumeSliderElement> MediaControlPanelVolumeSliderElement::create(Document* document)
{
    RefPtr<MediaControlPanelVolumeSliderElement> slider = adoptRef(new MediaControlPanelVolumeSliderElement(document));
    slider->ensureUserAgentShadowRoot();
    slider->setType("range");
    slider->setAttribute(precisionAttr, "float");
    slider->setAttribute(maxAttr, "1");
    return slider.release();
}

const AtomicString& MediaControlPanelVolumeSliderElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-volume-slider", AtomicString::ConstructFromLiteral));
    return id;
}

// ----------------------------

MediaControlFullscreenVolumeSliderElement::MediaControlFullscreenVolumeSliderElement(Document* document)
    : MediaControlVolumeSliderElement(document)
{
}

PassRefPtr<MediaControlFullscreenVolumeSliderElement> MediaControlFullscreenVolumeSliderElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenVolumeSliderElement> slider = adoptRef(new MediaControlFullscreenVolumeSliderElement(document));
    slider->ensureUserAgentShadowRoot();
    slider->setType("range");
    slider->setAttribute(precisionAttr, "float");
    slider->setAttribute(maxAttr, "1");
    return slider.release();
}

const AtomicString& MediaControlFullscreenVolumeSliderElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-volume-slider", AtomicString::ConstructFromLiteral));
    return id;
}

// ----------------------------

MediaControlFullscreenButtonElement::MediaControlFullscreenButtonElement(Document* document)
    : MediaControlInputElement(document, MediaEnterFullscreenButton)
{
}

PassRefPtr<MediaControlFullscreenButtonElement> MediaControlFullscreenButtonElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenButtonElement> button = adoptRef(new MediaControlFullscreenButtonElement(document));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    button->hide();
    return button.release();
}

void MediaControlFullscreenButtonElement::defaultEventHandler(Event* event)
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

const AtomicString& MediaControlFullscreenButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-button", AtomicString::ConstructFromLiteral));
    return id;
}

void MediaControlFullscreenButtonElement::setIsFullscreen(bool isFullscreen)
{
    setDisplayType(isFullscreen ? MediaExitFullscreenButton : MediaEnterFullscreenButton);
}

// ----------------------------

MediaControlFullscreenVolumeMinButtonElement::MediaControlFullscreenVolumeMinButtonElement(Document* document)
    : MediaControlInputElement(document, MediaUnMuteButton)
{
}

PassRefPtr<MediaControlFullscreenVolumeMinButtonElement> MediaControlFullscreenVolumeMinButtonElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenVolumeMinButtonElement> button = adoptRef(new MediaControlFullscreenVolumeMinButtonElement(document));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    return button.release();
}

void MediaControlFullscreenVolumeMinButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        ExceptionCode code = 0;
        mediaController()->setVolume(0, code);
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlFullscreenVolumeMinButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-volume-min-button", AtomicString::ConstructFromLiteral));
    return id;
}

// ----------------------------

MediaControlFullscreenVolumeMaxButtonElement::MediaControlFullscreenVolumeMaxButtonElement(Document* document)
: MediaControlInputElement(document, MediaMuteButton)
{
}

PassRefPtr<MediaControlFullscreenVolumeMaxButtonElement> MediaControlFullscreenVolumeMaxButtonElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenVolumeMaxButtonElement> button = adoptRef(new MediaControlFullscreenVolumeMaxButtonElement(document));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    return button.release();
}

void MediaControlFullscreenVolumeMaxButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        ExceptionCode code = 0;
        mediaController()->setVolume(1, code);
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlFullscreenVolumeMaxButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-volume-max-button", AtomicString::ConstructFromLiteral));
    return id;
}

// ----------------------------

MediaControlTimeRemainingDisplayElement::MediaControlTimeRemainingDisplayElement(Document* document)
    : MediaControlTimeDisplayElement(document, MediaTimeRemainingDisplay)
{
}

PassRefPtr<MediaControlTimeRemainingDisplayElement> MediaControlTimeRemainingDisplayElement::create(Document* document)
{
    return adoptRef(new MediaControlTimeRemainingDisplayElement(document));
}

static const AtomicString& getMediaControlTimeRemainingDisplayElementShadowPseudoId()
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-time-remaining-display", AtomicString::ConstructFromLiteral));
    return id;
}

const AtomicString& MediaControlTimeRemainingDisplayElement::shadowPseudoId() const
{
    return getMediaControlTimeRemainingDisplayElementShadowPseudoId();
}

// ----------------------------

MediaControlCurrentTimeDisplayElement::MediaControlCurrentTimeDisplayElement(Document* document)
    : MediaControlTimeDisplayElement(document, MediaCurrentTimeDisplay)
{
}

PassRefPtr<MediaControlCurrentTimeDisplayElement> MediaControlCurrentTimeDisplayElement::create(Document* document)
{
    return adoptRef(new MediaControlCurrentTimeDisplayElement(document));
}

static const AtomicString& getMediaControlCurrentTimeDisplayElementShadowPseudoId()
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-current-time-display", AtomicString::ConstructFromLiteral));
    return id;
}

const AtomicString& MediaControlCurrentTimeDisplayElement::shadowPseudoId() const
{
    return getMediaControlCurrentTimeDisplayElementShadowPseudoId();
}

// ----------------------------

#if ENABLE(VIDEO_TRACK)

MediaControlTextTrackContainerElement::MediaControlTextTrackContainerElement(Document* document)
    : MediaControlDivElement(document, MediaTextTrackDisplayContainer)
    , m_updateTimer(this, &MediaControlTextTrackContainerElement::updateTimerFired)
    , m_fontSize(0)
    , m_fontSizeIsImportant(false)
{
}

PassRefPtr<MediaControlTextTrackContainerElement> MediaControlTextTrackContainerElement::create(Document* document)
{
    RefPtr<MediaControlTextTrackContainerElement> element = adoptRef(new MediaControlTextTrackContainerElement(document));
    element->hide();
    return element.release();
}

RenderObject* MediaControlTextTrackContainerElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderTextTrackContainerElement(this);
}

const AtomicString& MediaControlTextTrackContainerElement::textTrackContainerElementShadowPseudoId()
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-text-track-container", AtomicString::ConstructFromLiteral));
    return id;
}
    
const AtomicString& MediaControlTextTrackContainerElement::shadowPseudoId() const
{
    return textTrackContainerElementShadowPseudoId();
}

void MediaControlTextTrackContainerElement::updateDisplay()
{
    if (!mediaController()->closedCaptionsVisible())
        removeChildren();

    HTMLMediaElement* mediaElement = toParentMediaElement(this);
    // 1. If the media element is an audio element, or is another playback
    // mechanism with no rendering area, abort these steps. There is nothing to
    // render.
    if (!mediaElement || !mediaElement->isVideo())
        return;

    // 2. Let video be the media element or other playback mechanism.
    HTMLVideoElement* video = toHTMLVideoElement(mediaElement);

    // 3. Let output be an empty list of absolutely positioned CSS block boxes.
    Vector<RefPtr<HTMLDivElement> > output;

    // 4. If the user agent is exposing a user interface for video, add to
    // output one or more completely transparent positioned CSS block boxes that
    // cover the same region as the user interface.

    // 5. If the last time these rules were run, the user agent was not exposing
    // a user interface for video, but now it is, let reset be true. Otherwise,
    // let reset be false.

    // There is nothing to be done explicitly for 4th and 5th steps, as
    // everything is handled through CSS. The caption box is on top of the
    // controls box, in a container set with the -webkit-box display property.

    // 6. Let tracks be the subset of video's list of text tracks that have as
    // their rules for updating the text track rendering these rules for
    // updating the display of WebVTT text tracks, and whose text track mode is
    // showing or showing by default.
    // 7. Let cues be an empty list of text track cues.
    // 8. For each track track in tracks, append to cues all the cues from
    // track's list of cues that have their text track cue active flag set.
    CueList activeCues = video->currentlyActiveCues();

    // 9. If reset is false, then, for each text track cue cue in cues: if cue's
    // text track cue display state has a set of CSS boxes, then add those boxes
    // to output, and remove cue from cues.

    // There is nothing explicitly to be done here, as all the caching occurs
    // within the TextTrackCue instance itself. If parameters of the cue change,
    // the display tree is cleared.

    // 10. For each text track cue cue in cues that has not yet had
    // corresponding CSS boxes added to output, in text track cue order, run the
    // following substeps:
    for (size_t i = 0; i < activeCues.size(); ++i) {
        TextTrackCue* cue = activeCues[i].data();

        ASSERT(cue->isActive());
        if (!cue->track() || !cue->track()->isRendered() || !cue->isActive() || cue->text().isEmpty())
            continue;

        RefPtr<TextTrackCueBox> displayBox = cue->getDisplayTree(m_videoDisplaySize.size());
        if (displayBox->hasChildNodes() && !contains(displayBox.get())) {
            // Note: the display tree of a cue is removed when the active flag of the cue is unset.
            appendChild(displayBox, ASSERT_NO_EXCEPTION, AttachNow);
            cue->setFontSize(m_fontSize, m_videoDisplaySize.size(), m_fontSizeIsImportant);
        }
    }

    // 11. Return output.
    if (hasChildNodes()) {
        show();
        if (mediaElement->requiresTextTrackRepresentation()) {
            if (!m_textTrackRepresentation)
                m_textTrackRepresentation = TextTrackRepresentation::create(this);
            mediaElement->setTextTrackRepresentation(m_textTrackRepresentation.get());

            if (Page* page = document()->page())
                m_textTrackRepresentation->setContentScale(page->deviceScaleFactor());

            m_textTrackRepresentation->update();
            setInlineStyleProperty(CSSPropertyWidth, String::number(m_videoDisplaySize.size().width()) + "px");
            setInlineStyleProperty(CSSPropertyHeight, String::number(m_videoDisplaySize.size().height()) + "px");
        }
    } else {
        hide();
        clearTextTrackRepresentation();
    }
}

void MediaControlTextTrackContainerElement::updateTimerFired(Timer<MediaControlTextTrackContainerElement>*)
{
    if (!document()->page())
        return;

    if (m_textTrackRepresentation) {
        setInlineStyleProperty(CSSPropertyWidth, String::number(m_videoDisplaySize.size().width()) + "px");
        setInlineStyleProperty(CSSPropertyHeight, String::number(m_videoDisplaySize.size().height()) + "px");
    }
    
    HTMLMediaElement* mediaElement = toParentMediaElement(this);
    if (!mediaElement)
        return;

    float smallestDimension = std::min(m_videoDisplaySize.size().height(), m_videoDisplaySize.size().width());
    float fontScale = document()->page()->group().captionPreferences()->captionFontSizeScaleAndImportance(m_fontSizeIsImportant);
    m_fontSize = lroundf(smallestDimension * fontScale);
    
    CueList activeCues = mediaElement->currentlyActiveCues();
    for (size_t i = 0; i < activeCues.size(); ++i) {
        TextTrackCue* cue = activeCues[i].data();
        cue->setFontSize(m_fontSize, m_videoDisplaySize.size(), m_fontSizeIsImportant);
    }
    updateDisplay();
}

void MediaControlTextTrackContainerElement::clearTextTrackRepresentation()
{
    if (HTMLMediaElement* mediaElement = toParentMediaElement(this))
        mediaElement->setTextTrackRepresentation(0);
    m_textTrackRepresentation = nullptr;
    removeInlineStyleProperty(CSSPropertyPosition);
    removeInlineStyleProperty(CSSPropertyWidth);
    removeInlineStyleProperty(CSSPropertyHeight);
    removeInlineStyleProperty(CSSPropertyLeft);
    removeInlineStyleProperty(CSSPropertyTop);
}

void MediaControlTextTrackContainerElement::enteredFullscreen()
{
    updateSizes(true);
}

void MediaControlTextTrackContainerElement::exitedFullscreen()
{
    clearTextTrackRepresentation();
    updateSizes(true);
}

void MediaControlTextTrackContainerElement::updateSizes(bool forceUpdate)
{
    HTMLMediaElement* mediaElement = toParentMediaElement(this);
    if (!mediaElement)
        return;

    if (!document()->page())
        return;

    IntRect videoBox;

    if (m_textTrackRepresentation)
        videoBox = m_textTrackRepresentation->bounds();
    else {
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
        if (!mediaElement->renderer() || !mediaElement->renderer()->isRenderPart())
            return;
        videoBox = pixelSnappedIntRect(toRenderPart(mediaElement->renderer())->contentBoxRect());
#else
        if (!mediaElement->renderer() || !mediaElement->renderer()->isVideo())
            return;
        videoBox = toRenderVideo(mediaElement->renderer())->videoBox();
#endif
    }

    if (!forceUpdate && m_videoDisplaySize == videoBox)
        return;
    m_videoDisplaySize = videoBox;

    m_updateTimer.startOneShot(0);
}

PassRefPtr<Image> MediaControlTextTrackContainerElement::createTextTrackRepresentationImage()
{
    if (!hasChildNodes())
        return 0;

    Frame* frame = document()->frame();
    if (!frame)
        return 0;

    document()->updateLayout();

    RenderObject* renderer = this->renderer();
    if (!renderer)
        return 0;

    if (!renderer->hasLayer())
        return 0;

    RenderLayer* layer = toRenderLayerModelObject(renderer)->layer();

    float deviceScaleFactor = 1;
    if (Page* page = document()->page())
        deviceScaleFactor = page->deviceScaleFactor();

    IntRect paintingRect = IntRect(IntPoint(), layer->size());
    OwnPtr<ImageBuffer> buffer(ImageBuffer::create(paintingRect.size(), deviceScaleFactor, ColorSpaceDeviceRGB));
    if (!buffer)
        return 0;

    layer->paint(buffer->context(), paintingRect, PaintBehaviorFlattenCompositingLayers, 0, 0, RenderLayer::PaintLayerPaintingCompositingAllPhases);

    return buffer->copyImage();
}

void MediaControlTextTrackContainerElement::textTrackRepresentationBoundsChanged(const IntRect&)
{
    updateSizes();
}

#endif // ENABLE(VIDEO_TRACK)

// ----------------------------

} // namespace WebCore

#endif // ENABLE(VIDEO)
