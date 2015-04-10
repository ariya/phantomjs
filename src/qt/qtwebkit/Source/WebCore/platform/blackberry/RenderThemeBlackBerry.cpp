/*
 * Copyright (C) 2006, 2007 Apple Inc.
 * Copyright (C) 2009 Google Inc.
 * Copyright (C) 2009, 2010, 2011, 2012, 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "RenderThemeBlackBerry.h"

#include "CSSValueKeywords.h"
#include "Frame.h"
#include "HTMLMediaElement.h"
#include "HostWindow.h"
#include "InputType.h"
#include "InputTypeNames.h"
#include "MediaControlElements.h"
#include "MediaPlayerPrivateBlackBerry.h"
#include "Page.h"
#include "PaintInfo.h"
#include "RenderFullScreen.h"
#include "RenderProgress.h"
#include "RenderSlider.h"
#include "RenderView.h"
#include "UserAgentStyleSheets.h"

#include <BlackBerryPlatformLog.h>
#include <BlackBerryPlatformScreen.h>

namespace WebCore {

// Sizes (unit px)
const float progressMinWidth = 16;
const float progressTextureUnitWidth = 9.0;
const float mediaControlsHeight = 44;
const float mediaBackButtonHeight = 33;
// Scale exit-fullscreen button size.
const float mediaFullscreenButtonHeightRatio = 5 / 11.0;
const float mediaFullscreenButtonWidthRatio = 3 / 11.0;
const float mediaSliderEndAdjust = 2;
const float mediaSliderTrackRadius = 3;
const float mediaSliderThumbWidth = 25;
const float mediaSliderThumbHeight = 25;
const float mediaSliderThumbRadius = 3;
const float sliderThumbWidth = 15;
const float sliderThumbHeight = 25;

// Multipliers
const unsigned paddingDivisor = 10;
const unsigned fullScreenEnlargementFactor = 2;
const float scaleFactorThreshold = 2.0;

// Slice length
const int smallSlice = 8;
const int mediumSlice = 10;
const int largeSlice = 13;

// Slider Aura, calculated from UX spec
const float auraRatio = 1.62;

// Dropdown arrow position, calculated from UX spec
const float xPositionRatio = 3;
const float yPositionRatio = 0.38;
const float widthRatio = 3;
const float heightRatio = 0.23;

// Colors
const RGBA32 focusRingPen = 0xffa3c8fe;

float RenderThemeBlackBerry::defaultFontSize = 16;

const String& RenderThemeBlackBerry::defaultGUIFont()
{
    DEFINE_STATIC_LOCAL(String, fontFace, (ASCIILiteral("Slate Pro")));
    return fontFace;
}

static RenderSlider* determineRenderSlider(RenderObject* object)
{
    ASSERT(object->isSliderThumb());
    // The RenderSlider is an ancestor of the slider thumb.
    while (object && !object->isSlider())
        object = object->parent();
    return toRenderSlider(object);
}

static float determineFullScreenMultiplier(Element* element)
{
    float fullScreenMultiplier = 1.0;
#if ENABLE(FULLSCREEN_API) && ENABLE(VIDEO)
    if (element && element->document()->webkitIsFullScreen() && element->document()->webkitCurrentFullScreenElement() == toParentMediaElement(element)) {
        if (Page* page = element->document()->page()) {
            if (page->deviceScaleFactor() < scaleFactorThreshold)
                fullScreenMultiplier = fullScreenEnlargementFactor;

            // The way the BlackBerry port implements the FULLSCREEN_API for media elements
            // might result in the controls being oversized, proportionally to the current page
            // scale. That happens because the fullscreen element gets sized to be as big as the
            // viewport size, and the viewport size might get outstretched to fit to the screen dimensions.
            // To fix that, lets strips out the Page scale factor from the media controls multiplier.
            float scaleFactor = element->document()->view()->hostWindow()->platformPageClient()->currentZoomFactor();
            float scaleFactorFudge = 1 / page->deviceScaleFactor();
            fullScreenMultiplier /= scaleFactor * scaleFactorFudge;
        }
    }
#endif
    return fullScreenMultiplier;
}

static void drawControl(GraphicsContext* gc, const FloatRect& rect, Image* img)
{
    if (!img)
        return;
    FloatRect srcRect(0, 0, img->width(), img->height());
    gc->drawImage(img, ColorSpaceDeviceRGB, rect, srcRect);
}

static void drawThreeSliceHorizontal(GraphicsContext* gc, const IntRect& rect, Image* img, int slice)
{
    if (!img)
        return;

    FloatSize dstSlice(rect.height() / 2, rect.height());
    FloatRect srcRect(0, 0, slice, img->height());
    FloatRect dstRect(rect.location(), dstSlice);

    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
    srcRect.move(img->width() - srcRect.width(), 0);
    dstRect.move(rect.width() - dstRect.width(), 0);
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);

    srcRect = FloatRect(slice, 0, img->width() - 2 * slice, img->height());
    dstRect = FloatRect(rect.x() + dstSlice.width(), rect.y(), rect.width() - 2 * dstSlice.width(), dstSlice.height());
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
}

static void drawThreeSliceVertical(GraphicsContext* gc, const IntRect& rect, Image* img, int slice)
{
    if (!img)
        return;

    FloatSize dstSlice(rect.width(), rect.width() / 2);
    FloatRect srcRect(0, 0, img->width(), slice);
    FloatRect dstRect(rect.location(), dstSlice);

    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
    srcRect.move(0, img->height() - srcRect.height());
    dstRect.move(0, rect.height() - dstRect.height());
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);

    srcRect = FloatRect(0, slice, img->width(), img->height() - 2 * slice);
    dstRect = FloatRect(rect.x(), rect.y() + dstSlice.height(), dstSlice.width(), rect.height() - 2 * dstSlice.height());
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
}

static void drawNineSlice(GraphicsContext* gc, const IntRect& rect, double scale, Image* img, int slice)
{
    if (!img)
        return;
    if (rect.height() * scale < 101.0)
        scale = 101.0 / rect.height();
    FloatSize dstSlice(slice / scale, slice / scale);
    FloatRect srcRect(0, 0, slice, slice);
    FloatRect dstRect(rect.location(), dstSlice);
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
    srcRect.move(img->width() - srcRect.width(), 0);
    dstRect.move(rect.width() - dstRect.width(), 0);
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
    srcRect.move(0, img->height() - srcRect.height());
    dstRect.move(0, rect.height() - dstRect.height());
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
    srcRect.move(-(img->width() - srcRect.width()), 0);
    dstRect.move(-(rect.width() - dstRect.width()), 0);
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);

    srcRect = FloatRect(slice, 0, img->width() - 2 * slice, slice);
    dstRect = FloatRect(rect.x() + dstSlice.width(), rect.y(), rect.width() - 2 * dstSlice.width(), dstSlice.height());
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
    srcRect.move(0, img->height() - srcRect.height());
    dstRect.move(0, rect.height() - dstRect.height());
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);

    srcRect = FloatRect(0, slice, slice, img->height() - 2 * slice);
    dstRect = FloatRect(rect.x(), rect.y() + dstSlice.height(), dstSlice.width(), rect.height() - 2 * dstSlice.height());
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
    srcRect.move(img->width() - srcRect.width(), 0);
    dstRect.move(rect.width() - dstRect.width(), 0);
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);

    srcRect = FloatRect(slice, slice, img->width() - 2 * slice, img->height() - 2 * slice);
    dstRect = FloatRect(rect.x() + dstSlice.width(), rect.y() + dstSlice.height(), rect.width() - 2 * dstSlice.width(), rect.height() - 2 * dstSlice.height());
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
}

static RefPtr<Image> loadImage(const char* filename)
{
    RefPtr<Image> resource;
    resource = Image::loadPlatformResource(filename).leakRef();
    if (!resource) {
        BlackBerry::Platform::logAlways(BlackBerry::Platform::LogLevelWarn, "RenderThemeBlackBerry failed to load %s.png", filename);
        return 0;
    }
    return resource;
}

PassRefPtr<RenderTheme> RenderTheme::themeForPage(Page*)
{
    static RenderTheme* theme = RenderThemeBlackBerry::create().leakRef();
    return theme;
}

PassRefPtr<RenderTheme> RenderThemeBlackBerry::create()
{
    return adoptRef(new RenderThemeBlackBerry());
}

RenderThemeBlackBerry::RenderThemeBlackBerry()
{
}

RenderThemeBlackBerry::~RenderThemeBlackBerry()
{
}

String RenderThemeBlackBerry::extraDefaultStyleSheet()
{
    return String(themeBlackBerryUserAgentStyleSheet, sizeof(themeBlackBerryUserAgentStyleSheet));
}

#if ENABLE(VIDEO)
String RenderThemeBlackBerry::extraMediaControlsStyleSheet()
{
    return String(mediaControlsBlackBerryUserAgentStyleSheet, sizeof(mediaControlsBlackBerryUserAgentStyleSheet));
}
#endif

#if ENABLE(FULLSCREEN_API)
String RenderThemeBlackBerry::extraFullScreenStyleSheet()
{
    return String(mediaControlsBlackBerryFullscreenUserAgentStyleSheet, sizeof(mediaControlsBlackBerryFullscreenUserAgentStyleSheet));
}
#endif

double RenderThemeBlackBerry::caretBlinkInterval() const
{
    return 0; // Turn off caret blinking.
}

void RenderThemeBlackBerry::systemFont(CSSValueID valueID, FontDescription& fontDescription) const
{
    float fontSize = defaultFontSize;

    // Both CSSValueWebkitControl and CSSValueWebkitSmallControl should use default font size which looks better on the controls.
    if (valueID == CSSValueWebkitMiniControl) {
        // Why 2 points smaller? Because that's what Gecko does. Note that we
        // are assuming a 96dpi screen, which is the default value we use on Windows.
        static const float pointsPerInch = 72.0f;
        static const float pixelsPerInch = 96.0f;
        fontSize -= (2.0f / pointsPerInch) * pixelsPerInch;
    }

    fontDescription.firstFamily().setFamily(defaultGUIFont());
    fontDescription.setSpecifiedSize(fontSize);
    fontDescription.setIsAbsoluteSize(true);
    fontDescription.setGenericFamily(FontDescription::NoFamily);
    fontDescription.setWeight(FontWeightNormal);
    fontDescription.setItalic(false);
}

void RenderThemeBlackBerry::setButtonStyle(RenderStyle* style) const
{
    Length vertPadding(int(style->fontSize() / paddingDivisor), Fixed);
    style->setPaddingTop(vertPadding);
    style->setPaddingBottom(vertPadding);
}

void RenderThemeBlackBerry::adjustButtonStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    setButtonStyle(style);
    style->setCursor(CURSOR_WEBKIT_GRAB);
}

void RenderThemeBlackBerry::adjustTextAreaStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    setButtonStyle(style);
}

bool RenderThemeBlackBerry::paintTextArea(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintTextFieldOrTextAreaOrSearchField(object, info, rect);
}

void RenderThemeBlackBerry::adjustTextFieldStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    setButtonStyle(style);
}

bool RenderThemeBlackBerry::paintTextFieldOrTextAreaOrSearchField(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    ASSERT(info.context);
    GraphicsContext* context = info.context;

    static RefPtr<Image> bg, bgDisabled, bgHighlight;
    if (!bg) {
        if (BlackBerry::Platform::Graphics::Screen::primaryScreen()->displayTechnology() == BlackBerry::Platform::Graphics::OledDisplayTechnology)
            bg = loadImage("core_textinput_bg_oled");
        else
            bg = loadImage("core_textinput_bg");
        bgDisabled = loadImage("core_textinput_bg_disabled");
        bgHighlight = loadImage("core_textinput_bg_highlight");
    }

    AffineTransform ctm = context->getCTM();
    if (isEnabled(object) && bg)
        drawNineSlice(context, rect, ctm.xScale(), bg.get(), smallSlice);
    if (!isEnabled(object) && bgDisabled)
        drawNineSlice(context, rect, ctm.xScale(), bgDisabled.get(), smallSlice);

    if ((isHovered(object) || isFocused(object) || isPressed(object)) && bgHighlight)
        drawNineSlice(context, rect, ctm.xScale(), bgHighlight.get(), smallSlice);

    return false;
}

bool RenderThemeBlackBerry::paintTextField(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintTextFieldOrTextAreaOrSearchField(object, info, rect);
}

void RenderThemeBlackBerry::adjustSearchFieldStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    setButtonStyle(style);
}

void RenderThemeBlackBerry::adjustSearchFieldCancelButtonStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    static const float defaultControlFontPixelSize = 10;
    static const float defaultCancelButtonSize = 13;
    static const float minCancelButtonSize = 5;

    // Scale the button size based on the font size
    float fontScale = style->fontSize() / defaultControlFontPixelSize;
    int cancelButtonSize = lroundf(std::max(minCancelButtonSize, defaultCancelButtonSize * fontScale));
    Length length(cancelButtonSize, Fixed);
    style->setWidth(length);
    style->setHeight(length);
}

bool RenderThemeBlackBerry::paintSearchField(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintTextFieldOrTextAreaOrSearchField(object, info, rect);
}

IntRect RenderThemeBlackBerry::convertToPaintingRect(RenderObject* inputRenderer, const RenderObject* partRenderer, LayoutRect partRect, const IntRect& localOffset) const
{
    // Compute an offset between the part renderer and the input renderer.
    LayoutSize offsetFromInputRenderer = -partRenderer->offsetFromAncestorContainer(inputRenderer);
    // Move the rect into partRenderer's coords.
    partRect.move(offsetFromInputRenderer);
    // Account for the local drawing offset.
    partRect.move(localOffset.x(), localOffset.y());

    return pixelSnappedIntRect(partRect);
}


bool RenderThemeBlackBerry::paintSearchFieldCancelButton(RenderObject* cancelButtonObject, const PaintInfo& paintInfo, const IntRect& r)
{
    Node* input = cancelButtonObject->node()->deprecatedShadowAncestorNode();
    if (!input->renderer()->isBox())
        return false;

    RenderBox* inputRenderBox = toRenderBox(input->renderer());
    LayoutRect inputContentBox = inputRenderBox->contentBoxRect();

    // Make sure the scaled button stays square and will fit in its parent's box.
    LayoutUnit cancelButtonSize = std::min(inputContentBox.width(), std::min<LayoutUnit>(inputContentBox.height(), r.height()));
    // Calculate cancel button's coordinates relative to the input element.
    // Center the button vertically. Round up though, so if it has to be one pixel off-center, it will
    // be one pixel closer to the bottom of the field. This tends to look better with the text.
    LayoutRect cancelButtonRect(cancelButtonObject->offsetFromAncestorContainer(inputRenderBox).width(),
        inputContentBox.y() + (inputContentBox.height() - cancelButtonSize + 1) / 2, cancelButtonSize, cancelButtonSize);
    IntRect paintingRect = convertToPaintingRect(inputRenderBox, cancelButtonObject, cancelButtonRect, r);

    static Image* cancelImage = Image::loadPlatformResource("searchCancel").leakRef();
    static Image* cancelPressedImage = Image::loadPlatformResource("searchCancelPressed").leakRef();
    paintInfo.context->drawImage(isPressed(cancelButtonObject) ? cancelPressedImage : cancelImage,
        cancelButtonObject->style()->colorSpace(), paintingRect);
    return false;
}

void RenderThemeBlackBerry::adjustMenuListButtonStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    // These seem to be reasonable padding values from observation.
    const int paddingLeft = 8;
    const int paddingRight = 4;

    const int minHeight = style->fontSize() * 2;

    style->resetPadding();
    style->setMinHeight(Length(minHeight, Fixed));
    style->setLineHeight(RenderStyle::initialLineHeight());

    style->setPaddingRight(Length(minHeight + paddingRight, Fixed));
    style->setPaddingLeft(Length(paddingLeft, Fixed));
    style->setCursor(CURSOR_WEBKIT_GRAB);
}

void RenderThemeBlackBerry::calculateButtonSize(RenderStyle* style) const
{
    int size = style->fontSize();
    Length length(size, Fixed);
    if (style->appearance() == CheckboxPart || style->appearance() == RadioPart) {
        style->setWidth(length);
        style->setHeight(length);
        return;
    }

    // If the width and height are both specified, then we have nothing to do.
    if (!style->width().isIntrinsicOrAuto() && !style->height().isAuto())
        return;

    if (style->width().isIntrinsicOrAuto())
        style->setWidth(length);

    if (style->height().isAuto())
        style->setHeight(length);
}

bool RenderThemeBlackBerry::paintCheckbox(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    ASSERT(info.context);
    GraphicsContext* context = info.context;

    static RefPtr<Image> disabled, inactive, pressed, active, activeMark, disableMark, pressedMark;
    if (!disabled) {
        disabled = loadImage("core_checkbox_disabled");
        inactive = loadImage("core_checkbox_inactive");
        pressed = loadImage("core_checkbox_pressed");
        active = loadImage("core_checkbox_active");
        activeMark = loadImage("core_checkbox_active_mark");
        disableMark = loadImage("core_checkbox_disabled_mark");
        pressedMark = loadImage("core_checkbox_pressed_mark");
    }

    // Caculate where to put center checkmark.
    FloatRect tmpRect(rect);

    float centerX = ((float(inactive->width()) - float(activeMark->width())) / float(inactive->width()) * tmpRect.width() / 2) + tmpRect.x();
    float centerY = ((float(inactive->height()) - float(activeMark->height())) / float(inactive->height()) * tmpRect.height() / 2) + tmpRect.y();
    float width = float(activeMark->width()) / float(inactive->width()) * tmpRect.width();
    float height = float(activeMark->height()) / float(inactive->height()) * tmpRect.height();
    FloatRect centerRect(centerX, centerY, width, height);

    if (isEnabled(object)) {
        if (isPressed(object)) {
            drawControl(context, rect, pressed.get());
            if (isChecked(object)) {
                drawControl(context, centerRect, pressedMark.get());
            }
        } else {
            drawControl(context, rect, inactive.get());
            if (isChecked(object)) {
                drawControl(context, rect, active.get());
                drawControl(context, centerRect, activeMark.get());
            }
        }
    } else {
        drawControl(context, rect, disabled.get());
        if (isChecked(object))
            drawControl(context, rect, disableMark.get());
    }
    return false;
}

void RenderThemeBlackBerry::setCheckboxSize(RenderStyle* style) const
{
    calculateButtonSize(style);
}

bool RenderThemeBlackBerry::paintRadio(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    ASSERT(info.context);
    GraphicsContext* context = info.context;

    static RefPtr<Image> disabled, disabledDot, inactive, pressed, active, activeDot, pressedDot;
    if (!disabled) {
        disabled = loadImage("core_radiobutton_disabled");
        disabledDot = loadImage("core_radiobutton_dot_disabled");
        inactive = loadImage("core_radiobutton_inactive");
        pressed = loadImage("core_radiobutton_pressed");
        active = loadImage("core_radiobutton_active");
        activeDot = loadImage("core_radiobutton_dot_selected");
        pressedDot = loadImage("core_radiobutton_dot_pressed");
    }

    // Caculate where to put center circle.
    FloatRect tmpRect(rect);

    float centerX = ((float(inactive->width()) - float(activeDot->width())) / float(inactive->width()) * tmpRect.width() / 2)+ tmpRect.x();
    float centerY = ((float(inactive->height()) - float(activeDot->height())) / float(inactive->height()) * tmpRect.height() / 2) + tmpRect.y();
    float width = float(activeDot->width()) / float(inactive->width()) * tmpRect.width();
    float height = float(activeDot->height()) / float(inactive->height()) * tmpRect.height();
    FloatRect centerRect(centerX, centerY, width, height);

    if (isEnabled(object)) {
        if (isPressed(object)) {
            drawControl(context, rect, pressed.get());
            if (isChecked(object))
                drawControl(context, centerRect, pressedDot.get());
        } else {
            drawControl(context, rect, inactive.get());
            if (isChecked(object)) {
                drawControl(context, rect, active.get());
                drawControl(context, centerRect, activeDot.get());
            }
        }
    } else {
        drawControl(context, rect, disabled.get());
        if (isChecked(object))
            drawControl(context, rect, disabledDot.get());
    }
    return false;
}

void RenderThemeBlackBerry::setRadioSize(RenderStyle* style) const
{
    calculateButtonSize(style);
}

// If this function returns false, WebCore assumes the button is fully decorated
bool RenderThemeBlackBerry::paintButton(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    ASSERT(info.context);
    info.context->save();
    GraphicsContext* context = info.context;

    static RefPtr<Image> disabled, inactive, pressed;
    if (!disabled) {
        disabled = loadImage("core_button_disabled");
        inactive = loadImage("core_button_inactive");
        pressed = loadImage("core_button_pressed");
    }

    AffineTransform ctm = context->getCTM();
    if (!isEnabled(object)) {
        drawNineSlice(context, rect, ctm.xScale(), inactive.get(), largeSlice);
        drawNineSlice(context, rect, ctm.xScale(), disabled.get(), largeSlice);
    } else if (isPressed(object))
        drawNineSlice(context, rect, ctm.xScale(), pressed.get(), largeSlice);
    else
        drawNineSlice(context, rect, ctm.xScale(), inactive.get(), largeSlice);

    context->restore();
    return false;
}

void RenderThemeBlackBerry::adjustMenuListStyle(StyleResolver* css, RenderStyle* style, Element* element) const
{
    adjustMenuListButtonStyle(css, style, element);
}

static IntRect computeMenuListArrowButtonRect(const IntRect& rect)
{
    // FIXME: The menu list arrow button should have a minimum and maximum width (to ensure usability) or
    // scale with respect to the font size used in the menu list control or some combination of both.
    return IntRect(IntPoint(rect.maxX() - rect.height(), rect.y()), IntSize(rect.height(), rect.height()));
}

bool RenderThemeBlackBerry::paintMenuList(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    ASSERT(info.context);
    info.context->save();
    GraphicsContext* context = info.context;

    static RefPtr<Image> disabled, inactive, pressed, arrowUp, arrowUpDisabled;
    if (!disabled) {
        disabled = loadImage("core_button_disabled");
        inactive = loadImage("core_button_inactive");
        pressed = loadImage("core_button_pressed");
        arrowUp = loadImage("core_dropdown_button_arrowup");
        arrowUpDisabled = loadImage("core_dropdown_button_arrowup_disabled");
    }

    FloatRect arrowButtonRectangle(computeMenuListArrowButtonRect(rect));
    float x = arrowButtonRectangle.x() + arrowButtonRectangle.width() / xPositionRatio;
    float y = arrowButtonRectangle.y() + arrowButtonRectangle.height() * yPositionRatio;
    float width = arrowButtonRectangle.width() / widthRatio;
    float height = arrowButtonRectangle.height() * heightRatio;
    FloatRect tmpRect(x, y, width, height);

    AffineTransform ctm = context->getCTM();
    if (!isEnabled(object)) {
        drawNineSlice(context, rect, ctm.xScale(), inactive.get(), largeSlice);
        drawNineSlice(context, rect, ctm.xScale(), disabled.get(), largeSlice);
        drawControl(context, tmpRect, arrowUpDisabled.get());
    } else if (isPressed(object)) {
        drawNineSlice(context, rect, ctm.xScale(), pressed.get(), largeSlice);
        drawControl(context, tmpRect, arrowUp.get());
    } else {
        drawNineSlice(context, rect, ctm.xScale(), inactive.get(), largeSlice);
        drawControl(context, tmpRect, arrowUp.get());
    }
    context->restore();
    return false;
}

bool RenderThemeBlackBerry::paintMenuListButton(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintMenuList(object, info, rect);
}

void RenderThemeBlackBerry::adjustSliderThumbSize(RenderStyle* style, Element* element) const
{
    float fullScreenMultiplier = 1;
    ControlPart part = style->appearance();

    if (part == MediaSliderThumbPart || part == MediaVolumeSliderThumbPart) {
        RenderSlider* slider = determineRenderSlider(element->renderer());
        if (slider)
            fullScreenMultiplier = determineFullScreenMultiplier(toElement(slider->node()));
    }

    if (part == SliderThumbHorizontalPart || part == SliderThumbVerticalPart) {
        style->setWidth(Length((part == SliderThumbVerticalPart ? sliderThumbHeight : sliderThumbWidth) * fullScreenMultiplier, Fixed));
        style->setHeight(Length((part == SliderThumbVerticalPart ? sliderThumbWidth : sliderThumbHeight) * fullScreenMultiplier, Fixed));
    } else if (part == MediaVolumeSliderThumbPart || part == MediaSliderThumbPart) {
        style->setWidth(Length(mediaSliderThumbWidth * fullScreenMultiplier, Fixed));
        style->setHeight(Length(mediaSliderThumbHeight * fullScreenMultiplier, Fixed));
    }
}

bool RenderThemeBlackBerry::paintSliderTrack(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    const static int SliderTrackHeight = 5;
    IntRect rect2;
    if (object->style()->appearance() == SliderHorizontalPart) {
        rect2.setHeight(SliderTrackHeight);
        rect2.setWidth(rect.width());
        rect2.setX(rect.x());
        rect2.setY(rect.y() + (rect.height() - SliderTrackHeight) / 2);
    } else {
        rect2.setHeight(rect.height());
        rect2.setWidth(SliderTrackHeight);
        rect2.setX(rect.x() + (rect.width() - SliderTrackHeight) / 2);
        rect2.setY(rect.y());
    }
    static Image* sliderTrack = Image::loadPlatformResource("core_progressindicator_bg").leakRef();
    return paintSliderTrackRect(object, info, rect2, sliderTrack);
}

bool RenderThemeBlackBerry::paintSliderTrackRect(RenderObject* object, const PaintInfo& info, const IntRect& rect, Image* inactive)
{
    ASSERT(info.context);
    info.context->save();
    GraphicsContext* context = info.context;

    static RefPtr<Image> disabled;
    if (!disabled)
        disabled = loadImage("core_slider_fill_disabled");

    if (rect.width() > rect.height()) {
        if (isEnabled(object))
            drawThreeSliceHorizontal(context, rect, inactive, mediumSlice);
        else
            drawThreeSliceHorizontal(context, rect, disabled.get(), (smallSlice - 1));
    } else {
        if (isEnabled(object))
            drawThreeSliceVertical(context, rect, inactive, mediumSlice);
        else
            drawThreeSliceVertical(context, rect, disabled.get(), (smallSlice - 1));
    }

    context->restore();
    return false;
}

bool RenderThemeBlackBerry::paintSliderThumb(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    ASSERT(info.context);
    info.context->save();
    GraphicsContext* context = info.context;

    static RefPtr<Image> disabled, inactive, pressed, aura;
    if (!disabled) {
        disabled = loadImage("core_slider_handle_disabled");
        inactive = loadImage("core_slider_handle");
        pressed = loadImage("core_slider_handle_pressed");
        aura = loadImage("core_slider_aura");
    }

    FloatRect tmpRect(rect);
    float length = std::max(tmpRect.width(), tmpRect.height());
    if (tmpRect.width() > tmpRect.height()) {
        tmpRect.setY(tmpRect.y() - (length - tmpRect.height()) / 2);
        tmpRect.setHeight(length);
    } else {
        tmpRect.setX(tmpRect.x() - (length - tmpRect.width()) / 2);
        tmpRect.setWidth(length);
    }

    float auraHeight = length * auraRatio;
    float auraWidth = auraHeight;
    float auraX = tmpRect.x() - (auraWidth - tmpRect.width()) / 2;
    float auraY = tmpRect.y() - (auraHeight - tmpRect.height()) / 2;
    FloatRect auraRect(auraX, auraY, auraWidth, auraHeight);

    if (!isEnabled(object)) {
        // Disabled handle shrink 30%
        tmpRect.move(tmpRect.width() * 0.075, tmpRect.height() * 0.075);
        tmpRect.contract(tmpRect.width() * 0.15, tmpRect.height() * 0.15);
        drawControl(context, tmpRect, disabled.get());
    } else {
        if (isPressed(object) || isHovered(object) || isFocused(object)) {
            drawControl(context, tmpRect, pressed.get());
            drawControl(context, auraRect, aura.get());
        } else
            drawControl(context, tmpRect, inactive.get());
    }

    context->restore();
    return false;
}

void RenderThemeBlackBerry::adjustMediaControlStyle(StyleResolver*, RenderStyle* style, Element* element) const
{
    float fullScreenMultiplier = determineFullScreenMultiplier(element);
    HTMLMediaElement* mediaElement = toParentMediaElement(element);
    if (!mediaElement)
        return;

    // We use multiples of mediaControlsHeight to make all objects scale evenly
    Length zero(0, Fixed);
    Length controlsHeight(mediaControlsHeight * fullScreenMultiplier, Fixed);
    Length halfControlsWidth(mediaControlsHeight / 2 * fullScreenMultiplier, Fixed);
    Length displayHeight(mediaControlsHeight / 3 * fullScreenMultiplier, Fixed);
    Length volOffset(mediaControlsHeight * fullScreenMultiplier + 5, Fixed);
    Length padding(mediaControlsHeight / 10 * fullScreenMultiplier, Fixed);
    float fontSize = mediaControlsHeight / 3 * fullScreenMultiplier;

    switch (style->appearance()) {
    case MediaControlsBackgroundPart:
        if (element->shadowPseudoId() == "-webkit-media-controls-placeholder")
            style->setHeight(controlsHeight);
        break;
    case MediaPlayButtonPart:
        style->setWidth(controlsHeight);
        style->setHeight(controlsHeight);
        break;
    case MediaMuteButtonPart:
        style->setWidth(controlsHeight);
        style->setHeight(controlsHeight);
        break;
    case MediaRewindButtonPart:
        // We hi-jack the Rewind Button ID to use it for the divider image
        style->setWidth(halfControlsWidth);
        style->setHeight(controlsHeight);
        break;
    case MediaEnterFullscreenButtonPart:
        style->setWidth(controlsHeight);
        style->setHeight(controlsHeight);
        break;
    case MediaExitFullscreenButtonPart:
        style->setLeft(zero);
        style->setWidth(controlsHeight);
        style->setHeight(controlsHeight);
        break;
    case MediaCurrentTimePart:
    case MediaTimeRemainingPart:
        style->setHeight(displayHeight);
        style->setPaddingRight(padding);
        style->setPaddingLeft(padding);
        style->setPaddingBottom(padding);
        style->setFontSize(static_cast<int>(fontSize));
        break;
    case MediaVolumeSliderContainerPart:
        style->setHeight(controlsHeight);
        style->setBottom(volOffset);
        break;
    default:
        break;
    }
}

void RenderThemeBlackBerry::adjustSliderTrackStyle(StyleResolver*, RenderStyle* style, Element* element) const
{
    float fullScreenMultiplier = determineFullScreenMultiplier(element);

    // We use multiples of mediaControlsHeight to make all objects scale evenly
    Length controlsHeight(mediaControlsHeight / 2 * fullScreenMultiplier, Fixed);
    switch (style->appearance()) {
    case MediaSliderPart:
    case MediaVolumeSliderPart:
        style->setHeight(controlsHeight);
        break;
    default:
        break;
    }
}

static bool paintMediaButton(GraphicsContext* context, const IntRect& rect, Image* image)
{
    context->drawImage(image, ColorSpaceDeviceRGB, rect);
    return false;
}

bool RenderThemeBlackBerry::paintMediaPlayButton(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElement = toParentMediaElement(object);

    if (!mediaElement)
        return false;

    static Image* mediaPlay = Image::loadPlatformResource("play").leakRef();
    static Image* mediaPause = Image::loadPlatformResource("pause").leakRef();
    static Image* mediaStop = Image::loadPlatformResource("stop").leakRef();
    Image* nonPlayImage;

    // The BlackBerry port sets the movieLoadType to LiveStream if and only if
    // "stop" must be used instead of "pause".
    if (mediaElement->movieLoadType() == MediaPlayer::LiveStream)
        nonPlayImage = mediaStop;
    else
        nonPlayImage = mediaPause;

    return paintMediaButton(paintInfo.context, rect, mediaElement->canPlay() ? mediaPlay : nonPlayImage);
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

// We hi-jack the Rewind Button code to use it for the divider image
bool RenderThemeBlackBerry::paintMediaRewindButton(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElement = toParentMediaElement(object);

    if (!mediaElement)
        return false;

    if (!mediaElement->isFullscreen())
        return false;

    static Image* divider = Image::loadPlatformResource("divider").leakRef();

    return paintMediaButton(paintInfo.context, rect, divider);
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

bool RenderThemeBlackBerry::paintMediaMuteButton(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElement = toParentMediaElement(object);

    if (!mediaElement)
        return false;

    static Image* speaker = Image::loadPlatformResource("speaker").leakRef();

    return paintMediaButton(paintInfo.context, rect, speaker);
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

bool RenderThemeBlackBerry::paintMediaFullscreenButton(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElement = toParentMediaElement(object);
    if (!mediaElement)
        return false;

    static Image* mediaEnterFullscreen = Image::loadPlatformResource("fullscreen").leakRef();
    static Image* mediaExitFullscreen = Image::loadPlatformResource("back").leakRef();

    Image* buttonImage = mediaEnterFullscreen;
    IntRect currentRect(rect);
#if ENABLE(FULLSCREEN_API)
    if (mediaElement->document()->webkitIsFullScreen() && mediaElement->document()->webkitCurrentFullScreenElement() == mediaElement) {
        buttonImage = mediaExitFullscreen;
        IntRect fullscreenRect(rect.x() + (1 - mediaFullscreenButtonWidthRatio) * rect.width() / 2, rect.y() + (1 - mediaFullscreenButtonHeightRatio) * rect.height() / 2,
            rect.width() * mediaFullscreenButtonWidthRatio, rect.height() * mediaFullscreenButtonHeightRatio);
        currentRect = fullscreenRect;
    }
#endif
    return paintMediaButton(paintInfo.context, currentRect, buttonImage);
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

bool RenderThemeBlackBerry::paintMediaSliderTrack(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElement = toParentMediaElement(object);
    if (!mediaElement)
        return false;

    float fullScreenMultiplier = determineFullScreenMultiplier(mediaElement);
    float loaded = mediaElement->percentLoaded();
    float position = mediaElement->duration() > 0 ? (mediaElement->currentTime() / mediaElement->duration()) : 0;

    int intrinsicHeight = ceil(mediaSliderThumbHeight / 4);
    int x = ceil(rect.x() + mediaSliderEndAdjust * fullScreenMultiplier);
    int y = ceil(rect.y() + (mediaControlsHeight / 2 - intrinsicHeight) / 2 * fullScreenMultiplier + fullScreenMultiplier / 2);
    int w = ceil(rect.width() - mediaSliderEndAdjust * 2 * fullScreenMultiplier);
    int h = ceil(intrinsicHeight * fullScreenMultiplier);
    IntRect rect2(x, y, w, h);

    // We subtract a small amount from the width in the calculation below to
    // prevent the played bar from poking out past the thumb accidentally.
    int wPlayed = ceil((w - mediaSliderEndAdjust) * position);
    // We adjust the buffered bar to make it visible to the right of the thumb.
    // A small amount is subtracted from the mediaSliderThumbWidth in the first
    // part of the expression to account for the fact that the slider track's
    // width was shortened and x position was incremented above (to make sure
    // its rounded ends get covered by the thumb).
    int wLoaded = ceil((w - (mediaSliderThumbWidth - mediaSliderEndAdjust) * fullScreenMultiplier) * loaded + mediaSliderThumbWidth * fullScreenMultiplier);

    IntRect played(x, y, wPlayed, h);
    IntRect buffered(x, y, wLoaded > w ? w : wLoaded, h);

    static Image* mediaBackground = Image::loadPlatformResource("core_slider_video_bg").leakRef();
    static Image* mediaPlayer = Image::loadPlatformResource("core_slider_played_bg").leakRef();
    static Image* mediaCache = Image::loadPlatformResource("core_slider_cache").leakRef();

    bool result = paintSliderTrackRect(object, paintInfo, rect2, mediaBackground);

    if (loaded > 0 || position > 0) {
        // This is to paint buffered bar.
        paintSliderTrackRect(object, paintInfo, buffered, mediaCache);

        // This is to paint played part of bar (left of slider thumb) using selection color.
        paintSliderTrackRect(object, paintInfo, played, mediaPlayer);
    }

    return result;
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

bool RenderThemeBlackBerry::paintMediaSliderThumb(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    static Image* disabledMediaSliderThumb = Image::loadPlatformResource("core_slider_handle_disabled").leakRef();
    static Image* pressedMediaSliderThumb = Image::loadPlatformResource("core_slider_handle_pressed").leakRef();
    static Image* mediaSliderThumb = Image::loadPlatformResource("core_media_handle").leakRef();

    if (!isEnabled(object))
        return paintMediaButton(paintInfo.context, rect, disabledMediaSliderThumb);
    if (isPressed(object) || isHovered(object) || isFocused(object))
        return paintMediaButton(paintInfo.context, rect, pressedMediaSliderThumb);
    return paintMediaButton(paintInfo.context, rect, mediaSliderThumb);
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

bool RenderThemeBlackBerry::paintMediaVolumeSliderTrack(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElement = toParentMediaElement(object);
    if (!mediaElement)
        return false;

    float fullScreenMultiplier = determineFullScreenMultiplier(mediaElement);
    float volume = mediaElement->volume() > 0 ? mediaElement->volume() : 0;

    int intrinsicHeight = ceil(mediaSliderThumbHeight / 4);
    int x = ceil(rect.x() + (mediaControlsHeight - intrinsicHeight) / 2 * fullScreenMultiplier - fullScreenMultiplier / 2);
    int y = ceil(rect.y() + (mediaControlsHeight - intrinsicHeight) / 2 * fullScreenMultiplier + fullScreenMultiplier / 2);
    int w = ceil(rect.width() - (mediaControlsHeight - intrinsicHeight) * fullScreenMultiplier + fullScreenMultiplier / 2);
    int h = ceil(intrinsicHeight * fullScreenMultiplier);
    IntRect rect2(x, y, w, h);
    IntRect volumeRect(x, y, ceil(w * volume), h);

    static Image* volumeBackground = Image::loadPlatformResource("core_slider_video_bg").leakRef();
    static Image* volumeBar = Image::loadPlatformResource("core_slider_played_bg").leakRef();

    // This is to paint main volume slider bar.
    bool result = paintSliderTrackRect(object, paintInfo, rect2, volumeBackground);

    if (volume > 0) {
        // This is to paint volume bar (left of volume slider thumb) using selection color.
        result |= paintSliderTrackRect(object, paintInfo, volumeRect, volumeBar);
    }

    return result;
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

bool RenderThemeBlackBerry::paintMediaVolumeSliderThumb(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    RenderSlider* slider = determineRenderSlider(object);
    float fullScreenMultiplier = slider ? determineFullScreenMultiplier(toElement(slider->node())) : 1;

    int intrinsicHeight = ceil(mediaSliderThumbHeight / 4);
    int y = ceil(rect.y() + (mediaControlsHeight / 2 - intrinsicHeight / 2) / 2 * fullScreenMultiplier);
    IntRect adjustedRect(rect.x(), y, rect.width(), rect.height());

    return paintMediaSliderThumb(object, paintInfo, adjustedRect);
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

Color RenderThemeBlackBerry::platformFocusRingColor() const
{
    return focusRingPen;
}

#if ENABLE(TOUCH_EVENTS)
Color RenderThemeBlackBerry::platformTapHighlightColor() const
{
    return Color(0, 168, 223, 50);
}
#endif

Color RenderThemeBlackBerry::platformActiveSelectionBackgroundColor() const
{
    return Color(0, 168, 223, 50);
}

double RenderThemeBlackBerry::animationRepeatIntervalForProgressBar(RenderProgress* renderProgress) const
{
    return renderProgress->isDeterminate() ? 0.0 : 0.1;
}

double RenderThemeBlackBerry::animationDurationForProgressBar(RenderProgress* renderProgress) const
{
    return renderProgress->isDeterminate() ? 0.0 : 2.0;
}

bool RenderThemeBlackBerry::paintProgressTrackRect(const PaintInfo& info, const IntRect& rect, Image* image)
{
    ASSERT(info.context);
    info.context->save();
    GraphicsContext* context = info.context;
    drawThreeSliceHorizontal(context, rect, image, mediumSlice);
    context->restore();
    return false;
}

static void drawProgressTexture(GraphicsContext* gc, const FloatRect& rect, int n, Image* image)
{
    if (!image)
        return;
    float finalTexturePercentage = (int(rect.width()) % int(progressTextureUnitWidth)) / progressTextureUnitWidth;
    FloatSize dstSlice(progressTextureUnitWidth, rect.height() - 2);
    FloatRect srcRect(1, 2, image->width() - 2, image->height() - 4);
    FloatRect dstRect(FloatPoint(rect.location().x() + 1, rect.location().y() + 1), dstSlice);

    for (int i = 0; i < n; i++) {
        gc->drawImage(image, ColorSpaceDeviceRGB, dstRect, srcRect);
        dstRect.move(dstSlice.width(), 0);
    }
    if (finalTexturePercentage) {
        srcRect.setWidth(srcRect.width() * finalTexturePercentage * finalTexturePercentage);
        dstRect.setWidth(dstRect.width() * finalTexturePercentage * finalTexturePercentage);
        gc->drawImage(image, ColorSpaceDeviceRGB, dstRect, srcRect);
    }
}

bool RenderThemeBlackBerry::paintProgressBar(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    if (!object->isProgress())
        return true;

    RenderProgress* renderProgress = toRenderProgress(object);

    static Image* progressTrack = Image::loadPlatformResource("core_progressindicator_bg").leakRef();
    static Image* progressBar = Image::loadPlatformResource("core_progressindicator_progress").leakRef();
    static Image* progressPattern = Image::loadPlatformResource("core_progressindicator_pattern").leakRef();
    static Image* progressComplete = Image::loadPlatformResource("core_progressindicator_complete").leakRef();

    paintProgressTrackRect(info, rect, progressTrack);

    IntRect progressRect = rect;
    progressRect.setX(progressRect.x() + 1);
    progressRect.setHeight(progressRect.height() - 2);
    progressRect.setY(progressRect.y() + 1);

    if (renderProgress->isDeterminate())
        progressRect.setWidth((progressRect.width() - progressMinWidth) * renderProgress->position() + progressMinWidth - 2);
    else {
        // Animating
        progressRect.setWidth(progressRect.width() - 2);
    }

    if (renderProgress->position() < 1) {
        paintProgressTrackRect(info, progressRect, progressBar);
        int loop = floor((progressRect.width() - 2) / progressTextureUnitWidth);
        progressRect.setWidth(progressRect.width() - 2);
        drawProgressTexture(info.context, progressRect, loop, progressPattern);
    } else
        paintProgressTrackRect(info, progressRect, progressComplete);

    return false;
}

Color RenderThemeBlackBerry::platformActiveTextSearchHighlightColor() const
{
    return Color(255, 150, 50); // Orange.
}

Color RenderThemeBlackBerry::platformInactiveTextSearchHighlightColor() const
{
    return Color(255, 255, 0); // Yellow.
}

bool RenderThemeBlackBerry::supportsDataListUI(const AtomicString& type) const
{
#if ENABLE(DATALIST_ELEMENT)
    // We support all non-popup driven types.
    return type == InputTypeNames::text() || type == InputTypeNames::search() || type == InputTypeNames::url()
        || type == InputTypeNames::telephone() || type == InputTypeNames::email() || type == InputTypeNames::number()
        || type == InputTypeNames::range();
#else
    return false;
#endif
}

#if ENABLE(DATALIST_ELEMENT)
IntSize RenderThemeBlackBerry::sliderTickSize() const
{
    return IntSize(1, 3);
}

int RenderThemeBlackBerry::sliderTickOffsetFromTrackCenter() const
{
    return -9;
}
#endif

} // namespace WebCore
