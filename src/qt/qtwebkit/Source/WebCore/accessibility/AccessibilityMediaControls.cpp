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
#include "HTMLMediaElement.h"
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
    ASSERT(renderer->node());

    switch (mediaControlElementType(renderer->node())) {
    case MediaSlider:
        return AccessibilityMediaTimeline::create(renderer);

    case MediaCurrentTimeDisplay:
    case MediaTimeRemainingDisplay:
        return AccessibilityMediaTimeDisplay::create(renderer);

    case MediaControlsPanel:
        return AccessibilityMediaControlsContainer::create(renderer);

    default:
        return adoptRef(new AccessibilityMediaControl(renderer));
    }
}

MediaControlElementType AccessibilityMediaControl::controlType() const
{
    if (!renderer() || !renderer()->node())
        return MediaTimelineContainer; // Timeline container is not accessible.

    return mediaControlElementType(renderer()->node());
}

String AccessibilityMediaControl::controlTypeName() const
{
    DEFINE_STATIC_LOCAL(const String, mediaEnterFullscreenButtonName, (ASCIILiteral("EnterFullscreenButton")));
    DEFINE_STATIC_LOCAL(const String, mediaExitFullscreenButtonName, (ASCIILiteral("ExitFullscreenButton")));
    DEFINE_STATIC_LOCAL(const String, mediaMuteButtonName, (ASCIILiteral("MuteButton")));
    DEFINE_STATIC_LOCAL(const String, mediaPlayButtonName, (ASCIILiteral("PlayButton")));
    DEFINE_STATIC_LOCAL(const String, mediaSeekBackButtonName, (ASCIILiteral("SeekBackButton")));
    DEFINE_STATIC_LOCAL(const String, mediaSeekForwardButtonName, (ASCIILiteral("SeekForwardButton")));
    DEFINE_STATIC_LOCAL(const String, mediaRewindButtonName, (ASCIILiteral("RewindButton")));
    DEFINE_STATIC_LOCAL(const String, mediaReturnToRealtimeButtonName, (ASCIILiteral("ReturnToRealtimeButton")));
    DEFINE_STATIC_LOCAL(const String, mediaUnMuteButtonName, (ASCIILiteral("UnMuteButton")));
    DEFINE_STATIC_LOCAL(const String, mediaPauseButtonName, (ASCIILiteral("PauseButton")));
    DEFINE_STATIC_LOCAL(const String, mediaStatusDisplayName, (ASCIILiteral("StatusDisplay")));
    DEFINE_STATIC_LOCAL(const String, mediaCurrentTimeDisplay, (ASCIILiteral("CurrentTimeDisplay")));
    DEFINE_STATIC_LOCAL(const String, mediaTimeRemainingDisplay, (ASCIILiteral("TimeRemainingDisplay")));
    DEFINE_STATIC_LOCAL(const String, mediaShowClosedCaptionsButtonName, (ASCIILiteral("ShowClosedCaptionsButton")));
    DEFINE_STATIC_LOCAL(const String, mediaHideClosedCaptionsButtonName, (ASCIILiteral("HideClosedCaptionsButton")));

    switch (controlType()) {
    case MediaEnterFullscreenButton:
        return mediaEnterFullscreenButtonName;
    case MediaExitFullscreenButton:
        return mediaExitFullscreenButtonName;
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

void AccessibilityMediaControl::accessibilityText(Vector<AccessibilityText>& textOrder)
{
    String description = accessibilityDescription();
    if (!description.isEmpty())
        textOrder.append(AccessibilityText(description, AlternativeText));

    String title = this->title();
    if (!title.isEmpty())
        textOrder.append(AccessibilityText(title, AlternativeText));

    String helptext = helpText();
    if (!helptext.isEmpty())
        textOrder.append(AccessibilityText(helptext, HelpText));
}
    

String AccessibilityMediaControl::title() const
{
    DEFINE_STATIC_LOCAL(const String, controlsPanel, (ASCIILiteral("ControlsPanel")));

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

bool AccessibilityMediaControl::computeAccessibilityIsIgnored() const
{
    if (!m_renderer || !m_renderer->style() || m_renderer->style()->visibility() != VISIBLE || controlType() == MediaTimelineContainer)
        return true;

    return accessibilityIsIgnoredByDefault();
}

AccessibilityRole AccessibilityMediaControl::roleValue() const
{
    switch (controlType()) {
    case MediaEnterFullscreenButton:
    case MediaExitFullscreenButton:
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

    return toParentMediaElement(element)->isVideo();
}

const String AccessibilityMediaControlsContainer::elementTypeName() const
{
    DEFINE_STATIC_LOCAL(const String, videoElement, (ASCIILiteral("VideoElement")));
    DEFINE_STATIC_LOCAL(const String, audioElement, (ASCIILiteral("AudioElement")));

    if (controllingVideoElement())
        return videoElement;
    return audioElement;
}

bool AccessibilityMediaControlsContainer::computeAccessibilityIsIgnored() const
{
    return accessibilityIsIgnoredByDefault();
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
    Node* node = m_renderer->node();
    if (!isHTMLInputElement(node))
        return String();

    float time = toHTMLInputElement(node)->value().toFloat();
    return localizedMediaTimeDescription(time);
}

String AccessibilityMediaTimeline::helpText() const
{
    DEFINE_STATIC_LOCAL(const String, slider, (ASCIILiteral("Slider")));
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

bool AccessibilityMediaTimeDisplay::computeAccessibilityIsIgnored() const
{
    if (!m_renderer || !m_renderer->style() || m_renderer->style()->visibility() != VISIBLE)
        return true;

    if (!m_renderer->style()->width().value())
        return true;
    
    return accessibilityIsIgnoredByDefault();
}

String AccessibilityMediaTimeDisplay::accessibilityDescription() const
{
    DEFINE_STATIC_LOCAL(const String, currentTimeDisplay, (ASCIILiteral("CurrentTimeDisplay")));
    DEFINE_STATIC_LOCAL(const String, timeRemainingDisplay, (ASCIILiteral("TimeRemainingDisplay")));

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
