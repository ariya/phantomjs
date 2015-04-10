/*
 * Copyright (C) 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Google Inc.
 * All rights reserved.
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

#if ENABLE(VIDEO)
#include "RenderMediaControlElements.h"

#include "RenderTheme.h"
#include "RenderView.h"

namespace WebCore {

RenderMediaVolumeSliderContainer::RenderMediaVolumeSliderContainer(Element* element)
    : RenderBlock(element)
{
}

void RenderMediaVolumeSliderContainer::layout()
{
    RenderBlock::layout();

    if (style()->display() == NONE || !nextSibling() || !nextSibling()->isBox())
        return;

    RenderBox* buttonBox = toRenderBox(nextSibling());
    int absoluteOffsetTop = buttonBox->localToAbsolute(FloatPoint(0, -size().height())).y();

    LayoutStateDisabler layoutStateDisabler(view());

    // If the slider would be rendered outside the page, it should be moved below the controls.
    if (UNLIKELY(absoluteOffsetTop < 0))
        setY(buttonBox->offsetTop() + theme()->volumeSliderOffsetFromMuteButton(buttonBox, pixelSnappedSize()).y());
}

// ----------------------------

RenderMediaControlTimelineContainer::RenderMediaControlTimelineContainer(Element* element)
    : RenderFlexibleBox(element)
{
}

// We want the timeline slider to be at least 100 pixels wide.
// FIXME: Eliminate hard-coded widths altogether.
static const int minWidthToDisplayTimeDisplays = 45 + 100 + 45;

void RenderMediaControlTimelineContainer::layout()
{
    RenderFlexibleBox::layout();

    LayoutStateDisabler layoutStateDisabler(view());
    MediaControlTimelineContainerElement* container = static_cast<MediaControlTimelineContainerElement*>(node());
    container->setTimeDisplaysHidden(width().toInt() < minWidthToDisplayTimeDisplays);
}

// ----------------------------

#if ENABLE(VIDEO_TRACK)

RenderTextTrackContainerElement::RenderTextTrackContainerElement(Element* element)
    : RenderBlock(element)
{
}

void RenderTextTrackContainerElement::layout()
{
    RenderBlock::layout();
    if (style()->display() == NONE)
        return;

    ASSERT(mediaControlElementType(node()) == MediaTextTrackDisplayContainer);

    LayoutStateDisabler layoutStateDisabler(view());
    static_cast<MediaControlTextTrackContainerElement*>(node())->updateSizes();
}

#endif // ENABLE(VIDEO_TRACK)

} // namespace WebCore

#endif // ENABLE(VIDEO)

