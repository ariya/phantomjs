/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 *               2006 Dirk Mueller <mueller@kde.org>
 *               2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2008 Holger Hans Peter Freyther
 *
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "RenderThemeQt.h"

#include "CSSValueKeywords.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Color.h"
#include "ExceptionCodePlaceholder.h"
#include "FileList.h"
#include "Font.h"
#include "FontSelector.h"
#include "GraphicsContext.h"
#include "HTMLInputElement.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "LocalizedStrings.h"
#if ENABLE(VIDEO)
#include "MediaControlElements.h"
#endif
#include "NotImplemented.h"
#include "Page.h"
#include "PaintInfo.h"
#include "RenderBox.h"
#if ENABLE(PROGRESS_ELEMENT)
#include "RenderProgress.h"
#endif
#include "RenderTheme.h"
#include "RenderThemeQtMobile.h"
#include "ScrollbarTheme.h"
#include "StyleResolver.h"
#include "TimeRanges.h"
#include "UserAgentStyleSheets.h"
#include <wtf/text/StringBuilder.h>

#include <QColor>
#include <QFile>
#include <QFontMetrics>
#include <QGuiApplication>

#include <QStyleHints>

namespace WebCore {

using namespace HTMLNames;

// These values all match Safari/Win/Chromium
static const float defaultControlFontPixelSize = 13;
static const float defaultCancelButtonSize = 9;
static const float minCancelButtonSize = 5;
static const float maxCancelButtonSize = 21;
static const float defaultSearchFieldResultsDecorationSize = 13;
static const float minSearchFieldResultsDecorationSize = 9;
static const float maxSearchFieldResultsDecorationSize = 30;
static const float defaultSearchFieldResultsButtonWidth = 18;

static QtThemeFactoryFunction themeFactory;
static ScrollbarTheme* scrollbarTheme;

RenderThemeQt::RenderThemeQt(Page* page)
    : RenderTheme()
    , m_page(page)
{
    m_buttonFontFamily = QGuiApplication::font().family();
}

void RenderThemeQt::setCustomTheme(QtThemeFactoryFunction factory, ScrollbarTheme* customScrollbarTheme)
{
    themeFactory = factory;
    scrollbarTheme = customScrollbarTheme;
}

ScrollbarTheme* RenderThemeQt::customScrollbarTheme()
{
    return scrollbarTheme;
}

static PassRefPtr<RenderTheme> createTheme(Page* page)
{
    if (themeFactory)
        return themeFactory(page);
    return RenderThemeQtMobile::create(page);
}

PassRefPtr<RenderTheme> RenderTheme::themeForPage(Page* page)
{
    if (page)
        return createTheme(page);
    static RenderTheme* fallback = createTheme(0).leakRef();
    return fallback;
}

// Remove this when SearchFieldPart is style-able in RenderTheme::isControlStyled()
bool RenderThemeQt::isControlStyled(const RenderStyle* style, const BorderData& border, const FillLayer& fill, const Color& backgroundColor) const
{
    switch (style->appearance()) {
    case SearchFieldPart:
        // Test the style to see if the UA border and background match.
        return (style->border() != border
                || *style->backgroundLayers() != fill
                || style->visitedDependentColor(CSSPropertyBackgroundColor) != backgroundColor);
    default:
        return RenderTheme::isControlStyled(style, border, fill, backgroundColor);
    }
}

String RenderThemeQt::extraDefaultStyleSheet()
{
    StringBuilder result;
    result.append(RenderTheme::extraDefaultStyleSheet());
    // When no theme factory is provided we default to using our platform independent "Mobile Qt" theme,
    // which requires the following stylesheets.
    if (!themeFactory) {
        result.append(String(themeQtNoListboxesUserAgentStyleSheet, sizeof(themeQtNoListboxesUserAgentStyleSheet)));
        result.append(String(mobileThemeQtUserAgentStyleSheet, sizeof(mobileThemeQtUserAgentStyleSheet)));
    }
    return result.toString();
}

bool RenderThemeQt::supportsHover(const RenderStyle*) const
{
    return true;
}

bool RenderThemeQt::supportsFocusRing(const RenderStyle* style) const
{
    switch (style->appearance()) {
    case CheckboxPart:
    case RadioPart:
    case PushButtonPart:
    case SquareButtonPart:
    case ButtonPart:
    case ButtonBevelPart:
    case ListboxPart:
    case ListItemPart:
    case MenulistPart:
    case MenulistButtonPart:
    case SliderHorizontalPart:
    case SliderVerticalPart:
    case SliderThumbHorizontalPart:
    case SliderThumbVerticalPart:
    case SearchFieldPart:
    case SearchFieldResultsButtonPart:
    case SearchFieldCancelButtonPart:
    case TextFieldPart:
    case TextAreaPart:
        return true;
    default:
        return false;
    }
}

int RenderThemeQt::baselinePosition(const RenderObject* o) const
{
    if (!o->isBox())
        return 0;

    if (o->style()->appearance() == CheckboxPart || o->style()->appearance() == RadioPart)
        return toRenderBox(o)->marginTop() + toRenderBox(o)->height() - 2; // Same as in old khtml
    return RenderTheme::baselinePosition(o);
}

bool RenderThemeQt::controlSupportsTints(const RenderObject* o) const
{
    if (!isEnabled(o))
        return false;

    // Checkboxes only have tint when checked.
    if (o->style()->appearance() == CheckboxPart)
        return isChecked(o);

    // For now assume other controls have tint if enabled.
    return true;
}

bool RenderThemeQt::supportsControlTints() const
{
    return true;
}

QRect RenderThemeQt::inflateButtonRect(const QRect& originalRect) const
{
    return originalRect;
}

void RenderThemeQt::adjustRepaintRect(const RenderObject* o, IntRect& rect)
{
    switch (o->style()->appearance()) {
    case CheckboxPart:
        break;
    case RadioPart:
        break;
    case PushButtonPart:
    case ButtonPart: {
        QRect inflatedRect = inflateButtonRect(rect);
        rect = IntRect(inflatedRect.x(), inflatedRect.y(), inflatedRect.width(), inflatedRect.height());
        break;
    }
    case MenulistPart:
        break;
    default:
        break;
    }
}

Color RenderThemeQt::platformActiveSelectionBackgroundColor() const
{
    return colorPalette().brush(QPalette::Active, QPalette::Highlight).color();
}

Color RenderThemeQt::platformInactiveSelectionBackgroundColor() const
{
    return colorPalette().brush(QPalette::Inactive, QPalette::Highlight).color();
}

Color RenderThemeQt::platformActiveSelectionForegroundColor() const
{
    return colorPalette().brush(QPalette::Active, QPalette::HighlightedText).color();
}

Color RenderThemeQt::platformInactiveSelectionForegroundColor() const
{
    return colorPalette().brush(QPalette::Inactive, QPalette::HighlightedText).color();
}

Color RenderThemeQt::platformFocusRingColor() const
{
    return colorPalette().brush(QPalette::Active, QPalette::Highlight).color();
}

void RenderThemeQt::systemFont(CSSValueID, FontDescription&) const
{
    // no-op
}

Color RenderThemeQt::systemColor(CSSValueID cssValueId) const
{
    QPalette pal = colorPalette();
    switch (cssValueId) {
    case CSSValueButtontext:
        return pal.brush(QPalette::Active, QPalette::ButtonText).color();
    case CSSValueCaptiontext:
        return pal.brush(QPalette::Active, QPalette::Text).color();
    default:
        return RenderTheme::systemColor(cssValueId);
    }
}

int RenderThemeQt::minimumMenuListSize(RenderStyle*) const
{
    // FIXME: Later we need a way to query the UI process for the dpi
    const QFontMetrics fm(QGuiApplication::font());
    return fm.width(QLatin1Char('x'));
}

void RenderThemeQt::setCheckboxSize(RenderStyle* style) const
{
    computeSizeBasedOnStyle(style);
}

bool RenderThemeQt::paintCheckbox(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    return paintButton(o, i, r);
}

void RenderThemeQt::setRadioSize(RenderStyle* style) const
{
    computeSizeBasedOnStyle(style);
}

bool RenderThemeQt::paintRadio(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    return paintButton(o, i, r);
}

void RenderThemeQt::setButtonSize(RenderStyle* style) const
{
    computeSizeBasedOnStyle(style);
}

void RenderThemeQt::adjustTextFieldStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    // Resetting the style like this leads to differences like:
    // - RenderTextControl {INPUT} at (2,2) size 168x25 [bgcolor=#FFFFFF] border: (2px inset #000000)]
    // + RenderTextControl {INPUT} at (2,2) size 166x26
    // in layout tests when a CSS style is applied that doesn't affect background color, border or
    // padding. Just worth keeping in mind!
    style->setBackgroundColor(Color::transparent);
    style->resetBorder();
    style->resetPadding();
    computeSizeBasedOnStyle(style);
}

void RenderThemeQt::adjustTextAreaStyle(StyleResolver* selector, RenderStyle* style, Element* element) const
{
    adjustTextFieldStyle(selector, style, element);
}

bool RenderThemeQt::paintTextArea(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    return paintTextField(o, i, r);
}

void RenderThemeQt::adjustMenuListStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    style->resetBorder();

    // Height is locked to auto.
    style->setHeight(Length(Auto));

    // White-space is locked to pre
    style->setWhiteSpace(PRE);

    computeSizeBasedOnStyle(style);

    // Add in the padding that we'd like to use.
    setPopupPadding(style);
}

void RenderThemeQt::adjustMenuListButtonStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    // Height is locked to auto.
    style->setHeight(Length(Auto));

    // White-space is locked to pre
    style->setWhiteSpace(PRE);

    computeSizeBasedOnStyle(style);

    // Add in the padding that we'd like to use.
    setPopupPadding(style);
}

#if ENABLE(PROGRESS_ELEMENT)
double RenderThemeQt::animationRepeatIntervalForProgressBar(RenderProgress* renderProgress) const
{
    if (renderProgress->position() >= 0)
        return 0;

    // FIXME: Use hard-coded value until http://bugreports.qt.nokia.com/browse/QTBUG-9171 is fixed.
    // Use the value from windows style which is 10 fps.
    return 0.1;
}

void RenderThemeQt::adjustProgressBarStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    style->setBoxShadow(nullptr);
}
#endif

void RenderThemeQt::adjustSliderTrackStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    style->setBoxShadow(nullptr);
}

void RenderThemeQt::adjustSliderThumbStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
    RenderTheme::adjustSliderThumbStyle(styleResolver, style, element);
    style->setBoxShadow(nullptr);
}

#if ENABLE(DATALIST_ELEMENT)
IntSize RenderThemeQt::sliderTickSize() const
{
    // FIXME: We need to set this to the size of one tick mark.
    return IntSize(0, 0);
}

int RenderThemeQt::sliderTickOffsetFromTrackCenter() const
{
    // FIXME: We need to set this to the position of the tick marks.
    return 0;
}
#endif

bool RenderThemeQt::paintSearchField(RenderObject* o, const PaintInfo& pi,
                                     const IntRect& r)
{
    return paintTextField(o, pi, r);
}

void RenderThemeQt::adjustSearchFieldStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    // Resetting the style like this leads to differences like:
    // - RenderTextControl {INPUT} at (2,2) size 168x25 [bgcolor=#FFFFFF] border: (2px inset #000000)]
    // + RenderTextControl {INPUT} at (2,2) size 166x26
    // in layout tests when a CSS style is applied that doesn't affect background color, border or
    // padding. Just worth keeping in mind!
    style->setBackgroundColor(Color::transparent);
    style->resetBorder();
    style->resetPadding();
    computeSizeBasedOnStyle(style);
}

void RenderThemeQt::adjustSearchFieldCancelButtonStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    // Logic taken from RenderThemeChromium.cpp.
    // Scale the button size based on the font size.
    float fontScale = style->fontSize() / defaultControlFontPixelSize;
    int cancelButtonSize = lroundf(qMin(qMax(minCancelButtonSize, defaultCancelButtonSize * fontScale), maxCancelButtonSize));
    style->setWidth(Length(cancelButtonSize, Fixed));
    style->setHeight(Length(cancelButtonSize, Fixed));
}

// Function taken from RenderThemeChromium.cpp
IntRect RenderThemeQt::convertToPaintingRect(RenderObject* inputRenderer, const RenderObject* partRenderer, IntRect partRect, const IntRect& localOffset) const
{
    // Compute an offset between the part renderer and the input renderer.
    IntSize offsetFromInputRenderer = -roundedIntSize(partRenderer->offsetFromAncestorContainer(inputRenderer));
    // Move the rect into partRenderer's coords.
    partRect.move(offsetFromInputRenderer);
    // Account for the local drawing offset.
    partRect.move(localOffset.x(), localOffset.y());

    return partRect;
}

QPalette RenderThemeQt::colorPalette() const
{
    return QGuiApplication::palette();
}

bool RenderThemeQt::paintSearchFieldCancelButton(RenderObject* o, const PaintInfo& pi,
                                                 const IntRect& r)
{
    // Logic copied from RenderThemeChromium.cpp.

    // Get the renderer of <input> element.
    Node* input = o->node()->shadowHost();
    if (!input)
        input = o->node();
    if (!input->renderer()->isBox())
        return false;
    RenderBox* inputRenderBox = toRenderBox(input->renderer());
    IntRect inputContentBox = pixelSnappedIntRect(inputRenderBox->contentBoxRect());

    // Make sure the scaled button stays square and will fit in its parent's box.
    int cancelButtonSize = qMin(inputContentBox.width(), qMin(inputContentBox.height(), r.height()));
    // Calculate cancel button's coordinates relative to the input element.
    // Center the button vertically. Round up though, so if it has to be one pixel off-center, it will
    // be one pixel closer to the bottom of the field. This tends to look better with the text.
    IntRect cancelButtonRect(o->offsetFromAncestorContainer(inputRenderBox).width(),
                             inputContentBox.y() + (inputContentBox.height() - cancelButtonSize + 1) / 2,
                             cancelButtonSize, cancelButtonSize);
    IntRect paintingRect = convertToPaintingRect(inputRenderBox, o, cancelButtonRect, r);
    static Image* cancelImage = Image::loadPlatformResource("searchCancelButton").leakRef();
    static Image* cancelPressedImage = Image::loadPlatformResource("searchCancelButtonPressed").leakRef();
    pi.context->drawImage(isPressed(o) ? cancelPressedImage : cancelImage,
                                 o->style()->colorSpace(), paintingRect);
    return false;
}

void RenderThemeQt::adjustSearchFieldDecorationStyle(StyleResolver* styleResolver, RenderStyle* style, Element* e) const
{
    notImplemented();
    RenderTheme::adjustSearchFieldDecorationStyle(styleResolver, style, e);
}

bool RenderThemeQt::paintSearchFieldDecoration(RenderObject* o, const PaintInfo& pi,
                                               const IntRect& r)
{
    notImplemented();
    return RenderTheme::paintSearchFieldDecoration(o, pi, r);
}

void RenderThemeQt::adjustSearchFieldResultsDecorationStyle(StyleResolver* styleResolver, RenderStyle* style, Element* e) const
{
    notImplemented();
    RenderTheme::adjustSearchFieldResultsDecorationStyle(styleResolver, style, e);
}

bool RenderThemeQt::paintSearchFieldResultsDecoration(RenderObject* o, const PaintInfo& pi,
                                                      const IntRect& r)
{
    notImplemented();
    return RenderTheme::paintSearchFieldResultsDecoration(o, pi, r);
}

#ifndef QT_NO_SPINBOX
void RenderThemeQt::adjustInnerSpinButtonStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    // Use the same width as our native scrollbar
    int width = ScrollbarTheme::theme()->scrollbarThickness();
    style->setWidth(Length(width, Fixed));
    style->setMinWidth(Length(width, Fixed));
}
#endif

bool RenderThemeQt::supportsFocus(ControlPart appearance) const
{
    switch (appearance) {
    case PushButtonPart:
    case ButtonPart:
    case TextFieldPart:
    case TextAreaPart:
    case ListboxPart:
    case MenulistPart:
    case RadioPart:
    case CheckboxPart:
    case SliderHorizontalPart:
    case SliderVerticalPart:
        return true;
    default: // No for all others...
        return false;
    }
}

#if ENABLE(VIDEO)

String RenderThemeQt::extraMediaControlsStyleSheet()
{
    String result = String(mediaControlsQtUserAgentStyleSheet, sizeof(mediaControlsQtUserAgentStyleSheet));

    if (m_page && m_page->chrome().requiresFullscreenForVideoPlayback())
        result.append(String(mediaControlsQtFullscreenUserAgentStyleSheet, sizeof(mediaControlsQtFullscreenUserAgentStyleSheet)));

    return result;
}

// Helper class to transform the painter's world matrix to the object's content area, scaled to 0,0,100,100
class WorldMatrixTransformer {
public:
    WorldMatrixTransformer(QPainter* painter, RenderObject* renderObject, const IntRect& r) : m_painter(painter)
    {
        RenderStyle* style = renderObject->style();
        m_originalTransform = m_painter->transform();
        m_painter->translate(r.x() + style->paddingLeft().value(), r.y() + style->paddingTop().value());
        m_painter->scale((r.width() - style->paddingLeft().value() - style->paddingRight().value()) / 100.0,
             (r.height() - style->paddingTop().value() - style->paddingBottom().value()) / 100.0);
    }

    ~WorldMatrixTransformer() { m_painter->setTransform(m_originalTransform); }

private:
    QPainter* m_painter;
    QTransform m_originalTransform;
};

double RenderThemeQt::mediaControlsBaselineOpacity() const
{
    return 0.4;
}

void RenderThemeQt::paintMediaBackground(QPainter* painter, const IntRect& r) const
{
    painter->setPen(Qt::NoPen);
    static QColor transparentBlack(0, 0, 0, mediaControlsBaselineOpacity() * 255);
    painter->setBrush(transparentBlack);
    painter->drawRoundedRect(r.x(), r.y(), r.width(), r.height(), 5.0, 5.0);
}

static bool mediaElementCanPlay(RenderObject* o)
{
    HTMLMediaElement* mediaElement = toParentMediaElement(o);
    if (!mediaElement)
        return false;

    return mediaElement->readyState() > HTMLMediaElement::HAVE_METADATA
           || (mediaElement->readyState() == HTMLMediaElement::HAVE_NOTHING
               && o->style()->appearance() == MediaPlayButtonPart && mediaElement->preload() == "none");
}

QColor RenderThemeQt::getMediaControlForegroundColor(RenderObject* o) const
{
    QColor fgColor = platformActiveSelectionBackgroundColor();
    if (!o)
        return fgColor;

    if (o->node() && o->node()->isElementNode() && toElement(o->node())->active())
        fgColor = fgColor.lighter();

    if (!mediaElementCanPlay(o))
        fgColor = colorPalette().brush(QPalette::Disabled, QPalette::Text).color();

    return fgColor;
}

bool RenderThemeQt::paintMediaFullscreenButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    HTMLMediaElement* mediaElement = toParentMediaElement(o);
    if (!mediaElement)
        return false;

    QSharedPointer<StylePainter> p = getStylePainter(paintInfo);
    if (p.isNull() || !p->isValid())
        return true;

    p->painter->setRenderHint(QPainter::Antialiasing, true);

    paintMediaBackground(p->painter, r);

    WorldMatrixTransformer transformer(p->painter, o, r);
    const QPointF arrowPolygon[9] = { QPointF(20, 0), QPointF(100, 0), QPointF(100, 80),
            QPointF(80, 80), QPointF(80, 30), QPointF(10, 100), QPointF(0, 90), QPointF(70, 20), QPointF(20, 20)};

    p->painter->setBrush(getMediaControlForegroundColor(o));
    p->painter->drawPolygon(arrowPolygon, 9);

    return false;
}

bool RenderThemeQt::paintMediaMuteButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    HTMLMediaElement* mediaElement = toParentMediaElement(o);
    if (!mediaElement)
        return false;

    QSharedPointer<StylePainter> p = getStylePainter(paintInfo);
    if (p.isNull() || !p->isValid())
        return true;

    p->painter->setRenderHint(QPainter::Antialiasing, true);

    paintMediaBackground(p->painter, r);

    WorldMatrixTransformer transformer(p->painter, o, r);
    const QPointF speakerPolygon[6] = { QPointF(20, 30), QPointF(50, 30), QPointF(80, 0),
            QPointF(80, 100), QPointF(50, 70), QPointF(20, 70)};

    p->painter->setBrush(mediaElement->muted() ? Qt::darkRed : getMediaControlForegroundColor(o));
    p->painter->drawPolygon(speakerPolygon, 6);

    return false;
}

bool RenderThemeQt::paintMediaPlayButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    HTMLMediaElement* mediaElement = toParentMediaElement(o);
    if (!mediaElement)
        return false;

    QSharedPointer<StylePainter> p = getStylePainter(paintInfo);
    if (p.isNull() || !p->isValid())
        return true;

    p->painter->setRenderHint(QPainter::Antialiasing, true);

    paintMediaBackground(p->painter, r);

    WorldMatrixTransformer transformer(p->painter, o, r);
    p->painter->setBrush(getMediaControlForegroundColor(o));
    if (mediaElement->canPlay()) {
        const QPointF playPolygon[3] = { QPointF(0, 0), QPointF(100, 50), QPointF(0, 100)};
        p->painter->drawPolygon(playPolygon, 3);
    } else {
        p->painter->drawRect(0, 0, 30, 100);
        p->painter->drawRect(70, 0, 30, 100);
    }

    return false;
}

bool RenderThemeQt::paintMediaToggleClosedCaptionsButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    HTMLMediaElement* mediaElement = toParentMediaElement(o);
    if (!mediaElement)
        return false;

    QSharedPointer<StylePainter> p = getStylePainter(paintInfo);
    if (p.isNull() || !p->isValid())
        return true;

    p->painter->setRenderHint(QPainter::Antialiasing, true);

    paintMediaBackground(p->painter, r);

    WorldMatrixTransformer transformer(p->painter, o, r);
    p->painter->setBrush(getMediaControlForegroundColor(o));

    QPainterPath captionBubble;
    captionBubble.moveTo(98.766, 43.244);
    captionBubble.cubicTo(captionBubble.currentPosition() + QPointF(0, -23.163), captionBubble.currentPosition() + QPointF(-21.775, -41.94), captionBubble.currentPosition() + QPointF(-48.637, -41.94));
    captionBubble.cubicTo(captionBubble.currentPosition() + QPointF(-26.859, 0), captionBubble.currentPosition() + QPointF(-48.635, 18.777), captionBubble.currentPosition() + QPointF(-48.635, 41.94));
    captionBubble.cubicTo(captionBubble.currentPosition() + QPointF(0, 18.266), captionBubble.currentPosition() + QPointF(13.546, 33.796), captionBubble.currentPosition() + QPointF(32.444, 39.549));
    captionBubble.cubicTo(captionBubble.currentPosition() + QPointF(1.131, 8.356), captionBubble.currentPosition() + QPointF(26.037, 24.255), captionBubble.currentPosition() + QPointF(22.864, 19.921));
    captionBubble.cubicTo(captionBubble.currentPosition() + QPointF(-4.462, -6.096), captionBubble.currentPosition() + QPointF(-5.159, -13.183), captionBubble.currentPosition() + QPointF(-5.07, -17.566));
    captionBubble.cubicTo(QPointF(77.85, 84.397), QPointF(98.766, 65.923), QPointF(98.766, 43.224));
    captionBubble.closeSubpath();

    p->painter->drawPath(captionBubble);
    return false;
}

bool RenderThemeQt::paintMediaSeekBackButton(RenderObject*, const PaintInfo&, const IntRect&)
{
    // We don't want to paint this at the moment.
    return false;
}

bool RenderThemeQt::paintMediaSeekForwardButton(RenderObject*, const PaintInfo&, const IntRect&)
{
    // We don't want to paint this at the moment.
    return false;
}

bool RenderThemeQt::paintMediaCurrentTime(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    QSharedPointer<StylePainter> p = getStylePainter(paintInfo);
    if (p.isNull() || !p->isValid())
        return true;

    p->painter->setRenderHint(QPainter::Antialiasing, true);
    paintMediaBackground(p->painter, r);

    return false;
}

String RenderThemeQt::formatMediaControlsCurrentTime(float currentTime, float duration) const
{
    return formatMediaControlsTime(currentTime) + " / " + formatMediaControlsTime(duration);
}

String RenderThemeQt::formatMediaControlsRemainingTime(float currentTime, float duration) const
{
    return String();
}

bool RenderThemeQt::paintMediaVolumeSliderTrack(RenderObject *o, const PaintInfo &paintInfo, const IntRect &r)
{
    QSharedPointer<StylePainter> p = getStylePainter(paintInfo);
    if (p.isNull() || !p->isValid())
        return true;

    p->painter->setRenderHint(QPainter::Antialiasing, true);

    paintMediaBackground(p->painter, r);

    if (!o->isSlider())
        return false;

    IntRect b = pixelSnappedIntRect(toRenderBox(o)->contentBoxRect());

    // Position the outer rectangle
    int top = r.y() + b.y();
    int left = r.x() + b.x();
    int width = b.width();
    int height = b.height();

    QPalette pal = colorPalette();
    const QColor highlightText = pal.brush(QPalette::Active, QPalette::HighlightedText).color();
    const QColor scaleColor(highlightText.red(), highlightText.green(), highlightText.blue(), mediaControlsBaselineOpacity() * 255);

    // Draw the outer rectangle
    p->painter->setBrush(scaleColor);
    p->painter->drawRect(left, top, width, height);

    if (!o->node() || !isHTMLInputElement(o->node()))
        return false;

    HTMLInputElement* slider = toHTMLInputElement(o->node());

    // Position the inner rectangle
    height = height * slider->valueAsNumber();
    top += b.height() - height;

    // Draw the inner rectangle
    p->painter->setPen(Qt::NoPen);
    p->painter->setBrush(getMediaControlForegroundColor(o));
    p->painter->drawRect(left, top, width, height);

    return false;
}

bool RenderThemeQt::paintMediaVolumeSliderThumb(RenderObject *o, const PaintInfo &paintInfo, const IntRect &r)
{
    QSharedPointer<StylePainter> p = getStylePainter(paintInfo);
    if (p.isNull() || !p->isValid())
        return true;

    // Nothing to draw here, this is all done in the track
    return false;
}

bool RenderThemeQt::paintMediaSliderTrack(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    HTMLMediaElement* mediaElement = toParentMediaElement(o);
    if (!mediaElement)
        return false;

    QSharedPointer<StylePainter> p = getStylePainter(paintInfo);
    if (p.isNull() || !p->isValid())
        return true;

    p->painter->setRenderHint(QPainter::Antialiasing, true);

    paintMediaBackground(p->painter, r);

    if (MediaPlayer* player = mediaElement->player()) {
        // Get the buffered parts of the media
        RefPtr<TimeRanges> buffered = player->buffered();
        if (buffered->length() > 0 && player->duration() < std::numeric_limits<float>::infinity()) {
            // Set the transform and brush
            WorldMatrixTransformer transformer(p->painter, o, r);
            p->painter->setBrush(getMediaControlForegroundColor());

            // Paint each buffered section
            for (int i = 0; i < buffered->length(); i++) {
                float startX = (buffered->start(i, IGNORE_EXCEPTION) / player->duration()) * 100;
                float width = ((buffered->end(i, IGNORE_EXCEPTION) / player->duration()) * 100) - startX;
                p->painter->drawRect(startX, 37, width, 26);
            }
        }
    }

    return false;
}

bool RenderThemeQt::paintMediaSliderThumb(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    ASSERT(o->node());
    Node* hostNode = o->node()->shadowHost();
    if (!hostNode)
        hostNode = o->node();
    HTMLMediaElement* mediaElement = toParentMediaElement(hostNode);
    if (!mediaElement)
        return false;

    QSharedPointer<StylePainter> p = getStylePainter(paintInfo);
    if (p.isNull() || !p->isValid())
        return true;

    p->painter->setRenderHint(QPainter::Antialiasing, true);

    p->painter->setPen(Qt::NoPen);
    p->painter->setBrush(getMediaControlForegroundColor(hostNode->renderer()));
    p->painter->drawRect(r.x(), r.y(), r.width(), r.height());

    return false;
}
#endif

void RenderThemeQt::adjustSliderThumbSize(RenderStyle* style, Element*) const
{
    // timelineThumbHeight should match the height property of -webkit-media-controls-timeline in mediaControlsQt.css.
    const int timelineThumbHeight = 12;
    const int timelineThumbWidth = timelineThumbHeight / 3;
    // volumeThumbWidth should match the width property of -webkit-media-controls-volume-slider in mediaControlsQt.css.
    const int volumeThumbWidth = 12;
    const int volumeThumbHeight = volumeThumbWidth / 3;
    ControlPart part = style->appearance();

    if (part == MediaSliderThumbPart) {
        style->setWidth(Length(timelineThumbWidth, Fixed));
        style->setHeight(Length(timelineThumbHeight, Fixed));
    } else if (part == MediaVolumeSliderThumbPart) {
        style->setHeight(Length(volumeThumbHeight, Fixed));
        style->setWidth(Length(volumeThumbWidth, Fixed));
    }
}

double RenderThemeQt::caretBlinkInterval() const
{
    return static_cast<QGuiApplication*>(qApp)->styleHints()->cursorFlashTime() / 1000.0 / 2.0;
}

String RenderThemeQt::fileListNameForWidth(const FileList* fileList, const Font& font, int width, bool multipleFilesAllowed) const
{
    UNUSED_PARAM(multipleFilesAllowed);
    if (width <= 0)
        return String();

    String string;
    if (fileList->isEmpty())
        string = fileButtonNoFileSelectedLabel();
    else if (fileList->length() == 1) {
        String fname = fileList->item(0)->path();
        QFontMetrics fm(font.syntheticFont());
        string = fm.elidedText(fname, Qt::ElideLeft, width);
    } else {
        int n = fileList->length();
        string = QCoreApplication::translate("QWebPage", "%n file(s)",
                                             "number of chosen file",
                                             n);
    }

    return string;
}

StylePainter::StylePainter(GraphicsContext* context)
    : painter(context->platformContext())
{
    ASSERT(context);

    if (painter) {
        // the styles often assume being called with a pristine painter where no brush is set,
        // so reset it manually
        m_previousBrush = painter->brush();
        painter->setBrush(Qt::NoBrush);

        // painting the widget with anti-aliasing will make it blurry
        // disable it here and restore it later
        m_previousAntialiasing = painter->testRenderHint(QPainter::Antialiasing);
        painter->setRenderHint(QPainter::Antialiasing, false);
    }
}

StylePainter::~StylePainter()
{
    if (painter) {
        painter->setBrush(m_previousBrush);
        painter->setRenderHints(QPainter::Antialiasing, m_previousAntialiasing);
    }
}

}

// vim: ts=4 sw=4 et
