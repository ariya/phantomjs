/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2008-2012 Nokia Corporation and/or its subsidiary(-ies)
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
#include "RenderThemeQStyle.h"

#include "CSSFontSelector.h"
#include "CSSValueKeywords.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Color.h"
#include "Document.h"
#include "Font.h"
#include "FontSelector.h"
#include "GraphicsContext.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "LocalizedStrings.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PaintInfo.h"
#include "QWebPageClient.h"
#include "RenderBox.h"
#if ENABLE(PROGRESS_ELEMENT)
#include "RenderProgress.h"
#endif
#include "RenderSlider.h"
#include "ScrollbarThemeQStyle.h"
#include "SliderThumbElement.h"
#include "StyleResolver.h"
#include "UserAgentStyleSheets.h"

#include <QPainter>

namespace WebCore {

using namespace HTMLNames;

QSharedPointer<StylePainter> RenderThemeQStyle::getStylePainter(const PaintInfo& paintInfo)
{
    return QSharedPointer<StylePainter>(new StylePainterQStyle(this, paintInfo, /*RenderObject*/0));
}

StylePainterQStyle::StylePainterQStyle(RenderThemeQStyle* theme, const PaintInfo& paintInfo, RenderObject* renderObject)
    : StylePainter(paintInfo.context)
    , qStyle(theme->qStyle())
    , appearance(NoControlPart)
{
    setupStyleOption();
    if (renderObject)
        appearance = theme->initializeCommonQStyleOptions(styleOption, renderObject);
}

StylePainterQStyle::StylePainterQStyle(ScrollbarThemeQStyle* theme, GraphicsContext* context)
    : StylePainter(context)
    , qStyle(theme->qStyle())
    , appearance(NoControlPart)
{
    setupStyleOption();
}

void StylePainterQStyle::setupStyleOption()
{
    if (QObject* widget = qStyle->widgetForPainter(painter)) {
        styleOption.palette = widget->property("palette").value<QPalette>();
        styleOption.rect = widget->property("rect").value<QRect>();
        styleOption.direction = static_cast<Qt::LayoutDirection>(widget->property("layoutDirection").toInt());
    }
}

PassRefPtr<RenderTheme> RenderThemeQStyle::create(Page* page)
{
    return adoptRef(new RenderThemeQStyle(page));
}

static QtStyleFactoryFunction styleFactoryFunction;

void RenderThemeQStyle::setStyleFactoryFunction(QtStyleFactoryFunction function)
{
    styleFactoryFunction = function;
}

QtStyleFactoryFunction RenderThemeQStyle::styleFactory()
{
    return styleFactoryFunction;
}

RenderThemeQStyle::RenderThemeQStyle(Page* page)
    : RenderThemeQt(page)
    , m_qStyle(adoptPtr(styleFactoryFunction(page)))
{
    int buttonPixelSize = 0;
    m_qStyle->getButtonMetrics(&m_buttonFontFamily, &buttonPixelSize);
#ifdef Q_WS_MAC
    m_buttonFontPixelSize = buttonPixelSize;
#endif
}

RenderThemeQStyle::~RenderThemeQStyle()
{
}

void RenderThemeQStyle::setPaletteFromPageClientIfExists(QPalette& palette) const
{
    if (!m_page)
        return;

    ChromeClient* chromeClient = m_page->chrome().client();
    if (!chromeClient)
        return;

    if (QWebPageClient* pageClient = chromeClient->platformPageClient())
        palette = pageClient->palette();
}

QRect RenderThemeQStyle::inflateButtonRect(const QRect& originalRect) const
{
    QRect layoutRect = m_qStyle->buttonSubElementRect(QStyleFacade::PushButtonLayoutItem, QStyleFacade::State_Small, originalRect);
    if (!layoutRect.isNull()) {
        int paddingLeft = layoutRect.left() - originalRect.left();
        int paddingRight = originalRect.right() - layoutRect.right();
        int paddingTop = layoutRect.top() - originalRect.top();
        int paddingBottom = originalRect.bottom() - layoutRect.bottom();

        return originalRect.adjusted(-paddingLeft, -paddingTop, paddingRight, paddingBottom);
    }
    return originalRect;
}

void RenderThemeQStyle::computeSizeBasedOnStyle(RenderStyle* renderStyle) const
{
    QSize size(0, 0);
    const QFontMetrics fm(renderStyle->font().syntheticFont());

    switch (renderStyle->appearance()) {
    case TextAreaPart:
    case SearchFieldPart:
    case TextFieldPart: {
        int padding = m_qStyle->findFrameLineWidth();
        renderStyle->setPaddingLeft(Length(padding, Fixed));
        renderStyle->setPaddingRight(Length(padding, Fixed));
        renderStyle->setPaddingTop(Length(padding, Fixed));
        renderStyle->setPaddingBottom(Length(padding, Fixed));
        break;
    }
    default:
        break;
    }
    // If the width and height are both specified, then we have nothing to do.
    if (!renderStyle->width().isIntrinsicOrAuto() && !renderStyle->height().isAuto())
        return;

    switch (renderStyle->appearance()) {
    case CheckboxPart: {
        int checkBoxWidth = m_qStyle->simplePixelMetric(QStyleFacade::PM_IndicatorWidth, QStyleFacade::State_Small);
        checkBoxWidth *= renderStyle->effectiveZoom();
        size = QSize(checkBoxWidth, checkBoxWidth);
        break;
    }
    case RadioPart: {
        int radioWidth = m_qStyle->simplePixelMetric(QStyleFacade::PM_ExclusiveIndicatorWidth, QStyleFacade::State_Small);
        radioWidth *= renderStyle->effectiveZoom();
        size = QSize(radioWidth, radioWidth);
        break;
    }
    case PushButtonPart:
    case ButtonPart: {
        QSize contentSize = fm.size(Qt::TextShowMnemonic, QString::fromLatin1("X"));
        QSize pushButtonSize = m_qStyle->pushButtonSizeFromContents(QStyleFacade::State_Small, contentSize);
        QRect layoutRect = m_qStyle->buttonSubElementRect(QStyleFacade::PushButtonLayoutItem, QStyleFacade::State_Small, QRect(0, 0, pushButtonSize.width(), pushButtonSize.height()));

        // If the style supports layout rects we use that, and  compensate accordingly
        // in paintButton() below.
        if (!layoutRect.isNull())
            size.setHeight(layoutRect.height());
        else
            size.setHeight(pushButtonSize.height());

        break;
    }
    case MenulistPart: {
        int contentHeight = qMax(fm.lineSpacing(), 14) + 2;
        QSize menuListSize = m_qStyle->comboBoxSizeFromContents(QStyleFacade::State_Small, QSize(0, contentHeight));
        size.setHeight(menuListSize.height());
        break;
    }
    default:
        break;
    }

    // FIXME: Check is flawed, since it doesn't take min-width/max-width into account.
    if (renderStyle->width().isIntrinsicOrAuto() && size.width() > 0)
        renderStyle->setMinWidth(Length(size.width(), Fixed));
    if (renderStyle->height().isAuto() && size.height() > 0)
        renderStyle->setMinHeight(Length(size.height(), Fixed));
}



void RenderThemeQStyle::adjustButtonStyle(StyleResolver* styleResolver, RenderStyle* style, Element*) const
{
    // Ditch the border.
    style->resetBorder();

#ifdef Q_WS_MAC
    if (style->appearance() == PushButtonPart) {
        // The Mac ports ignore the specified height for <input type="button"> elements
        // unless a border and/or background CSS property is also specified.
        style->setHeight(Length(Auto));
    }
#endif

    FontDescription fontDescription = style->fontDescription();
    fontDescription.setIsAbsoluteSize(true);

#ifdef Q_WS_MAC // Use fixed font size and family on Mac (like Safari does)
    fontDescription.setSpecifiedSize(m_buttonFontPixelSize);
    fontDescription.setComputedSize(m_buttonFontPixelSize);
#else
    fontDescription.setSpecifiedSize(style->fontSize());
    fontDescription.setComputedSize(style->fontSize());
#endif

    Vector<AtomicString, 1> families;
    families.append(m_buttonFontFamily);
    fontDescription.setFamilies(families);
    style->setFontDescription(fontDescription);
    style->font().update(styleResolver->fontSelector());
    style->setLineHeight(RenderStyle::initialLineHeight());
    setButtonSize(style);
    setButtonPadding(style);
}

void RenderThemeQStyle::setButtonPadding(RenderStyle* style) const
{
    // Fake a button rect here, since we're just computing deltas
    QRect originalRect = QRect(0, 0, 100, 30);

    // Default padding is based on the button margin pixel metric
    int buttonMargin = m_qStyle->buttonMargin(QStyleFacade::State_Small, originalRect);
    int paddingLeft = buttonMargin;
    int paddingRight = buttonMargin;
    int paddingTop = buttonMargin;
    int paddingBottom = buttonMargin;

    // Then check if the style uses layout margins
    QRect layoutRect = m_qStyle->buttonSubElementRect(QStyleFacade::PushButtonLayoutItem, QStyleFacade::State_Small, originalRect);
    if (!layoutRect.isNull()) {
        QRect contentsRect = m_qStyle->buttonSubElementRect(QStyleFacade::PushButtonContents, QStyleFacade::State_Small, originalRect);
        paddingLeft = contentsRect.left() - layoutRect.left();
        paddingRight = layoutRect.right() - contentsRect.right();
        paddingTop = contentsRect.top() - layoutRect.top();

        // Can't use this right now because we don't have the baseline to compensate
        // paddingBottom = layoutRect.bottom() - contentsRect.bottom();
    }
    style->setPaddingLeft(Length(paddingLeft, Fixed));
    style->setPaddingRight(Length(paddingRight, Fixed));
    style->setPaddingTop(Length(paddingTop, Fixed));
    style->setPaddingBottom(Length(paddingBottom, Fixed));
}

bool RenderThemeQStyle::paintButton(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    StylePainterQStyle p(this, i, o);
    if (!p.isValid())
        return true;

    p.styleOption.rect = r;
    p.styleOption.state |= QStyleFacade::State_Small;

    if (p.appearance == PushButtonPart || p.appearance == ButtonPart) {
        p.styleOption.rect = inflateButtonRect(p.styleOption.rect);
        p.paintButton(QStyleFacade::PushButton);
    } else if (p.appearance == RadioPart)
        p.paintButton(QStyleFacade::RadioButton);
    else if (p.appearance == CheckboxPart)
        p.paintButton(QStyleFacade::CheckBox);

    return false;
}

bool RenderThemeQStyle::paintTextField(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    StylePainterQStyle p(this, i, o);
    if (!p.isValid())
        return true;

    p.styleOption.rect = r;
    p.styleOption.state |= QStyleFacade::State_Sunken;

    // Get the correct theme data for a text field
    if (p.appearance != TextFieldPart
        && p.appearance != SearchFieldPart
        && p.appearance != TextAreaPart
        && p.appearance != ListboxPart)
        return true;

    // Now paint the text field.
    p.paintTextField();
    return false;
}

void RenderThemeQStyle::adjustTextAreaStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
    adjustTextFieldStyle(styleResolver, style, element);
}

bool RenderThemeQStyle::paintTextArea(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    return paintTextField(o, i, r);
}

void RenderThemeQStyle::setPopupPadding(RenderStyle* style) const
{
    const int paddingLeft = 4;
    const int paddingRight = style->width().isFixed() || style->width().isPercent() ? 5 : 8;

    style->setPaddingLeft(Length(paddingLeft, Fixed));

    int w = m_qStyle->simplePixelMetric(QStyleFacade::PM_ButtonIconSize);
    style->setPaddingRight(Length(paddingRight + w, Fixed));

    style->setPaddingTop(Length(2, Fixed));
    style->setPaddingBottom(Length(2, Fixed));
}

QPalette RenderThemeQStyle::colorPalette() const
{
    QPalette palette = RenderThemeQt::colorPalette();
    setPaletteFromPageClientIfExists(palette);
    return palette;
}

bool RenderThemeQStyle::paintMenuList(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    StylePainterQStyle p(this, i, o);
    if (!p.isValid())
        return true;

    p.styleOption.rect = r;
    p.paintComboBox();
    return false;
}

void RenderThemeQStyle::adjustMenuListButtonStyle(StyleResolver* styleResolver, RenderStyle* style, Element* e) const
{
    // WORKAROUND because html.css specifies -webkit-border-radius for <select> so we override it here
    // see also http://bugs.webkit.org/show_bug.cgi?id=18399
    style->resetBorderRadius();

    RenderThemeQt::adjustMenuListButtonStyle(styleResolver, style, e);
}

bool RenderThemeQStyle::paintMenuListButton(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    StylePainterQStyle p(this, i, o);
    if (!p.isValid())
        return true;

    p.styleOption.rect = r;
    p.paintComboBoxArrow();
    return false;
}

#if ENABLE(PROGRESS_ELEMENT)
double RenderThemeQStyle::animationDurationForProgressBar(RenderProgress* renderProgress) const
{
    if (renderProgress->position() >= 0)
        return 0;

    IntSize size = renderProgress->pixelSnappedSize();
    // FIXME: Until http://bugreports.qt.nokia.com/browse/QTBUG-9171 is fixed,
    // we simulate one square animating across the progress bar.
    return (size.width() / m_qStyle->progressBarChunkWidth(size)) * animationRepeatIntervalForProgressBar(renderProgress);
}

bool RenderThemeQStyle::paintProgressBar(RenderObject* o, const PaintInfo& pi, const IntRect& r)
{
    if (!o->isProgress())
        return true;

    StylePainterQStyle p(this, pi, o);
    if (!p.isValid())
        return true;

    p.styleOption.rect = r;
    RenderProgress* renderProgress = toRenderProgress(o);
    p.paintProgressBar(renderProgress->position(), renderProgress->animationProgress());
    return false;
}
#endif

bool RenderThemeQStyle::paintSliderTrack(RenderObject* o, const PaintInfo& pi, const IntRect& r)
{
    StylePainterQStyle p(this, pi, o);
    if (!p.isValid())
        return true;

    const QPoint topLeft = r.location();
    p.painter->translate(topLeft);

    p.styleOption.rect = r;
    p.styleOption.rect.moveTo(QPoint(0, 0));

    if (p.appearance == SliderVerticalPart)
        p.styleOption.slider.orientation = Qt::Vertical;

    if (isPressed(o))
        p.styleOption.state |= QStyleFacade::State_Sunken;

    // some styles need this to show a highlight on one side of the groove
    HTMLInputElement* slider = o->node() ? o->node()->toInputElement() : 0;
    if (slider && slider->isSteppable()) {
        p.styleOption.slider.upsideDown = (p.appearance == SliderHorizontalPart) && !o->style()->isLeftToRightDirection();
        // Use the width as a multiplier in case the slider values are <= 1
        const int width = r.width() > 0 ? r.width() : 100;
        p.styleOption.slider.maximum = slider->maximum() * width;
        p.styleOption.slider.minimum = slider->minimum() * width;
        if (!p.styleOption.slider.upsideDown)
            p.styleOption.slider.position = slider->valueAsNumber() * width;
        else
            p.styleOption.slider.position = p.styleOption.slider.minimum + p.styleOption.slider.maximum - slider->valueAsNumber() * width;
    }

    p.paintSliderTrack();

    p.painter->translate(-topLeft);
    return false;
}

void RenderThemeQStyle::adjustSliderTrackStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    style->setBoxShadow(nullptr);
}

bool RenderThemeQStyle::paintSliderThumb(RenderObject* o, const PaintInfo& pi, const IntRect& r)
{
    StylePainterQStyle p(this, pi, o);
    if (!p.isValid())
        return true;

    const QPoint topLeft = r.location();
    p.painter->translate(topLeft);

    p.styleOption.rect = r;
    p.styleOption.rect.moveTo(QPoint(0, 0));
    p.styleOption.slider.orientation = Qt::Horizontal;
    if (p.appearance == SliderThumbVerticalPart)
        p.styleOption.slider.orientation = Qt::Vertical;
    if (isPressed(o))
        p.styleOption.state |= QStyleFacade::State_Sunken;

    p.paintSliderThumb();

    p.painter->translate(-topLeft);
    return false;
}

void RenderThemeQStyle::adjustSliderThumbStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
    RenderTheme::adjustSliderThumbStyle(styleResolver, style, element);
    style->setBoxShadow(nullptr);
}

bool RenderThemeQStyle::paintSearchField(RenderObject* o, const PaintInfo& pi, const IntRect& r)
{
    return paintTextField(o, pi, r);
}

void RenderThemeQStyle::adjustSearchFieldDecorationStyle(StyleResolver* styleResolver, RenderStyle* style, Element* e) const
{
    notImplemented();
    RenderTheme::adjustSearchFieldDecorationStyle(styleResolver, style, e);
}

bool RenderThemeQStyle::paintSearchFieldDecoration(RenderObject* o, const PaintInfo& pi, const IntRect& r)
{
    notImplemented();
    return RenderTheme::paintSearchFieldDecoration(o, pi, r);
}

void RenderThemeQStyle::adjustSearchFieldResultsDecorationStyle(StyleResolver* styleResolver, RenderStyle* style, Element* e) const
{
    notImplemented();
    RenderTheme::adjustSearchFieldResultsDecorationStyle(styleResolver, style, e);
}

bool RenderThemeQStyle::paintSearchFieldResultsDecoration(RenderObject* o, const PaintInfo& pi, const IntRect& r)
{
    notImplemented();
    return RenderTheme::paintSearchFieldResultsDecoration(o, pi, r);
}

#ifndef QT_NO_SPINBOX

bool RenderThemeQStyle::paintInnerSpinButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& rect)
{
    StylePainterQStyle p(this, paintInfo, o);
    if (!p.isValid())
        return true;

    p.styleOption.rect = rect;
    p.paintInnerSpinButton(isSpinUpButtonPartPressed(o));
    return false;
}
#endif

ControlPart RenderThemeQStyle::initializeCommonQStyleOptions(QStyleFacadeOption &option, RenderObject* o) const
{
    // Default bits: no focus, no mouse over, enabled
    option.state &= ~(QStyleFacade::State_HasFocus | QStyleFacade::State_MouseOver);
    option.state |= QStyleFacade::State_Enabled;

    if (isReadOnlyControl(o))
        // Readonly is supported on textfields.
        option.state |= QStyleFacade::State_ReadOnly;

    option.direction = Qt::LeftToRight;

    if (isHovered(o))
        option.state |= QStyleFacade::State_MouseOver;

    setPaletteFromPageClientIfExists(option.palette);

    if (!isEnabled(o)) {
        option.palette.setCurrentColorGroup(QPalette::Disabled);
        option.state &= ~QStyleFacade::State_Enabled;
    }

    RenderStyle* style = o->style();
    if (!style)
        return NoControlPart;

    ControlPart result = style->appearance();
    if (supportsFocus(result) && isFocused(o)) {
        option.state |= QStyleFacade::State_HasFocus;
        option.state |= QStyleFacade::State_KeyboardFocusChange;
    }

    if (style->direction() == WebCore::RTL)
        option.direction = Qt::RightToLeft;

    switch (result) {
    case PushButtonPart:
    case SquareButtonPart:
    case ButtonPart:
    case ButtonBevelPart:
    case ListItemPart:
    case MenulistButtonPart:
    case InnerSpinButtonPart:
    case SearchFieldResultsButtonPart:
    case SearchFieldCancelButtonPart: {
        if (isPressed(o))
            option.state |= QStyleFacade::State_Sunken;
        else if (result == PushButtonPart || result == ButtonPart)
            option.state |= QStyleFacade::State_Raised;
        break;
    }
    case RadioPart:
    case CheckboxPart:
        option.state |= (isChecked(o) ? QStyleFacade::State_On : QStyleFacade::State_Off);
    }

    return result;
}

void RenderThemeQStyle::adjustSliderThumbSize(RenderStyle* style, Element* element) const
{
    const ControlPart part = style->appearance();
    if (part == SliderThumbHorizontalPart || part == SliderThumbVerticalPart) {
        Qt::Orientation orientation = Qt::Horizontal;
        if (part == SliderThumbVerticalPart)
            orientation = Qt::Vertical;

        int length = m_qStyle->sliderLength(orientation);
        int thickness = m_qStyle->sliderThickness(orientation);
        if (orientation == Qt::Vertical) {
            style->setWidth(Length(thickness, Fixed));
            style->setHeight(Length(length, Fixed));
        } else {
            style->setWidth(Length(length, Fixed));
            style->setHeight(Length(thickness, Fixed));
        }
    } else
        RenderThemeQt::adjustSliderThumbSize(style, element);
}

}

// vim: ts=4 sw=4 et
