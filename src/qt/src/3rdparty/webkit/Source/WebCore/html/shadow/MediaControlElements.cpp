/*
 * Copyright (C) 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
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

#include "CSSStyleSelector.h"
#include "CSSValueKeywords.h"
#include "EventNames.h"
#include "FloatConversion.h"
#include "Frame.h"
#include "HTMLNames.h"
#include "LocalizedStrings.h"
#include "MediaControls.h"
#include "MouseEvent.h"
#include "Page.h"
#include "RenderFlexibleBox.h"
#include "RenderMedia.h"
#include "RenderSlider.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "Settings.h"

namespace WebCore {

using namespace HTMLNames;

HTMLMediaElement* toParentMediaElement(RenderObject* o)
{
    Node* node = o->node();
    Node* mediaNode = node ? node->shadowAncestorNode() : 0;
    if (!mediaNode || !mediaNode->isElementNode() || !static_cast<Element*>(mediaNode)->isMediaElement())
        return 0;

    return static_cast<HTMLMediaElement*>(mediaNode);
}

// FIXME: These constants may need to be tweaked to better match the seeking in the QuickTime plug-in.
static const float cSeekRepeatDelay = 0.1f;
static const float cStepTime = 0.07f;
static const float cSeekTime = 0.2f;

// ----------------------------

MediaControlElement::MediaControlElement(HTMLMediaElement* mediaElement)
    : HTMLDivElement(divTag, mediaElement->document())
    , m_mediaElement(mediaElement)
{
}

static const String& displayString()
{
    DEFINE_STATIC_LOCAL(String, s, ("display"));
    return s;
}

void MediaControlElement::show()
{
    ExceptionCode ec;
    // FIXME: Make more efficient <http://webkit.org/b/58157>
    style()->removeProperty(displayString(), ec);
}

void MediaControlElement::hide()
{
    ExceptionCode ec;
    // FIXME: Make more efficient <http://webkit.org/b/58157>
    DEFINE_STATIC_LOCAL(String, none, ("none"));
    style()->setProperty(displayString(), none, ec);
}

// ----------------------------

inline MediaControlPanelElement::MediaControlPanelElement(HTMLMediaElement* mediaElement)
    : MediaControlElement(mediaElement)
    , m_canBeDragged(false)
    , m_isBeingDragged(false)
{
}

PassRefPtr<MediaControlPanelElement> MediaControlPanelElement::create(HTMLMediaElement* mediaElement)
{
    return adoptRef(new MediaControlPanelElement(mediaElement));
}

MediaControlElementType MediaControlPanelElement::displayType() const
{
    return MediaControlsPanel;
}

const AtomicString& MediaControlPanelElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-panel"));
    return id;
}

void MediaControlPanelElement::startDrag(const IntPoint& eventLocation)
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

    m_dragStartPosition = toRenderBox(renderer)->location();
    m_dragStartEventLocation = eventLocation;

    frame->eventHandler()->setCapturingMouseEventsNode(this);

    m_isBeingDragged = true;
}

void MediaControlPanelElement::continueDrag(const IntPoint& eventLocation)
{
    if (!m_isBeingDragged)
        return;

    IntSize distanceDragged = eventLocation - m_dragStartEventLocation;
    setPosition(m_dragStartPosition + distanceDragged);
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

void MediaControlPanelElement::setPosition(const IntPoint& position)
{
    CSSMutableStyleDeclaration* style = getInlineStyleDecl();

    double left = position.x();
    double top = position.y();

    // Set the left and top to control the panel's position; this depends on it being absolute positioned.
    // Set the margin to zero since the position passed in will already include the effect of the margin.
    style->setProperty(CSSPropertyLeft, left, CSSPrimitiveValue::CSS_PX);
    style->setProperty(CSSPropertyTop, top, CSSPrimitiveValue::CSS_PX);
    style->setProperty(CSSPropertyMarginLeft, 0.0, CSSPrimitiveValue::CSS_PX);
    style->setProperty(CSSPropertyMarginTop, 0.0, CSSPrimitiveValue::CSS_PX);
}

void MediaControlPanelElement::resetPosition()
{
    CSSMutableStyleDeclaration* style = getInlineStyleDecl();

    style->removeProperty(CSSPropertyLeft);
    style->removeProperty(CSSPropertyTop);
    style->removeProperty(CSSPropertyMarginLeft);
    style->removeProperty(CSSPropertyMarginTop);
}

void MediaControlPanelElement::defaultEventHandler(Event* event)
{
    MediaControlElement::defaultEventHandler(event);

    if (event->isMouseEvent()) {
        IntPoint location = static_cast<MouseEvent*>(event)->absoluteLocation();
        if (event->type() == eventNames().mousedownEvent) {
            startDrag(location);
            event->setDefaultHandled();
        } else if (event->type() == eventNames().mousemoveEvent)
            continueDrag(location);
        else if (event->type() == eventNames().mouseupEvent) {
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

// ----------------------------

inline MediaControlTimelineContainerElement::MediaControlTimelineContainerElement(HTMLMediaElement* mediaElement)
    : MediaControlElement(mediaElement)
{
}

PassRefPtr<MediaControlTimelineContainerElement> MediaControlTimelineContainerElement::create(HTMLMediaElement* mediaElement)
{
    RefPtr<MediaControlTimelineContainerElement> element = adoptRef(new MediaControlTimelineContainerElement(mediaElement));
    element->hide();
    return element.release();
}

MediaControlElementType MediaControlTimelineContainerElement::displayType() const
{
    return MediaTimelineContainer;
}

const AtomicString& MediaControlTimelineContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-timeline-container"));
    return id;
}

// ----------------------------

class RenderMediaVolumeSliderContainer : public RenderBlock {
public:
    RenderMediaVolumeSliderContainer(Node*);

private:
    virtual void layout();
};

RenderMediaVolumeSliderContainer::RenderMediaVolumeSliderContainer(Node* node)
    : RenderBlock(node)
{
}

void RenderMediaVolumeSliderContainer::layout()
{
    RenderBlock::layout();
    if (style()->display() == NONE || !previousSibling() || !previousSibling()->isBox())
        return;

    RenderBox* buttonBox = toRenderBox(previousSibling());

    if (view())
        view()->disableLayoutState();

    IntPoint offset = theme()->volumeSliderOffsetFromMuteButton(buttonBox, IntSize(width(), height()));
    setX(offset.x() + buttonBox->offsetLeft());
    setY(offset.y() + buttonBox->offsetTop());

    if (view())
        view()->enableLayoutState();
}

inline MediaControlVolumeSliderContainerElement::MediaControlVolumeSliderContainerElement(HTMLMediaElement* mediaElement)
    : MediaControlElement(mediaElement)
{
}

PassRefPtr<MediaControlVolumeSliderContainerElement> MediaControlVolumeSliderContainerElement::create(HTMLMediaElement* mediaElement)
{
    RefPtr<MediaControlVolumeSliderContainerElement> element = adoptRef(new MediaControlVolumeSliderContainerElement(mediaElement));
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


MediaControlElementType MediaControlVolumeSliderContainerElement::displayType() const
{
    return MediaVolumeSliderContainer;
}

const AtomicString& MediaControlVolumeSliderContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-volume-slider-container"));
    return id;
}

// ----------------------------

inline MediaControlStatusDisplayElement::MediaControlStatusDisplayElement(HTMLMediaElement* mediaElement)
    : MediaControlElement(mediaElement)
    , m_stateBeingDisplayed(Nothing)
{
}

PassRefPtr<MediaControlStatusDisplayElement> MediaControlStatusDisplayElement::create(HTMLMediaElement* mediaElement)
{
    RefPtr<MediaControlStatusDisplayElement> element = adoptRef(new MediaControlStatusDisplayElement(mediaElement));
    element->hide();
    return element.release();
}

void MediaControlStatusDisplayElement::update()
{
    // Get the new state that we'll have to display.
    StateBeingDisplayed newStateToDisplay = Nothing;

    if (mediaElement()->readyState() <= HTMLMediaElement::HAVE_METADATA && !mediaElement()->currentSrc().isEmpty())
        newStateToDisplay = Loading;
    else if (mediaElement()->movieLoadType() == MediaPlayer::LiveStream)
        newStateToDisplay = LiveBroadcast;

    if (newStateToDisplay == m_stateBeingDisplayed)
        return;

    ExceptionCode e;

    if (m_stateBeingDisplayed == Nothing)
        show();
    else if (newStateToDisplay == Nothing)
        hide();

    m_stateBeingDisplayed = newStateToDisplay;

    switch (m_stateBeingDisplayed) {
    case Nothing:
        setInnerText("", e);
        break;
    case Loading:
        setInnerText(mediaElementLoadingStateText(), e);
        break;
    case LiveBroadcast:
        setInnerText(mediaElementLiveBroadcastStateText(), e);
        break;
    }
}

MediaControlElementType MediaControlStatusDisplayElement::displayType() const
{
    return MediaStatusDisplay;
}

const AtomicString& MediaControlStatusDisplayElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-status-display"));
    return id;
}

// ----------------------------
    
MediaControlInputElement::MediaControlInputElement(HTMLMediaElement* mediaElement, MediaControlElementType displayType)
    : HTMLInputElement(inputTag, mediaElement->document(), 0, false)
    , m_mediaElement(mediaElement)
    , m_displayType(displayType)
{
}

void MediaControlInputElement::show()
{
    ExceptionCode ec;
    style()->removeProperty(displayString(), ec);
}

void MediaControlInputElement::hide()
{
    ExceptionCode ec;
    DEFINE_STATIC_LOCAL(String, none, ("none"));
    style()->setProperty(displayString(), none, ec);
}


void MediaControlInputElement::setDisplayType(MediaControlElementType displayType)
{
    if (displayType == m_displayType)
        return;

    m_displayType = displayType;
    if (RenderObject* object = renderer())
        object->repaint();
}

// ----------------------------

inline MediaControlMuteButtonElement::MediaControlMuteButtonElement(HTMLMediaElement* mediaElement, MediaControlElementType displayType)
    : MediaControlInputElement(mediaElement, displayType)
{
}

void MediaControlMuteButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        mediaElement()->setMuted(!mediaElement()->muted());
        event->setDefaultHandled();
    }

    HTMLInputElement::defaultEventHandler(event);
}

void MediaControlMuteButtonElement::changedMute()
{
    updateDisplayType();
}

void MediaControlMuteButtonElement::updateDisplayType()
{
    setDisplayType(mediaElement()->muted() ? MediaUnMuteButton : MediaMuteButton);
}

// ----------------------------

inline MediaControlPanelMuteButtonElement::MediaControlPanelMuteButtonElement(HTMLMediaElement* mediaElement, MediaControls* controls)
    : MediaControlMuteButtonElement(mediaElement, MediaMuteButton)
    , m_controls(controls)
{
}

PassRefPtr<MediaControlPanelMuteButtonElement> MediaControlPanelMuteButtonElement::create(HTMLMediaElement* mediaElement, MediaControls* controls)
{
    ASSERT(controls);

    RefPtr<MediaControlPanelMuteButtonElement> button = adoptRef(new MediaControlPanelMuteButtonElement(mediaElement, controls));
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
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-mute-button"));
    return id;
}

// ----------------------------

inline MediaControlVolumeSliderMuteButtonElement::MediaControlVolumeSliderMuteButtonElement(HTMLMediaElement* mediaElement)
    : MediaControlMuteButtonElement(mediaElement, MediaMuteButton)
{
}

PassRefPtr<MediaControlVolumeSliderMuteButtonElement> MediaControlVolumeSliderMuteButtonElement::create(HTMLMediaElement* mediaElement)
{
    RefPtr<MediaControlVolumeSliderMuteButtonElement> button = adoptRef(new MediaControlVolumeSliderMuteButtonElement(mediaElement));
    button->setType("button");
    return button.release();
}

const AtomicString& MediaControlVolumeSliderMuteButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-volume-slider-mute-button"));
    return id;
}

// ----------------------------

inline MediaControlPlayButtonElement::MediaControlPlayButtonElement(HTMLMediaElement* mediaElement)
    : MediaControlInputElement(mediaElement, MediaPlayButton)
{
}

PassRefPtr<MediaControlPlayButtonElement> MediaControlPlayButtonElement::create(HTMLMediaElement* mediaElement)
{
    RefPtr<MediaControlPlayButtonElement> button = adoptRef(new MediaControlPlayButtonElement(mediaElement));
    button->setType("button");
    return button.release();
}

void MediaControlPlayButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        mediaElement()->togglePlayState();
        updateDisplayType();
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

void MediaControlPlayButtonElement::updateDisplayType()
{
    setDisplayType(mediaElement()->canPlay() ? MediaPlayButton : MediaPauseButton);
}

const AtomicString& MediaControlPlayButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-play-button"));
    return id;
}

// ----------------------------

inline MediaControlSeekButtonElement::MediaControlSeekButtonElement(HTMLMediaElement* mediaElement, MediaControlElementType displayType)
    : MediaControlInputElement(mediaElement, displayType)
    , m_seeking(false)
    , m_capturing(false)
    , m_seekTimer(this, &MediaControlSeekButtonElement::seekTimerFired)
{
}

void MediaControlSeekButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().mousedownEvent) {
        if (Frame* frame = document()->frame()) {
            m_capturing = true;
            frame->eventHandler()->setCapturingMouseEventsNode(this);
        }
        mediaElement()->pause(event->fromUserGesture());
        m_seekTimer.startRepeating(cSeekRepeatDelay);
        event->setDefaultHandled();
    } else if (event->type() == eventNames().mouseupEvent) {
        if (m_capturing)
            if (Frame* frame = document()->frame()) {
                m_capturing = false;
                frame->eventHandler()->setCapturingMouseEventsNode(0);
            }
        ExceptionCode ec;
        if (m_seeking || m_seekTimer.isActive()) {
            if (!m_seeking) {
                float stepTime = isForwardButton() ? cStepTime : -cStepTime;
                mediaElement()->setCurrentTime(mediaElement()->currentTime() + stepTime, ec);
            }
            m_seekTimer.stop();
            m_seeking = false;
            event->setDefaultHandled();
        }
    }
    HTMLInputElement::defaultEventHandler(event);
}

void MediaControlSeekButtonElement::seekTimerFired(Timer<MediaControlSeekButtonElement>*)
{
    ExceptionCode ec;
    m_seeking = true;
    float seekTime = isForwardButton() ? cSeekTime : -cSeekTime;
    mediaElement()->setCurrentTime(mediaElement()->currentTime() + seekTime, ec);
}

void MediaControlSeekButtonElement::detach()
{
    if (m_capturing) {
        if (Frame* frame = document()->frame())
            frame->eventHandler()->setCapturingMouseEventsNode(0);      
    }
    MediaControlInputElement::detach();
}

// ----------------------------

inline MediaControlSeekForwardButtonElement::MediaControlSeekForwardButtonElement(HTMLMediaElement* mediaElement)
    : MediaControlSeekButtonElement(mediaElement, MediaSeekForwardButton)
{
}

PassRefPtr<MediaControlSeekForwardButtonElement> MediaControlSeekForwardButtonElement::create(HTMLMediaElement* mediaElement)
{
    RefPtr<MediaControlSeekForwardButtonElement> button = adoptRef(new MediaControlSeekForwardButtonElement(mediaElement));
    button->setType("button");
    return button.release();
}

const AtomicString& MediaControlSeekForwardButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-seek-forward-button"));
    return id;
}

// ----------------------------

inline MediaControlSeekBackButtonElement::MediaControlSeekBackButtonElement(HTMLMediaElement* mediaElement)
    : MediaControlSeekButtonElement(mediaElement, MediaSeekBackButton)
{
}

PassRefPtr<MediaControlSeekBackButtonElement> MediaControlSeekBackButtonElement::create(HTMLMediaElement* mediaElement)
{
    RefPtr<MediaControlSeekBackButtonElement> button = adoptRef(new MediaControlSeekBackButtonElement(mediaElement));
    button->setType("button");
    return button.release();
}

const AtomicString& MediaControlSeekBackButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-seek-back-button"));
    return id;
}

// ----------------------------

inline MediaControlRewindButtonElement::MediaControlRewindButtonElement(HTMLMediaElement* element)
    : MediaControlInputElement(element, MediaRewindButton)
{
}

PassRefPtr<MediaControlRewindButtonElement> MediaControlRewindButtonElement::create(HTMLMediaElement* mediaElement)
{
    RefPtr<MediaControlRewindButtonElement> button = adoptRef(new MediaControlRewindButtonElement(mediaElement));
    button->setType("button");
    return button.release();
}

void MediaControlRewindButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        mediaElement()->rewind(30);
        event->setDefaultHandled();
    }    
    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlRewindButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-rewind-button"));
    return id;
}

// ----------------------------

inline MediaControlReturnToRealtimeButtonElement::MediaControlReturnToRealtimeButtonElement(HTMLMediaElement* mediaElement)
    : MediaControlInputElement(mediaElement, MediaReturnToRealtimeButton)
{
}

PassRefPtr<MediaControlReturnToRealtimeButtonElement> MediaControlReturnToRealtimeButtonElement::create(HTMLMediaElement* mediaElement)
{
    RefPtr<MediaControlReturnToRealtimeButtonElement> button = adoptRef(new MediaControlReturnToRealtimeButtonElement(mediaElement));
    button->setType("button");
    button->hide();
    return button.release();
}

void MediaControlReturnToRealtimeButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        mediaElement()->returnToRealtime();
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlReturnToRealtimeButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-return-to-realtime-button"));
    return id;
}

// ----------------------------

inline MediaControlToggleClosedCaptionsButtonElement::MediaControlToggleClosedCaptionsButtonElement(HTMLMediaElement* mediaElement)
    : MediaControlInputElement(mediaElement, MediaShowClosedCaptionsButton)
{
}

PassRefPtr<MediaControlToggleClosedCaptionsButtonElement> MediaControlToggleClosedCaptionsButtonElement::create(HTMLMediaElement* mediaElement)
{
    RefPtr<MediaControlToggleClosedCaptionsButtonElement> button = adoptRef(new MediaControlToggleClosedCaptionsButtonElement(mediaElement));
    button->setType("button");
    button->hide();
    return button.release();
}

void MediaControlToggleClosedCaptionsButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        mediaElement()->setClosedCaptionsVisible(!mediaElement()->closedCaptionsVisible());
        setChecked(mediaElement()->closedCaptionsVisible());
        updateDisplayType();
        event->setDefaultHandled();
    }

    HTMLInputElement::defaultEventHandler(event);
}

void MediaControlToggleClosedCaptionsButtonElement::updateDisplayType()
{
    setDisplayType(mediaElement()->closedCaptionsVisible() ? MediaHideClosedCaptionsButton : MediaShowClosedCaptionsButton);
}

const AtomicString& MediaControlToggleClosedCaptionsButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-toggle-closed-captions-button"));
    return id;
}

// ----------------------------

MediaControlTimelineElement::MediaControlTimelineElement(HTMLMediaElement* mediaElement, MediaControls* controls)
    : MediaControlInputElement(mediaElement, MediaSlider)
    , m_controls(controls)
{
}

PassRefPtr<MediaControlTimelineElement> MediaControlTimelineElement::create(HTMLMediaElement* mediaElement, MediaControls* controls)
{
    ASSERT(controls);

    RefPtr<MediaControlTimelineElement> timeline = adoptRef(new MediaControlTimelineElement(mediaElement, controls));
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
        mediaElement()->beginScrubbing();

    if (event->type() == eventNames().mouseupEvent)
        mediaElement()->endScrubbing();

    MediaControlInputElement::defaultEventHandler(event);

    if (event->type() == eventNames().mouseoverEvent || event->type() == eventNames().mouseoutEvent || event->type() == eventNames().mousemoveEvent)
        return;

    float time = narrowPrecisionToFloat(value().toDouble());
    if (time != mediaElement()->currentTime()) {
        // FIXME: This is fired 3 times on every click. We should not be doing that <http:/webkit.org/b/58160>.
        ExceptionCode ec;
        mediaElement()->setCurrentTime(time, ec);
    }

    RenderSlider* slider = toRenderSlider(renderer());
    if (slider && slider->inDragMode())
        m_controls->updateTimeDisplay();
}

void MediaControlTimelineElement::setPosition(float currentTime) 
{
    setValue(String::number(currentTime));
}

void MediaControlTimelineElement::setDuration(float duration)
{
    setAttribute(maxAttr, String::number(isfinite(duration) ? duration : 0));
}


const AtomicString& MediaControlTimelineElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-timeline"));
    return id;
}

// ----------------------------

inline MediaControlVolumeSliderElement::MediaControlVolumeSliderElement(HTMLMediaElement* mediaElement)
    : MediaControlInputElement(mediaElement, MediaVolumeSlider)
{
}

PassRefPtr<MediaControlVolumeSliderElement> MediaControlVolumeSliderElement::create(HTMLMediaElement* mediaElement)
{
    RefPtr<MediaControlVolumeSliderElement> slider = adoptRef(new MediaControlVolumeSliderElement(mediaElement));
    slider->setType("range");
    slider->setAttribute(precisionAttr, "float");
    slider->setAttribute(maxAttr, "1");
    slider->setAttribute(valueAttr, String::number(mediaElement->volume()));
    return slider.release();
}

void MediaControlVolumeSliderElement::defaultEventHandler(Event* event)
{
    // Left button is 0. Rejects mouse events not from left button.
    if (event->isMouseEvent() && static_cast<MouseEvent*>(event)->button())
        return;

    if (!attached())
        return;

    MediaControlInputElement::defaultEventHandler(event);

    if (event->type() == eventNames().mouseoverEvent || event->type() == eventNames().mouseoutEvent || event->type() == eventNames().mousemoveEvent)
        return;

    float volume = narrowPrecisionToFloat(value().toDouble());
    if (volume != mediaElement()->volume()) {
        ExceptionCode ec = 0;
        mediaElement()->setVolume(volume, ec);
        ASSERT(!ec);
    }
}

void MediaControlVolumeSliderElement::setVolume(float volume)
{
    if (value().toFloat() != volume)
        setValue(String::number(volume));
}

const AtomicString& MediaControlVolumeSliderElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-volume-slider"));
    return id;
}

// ----------------------------

inline MediaControlFullscreenVolumeSliderElement::MediaControlFullscreenVolumeSliderElement(HTMLMediaElement* mediaElement)
    : MediaControlVolumeSliderElement(mediaElement)
{
}

PassRefPtr<MediaControlFullscreenVolumeSliderElement> MediaControlFullscreenVolumeSliderElement::create(HTMLMediaElement* mediaElement)
{
    RefPtr<MediaControlFullscreenVolumeSliderElement> slider = adoptRef(new MediaControlFullscreenVolumeSliderElement(mediaElement));
    slider->setType("range");
    slider->setAttribute(precisionAttr, "float");
    slider->setAttribute(maxAttr, "1");
    slider->setAttribute(valueAttr, String::number(mediaElement->volume()));
    return slider.release();
}

const AtomicString& MediaControlFullscreenVolumeSliderElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-volume-slider"));
    return id;
}

// ----------------------------

inline MediaControlFullscreenButtonElement::MediaControlFullscreenButtonElement(HTMLMediaElement* mediaElement, MediaControls* controls)
    : MediaControlInputElement(mediaElement, MediaFullscreenButton)
    , m_controls(controls)
{
}

PassRefPtr<MediaControlFullscreenButtonElement> MediaControlFullscreenButtonElement::create(HTMLMediaElement* mediaElement, MediaControls* controls)
{
    ASSERT(controls);

    RefPtr<MediaControlFullscreenButtonElement> button = adoptRef(new MediaControlFullscreenButtonElement(mediaElement, controls));
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
            if (document()->webkitIsFullScreen() && document()->webkitCurrentFullScreenElement() == mediaElement()) {
                document()->webkitCancelFullScreen();
                m_controls->exitedFullscreen();
            } else {
                document()->requestFullScreenForElement(mediaElement(), 0, Document::ExemptIFrameAllowFulScreenRequirement);
                m_controls->enteredFullscreen();
            }
        } else
#endif
            mediaElement()->enterFullscreen();
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlFullscreenButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-button"));
    return id;
}

// ----------------------------

inline MediaControlFullscreenVolumeMinButtonElement::MediaControlFullscreenVolumeMinButtonElement(HTMLMediaElement* mediaElement)
    : MediaControlInputElement(mediaElement, MediaUnMuteButton)
{
}

PassRefPtr<MediaControlFullscreenVolumeMinButtonElement> MediaControlFullscreenVolumeMinButtonElement::create(HTMLMediaElement* mediaElement)
{
    RefPtr<MediaControlFullscreenVolumeMinButtonElement> button = adoptRef(new MediaControlFullscreenVolumeMinButtonElement(mediaElement));
    button->setType("button");
    return button.release();
}

void MediaControlFullscreenVolumeMinButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        ExceptionCode code = 0;
        mediaElement()->setVolume(0, code);
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlFullscreenVolumeMinButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-volume-min-button"));
    return id;
}

// ----------------------------

inline MediaControlFullscreenVolumeMaxButtonElement::MediaControlFullscreenVolumeMaxButtonElement(HTMLMediaElement* mediaElement)
: MediaControlInputElement(mediaElement, MediaMuteButton)
{
}

PassRefPtr<MediaControlFullscreenVolumeMaxButtonElement> MediaControlFullscreenVolumeMaxButtonElement::create(HTMLMediaElement* mediaElement)
{
    RefPtr<MediaControlFullscreenVolumeMaxButtonElement> button = adoptRef(new MediaControlFullscreenVolumeMaxButtonElement(mediaElement));
    button->setType("button");
    return button.release();
}

void MediaControlFullscreenVolumeMaxButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        ExceptionCode code = 0;
        mediaElement()->setVolume(1, code);
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlFullscreenVolumeMaxButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-volume-max-button"));
    return id;
}

// ----------------------------

class RenderMediaControlTimeDisplay : public RenderFlexibleBox {
public:
    RenderMediaControlTimeDisplay(Node*);

private:
    virtual void layout();
};

RenderMediaControlTimeDisplay::RenderMediaControlTimeDisplay(Node* node)
    : RenderFlexibleBox(node)
{
}

// We want the timeline slider to be at least 100 pixels wide.
// FIXME: Eliminate hard-coded widths altogether.
static const int minWidthToDisplayTimeDisplays = 45 + 100 + 45;

void RenderMediaControlTimeDisplay::layout()
{
    RenderFlexibleBox::layout();
    RenderBox* timelineContainerBox = parentBox();
    while (timelineContainerBox && timelineContainerBox->isAnonymous())
        timelineContainerBox = timelineContainerBox->parentBox();

    if (timelineContainerBox && timelineContainerBox->width() < minWidthToDisplayTimeDisplays)
        setWidth(0);
}

inline MediaControlTimeDisplayElement::MediaControlTimeDisplayElement(HTMLMediaElement* mediaElement)
    : MediaControlElement(mediaElement)
    , m_currentValue(0)
{
}

void MediaControlTimeDisplayElement::setCurrentValue(float time)
{
    m_currentValue = time;
}

RenderObject* MediaControlTimeDisplayElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderMediaControlTimeDisplay(this);
}

// ----------------------------

PassRefPtr<MediaControlTimeRemainingDisplayElement> MediaControlTimeRemainingDisplayElement::create(HTMLMediaElement* mediaElement)
{
    return adoptRef(new MediaControlTimeRemainingDisplayElement(mediaElement));
}

MediaControlTimeRemainingDisplayElement::MediaControlTimeRemainingDisplayElement(HTMLMediaElement* mediaElement)
    : MediaControlTimeDisplayElement(mediaElement)
{
}

MediaControlElementType MediaControlTimeRemainingDisplayElement::displayType() const
{
    return MediaTimeRemainingDisplay;
}

const AtomicString& MediaControlTimeRemainingDisplayElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-time-remaining-display"));
    return id;
}

// ----------------------------

PassRefPtr<MediaControlCurrentTimeDisplayElement> MediaControlCurrentTimeDisplayElement::create(HTMLMediaElement* mediaElement)
{
    return adoptRef(new MediaControlCurrentTimeDisplayElement(mediaElement));
}

MediaControlCurrentTimeDisplayElement::MediaControlCurrentTimeDisplayElement(HTMLMediaElement* mediaElement)
    : MediaControlTimeDisplayElement(mediaElement)
{
}

MediaControlElementType MediaControlCurrentTimeDisplayElement::displayType() const
{
    return MediaCurrentTimeDisplay;
}

const AtomicString& MediaControlCurrentTimeDisplayElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-current-time-display"));
    return id;
}

} // namespace WebCore

#endif // ENABLE(VIDEO)
