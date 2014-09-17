/*
 * Copyright (C) 2009 Apple Inc.
 * Copyright (C) 2009 Google Inc.
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
#include "RenderMediaControlsChromium.h"

#include "Gradient.h"
#include "GraphicsContext.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "PaintInfo.h"

namespace WebCore {

#if ENABLE(VIDEO)

typedef WTF::HashMap<const char*, Image*> MediaControlImageMap;
static MediaControlImageMap* gMediaControlImageMap = 0;

static Image* platformResource(const char* name)
{
    if (!gMediaControlImageMap)
        gMediaControlImageMap = new MediaControlImageMap();
    if (Image* image = gMediaControlImageMap->get(name))
        return image;
    if (Image* image = Image::loadPlatformResource(name).releaseRef()) {
        gMediaControlImageMap->set(name, image);
        return image;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

static bool hasSource(const HTMLMediaElement* mediaElement)
{
    return mediaElement->networkState() != HTMLMediaElement::NETWORK_EMPTY
        && mediaElement->networkState() != HTMLMediaElement::NETWORK_NO_SOURCE;
}

static bool paintMediaButton(GraphicsContext* context, const IntRect& rect, Image* image)
{
    IntRect imageRect = image->rect();
    context->drawImage(image, ColorSpaceDeviceRGB, rect);
    return true;
}

static bool paintMediaMuteButton(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
    HTMLMediaElement* mediaElement = toParentMediaElement(object);
    if (!mediaElement)
      return false;

    static Image* soundFull = platformResource("mediaSoundFull");
    static Image* soundNone = platformResource("mediaSoundNone");
    static Image* soundDisabled = platformResource("mediaSoundDisabled");

    if (!hasSource(mediaElement) || !mediaElement->hasAudio())
        return paintMediaButton(paintInfo.context, rect, soundDisabled);

    return paintMediaButton(paintInfo.context, rect, mediaElement->muted() ? soundNone: soundFull);
}

static bool paintMediaPlayButton(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
    HTMLMediaElement* mediaElement = toParentMediaElement(object);
    if (!mediaElement)
        return false;

    static Image* mediaPlay = platformResource("mediaPlay");
    static Image* mediaPause = platformResource("mediaPause");
    static Image* mediaPlayDisabled = platformResource("mediaPlayDisabled");

    if (!hasSource(mediaElement))
        return paintMediaButton(paintInfo.context, rect, mediaPlayDisabled);

    return paintMediaButton(paintInfo.context, rect, mediaElement->canPlay() ? mediaPlay : mediaPause);
}

static Image* getMediaSliderThumb()
{
    static Image* mediaSliderThumb = platformResource("mediaSliderThumb");
    return mediaSliderThumb;
}

static bool paintMediaSlider(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
    HTMLMediaElement* mediaElement = toParentMediaElement(object);
    if (!mediaElement)
        return false;

    RenderStyle* style = object->style();
    GraphicsContext* context = paintInfo.context;

    // Draw the border of the time bar.
    // FIXME: this should be a rounded rect but need to fix GraphicsContextSkia first.
    // https://bugs.webkit.org/show_bug.cgi?id=30143
    context->save();
    context->setShouldAntialias(true);
    context->setStrokeStyle(SolidStroke);
    context->setStrokeColor(style->visitedDependentColor(CSSPropertyBorderLeftColor), ColorSpaceDeviceRGB);
    context->setStrokeThickness(style->borderLeftWidth());
    context->setFillColor(style->visitedDependentColor(CSSPropertyBackgroundColor), ColorSpaceDeviceRGB);
    context->drawRect(rect);
    context->restore();

    // Draw the buffered ranges.
    // FIXME: Draw multiple ranges if there are multiple buffered ranges.
    IntRect bufferedRect = rect;
    bufferedRect.inflate(-style->borderLeftWidth());

    double bufferedWidth = 0.0;
    if (mediaElement->percentLoaded() > 0.0) {
        // Account for the width of the slider thumb.
        Image* mediaSliderThumb = getMediaSliderThumb();
        double thumbWidth = mediaSliderThumb->width() / 2.0 + 1.0;
        double rectWidth = bufferedRect.width() - thumbWidth;
        if (rectWidth < 0.0)
            rectWidth = 0.0;
        bufferedWidth = rectWidth * mediaElement->percentLoaded() + thumbWidth;
    }
    bufferedRect.setWidth(bufferedWidth);

    // Don't bother drawing an empty area.
    if (!bufferedRect.isEmpty()) {
        IntPoint sliderTopLeft = bufferedRect.location();
        IntPoint sliderTopRight = sliderTopLeft;
        sliderTopRight.move(0, bufferedRect.height());

        RefPtr<Gradient> gradient = Gradient::create(sliderTopLeft, sliderTopRight);
        Color startColor = object->style()->visitedDependentColor(CSSPropertyColor);
        gradient->addColorStop(0.0, startColor);
        gradient->addColorStop(1.0, Color(startColor.red() / 2, startColor.green() / 2, startColor.blue() / 2, startColor.alpha()));

        context->save();
        context->setStrokeStyle(NoStroke);
        context->setFillGradient(gradient);
        context->fillRect(bufferedRect);
        context->restore();
    }

    return true;
}

static bool paintMediaSliderThumb(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
    if (!object->parent()->isSlider())
        return false;

    HTMLMediaElement* mediaElement = toParentMediaElement(object->parent());
    if (!mediaElement)
        return false;

    if (!hasSource(mediaElement))
        return true;

    Image* mediaSliderThumb = getMediaSliderThumb();
    return paintMediaButton(paintInfo.context, rect, mediaSliderThumb);
}

static bool paintMediaVolumeSlider(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
    HTMLMediaElement* mediaElement = toParentMediaElement(object);
    if (!mediaElement)
        return false;

    GraphicsContext* context = paintInfo.context;
    Color originalColor = context->strokeColor();
    if (originalColor != Color::white)
        context->setStrokeColor(Color::white, ColorSpaceDeviceRGB);

    int x = rect.x() + rect.width() / 2;
    context->drawLine(IntPoint(x, rect.y()),  IntPoint(x, rect.y() + rect.height()));

    if (originalColor != Color::white)
        context->setStrokeColor(originalColor, ColorSpaceDeviceRGB);
    return true;
}

static bool paintMediaVolumeSliderThumb(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
    if (!object->parent()->isSlider())
        return false;

    static Image* mediaVolumeSliderThumb = platformResource("mediaVolumeSliderThumb");
    return paintMediaButton(paintInfo.context, rect, mediaVolumeSliderThumb);
}

static bool paintMediaTimelineContainer(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
    HTMLMediaElement* mediaElement = toParentMediaElement(object);
    if (!mediaElement)
        return false;

    if (!rect.isEmpty()) {
        GraphicsContext* context = paintInfo.context;
        Color originalColor = context->strokeColor();
        float originalThickness = context->strokeThickness();
        StrokeStyle originalStyle = context->strokeStyle();

        context->setStrokeStyle(SolidStroke);

        // Draw the left border using CSS defined width and color.
        context->setStrokeThickness(object->style()->borderLeftWidth());
        context->setStrokeColor(object->style()->visitedDependentColor(CSSPropertyBorderLeftColor).rgb(), ColorSpaceDeviceRGB);
        context->drawLine(IntPoint(rect.x() + 1, rect.y()),
                          IntPoint(rect.x() + 1, rect.y() + rect.height()));

        // Draw the right border using CSS defined width and color.
        context->setStrokeThickness(object->style()->borderRightWidth());
        context->setStrokeColor(object->style()->visitedDependentColor(CSSPropertyBorderRightColor).rgb(), ColorSpaceDeviceRGB);
        context->drawLine(IntPoint(rect.x() + rect.width() - 1, rect.y()),
                          IntPoint(rect.x() + rect.width() - 1, rect.y() + rect.height()));

        context->setStrokeColor(originalColor, ColorSpaceDeviceRGB);
        context->setStrokeThickness(originalThickness);
        context->setStrokeStyle(originalStyle);
    }
    return true;
}

bool RenderMediaControlsChromium::paintMediaControlsPart(MediaControlElementType part, RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
    switch (part) {
    case MediaMuteButton:
    case MediaUnMuteButton:
        return paintMediaMuteButton(object, paintInfo, rect);
    case MediaPauseButton:
    case MediaPlayButton:
        return paintMediaPlayButton(object, paintInfo, rect);
    case MediaSlider:
        return paintMediaSlider(object, paintInfo, rect);
    case MediaSliderThumb:
        return paintMediaSliderThumb(object, paintInfo, rect);
    case MediaVolumeSlider:
        return paintMediaVolumeSlider(object, paintInfo, rect);
    case MediaVolumeSliderThumb:
        return paintMediaVolumeSliderThumb(object, paintInfo, rect);
    case MediaTimelineContainer:
        return paintMediaTimelineContainer(object, paintInfo, rect);
    case MediaVolumeSliderMuteButton:
    case MediaFullscreenButton:
    case MediaSeekBackButton:
    case MediaSeekForwardButton:
    case MediaVolumeSliderContainer:
    case MediaCurrentTimeDisplay:
    case MediaTimeRemainingDisplay:
    case MediaControlsPanel:
    case MediaRewindButton:
    case MediaReturnToRealtimeButton:
    case MediaStatusDisplay:
    case MediaShowClosedCaptionsButton:
    case MediaHideClosedCaptionsButton:
        ASSERT_NOT_REACHED();
        break;
    }
    return false;
}

void RenderMediaControlsChromium::adjustMediaSliderThumbSize(RenderObject* object)
{
    static Image* mediaSliderThumb = platformResource("mediaSliderThumb");
    static Image* mediaVolumeSliderThumb = platformResource("mediaVolumeSliderThumb");

    Image* thumbImage = 0;
    if (object->style()->appearance() == MediaSliderThumbPart)
        thumbImage = mediaSliderThumb;
    else if (object->style()->appearance() == MediaVolumeSliderThumbPart)
        thumbImage = mediaVolumeSliderThumb;

    float zoomLevel = object->style()->effectiveZoom();
    if (thumbImage) {
        object->style()->setWidth(Length(static_cast<int>(thumbImage->width() * zoomLevel), Fixed));
        object->style()->setHeight(Length(static_cast<int>(thumbImage->height() * zoomLevel), Fixed));
    }
}

#endif  // #if ENABLE(VIDEO)

} // namespace WebCore
