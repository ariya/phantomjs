/*
 * This file is part of the theme implementation for form controls in WebCore.
 *
 * Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
 * Copyright (C) 2011-2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "QStyleFacadeImp.h"

#include <QApplication>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QStyleFactory>
#include <QStyleOption>

#include <QWebPageAdapter.h>
#include <QWebPageClient.h>

using namespace WebCore;

namespace WebKit {

static QStyle::State convertToQStyleState(QStyleFacade::State state)
{
    QStyle::State result;
#define CONVERT_STATE(ProxiedState, Value) \
    if (state & QStyleFacade::ProxiedState) \
        result |= QStyle::ProxiedState;

    FOR_EACH_MAPPED_STATE(CONVERT_STATE, )

#undef CONVERT_STATE

    return result;
}

static QStyle::SubControl convertToQStyleSubControl(QStyleFacade::SubControl sc)
{
#define CONVERT_SUBCONTROL(F, Value) \
    case QStyleFacade::F: return QStyle::F

    switch (sc) {
        FOR_EACH_SUBCONTROL(CONVERT_SUBCONTROL, SEMICOLON);
    }
    ASSERT_NOT_REACHED();
    return QStyle::SC_None;
#undef CONVERT_SUBCONTROL
}

static void initGenericStyleOption(QStyleOption* option, QWidget* widget, const QStyleFacadeOption& facadeOption)
{
    if (widget)
        option->init(widget);
    else
        // If a widget is not directly available for rendering, we fallback to default
        // value for an active widget.
        option->state = QStyle::State_Active | QStyle::State_Enabled;

    option->rect = facadeOption.rect;
    option->state = convertToQStyleState(facadeOption.state);
    if (facadeOption.direction != Qt::LayoutDirectionAuto)
        option->direction = facadeOption.direction;
    option->palette = facadeOption.palette;
}

static void initSpecificStyleOption(QStyleOption*, const QStyleFacadeOption&)
{
}

static void initSpecificStyleOption(QStyleOptionSlider* sliderOption, const QStyleFacadeOption& facadeOption)
{
    sliderOption->orientation = facadeOption.slider.orientation;
    sliderOption->upsideDown = facadeOption.slider.upsideDown;
    sliderOption->minimum = facadeOption.slider.minimum;
    sliderOption->maximum = facadeOption.slider.maximum;
    sliderOption->sliderPosition = facadeOption.slider.position;
    sliderOption->sliderValue = facadeOption.slider.value;
    sliderOption->singleStep = facadeOption.slider.singleStep;
    sliderOption->pageStep = facadeOption.slider.pageStep;
    sliderOption->activeSubControls = convertToQStyleSubControl(facadeOption.slider.activeSubControls);
}

template <typename StyleOption>
class MappedStyleOption : public StyleOption {

public:
    MappedStyleOption(QWidget* widget, const QStyleFacadeOption& facadeOption)
    {
        initGenericStyleOption(this, widget, facadeOption);
        initSpecificStyleOption(this, facadeOption);
    }
};

static QStyle::PixelMetric convertPixelMetric(QStyleFacade::PixelMetric state)
{
#define CONVERT_METRIC(Metric) \
    case QStyleFacade::Metric: return QStyle::Metric

    switch (state) {
        FOR_EACH_MAPPED_METRIC(CONVERT_METRIC, SEMICOLON);
    }
    ASSERT_NOT_REACHED();
    return QStyle::PM_CustomBase;

#undef CONVERT_METRIC
}

static QStyleFacade::SubControl convertToQStyleFacadeSubControl(QStyle::SubControl sc)
{
#define CONVERT_SUBCONTROL(F, Value) \
    case QStyle::F: return QStyleFacade::F

    switch (sc) {
        FOR_EACH_SUBCONTROL(CONVERT_SUBCONTROL, SEMICOLON);
    }
    ASSERT_NOT_REACHED();
    return QStyleFacade::SC_None;
#undef CONVERT_SUBCONTROL
}

QStyleFacadeImp::QStyleFacadeImp(QWebPageAdapter* page)
    : m_page(page)
    , m_style(0)
{
    m_fallbackStyle = QStyleFactory::create(QLatin1String("windows"));
    m_ownFallbackStyle = true;
    if (!m_fallbackStyle) {
        m_fallbackStyle = QApplication::style();
        m_ownFallbackStyle = false;
    }
}

QStyleFacadeImp::~QStyleFacadeImp()
{
    if (m_ownFallbackStyle)
        delete m_fallbackStyle;
}

QRect QStyleFacadeImp::buttonSubElementRect(QStyleFacade::ButtonSubElement buttonElement, State state, const QRect& originalRect) const
{
    QStyleOptionButton option;
    option.state = convertToQStyleState(state);
    option.rect = originalRect;

    QStyle::SubElement subElement = QStyle::SE_CustomBase;
    switch (buttonElement) {
    case PushButtonLayoutItem: subElement = QStyle::SE_PushButtonLayoutItem; break;
    case PushButtonContents: subElement = QStyle::SE_PushButtonContents; break;
    default: ASSERT_NOT_REACHED();
    }
    return style()->subElementRect(subElement, &option);
}

int QStyleFacadeImp::findFrameLineWidth() const
{
    if (!m_lineEdit)
        m_lineEdit.reset(new QLineEdit());

    return style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, m_lineEdit.data());
}

int QStyleFacadeImp::simplePixelMetric(QStyleFacade::PixelMetric metric, State state) const
{
    QStyleOption opt;
    opt.state = convertToQStyleState(state);
    return style()->pixelMetric(convertPixelMetric(metric), &opt, 0);
}

int QStyleFacadeImp::buttonMargin(State state, const QRect& originalRect) const
{
    QStyleOptionButton styleOption;
    styleOption.state = convertToQStyleState(state);
    styleOption.rect = originalRect;
    return style()->pixelMetric(QStyle::PM_ButtonMargin, &styleOption, 0);
}

int QStyleFacadeImp::sliderLength(Qt::Orientation orientation) const
{
    QStyleOptionSlider opt;
    opt.orientation = orientation;
    return style()->pixelMetric(QStyle::PM_SliderLength, &opt);
}

int QStyleFacadeImp::sliderThickness(Qt::Orientation orientation) const
{
    QStyleOptionSlider opt;
    opt.orientation = orientation;
    return style()->pixelMetric(QStyle::PM_SliderThickness, &opt);
}

int QStyleFacadeImp::progressBarChunkWidth(const QSize& size) const
{
    QStyleOptionProgressBarV2 option;
    option.rect.setSize(size);
    // FIXME: Until http://bugreports.qt.nokia.com/browse/QTBUG-9171 is fixed,
    // we simulate one square animating across the progress bar.
    return style()->pixelMetric(QStyle::PM_ProgressBarChunkWidth, &option);
}

void QStyleFacadeImp::getButtonMetrics(QString *buttonFontFamily, int *buttonFontPixelSize) const
{
    QPushButton button;
    QFont defaultButtonFont = QApplication::font(&button);
    *buttonFontFamily = defaultButtonFont.family();
    *buttonFontPixelSize = 0;
#ifdef Q_OS_MAC
    button.setAttribute(Qt::WA_MacSmallSize);
    QFontInfo fontInfo(defaultButtonFont);
    *buttonFontPixelSize = fontInfo.pixelSize();
#endif
}

QSize QStyleFacadeImp::comboBoxSizeFromContents(State state, const QSize& contentsSize) const
{
    QStyleOptionComboBox opt;
    opt.state = convertToQStyleState(state);
    return style()->sizeFromContents(QStyle::CT_ComboBox, &opt, contentsSize);
}

QSize QStyleFacadeImp::pushButtonSizeFromContents(State state, const QSize& contentsSize) const
{
    QStyleOptionButton opt;
    opt.state = convertToQStyleState(state);
    return style()->sizeFromContents(QStyle::CT_PushButton, &opt, contentsSize);
}

void QStyleFacadeImp::paintButton(QPainter* painter, QStyleFacade::ButtonType type, const QStyleFacadeOption &proxyOption)
{
    QWidget* widget = qobject_cast<QWidget*>(widgetForPainter(painter));
    MappedStyleOption<QStyleOptionButton> option(widget, proxyOption);

    if (m_style->inherits("QWindowsVistaStyle"))
        option.styleObject = 0;

    if (type == PushButton)
        style()->drawControl(QStyle::CE_PushButton, &option, painter, widget);
    else if (type == RadioButton)
        style()->drawControl(QStyle::CE_RadioButton, &option, painter, widget);
    else if (type == CheckBox)
        style()->drawControl(QStyle::CE_CheckBox, &option, painter, widget);
}

void QStyleFacadeImp::paintTextField(QPainter *painter, const QStyleFacadeOption &proxyOption)
{
    QWidget* widget = qobject_cast<QWidget*>(widgetForPainter(painter));

    MappedStyleOption<QStyleOptionFrameV2> panel(widget, proxyOption);

    panel.lineWidth = findFrameLineWidth();
    panel.features = QStyleOptionFrameV2::None;

    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, painter, widget);
}

void QStyleFacadeImp::paintComboBox(QPainter *painter, const QStyleFacadeOption &proxyOption)
{
    QWidget* widget = qobject_cast<QWidget*>(widgetForPainter(painter));

    MappedStyleOption<QStyleOptionComboBox> opt(widget, proxyOption);

    QRect rect = opt.rect;

#if defined(Q_OS_MAC) && !defined(QT_NO_STYLE_MAC)
    // QMacStyle makes the combo boxes a little bit smaller to leave space for the focus rect.
    // Because of it, the combo button is drawn at a point to the left of where it was expect to be and may end up
    // overlapped with the text. This will force QMacStyle to draw the combo box with the expected width.
    if (m_style->inherits("QMacStyle")) {
        rect.setX(rect.x() - 3);
        rect.setWidth(rect.width() + 2 * 3);
    }
#endif

    painter->translate(rect.topLeft());
    opt.rect.moveTo(QPoint(0, 0));
    opt.rect.setSize(rect.size());

    style()->drawComplexControl(QStyle::CC_ComboBox, &opt, painter, widget);
    painter->translate(-rect.topLeft());
}

void QStyleFacadeImp::paintComboBoxArrow(QPainter *painter, const QStyleFacadeOption &proxyOption)
{
    QWidget* widget = qobject_cast<QWidget*>(widgetForPainter(painter));

    MappedStyleOption<QStyleOptionComboBox> opt(widget, proxyOption);
    opt.subControls = QStyle::SC_ComboBoxArrow;
    // for drawing the combo box arrow, rely only on the fallback style
    m_fallbackStyle->drawComplexControl(QStyle::CC_ComboBox, &opt, painter, widget);
}

void QStyleFacadeImp::paintSliderTrack(QPainter* painter, const QStyleFacadeOption& proxyOption)
{
    QWidget* widget = qobject_cast<QWidget*>(widgetForPainter(painter));

    MappedStyleOption<QStyleOptionSlider> option(widget, proxyOption);
    option.subControls = QStyle::SC_SliderGroove;

    option.upsideDown = proxyOption.slider.upsideDown;
    option.minimum = proxyOption.slider.minimum;
    option.maximum = proxyOption.slider.maximum;
    option.sliderPosition = proxyOption.slider.position;
    option.orientation = proxyOption.slider.orientation;

    style()->drawComplexControl(QStyle::CC_Slider, &option, painter, widget);

    if (option.state & QStyle::State_HasFocus) {
        QStyleOptionFocusRect focusOption;
        focusOption.rect = option.rect;
        style()->drawPrimitive(QStyle::PE_FrameFocusRect, &focusOption, painter, widget);
    }
}

void QStyleFacadeImp::paintSliderThumb(QPainter* painter, const QStyleFacadeOption& proxyOption)
{
    QWidget* widget = qobject_cast<QWidget*>(widgetForPainter(painter));

    MappedStyleOption<QStyleOptionSlider> option(widget, proxyOption);

    option.subControls = QStyle::SC_SliderHandle;
    if (option.state & QStyle::State_Sunken)
        option.activeSubControls = QStyle::SC_SliderHandle;

    style()->drawComplexControl(QStyle::CC_Slider, &option, painter, widget);
}

void QStyleFacadeImp::paintInnerSpinButton(QPainter* painter, const QStyleFacadeOption& proxyOption, bool spinBoxUp)
{
    QWidget* widget = qobject_cast<QWidget*>(widgetForPainter(painter));

    MappedStyleOption<QStyleOptionSpinBox> option(widget, proxyOption);

    option.subControls = QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;
    if (!(option.state & QStyle::State_ReadOnly)) {
        if (option.state & QStyle::State_Enabled)
            option.stepEnabled = QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;
        if (option.state & QStyle::State_Sunken) {
            if (spinBoxUp)
                option.activeSubControls = QStyle::SC_SpinBoxUp;
            else
                option.activeSubControls = QStyle::SC_SpinBoxDown;
        }
    }

    QRect buttonRect = option.rect;
    // Default to moving the buttons a little bit within the editor frame.
    int inflateX = -2;
    int inflateY = -2;
#if defined(Q_OS_MAC) && !defined(QT_NO_STYLE_MAC)
    // QMacStyle will position the aqua buttons flush to the right.
    // This will move them more within the control for better style, a la
    // Chromium look & feel.
    if (m_style->inherits("QMacStyle")) {
        inflateX = -4;
        // Render mini aqua spin buttons for QMacStyle to fit nicely into
        // the editor area, like Chromium.
        option.state |= QStyle::State_Mini;
    }
#endif

    buttonRect.setX(buttonRect.x() - inflateX);
    buttonRect.setWidth(buttonRect.width() + 2 * inflateX);
    buttonRect.setY(buttonRect.y() - inflateY);
    buttonRect.setHeight(buttonRect.height() + 2 * inflateY);
    option.rect = buttonRect;

    style()->drawComplexControl(QStyle::CC_SpinBox, &option, painter, widget);
}

void QStyleFacadeImp::paintProgressBar(QPainter* painter, const QStyleFacadeOption& proxyOption, double progress, double animationProgress)
{
    QWidget* widget = qobject_cast<QWidget*>(widgetForPainter(painter));

    MappedStyleOption<QStyleOptionProgressBarV2> option(widget, proxyOption);

    option.maximum = std::numeric_limits<int>::max();
    option.minimum = 0;
    option.progress = progress * std::numeric_limits<int>::max();

    const QPoint topLeft = option.rect.topLeft();
    painter->translate(topLeft);
    option.rect.moveTo(QPoint(0, 0));

    if (progress < 0) {
        // FIXME: Until http://bugreports.qt.nokia.com/browse/QTBUG-9171 is fixed,
        // we simulate one square animating across the progress bar.
        style()->drawControl(QStyle::CE_ProgressBarGroove, &option, painter, widget);
        int chunkWidth = style()->pixelMetric(QStyle::PM_ProgressBarChunkWidth, &option);
        QColor color = (option.palette.highlight() == option.palette.background()) ? option.palette.color(QPalette::Active, QPalette::Highlight) : option.palette.color(QPalette::Highlight);
        if (option.direction == Qt::RightToLeft)
            painter->fillRect(option.rect.right() - chunkWidth  - animationProgress * option.rect.width(), 0, chunkWidth, option.rect.height(), color);
        else
            painter->fillRect(animationProgress * option.rect.width(), 0, chunkWidth, option.rect.height(), color);
    } else
        style()->drawControl(QStyle::CE_ProgressBar, &option, painter, widget);

    painter->translate(-topLeft);
}

int QStyleFacadeImp::scrollBarExtent(bool mini)
{
    QStyleOptionSlider o;
    o.orientation = Qt::Vertical;
    o.state &= ~QStyle::State_Horizontal;
    if (mini)
        o.state |= QStyle::State_Mini;
    return style()->pixelMetric(QStyle::PM_ScrollBarExtent, &o, 0);
}

bool QStyleFacadeImp::scrollBarMiddleClickAbsolutePositionStyleHint() const
{
    return style()->styleHint(QStyle::SH_ScrollBar_MiddleClickAbsolutePosition);
}

void QStyleFacadeImp::paintScrollCorner(QPainter* painter, const QRect& rect)
{
    QWidget* widget = qobject_cast<QWidget*>(widgetForPainter(painter));

    QStyleOption option;
    option.rect = rect;
    style()->drawPrimitive(QStyle::PE_PanelScrollAreaCorner, &option, painter, widget);
}

QStyleFacade::SubControl QStyleFacadeImp::hitTestScrollBar(const QStyleFacadeOption &proxyOption, const QPoint &pos)
{
    MappedStyleOption<QStyleOptionSlider> opt(0, proxyOption);
    QStyle::SubControl sc = style()->hitTestComplexControl(QStyle::CC_ScrollBar, &opt, pos, 0);
    return convertToQStyleFacadeSubControl(sc);
}

QRect QStyleFacadeImp::scrollBarSubControlRect(const QStyleFacadeOption &proxyOption, QStyleFacade::SubControl subControl)
{
    MappedStyleOption<QStyleOptionSlider> opt(0, proxyOption);
    return style()->subControlRect(QStyle::CC_ScrollBar, &opt, convertToQStyleSubControl(subControl), 0);
}

void QStyleFacadeImp::paintScrollBar(QPainter *painter, const QStyleFacadeOption &proxyOption)
{
    QWidget* widget = qobject_cast<QWidget*>(widgetForPainter(painter));

    MappedStyleOption<QStyleOptionSlider> opt(widget, proxyOption);

    if (m_style->inherits("QMacStyle")) {
        // FIXME: Disable transient scrollbar animations on OSX to avoid hiding the whole webview with the scrollbar fade out animation.
        opt.styleObject = 0;
    }

    painter->fillRect(opt.rect, opt.palette.background());

    const QPoint topLeft = opt.rect.topLeft();
    painter->translate(topLeft);
    opt.rect.moveTo(QPoint(0, 0));
    style()->drawComplexControl(QStyle::CC_ScrollBar, &opt, painter, widget);
    opt.rect.moveTo(topLeft);
}

QObject* QStyleFacadeImp::widgetForPainter(QPainter* painter)
{
    QPaintDevice* dev = 0;
    if (painter)
        dev = painter->device();
    if (dev && dev->devType() == QInternal::Widget)
        return static_cast<QWidget*>(dev);
    return 0;
}

QStyle* QStyleFacadeImp::style() const
{
    if (m_style)
        return m_style;

    if (m_page) {
        if (QWebPageClient* pageClient = m_page->client.data())
            m_style = pageClient->style();
    }

    if (!m_style)
        m_style = QApplication::style();

    return m_style;
}

}
