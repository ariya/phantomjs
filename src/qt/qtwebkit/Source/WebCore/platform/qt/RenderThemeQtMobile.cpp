/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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
#include "RenderThemeQtMobile.h"

#include "CSSValueKeywords.h"
#include "Color.h"
#include "Document.h"
#include "Font.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLSelectElement.h"
#include "LocalizedStrings.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PaintInfo.h"
#include "QWebPageClient.h"
#include "RenderBox.h"
#if ENABLE(PROGRESS_ELEMENT)
#include "RenderProgress.h"
#endif
#include "StyleResolver.h"

#include <wtf/PassRefPtr.h>

#include <QColor>
#include <QFile>
#include <QPainter>
#include <QPixmapCache>

namespace WebCore {

using namespace HTMLNames;

// Constants used by the mobile theme
static const int arrowBoxWidth = 26;
static const int frameWidth = 2;
static const int checkBoxWidth = 21;
static const int radioWidth = 21;
static const int sliderSize = 20;
static const int buttonHeightRatio = 1.5;

static const float multipleComboDotsOffsetFactor = 1.8;
static const float buttonPaddingLeft = 18;
static const float buttonPaddingRight = 18;
static const float buttonPaddingTop = 2;
static const float buttonPaddingBottom = 3;
static const float menuListPadding = 9;
static const float textFieldPadding = 10;
static const float radiusFactor = 0.36;
static const float progressBarChunkPercentage = 0.2;
#if ENABLE(PROGRESS_ELEMENT)
static const int progressAnimationGranularity = 2;
#endif
static const float sliderGrooveBorderRatio = 0.2;
static const QColor darkColor(40, 40, 40);
static const QColor highlightColor(16, 128, 221);
static const QColor buttonGradientBottom(245, 245, 245);
static const QColor shadowColor(80, 80, 80, 160);

static QHash<KeyIdentifier, CacheKey> cacheKeys;

static qreal painterScale(QPainter* painter)
{
    if (!painter)
        return 1;

    const QTransform& transform = painter->transform();
    qreal scale = 1;

    if (transform.type() == QTransform::TxScale)
        scale = qAbs(transform.m11());
    else if (transform.type() >= QTransform::TxRotate) {
        const QLineF l1(0, 0, 1, 0);
        const QLineF l2 = transform.map(l1);
        scale = qAbs(l2.length() / l1.length());
    }
    return scale;
}

uint qHash(const KeyIdentifier& id)
{
    const quint32 value = id.trait1 + (id.trait2 << 1) + (uint(id.type) << 2) + (id.height << 5) + (id.width << 14) + (id.trait3 << 25);
    const unsigned char* p = reinterpret_cast<const unsigned char*>(&value);
    uint hash = 0;
    for (int i = 0; i < 4; ++i)
        hash ^= (hash << 5) + (hash >> 2) + p[i];
    return hash;
}

/*
 * The octants' indices are identified below, for each point (x,y)
 * in the first octant, we can populate the 7 others with the corresponding
 * point.
 *
 *                                       index |   xpos   |   ypos
 *                xd                    ---------------------------
 *      4      |<--->| 3                    0  |  xd + x  |    y
 *     __________________                   1  |  xd + y  |    x
 *    /                  \                  2  |  xd + y  |   -x
 * 5 |         .(c)       |  2              3  |  xd + x  |   -y
 * 6 |                    |  1              4  | -xd - x  |   -y
 *    \__________________/                  5  | -xd - y  |   -x
 *                                          6  | -xd - y  |    x
 *      7              0                    7  | -xd - x  |    y
 *
 **/

static void addPointToOctants(QVector<QPainterPath>& octants, const QPointF& center, qreal x, qreal y , int xDelta = 0)
{
    ASSERT(octants.count() == 8);

    for (short i = 0; i < 8; ++i) {
        QPainterPath& octant = octants[i];
        QPointF pos(center);
        // The Gray code corresponding to the octant's index helps doing the math in a more generic way.
        const short gray = (i >> 1) ^ i;
        const qreal xOffset = xDelta + ((gray & 1) ? y : x);
        pos.ry() += ((gray & 2)? -1 : 1) * ((gray & 1) ? x : y);
        pos.rx() += (i < 4) ? xOffset : -xOffset;

        if (octant.elementCount())
            octant.lineTo(pos);
        else // The path is empty. Initialize the start point.
            octant.moveTo(pos);
    }
}

static void drawControlBackground(QPainter* painter, const QPen& pen, const QRect& rect, const QBrush& brush)
{
    QPen oldPen = painter->pen();
    QBrush oldBrush = painter->brush();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(pen);
    painter->setBrush(brush);

    static const qreal line = 1.5;
    const QRectF paddedRect = rect.adjusted(line, line, -line, -line);

    static const int n = 3;
    const qreal invPow = 1 / double(n);
    ASSERT(paddedRect.width() >= paddedRect.height());
    const int radius = paddedRect.height() / 2;
    const int xDelta = paddedRect.width() / 2 - radius;
    const QPointF center = paddedRect.center();
    qreal x = 0;
    qreal y;
    QVector<QPainterPath> octants(8);
    // Stay within reasonable distance from edge values, which can cause artifacts at certain zoom levels.
    static const float epsilon = 0.02;
    for (y = radius - epsilon; y - epsilon > x; y -= 0.5) {
        x = radius * pow(1 - pow(qAbs(y) / radius , n), invPow);
        addPointToOctants(octants, center, x, y, xDelta);
    }

    QPainterPath path = octants.first();
    for (int i = 1; i < 8; ++i) {
        // Due to the orientation of the arcs, we need to reverse the paths with odd indices.
        QPainterPath subPath = (i % 2) ?  octants.at(i).toReversed() : octants.at(i);
        path.connectPath(subPath);
    }
    path.closeSubpath();

    painter->drawPath(path);
    painter->setPen(oldPen);
    painter->setBrush(oldBrush);
}

static inline QRect shrinkRectToSquare(const QRect& rect)
{
    const int side = qMin(rect.height(), rect.width());
    return QRect(rect.topLeft(), QSize(side, side));
}

static inline QPen borderPen(QPainter* painter = 0)
{
    return QPen(darkColor, qMin(1.0, 0.4 * painterScale(painter)));
}

QSharedPointer<StylePainter> RenderThemeQtMobile::getStylePainter(const PaintInfo& pi)
{
    return QSharedPointer<StylePainter>(new StylePainterMobile(this, pi));
}

QPalette RenderThemeQtMobile::colorPalette() const
{
    static QPalette lightGrayPalette(Qt::lightGray);
    return lightGrayPalette;
}

StylePainterMobile::StylePainterMobile(RenderThemeQtMobile* theme, const PaintInfo& paintInfo)
    : StylePainter(paintInfo.context)
{
    m_previousSmoothPixmapTransform = painter->testRenderHint(QPainter::SmoothPixmapTransform);
    if (!m_previousSmoothPixmapTransform)
        painter->setRenderHint(QPainter::SmoothPixmapTransform);
}

StylePainterMobile::~StylePainterMobile()
{
    painter->setRenderHints(QPainter::SmoothPixmapTransform, m_previousSmoothPixmapTransform);
}

bool StylePainterMobile::findCachedControl(const KeyIdentifier& keyId, QPixmap* result)
{
    static CacheKey emptyKey;
    CacheKey key = cacheKeys.value(keyId, emptyKey);
    if (key == emptyKey)
        return false;
    const bool ret = QPixmapCache::find(key, result);
    if (!ret)
        cacheKeys.remove(keyId);
    return ret;
}

void StylePainterMobile::insertIntoCache(const KeyIdentifier& keyId, const QPixmap& pixmap)
{
    ASSERT(keyId.type);
    const int sizeInKiloBytes = pixmap.width() * pixmap.height() * pixmap.depth() / (8 * 1024);
    // Don't cache pixmaps over 512 KB;
    if (sizeInKiloBytes > 512)
        return;
    cacheKeys.insert(keyId, QPixmapCache::insert(pixmap));
}

void StylePainterMobile::drawCheckableBackground(QPainter* painter, const QRect& rect, bool checked, bool enabled) const
{
    QBrush brush;
    QColor color = Qt::gray;
    if (checked && enabled)
        color = highlightColor;

    QLinearGradient gradient;
    gradient.setStart(rect.topLeft());
    gradient.setFinalStop(rect.bottomLeft());
    gradient.setColorAt(0.0, color);
    gradient.setColorAt(1.0, color.lighter(130));
    brush = gradient;

    drawControlBackground(painter, borderPen(painter), rect, brush);
}

QSize StylePainterMobile::sizeForPainterScale(const QRect& rect) const
{
    qreal scale = painterScale(painter);
    QTransform scaleTransform = QTransform::fromScale(scale, scale);

    return scaleTransform.mapRect(rect).size();
}

void StylePainterMobile::drawChecker(QPainter* painter, const QRect& rect, const QColor& color) const
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    QPen pen(Qt::darkGray);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->scale(rect.width(), rect.height());
    QPainterPath path;
    path.moveTo(0.18, 0.47);
    path.lineTo(0.25, 0.4);
    path.lineTo(0.4, 0.55);
    path.quadTo(0.64, 0.29, 0.78, 0.2);
    path.lineTo(0.8, 0.25);
    path.quadTo(0.53, 0.55, 0.45, 0.75);
    path.closeSubpath();
    painter->setBrush(color);
    painter->drawPath(path);
}

QPixmap StylePainterMobile::findCheckBox(const QSize& size, bool checked, bool enabled) const
{
    ASSERT(size.width() == size.height());
    QPixmap result;
    KeyIdentifier id;
    id.type = KeyIdentifier::CheckBox;
    id.height = size.height();
    id.trait1 = enabled;
    id.trait2 = checked;
    if (!findCachedControl(id, &result)) {
        result = QPixmap(size);
        result.fill(Qt::transparent);
        QPainter cachePainter(&result);
        QRect rect(QPoint(0, 0), size);
        drawCheckableBackground(&cachePainter, rect, checked, enabled);
        if (checked || !enabled)
            drawChecker(&cachePainter, rect, enabled ? Qt::white : Qt::gray);
        insertIntoCache(id, result);
    }
    return result;
}

void StylePainterMobile::drawRadio(QPainter* painter, const QSize& size, bool checked, bool enabled) const
{
    QRect rect(QPoint(0, 0), size);

    drawCheckableBackground(painter, rect, checked, enabled);
    const int border = size.width() / 4;
    rect.adjust(border, border, -border, -border);
    drawControlBackground(painter, borderPen(), rect, enabled ? Qt::white : Qt::gray);
}

QPixmap StylePainterMobile::findRadio(const QSize& size, bool checked, bool enabled) const
{
    ASSERT(size.width() == size.height());
    QPixmap result;
    KeyIdentifier id;
    id.type = KeyIdentifier::Radio;
    id.height = size.height();
    id.trait1 = enabled;
    id.trait2 = checked;
    if (!findCachedControl(id, &result)) {
        result = QPixmap(size);
        result.fill(Qt::transparent);
        QPainter cachePainter(&result);
        drawRadio(&cachePainter, size, checked, enabled);
        insertIntoCache(id, result);
    }
    return result;
}

void StylePainterMobile::drawMultipleComboButton(QPainter* painter, const QSizeF& size, const QColor& color) const
{
    const qreal dotDiameter = size.height();
    const qreal dotRadii = dotDiameter / 2;

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(color);
    painter->setBrush(color);

    for (int i = 0; i < 3; ++i) {
        QPointF center(dotRadii + i * multipleComboDotsOffsetFactor * dotDiameter, dotRadii);
        painter->drawEllipse(center, dotRadii, dotRadii);
    }
}

void StylePainterMobile::drawSimpleComboButton(QPainter* painter, const QSizeF& size, const QColor& color) const
{
    const qreal gap = size.height() / 5.0;
    const qreal arrowHeight = (size.height() - gap) / 2.0;
    const qreal right = arrowHeight * 2;
    const qreal bottomBaseline = size.height() - arrowHeight;
    QPolygonF upArrow, downArrow;
    upArrow << QPointF(0, arrowHeight) << QPointF(arrowHeight, 0) << QPointF(right, arrowHeight);
    downArrow << QPointF(0, bottomBaseline) << QPointF(arrowHeight, bottomBaseline + arrowHeight)
              << QPointF(right, bottomBaseline);

    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawPolygon(upArrow);
    painter->drawPolygon(downArrow);
}

QSizeF StylePainterMobile::getButtonImageSize(int buttonHeight, bool multiple) const
{
    if (multiple)
        return QSizeF(qreal(2 + buttonHeight * 3 * multipleComboDotsOffsetFactor/ 10.0)
                      , qreal(2 + buttonHeight / 10.0));

    const qreal height = buttonHeight / 2.5;
    const qreal width = 4 * height / 5.0;
    return QSizeF(2 + width, 2 + height);
}

QPixmap StylePainterMobile::findComboButton(const QSize& size, bool multiple, bool enabled) const
{
    if (size.isNull())
        return QPixmap();
    QPixmap result;
    KeyIdentifier id;
    id.type = KeyIdentifier::ComboButton;
    id.width = size.width();
    id.height = size.height();
    id.trait1 = multiple;
    id.trait2 = enabled;

    if (!findCachedControl(id, &result)) {
        result = QPixmap(size);
        const qreal border = painterScale(painter);
        const QSizeF padding(2 * border, 2 * border);
        const QSizeF innerSize = size - padding;
        ASSERT(innerSize.isValid());
        result.fill(Qt::transparent);
        QPainter cachePainter(&result);
        cachePainter.translate(border, border);
        if (multiple)
            drawMultipleComboButton(&cachePainter, innerSize, enabled ? darkColor : Qt::lightGray);
        else
            drawSimpleComboButton(&cachePainter, innerSize, enabled ? darkColor : Qt::lightGray);
        insertIntoCache(id, result);
    }
    return result;
}

void StylePainterMobile::drawLineEdit(const QRect& rect, bool focused, bool enabled)
{
    Q_UNUSED(enabled);
    QPixmap lineEdit = findLineEdit(sizeForPainterScale(rect), focused);
    if (lineEdit.isNull())
        return;
    painter->drawPixmap(rect, lineEdit);
}

QPixmap StylePainterMobile::findLineEdit(const QSize & size, bool focused) const
{
    QPixmap result;
    KeyIdentifier id;
    id.type = KeyIdentifier::LineEdit;
    id.width = size.width();
    id.height = size.height();
    id.trait1 = focused;

    if (!findCachedControl(id, &result)) {
        const int focusFrame = painterScale(painter);
        result = QPixmap(size);
        result.fill(Qt::transparent);
        const QRect rect = result.rect().adjusted(focusFrame, focusFrame, -focusFrame, -focusFrame);
        QPainter cachePainter(&result);
        drawControlBackground(&cachePainter, borderPen(painter), rect, Qt::white);

        if (focused) {
            QPen focusPen(highlightColor, 1.2 * painterScale(painter), Qt::SolidLine);
            drawControlBackground(&cachePainter, focusPen, rect, Qt::NoBrush);
        }
        insertIntoCache(id, result);
    }
    return result;
}

void StylePainterMobile::drawCheckBox(const QRect& rect, bool checked, bool enabled)
{
    const QRect square = shrinkRectToSquare(rect);
    QPixmap checkBox = findCheckBox(sizeForPainterScale(square), checked, enabled);
    if (checkBox.isNull())
        return;
    painter->drawPixmap(square, checkBox);
}

void StylePainterMobile::drawRadioButton(const QRect& rect, bool checked, bool enabled)
{
    const QRect square = shrinkRectToSquare(rect);
    QPixmap radio = findRadio(sizeForPainterScale(square), checked, enabled);
    if (radio.isNull())
        return;
    painter->drawPixmap(square, radio);
}

void StylePainterMobile::drawPushButton(const QRect& rect, bool sunken, bool enabled)
{
    QPixmap pushButton = findPushButton(sizeForPainterScale(rect), sunken, enabled);
    if (pushButton.isNull())
        return;
    painter->drawPixmap(rect, pushButton);
}

QPixmap StylePainterMobile::findPushButton(const QSize& size, bool sunken, bool enabled) const
{
    QPixmap result;
    KeyIdentifier id;
    id.type = KeyIdentifier::PushButton;
    id.width = size.width();
    id.height = size.height();
    id.trait1 = sunken;
    id.trait2 = enabled;
    if (!findCachedControl(id, &result)) {
        const qreal dropShadowSize = painterScale(painter);
        result = QPixmap(size);
        result.fill(Qt::transparent);
        const QRect rect = QRect(0, 0, size.width(), size.height() - dropShadowSize);
        QPainter cachePainter(&result);
        drawControlBackground(&cachePainter, Qt::NoPen, rect.adjusted(0, dropShadowSize, 0, dropShadowSize), shadowColor);

        QBrush brush;
        if (enabled && !sunken) {
            QLinearGradient linearGradient;
            linearGradient.setStart(rect.bottomLeft());
            linearGradient.setFinalStop(rect.topLeft());
            linearGradient.setColorAt(0.0, buttonGradientBottom);
            linearGradient.setColorAt(1.0, Qt::white);
            brush = linearGradient;
        } else if (!enabled)
            brush = QColor(241, 242, 243);
        else { // sunken
            QLinearGradient linearGradient;
            linearGradient.setStart(rect.bottomLeft());
            linearGradient.setFinalStop(rect.topLeft());
            linearGradient.setColorAt(0.0, highlightColor);
            linearGradient.setColorAt(1.0, highlightColor.lighter());
            brush = linearGradient;
        }
        drawControlBackground(&cachePainter, borderPen(painter), rect, brush);
        insertIntoCache(id, result);
    }
    return result;
}

void StylePainterMobile::drawComboBox(const QRect& rect, bool multiple, bool enabled)
{
    QPixmap pushButton = findPushButton(sizeForPainterScale(rect), /*sunken = */false, enabled);
    if (pushButton.isNull())
        return;
    painter->drawPixmap(rect, pushButton);
    QRectF targetRect(QPointF(0, 0), getButtonImageSize(rect.height() - 1, multiple));
    const QPointF buttonCenter(rect.right() - arrowBoxWidth / 2, rect.top() + (rect.height() - 1) / 2);
    targetRect.moveCenter(buttonCenter);
    QPixmap pic = findComboButton(sizeForPainterScale(targetRect.toRect()), multiple, enabled);
    if (pic.isNull())
        return;

    painter->drawPixmap(targetRect.toRect(), pic);
}

void StylePainterMobile::drawProgress(const QRect& rect, double progress, bool leftToRight, bool animated, bool vertical) const
{
    const int horizontalBorder = (vertical ? rect.width() / 4 : 0);
    const int verticalBorder = (vertical ? 0 : rect.height() / 4);
    const QRect targetRect = rect.adjusted(horizontalBorder, verticalBorder, -horizontalBorder, -verticalBorder);

    QPixmap result;
    QSize imageSize = sizeForPainterScale(targetRect);
    if (vertical)
        qSwap(imageSize.rheight(), imageSize.rwidth());
    KeyIdentifier id;
    id.type = KeyIdentifier::Progress;
    id.width = imageSize.width();
    id.height = imageSize.height();
    id.trait1 = animated;
    id.trait2 = (!animated && !leftToRight);
    id.trait3 = progress * 100;
    if (!findCachedControl(id, &result)) {
        if (imageSize.isNull())
            return;
        result = QPixmap(imageSize);
        result.fill(Qt::transparent);
        QPainter painter(&result);
        painter.setRenderHint(QPainter::Antialiasing);
        QRect progressRect(QPoint(0, 0), imageSize);
        qreal radius = radiusFactor * progressRect.height();
        painter.setBrush(Qt::NoBrush);
        painter.setPen(borderPen());
        progressRect.adjust(1, 1, -1, -1);
        painter.drawRoundedRect(progressRect, radius, radius);
        progressRect.adjust(1, 1, -1, -1);
        if (animated) {
            const int right = progressRect.right();
            const int startPos = right * (1 - progressBarChunkPercentage) * 2 * fabs(progress - 0.5);
            progressRect.setWidth(progressBarChunkPercentage * right);
            progressRect.moveLeft(startPos);
        } else {
            progressRect.setWidth(progress * progressRect.width());
            if (!leftToRight)
                progressRect.moveRight(imageSize.width() - 2);
        }
        if (progressRect.width() > 0) {
            QLinearGradient gradient;
            gradient.setStart(progressRect.bottomLeft());
            gradient.setFinalStop(progressRect.topLeft());
            gradient.setColorAt(0.0, highlightColor);
            gradient.setColorAt(1.0, highlightColor.lighter());
            painter.setBrush(gradient);
            painter.setPen(Qt::NoPen);
            radius = radiusFactor * progressRect.height();
            painter.drawRoundedRect(progressRect, radius, radius);
        }
        insertIntoCache(id, result);
    }
    QTransform transform;
    transform.rotate(-90);
    painter->drawPixmap(targetRect, vertical ? result.transformed(transform) : result);
}

void StylePainterMobile::drawSliderThumb(const QRect & rect, bool pressed) const
{
    QPixmap result;
    const QSize size = sizeForPainterScale(rect);
    KeyIdentifier id;
    id.type = KeyIdentifier::SliderThumb;
    id.width = size.width();
    id.height = size.height();
    id.trait1 = pressed;
    if (!findCachedControl(id, &result)) {
        if (size.isNull())
            return;
        result = QPixmap(size);
        result.fill(Qt::transparent);
        QPainter cachePainter(&result);
        drawControlBackground(&cachePainter, borderPen(painter), QRect(QPoint(0, 0), size), pressed? Qt::lightGray : buttonGradientBottom);
        insertIntoCache(id, result);
    }
    painter->drawPixmap(rect, result);
}


PassRefPtr<RenderTheme> RenderThemeQtMobile::create(Page* page)
{
    return adoptRef(new RenderThemeQtMobile(page));
}

RenderThemeQtMobile::RenderThemeQtMobile(Page* page)
    : RenderThemeQt(page)
{
}

RenderThemeQtMobile::~RenderThemeQtMobile()
{
}

bool RenderThemeQtMobile::isControlStyled(const RenderStyle* style, const BorderData& border, const FillLayer& fill, const Color& backgroundColor) const
{
    switch (style->appearance()) {
    case CheckboxPart:
    case RadioPart:
        return false;
    default:
        return RenderThemeQt::isControlStyled(style, border, fill, backgroundColor);
    }
}

int RenderThemeQtMobile::popupInternalPaddingBottom(RenderStyle* style) const
{
    return 1;
}

void RenderThemeQtMobile::computeSizeBasedOnStyle(RenderStyle* renderStyle) const
{
    QSize size(0, 0);

    switch (renderStyle->appearance()) {
    case TextAreaPart:
    case SearchFieldPart:
    case TextFieldPart: {
        int padding = frameWidth;
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
        const int w = checkBoxWidth * renderStyle->effectiveZoom();
        size = QSize(w, w);
        break;
    }
    case RadioPart: {
        const int w = radioWidth * renderStyle->effectiveZoom();
        size = QSize(w, w);
        break;
    }
    case PushButtonPart:
    case SquareButtonPart:
    case DefaultButtonPart:
    case ButtonPart:
    case MenulistPart: {
        const int height = renderStyle->fontMetrics().height() * buttonHeightRatio * renderStyle->effectiveZoom();
        size = QSize(renderStyle->width().value(), height);
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

void RenderThemeQtMobile::adjustButtonStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    // Ditch the border.
    style->resetBorder();

    FontDescription fontDescription = style->fontDescription();
    fontDescription.setIsAbsoluteSize(true);

    fontDescription.setSpecifiedSize(style->fontSize());
    fontDescription.setComputedSize(style->fontSize());

    style->setLineHeight(RenderStyle::initialLineHeight());
    setButtonSize(style);
    setButtonPadding(style);
}

void RenderThemeQtMobile::setButtonPadding(RenderStyle* style) const
{
    if (!style)
        return;
    style->setPaddingLeft(Length(buttonPaddingLeft, Fixed));
    style->setPaddingRight(Length(buttonPaddingRight, Fixed));
    style->setPaddingTop(Length(buttonPaddingTop, Fixed));
    style->setPaddingBottom(Length(buttonPaddingBottom, Fixed));
}

bool RenderThemeQtMobile::paintButton(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    StylePainterMobile p(this, i);
    if (!p.isValid())
       return true;

    ControlPart appearance = o->style()->appearance();
    if (appearance == PushButtonPart || appearance == ButtonPart) {
        p.drawPushButton(r, isPressed(o), isEnabled(o));
    } else if (appearance == RadioPart)
       p.drawRadioButton(r, isChecked(o), isEnabled(o));
    else if (appearance == CheckboxPart)
       p.drawCheckBox(r, isChecked(o), isEnabled(o));

    return false;
}

void RenderThemeQtMobile::adjustTextFieldStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    // Resetting the style like this leads to differences like:
    // - RenderTextControl {INPUT} at (2,2) size 168x25 [bgcolor=#FFFFFF] border: (2px inset #000000)]
    // + RenderTextControl {INPUT} at (2,2) size 166x26
    // in layout tests when a CSS style is applied that doesn't affect background color, border or
    // padding. Just worth keeping in mind!
    style->setBackgroundColor(Color::transparent);
    style->resetBorder();
    style->setBorderTopWidth(frameWidth);
    style->setBorderRightWidth(frameWidth);
    style->setBorderBottomWidth(frameWidth);
    style->setBorderLeftWidth(frameWidth);
    style->resetPadding();
    computeSizeBasedOnStyle(style);
    style->setPaddingLeft(Length(textFieldPadding, Fixed));
    style->setPaddingRight(Length(textFieldPadding, Fixed));
}

bool RenderThemeQtMobile::paintTextField(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    StylePainterMobile p(this, i);
    if (!p.isValid())
        return true;

    ControlPart appearance = o->style()->appearance();
    if (appearance != TextFieldPart
        && appearance != SearchFieldPart
        && appearance != TextAreaPart)
        return true;

    // Now paint the text field.
    if (appearance == TextAreaPart) {
        const bool previousAntialiasing = p.painter->testRenderHint(QPainter::Antialiasing);
        p.painter->setRenderHint(QPainter::Antialiasing);
        p.painter->setPen(borderPen());
        p.painter->setBrush(Qt::white);
        const int radius = checkBoxWidth * radiusFactor;
        p.painter->drawRoundedRect(r, radius, radius);

        if (isFocused(o)) {
            QPen focusPen(highlightColor, 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            p.painter->setPen(focusPen);
            p.painter->setBrush(Qt::NoBrush);
            p.painter->drawRoundedRect(r, radius, radius);
        }
        p.painter->setRenderHint(QPainter::Antialiasing, previousAntialiasing);
    } else
        p.drawLineEdit(r, isFocused(o), isEnabled(o));
    return false;
}

void RenderThemeQtMobile::adjustMenuListStyle(StyleResolver* styleResolver, RenderStyle* style, Element* e) const
{
    RenderThemeQt::adjustMenuListStyle(styleResolver, style, e);
    style->setPaddingLeft(Length(menuListPadding, Fixed));
}

void RenderThemeQtMobile::setPopupPadding(RenderStyle* style) const
{
    const int paddingLeft = 4;
    const int paddingRight = style->width().isFixed() || style->width().isPercent() ? 5 : 8;

    style->setPaddingLeft(Length(paddingLeft, Fixed));
    style->setPaddingRight(Length(paddingRight + arrowBoxWidth, Fixed));

    style->setPaddingTop(Length(2, Fixed));
    style->setPaddingBottom(Length(2, Fixed));
}

bool RenderThemeQtMobile::paintMenuList(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    StylePainterMobile p(this, i);
    if (!p.isValid())
        return true;

    p.drawComboBox(r, checkMultiple(o), isEnabled(o));
    return false;
}

bool RenderThemeQtMobile::paintMenuListButton(RenderObject* o, const PaintInfo& i,
                                        const IntRect& r)
{
    StylePainterMobile p(this, i);
    if (!p.isValid())
        return true;

    p.drawComboBox(r, checkMultiple(o), isEnabled(o));

    return false;
}

#if ENABLE(PROGRESS_ELEMENT)
double RenderThemeQtMobile::animationDurationForProgressBar(RenderProgress* renderProgress) const
{
    if (renderProgress->isDeterminate())
        return 0;
    // Our animation goes back and forth so we need to make it last twice as long
    // and we need the numerator to be an odd number to ensure we get a progress value of 0.5.
    return (2 * progressAnimationGranularity +1) / progressBarChunkPercentage * animationRepeatIntervalForProgressBar(renderProgress);
}

bool RenderThemeQtMobile::paintProgressBar(RenderObject* o, const PaintInfo& pi, const IntRect& r)
{
    if (!o->isProgress())
        return true;

    StylePainterMobile p(this, pi);
    if (!p.isValid())
        return true;

    RenderProgress* renderProgress = toRenderProgress(o);
    const bool isRTL = (renderProgress->style()->direction() == RTL);

    if (renderProgress->isDeterminate())
        p.drawProgress(r, renderProgress->position(), !isRTL);
    else
        p.drawProgress(r, renderProgress->animationProgress(), !isRTL, true);

    return false;
}
#endif

bool RenderThemeQtMobile::paintSliderTrack(RenderObject* o, const PaintInfo& pi,
                                     const IntRect& r)
{
    StylePainterMobile p(this, pi);
    if (!p.isValid())
        return true;

    HTMLInputElement* slider = toHTMLInputElement(o->node());

    const double min = slider->minimum();
    const double max = slider->maximum();
    const double progress = (max - min > 0) ? (slider->valueAsNumber() - min) / (max - min) : 0;

    QRect rect(r);
    const bool vertical = (o->style()->appearance() == SliderVerticalPart);
    const int groovePadding = vertical ? r.width() * sliderGrooveBorderRatio : r.height() * sliderGrooveBorderRatio;
    if (vertical) {
        rect.adjust(groovePadding, 0, -groovePadding, 0);
        // Direction is ignored on vertical sliders and we assume LTR.
        p.drawProgress(rect, progress, true, /*animated = */ false, vertical);
    } else {
        rect.adjust(0, groovePadding, 0, -groovePadding);
        p.drawProgress(rect, progress, o->style()->isLeftToRightDirection(), /*animated = */ false, vertical);
    }

    return false;
}

bool RenderThemeQtMobile::paintSliderThumb(RenderObject* o, const PaintInfo& pi,
                                     const IntRect& r)
{
    StylePainterMobile p(this, pi);
    if (!p.isValid())
        return true;

    p.drawSliderThumb(r, isPressed(o));

    return false;
}

bool RenderThemeQtMobile::checkMultiple(RenderObject* o) const
{
    HTMLSelectElement* select = o ? toHTMLSelectElement(o->node()) : 0;
    return select ? select->multiple() : false;
}

void RenderThemeQtMobile::adjustSliderThumbSize(RenderStyle* style, Element* element) const
{
    const ControlPart part = style->appearance();
    if (part == SliderThumbHorizontalPart || part == SliderThumbVerticalPart) {
        const int size = sliderSize * style->effectiveZoom();
        style->setWidth(Length(size, Fixed));
        style->setHeight(Length(size, Fixed));
    } else
        RenderThemeQt::adjustSliderThumbSize(style, element);
}

}

// vim: ts=4 sw=4 et
