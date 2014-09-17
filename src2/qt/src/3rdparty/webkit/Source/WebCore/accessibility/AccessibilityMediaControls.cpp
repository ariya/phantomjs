/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#include "AccessibilityMediaControls.h"

#include "AXObjectCache.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "LocalizedStrings.h"
#include "MediaControlElements.h"
#include "RenderObject.h"
#include "RenderSlider.h"

namespace WebCore {

using namespace HTMLNames;


AccessibilityMediaControl::AccessibilityMediaControl(RenderObject* renderer)
    : AccessibilityRenderObject(renderer)
{
}

PassRefPtr<AccessibilityObject> AccessibilityMediaControl::create(RenderObject* renderer)
{
    ASSERT(renderer->node() && renderer->node()->isMediaControlElement());

    Node* node = renderer->node();
    MediaControlElementType controlType;

    if (node->hasTagName(inputTag))
        controlType = static_cast<MediaControlInputElement*>(node)->displayType();
    else
        controlType = static_cast<MediaControlElement*>(node)->displayType();

    PassRefPtr<AccessibilityObject> obj;
    switch (controlType) {
    case MediaSlider:
        obj = AccessibilityMediaTimeline::create(renderer);
        break;

    case MediaCurrentTimeDisplay:
    case MediaTimeRemainingDisplay:
        obj = AccessibilityMediaTimeDisplay::create(renderer);
        break;

    case MediaControlsPanel:
        obj = AccessibilityMediaControlsContainer::create(renderer);
        break;

    default:
        obj = adoptRef(new AccessibilityMediaControl(renderer));
        break;
    }

    return obj;
}

MediaControlElementType AccessibilityMediaControl::controlType() const
{
    if (!renderer() || !renderer()->node())
        return MediaTimelineContainer; // Timeline container is not accessible.

    Node* node = renderer()->node();

    if (node->hasTagName(inputTag))
        return static_cast<MediaControlInputElement*>(node)->displayType();

    return static_cast<MediaControlElement*>(node)->displayType();
}

String AccessibilityMediaControl::controlTypeName() const
{
    DEFINE_STATIC_LOCAL(const String, mediaFullscreenButtonName, ("FullscreenButton"));
    DEFINE_STATIC_LOCAL(const String, mediaMuteButtonName, ("MuteButton"));
    DEFINE_STATIC_LOCAL(const String, mediaPlayButtonName, ("PlayButton"));
    DEFINE_STATIC_LOCAL(const String, mediaSeekBackButtonName, ("SeekBackButton"));
    DEFINE_STATIC_LOCAL(const String, mediaSeekForwardButtonName, ("SeekForwardButton"));
    DEFINE_STATIC_LOCAL(const String, mediaRewindButtonName, ("RewindButton"));
    DEFINE_STATIC_LOCAL(const String, mediaReturnToRealtimeButtonName, ("ReturnToRealtimeButton"));
    DEFINE_STATIC_LOCAL(const String, mediaUnMuteButtonName, ("UnMuteButton"));
    DEFINE_STATIC_LOCAL(const String, mediaPauseButtonName, ("PauseButton"));
    DEFINE_STATIC_LOCAL(const String, mediaStatusDisplayName, ("StatusDisplay"));
    DEFINE_STATIC_LOCAL(const String, mediaCurrentTimeDisplay, ("CurrentTimeDisplay"));
    DEFINE_STATIC_LOCAL(const String, mediaTimeRemainingDisplay, ("TimeRemainingDisplay"));
    DEFINE_STATIC_LOCAL(const String, mediaShowClosedCaptionsButtonName, ("ShowClosedCaptionsButton"));
    DEFINE_STATIC_LOCAL(const String, mediaHideClosedCaptionsButtonName, ("HideClosedCaptionsButton"));

    switch (controlType()) {
    case MediaFullscreenButton:
        return mediaFullscreenButtonName;
    case MediaMuteButton:
        return mediaMuteButtonName;
    case MediaPlayButton:
        return mediaPlayButtonName;
    case MediaSeekBackButton:
        return mediaSeekBackButtonName;
    case MediaSeekForwardButton:
        return mediaSeekForwardButtonName;
    case MediaRewindButton:
        return mediaRewindButtonName;
    case MediaReturnToRealtimeButton:
        return mediaReturnToRealtimeButtonName;
    case MediaUnMuteButton:
        return mediaUnMuteButtonName;
    case MediaPauseButton:
        return mediaPauseButtonName;
    case MediaStatusDisplay:
        return mediaStatusDisplayName;
    case MediaCurrentTimeDisplay:
        return mediaCurrentTimeDisplay;
    case MediaTimeRemainingDisplay:
        return mediaTimeRemainingDisplay;
    case MediaShowClosedCaptionsButton:
        return mediaShowClosedCaptionsButtonName;
    case MediaHideClosedCaptionsButton:
        return mediaHideClosedCaptionsButtonName;

    default:
        break;
    }

    return String();
}

String AccessibilityMediaControl::title() const
{
    DEFINE_STATIC_LOCAL(const String, controlsPanel, ("ControlsPanel"));

    if (controlType() == MediaControlsPanel)
        return localizedMediaControlElementString(controlsPanel);

    return AccessibilityRenderObject::title();
}

String AccessibilityMediaControl::accessibilityDescription() const
{
    return localizedMediaControlElementString(controlTypeName());
}

String AccessibilityMediaControl::helpText() const
{
    return localizedMediaControlElementHelpText(controlTypeName());
}

bool AccessibilityMediaControl::accessibilityIsIgnored() const
{
    if (!m_renderer || !m_renderer->style() || m_renderer->style()->visibility() != VISIBLE || controlType() == MediaTimelineContainer)
        return true;

    return false;
}

AccessibilityRole AccessibilityMediaControl::roleValue() const
{
    switch (controlType()) {
    case MediaFullscreenButton:
    case MediaMuteButton:
    case MediaPlayButton:
    case MediaSeekBackButton:
    case MediaSeekForwardButton:
    case MediaRewindButton:
    case MediaReturnToRealtimeButton:
    case MediaUnMuteButton:
    case MediaPauseButton:
    case MediaShowClosedCaptionsButton:
    case MediaHideClosedCaptionsButton:
        return ButtonRole;

    case MediaStatusDisplay:
        return StaticTextRole;

    case MediaTimelineContainer:
        return GroupRole;

    default:
        break;
    }

    return UnknownRole;
}



//
// AccessibilityMediaControlsContainer

AccessibilityMediaControlsContainer::AccessibilityMediaControlsContainer(RenderObject* renderer)
    : AccessibilityMediaControl(renderer)
{
}

PassRefPtr<AccessibilityObject> AccessibilityMediaControlsContainer::create(RenderObject* renderer)
{
    return adoptRef(new AccessibilityMediaControlsContainer(renderer));
}

String AccessibilityMediaControlsContainer::accessibilityDescription() const
{
    return localizedMediaControlElementString(elementTypeName());
}

String AccessibilityMediaControlsContainer::helpText() const
{
    return localizedMediaControlElementHelpText(elementTypeName());
}

bool AccessibilityMediaControlsContainer::controllingVideoElement() const
{
    if (!m_renderer->node())
        return true;

    MediaControlTimeDisplayElement* element = static_cast<MediaControlTimeDisplayElement*>(m_renderer->node());

    return element->mediaElement()->isVideo();
}

const String AccessibilityMediaControlsContainer::elementTypeName() const
{
    DEFINE_STATIC_LOCAL(const String, videoElement, ("VideoElement"));
    DEFINE_STATIC_LOCAL(const String, audioElement, ("AudioElement"));

    if (controllingVideoElement())
        return videoElement;
    return audioElement;
}


//
// AccessibilityMediaTimeline

AccessibilityMediaTimeline::AccessibilityMediaTimeline(RenderObject* renderer)
    : AccessibilitySlider(renderer)
{
}

PassRefPtr<AccessibilityObject> AccessibilityMediaTimeline::create(RenderObject* renderer)
{
    return adoptRef(new AccessibilityMediaTimeline(renderer));
}

String AccessibilityMediaTimeline::valueDescription() const
{
    ASSERT(m_renderer->node()->hasTagName(inputTag));

    float time = static_cast<HTMLInputElement*>(m_renderer->node())->value().toFloat();
    return localizedMediaTimeDescription(time);
}

String AccessibilityMediaTimeline::helpText() const
{
    DEFINE_STATIC_LOCAL(const String, slider, ("Slider"));
    return localizedMediaControlElementHelpText(slider);
}


//
// AccessibilityMediaTimeDisplay

AccessibilityMediaTimeDisplay::AccessibilityMediaTimeDisplay(RenderObject* renderer)
    : AccessibilityMediaControl(renderer)
{
}

PassRefPtr<AccessibilityObject> AccessibilityMediaTimeDisplay::create(RenderObject* renderer)
{
    return adoptRef(new AccessibilityMediaTimeDisplay(renderer));
}

bool AccessibilityMediaTimeDisplay::accessibilityIsIgnored() const
{
    if (!m_renderer || !m_renderer->style() || m_renderer->style()->visibility() != VISIBLE)
        return true;

    return !m_renderer->style()->width().value();
}

String AccessibilityMediaTimeDisplay::accessibilityDescription() const
{
    DEFINE_STATIC_LOCAL(const String, currentTimeDisplay, ("CurrentTimeDisplay"));
    DEFINE_STATIC_LOCAL(const String, timeRemainingDisplay, ("TimeRemainingDisplay"));

    if (controlType() == MediaCurrentTimeDisplay)
        return localizedMediaControlElementString(currentTimeDisplay);

    return localizedMediaControlElementString(timeRemainingDisplay);
}

String AccessibilityMediaTimeDisplay::stringValue() const
{
    if (!m_renderer || !m_renderer->node())
        return String();

    MediaControlTimeDisplayElement* element = static_cast<MediaControlTimeDisplayElement*>(m_renderer->node());
    float time = element->currentValue();
    return localizedMediaTimeDescription(fabsf(time));
}

} // namespace WebCore

#endif // ENABLE(VIDEO)
