/*
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 George Staikos <staikos@kde.org>
 * Copyright (C) 2006 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2006 Allan Sandfeld Jensen <sandfeld@kde.org>
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2008 Dirk Schulze <vbs85@gmx.de>
 * Copyright (C) 2010, 2011 Sencha, Inc.
 * Copyright (C) 2011 Andreas Kling <kling@webkit.org>
 * Copyright (C) 2013 Digia Plc. and/or its subsidiary(-ies).
 *
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "GraphicsContext.h"

#if OS(WINDOWS)
#include <windows.h>
#endif

#include "AffineTransform.h"
#include "Color.h"
#include "FloatConversion.h"
#include "Font.h"
#include "ImageBuffer.h"
#include "NotImplemented.h"
#include "Path.h"
#include "Pattern.h"
#include "ShadowBlur.h"
#include "TransformationMatrix.h"
#include "TransparencyLayer.h"

#include <QBrush>
#include <QGradient>
#include <QPaintDevice>
#include <QPaintEngine>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPixmap>
#include <QPolygonF>
#include <QStack>
#include <QVector>
#include <wtf/MathExtras.h>

#if OS(WINDOWS)
QT_BEGIN_NAMESPACE
Q_GUI_EXPORT QPixmap qt_pixmapFromWinHBITMAP(HBITMAP, int hbitmapFormat = 0);
QT_END_NAMESPACE

enum HBitmapFormat {
    HBitmapNoAlpha,
    HBitmapPremultipliedAlpha,
    HBitmapAlpha
};
#endif

namespace WebCore {

static inline QPainter::CompositionMode toQtCompositionMode(CompositeOperator op)
{
    switch (op) {
    case CompositeClear:
        return QPainter::CompositionMode_Clear;
    case CompositeCopy:
        return QPainter::CompositionMode_Source;
    case CompositeSourceOver:
        return QPainter::CompositionMode_SourceOver;
    case CompositeSourceIn:
        return QPainter::CompositionMode_SourceIn;
    case CompositeSourceOut:
        return QPainter::CompositionMode_SourceOut;
    case CompositeSourceAtop:
        return QPainter::CompositionMode_SourceAtop;
    case CompositeDestinationOver:
        return QPainter::CompositionMode_DestinationOver;
    case CompositeDestinationIn:
        return QPainter::CompositionMode_DestinationIn;
    case CompositeDestinationOut:
        return QPainter::CompositionMode_DestinationOut;
    case CompositeDestinationAtop:
        return QPainter::CompositionMode_DestinationAtop;
    case CompositeXOR:
        return QPainter::CompositionMode_Xor;
    case CompositePlusDarker:
        // there is no exact match, but this is the closest
        return QPainter::CompositionMode_Darken;
    case CompositePlusLighter:
        return QPainter::CompositionMode_Plus;
    case CompositeDifference:
        return QPainter::CompositionMode_Difference;
    default:
        ASSERT_NOT_REACHED();
    }

    return QPainter::CompositionMode_SourceOver;
}

static inline QPainter::CompositionMode toQtCompositionMode(BlendMode op)
{
    switch (op) {
    case BlendModeNormal:
        return QPainter::CompositionMode_SourceOver;
    case BlendModeMultiply:
        return QPainter::CompositionMode_Multiply;
    case BlendModeScreen:
        return QPainter::CompositionMode_Screen;
    case BlendModeOverlay:
        return QPainter::CompositionMode_Overlay;
    case BlendModeDarken:
        return QPainter::CompositionMode_Darken;
    case BlendModeLighten:
        return QPainter::CompositionMode_Lighten;
    case BlendModeColorDodge:
        return QPainter::CompositionMode_ColorDodge;
    case BlendModeColorBurn:
        return QPainter::CompositionMode_ColorBurn;
    case BlendModeHardLight:
        return QPainter::CompositionMode_HardLight;
    case BlendModeSoftLight:
        return QPainter::CompositionMode_SoftLight;
    case BlendModeDifference:
        return QPainter::CompositionMode_Difference;
    case BlendModeExclusion:
        return QPainter::CompositionMode_Exclusion;
    case BlendModeHue:
    case BlendModeSaturation:
    case BlendModeColor:
    case BlendModeLuminosity:
        // Not supported.
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    return QPainter::CompositionMode_SourceOver;
}

static inline Qt::PenCapStyle toQtLineCap(LineCap lc)
{
    switch (lc) {
    case ButtCap:
        return Qt::FlatCap;
    case RoundCap:
        return Qt::RoundCap;
    case SquareCap:
        return Qt::SquareCap;
    default:
        ASSERT_NOT_REACHED();
    }

    return Qt::FlatCap;
}

static inline Qt::PenJoinStyle toQtLineJoin(LineJoin lj)
{
    switch (lj) {
    case MiterJoin:
        return Qt::SvgMiterJoin;
    case RoundJoin:
        return Qt::RoundJoin;
    case BevelJoin:
        return Qt::BevelJoin;
    default:
        ASSERT_NOT_REACHED();
    }

    return Qt::SvgMiterJoin;
}

static Qt::PenStyle toQPenStyle(StrokeStyle style)
{
    switch (style) {
    case NoStroke:
        return Qt::NoPen;
        break;
    case SolidStroke:
#if ENABLE(CSS3_TEXT)
    case DoubleStroke:
    case WavyStroke:
#endif
        return Qt::SolidLine;
        break;
    case DottedStroke:
        return Qt::DotLine;
        break;
    case DashedStroke:
        return Qt::DashLine;
        break;
    default:
        ASSERT_NOT_REACHED();
    }
    return Qt::NoPen;
}

static inline Qt::FillRule toQtFillRule(WindRule rule)
{
    switch (rule) {
    case RULE_EVENODD:
        return Qt::OddEvenFill;
    case RULE_NONZERO:
        return Qt::WindingFill;
    default:
        ASSERT_NOT_REACHED();
    }
    return Qt::OddEvenFill;
}

static inline void adjustPointsForDottedLine(FloatPoint& p1, FloatPoint& p2, float width, bool isVerticalLine)
{
    if (isVerticalLine) {
        p1.setY(p1.y() - width / 2);
        p2.setY(p2.y() + width / 2);
    } else {
        p1.setX(p1.x() - width / 2);
        p2.setX(p2.x() + width / 2);
    }
}

static inline void drawLineEndpointsForStyle(QPainter *painter, const FloatPoint& p1, const FloatPoint& p2, float width, bool isVerticalLine, StrokeStyle style, Color color)
{
    // Do a rect fill of our endpoints. This ensures we always have the
    // appearance of being a border.
    if (style == DashedStroke) {
        if (isVerticalLine) {
            painter->fillRect(FloatRect(p1.x() - width / 2, p1.y() - width, width, width), QColor(color));
            painter->fillRect(FloatRect(p2.x() - width / 2, p2.y(), width, width), QColor(color));
        } else {
            painter->fillRect(FloatRect(p1.x() - width, p1.y() - width / 2, width, width), QColor(color));
            painter->fillRect(FloatRect(p2.x(), p2.y() - width / 2, width, width), QColor(color));
        }
    }

    // As per css spec a dotted stroke should be made of circles so we're
    // drawing circles as endpoints.
    if (style == DottedStroke) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(color));
        painter->drawEllipse(p1.x() - width / 2, p1.y() - width / 2, width, width);
        painter->drawEllipse(p2.x() - width / 2, p2.y() - width / 2, width, width);
    }
}

class GraphicsContextPlatformPrivate {
    WTF_MAKE_NONCOPYABLE(GraphicsContextPlatformPrivate); WTF_MAKE_FAST_ALLOCATED;
public:
    GraphicsContextPlatformPrivate(QPainter*, const QColor& initialSolidColor);
    ~GraphicsContextPlatformPrivate();

    inline QPainter* p() const
    {
        if (layers.isEmpty())
            return painter;
        return &layers.top()->painter;
    }

    bool antiAliasingForRectsAndLines;

    QStack<TransparencyLayer*> layers;
    // Counting real layers. Required by isInTransparencyLayer() calls
    // For example, layers with valid alphaMask are not real layers
    int layerCount;

    // reuse this brush for solid color (to prevent expensive QBrush construction)
    QBrush solidColor;

    InterpolationQuality imageInterpolationQuality;
    bool initialSmoothPixmapTransformHint;

    QRectF clipBoundingRect() const
    {
        return p()->clipBoundingRect();
    }

    void takeOwnershipOfPlatformContext() { platformContextIsOwned = true; }

private:
    QPainter* painter;
    bool platformContextIsOwned;
};

GraphicsContextPlatformPrivate::GraphicsContextPlatformPrivate(QPainter* p, const QColor& initialSolidColor)
    : antiAliasingForRectsAndLines(false)
    , layerCount(0)
    , solidColor(initialSolidColor)
    , imageInterpolationQuality(InterpolationDefault)
    , initialSmoothPixmapTransformHint(false)
    , painter(p)
    , platformContextIsOwned(false)
{
    if (!painter)
        return;

    // Use the default the QPainter was constructed with.
    antiAliasingForRectsAndLines = painter->testRenderHint(QPainter::Antialiasing);

    // Used for default image interpolation quality.
    initialSmoothPixmapTransformHint = painter->testRenderHint(QPainter::SmoothPixmapTransform);

    painter->setRenderHint(QPainter::Antialiasing, true);

}

GraphicsContextPlatformPrivate::~GraphicsContextPlatformPrivate()
{
    if (!platformContextIsOwned)
        return;

    QPaintDevice* device = painter->device();
    painter->end();
    delete painter;
    delete device;
}

void GraphicsContext::platformInit(PlatformGraphicsContext* painter)
{
    m_data = new GraphicsContextPlatformPrivate(painter, fillColor());

    setPaintingDisabled(!painter);

    if (!painter)
        return;

    // solidColor is initialized with the fillColor().
    painter->setBrush(m_data->solidColor);

    QPen pen(painter->pen());
    pen.setColor(strokeColor());
    pen.setJoinStyle(toQtLineJoin(MiterJoin));
    painter->setPen(pen);
}

void GraphicsContext::platformDestroy()
{
    while (!m_data->layers.isEmpty())
        endTransparencyLayer();

    delete m_data;
}

PlatformGraphicsContext* GraphicsContext::platformContext() const
{
    return m_data->p();
}

AffineTransform GraphicsContext::getCTM(IncludeDeviceScale includeScale) const
{
    if (paintingDisabled())
        return AffineTransform();

    const QTransform& matrix = (includeScale == DefinitelyIncludeDeviceScale)
        ? platformContext()->combinedTransform()
        : platformContext()->worldTransform();
    return AffineTransform(matrix.m11(), matrix.m12(), matrix.m21(),
                           matrix.m22(), matrix.dx(), matrix.dy());
}

void GraphicsContext::savePlatformState()
{
    if (!m_data->layers.isEmpty() && !m_data->layers.top()->alphaMask.isNull())
        ++m_data->layers.top()->saveCounter;
    m_data->p()->save();
}

void GraphicsContext::restorePlatformState()
{
    if (!m_data->layers.isEmpty() && !m_data->layers.top()->alphaMask.isNull())
        if (!--m_data->layers.top()->saveCounter)
            endPlatformTransparencyLayer();

    m_data->p()->restore();
}

// Draws a filled rectangle with a stroked border.
// This is only used to draw borders (real fill is done via fillRect), and
// thus it must not cast any shadow.
void GraphicsContext::drawRect(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    ASSERT(!rect.isEmpty());

    QPainter* p = m_data->p();
    const bool antiAlias = p->testRenderHint(QPainter::Antialiasing);
    p->setRenderHint(QPainter::Antialiasing, m_data->antiAliasingForRectsAndLines);

    p->drawRect(rect);

    p->setRenderHint(QPainter::Antialiasing, antiAlias);
}

// This is only used to draw borders.
// Must not cast any shadow.
void GraphicsContext::drawLine(const IntPoint& point1, const IntPoint& point2)
{
    if (paintingDisabled())
        return;

    StrokeStyle style = strokeStyle();
    Color color = strokeColor();
    if (style == NoStroke)
        return;

    float width = strokeThickness();

    FloatPoint p1 = point1;
    FloatPoint p2 = point2;
    bool isVerticalLine = (p1.x() == p2.x());

    QPainter* p = m_data->p();
    const bool antiAlias = p->testRenderHint(QPainter::Antialiasing);
    p->setRenderHint(QPainter::Antialiasing, m_data->antiAliasingForRectsAndLines);
    adjustLineToPixelBoundaries(p1, p2, width, style);

    Qt::PenCapStyle capStyle = Qt::FlatCap;
    QVector<qreal> dashes;
    int patWidth = 0;

    switch (style) {
    case NoStroke:
    case SolidStroke:
#if ENABLE(CSS3_TEXT)
    case DoubleStroke:
    case WavyStroke:
#endif
        break;
    case DottedStroke: {
        capStyle = Qt::RoundCap;
        patWidth = static_cast<int>(width);
        // The actual length of one line element can not be set to zero and at 0.1 the dots
        // are still slightly elongated. Setting it to 0.01 will make it look like the
        // line endings are being stuck together, close enough to look like a circle.
        // For the distance of the line elements we subtract the small amount again.
        const qreal lineElementLength = 0.01;
        dashes << lineElementLength << qreal(2 * patWidth) / width - lineElementLength;
        adjustPointsForDottedLine(p1, p2, width, isVerticalLine);
        break;
    }
    case DashedStroke:
        capStyle = Qt::FlatCap;
        patWidth = 3 * static_cast<int>(width);
        dashes << qreal(patWidth) / width << qreal(patWidth) / width;
        break;
    }

    if (patWidth) {
        p->save();

        QPen pen = p->pen();

        drawLineEndpointsForStyle(p, p1, p2, width, isVerticalLine, style, color);

        // Example: 80 pixels with a width of 30 pixels.
        // Remainder is 20.  The maximum pixels of line we could paint
        // will be 50 pixels.
        int distance = (isVerticalLine ? (point2.y() - point1.y()) : (point2.x() - point1.x())) - 2*(int)width;
        int remainder = distance % patWidth;
        int coverage = distance - remainder;
        int numSegments = coverage / patWidth;

        float patternOffset = 0.0f;
        // Special case 1px dotted borders for speed.
        if (patWidth == 1)
            patternOffset = 1.0f;
        else {
            bool evenNumberOfSegments = !(numSegments % 2);
            if (remainder)
                evenNumberOfSegments = !evenNumberOfSegments;
            if (evenNumberOfSegments) {
                if (remainder) {
                    patternOffset += patWidth - remainder;
                    patternOffset += remainder / 2;
                } else
                    patternOffset = patWidth / 2;
            } else {
                if (remainder)
                    patternOffset = (patWidth - remainder) / 2;
            }
        }

        pen.setWidthF(width);
        pen.setCapStyle(capStyle);
        pen.setDashPattern(dashes);
        pen.setDashOffset(patternOffset / width);
        p->setPen(pen);
    }

#if ENABLE(CSS3_TEXT)
    if (style == WavyStroke) {
        const float step = 2 * width; // Make wave height equal to two times strokeThickness().
        const float flat = width; // Set size of flat lines between diagonal lines.
        short signal = -1;
        QPainterPath path;
        float x1, y1, x2, y2;

        if (isVerticalLine) {
            x1 = x2 = p1.x();

            // Make sure (x1, y1) < (x2, y2)
            if (p1.y() < p2.y()) {
                y1 = p1.y();
                y2 = p2.y();
            } else {
                y1 = p2.y();
                y2 = p1.y();
            }

            // Qt interprets geometric units as end-point inclusive, while WebCore interprets geometric units as endpoint exclusive.
            // This means we need to subtract one from the endpoint, or the line will be painted one pixel too long.
            y2 -= 1;
            path.moveTo(x1 + signal * step, y1);
            float y = y1 + 2 * step;

            while (y <= y2) {
                signal = -signal;
                path.lineTo(x1 + signal * step, y);
                path.lineTo(x1 + signal * step, y + flat); // Draw flat line between diagonal lines.
                y += 2 * step + flat;
            }
        } else {
            y1 = y2 = p1.y();

            // Make sure (x1, y1) < (x2, y2)
            if (p1.x() < p2.x()) {
                x1 = p1.x();
                x2 = p2.x();
            } else {
                x1 = p2.x();
                x2 = p1.x();
            }

            // Qt interprets geometric units as end-point inclusive, while WebCore interprets geometric units as endpoint exclusive.
            // This means we need to subtract one from the endpoint, or the line will be painted one pixel too long.
            x2 -= 1;
            path.moveTo(x1, y1 + signal * step);
            float x = x1 + 2 * step;

            while (x <= x2) {
                signal = -signal;
                path.lineTo(x, y1 + signal * step);
                path.lineTo(x + flat, y1 + signal * step); // Draw flat line between diagonal lines.
                x += 2 * step + flat;
            }
        }

        // The last point created by the while loops above may not be the end
        // point, so complete the wave by connecting the end point.
        path.lineTo(x2, y2);
        QPen pen = p->pen();
        pen.setJoinStyle(Qt::BevelJoin); // A bevelled line join is more suitable for wavy than miter or round.
        pen.setWidth(width);
        const bool oldAntiAliasing = p->testRenderHint(QPainter::Antialiasing);
        p->setRenderHint(QPainter::Antialiasing, true); // AntiAliasing is needed for diagonal lines of wavy stroke
        p->strokePath(path, pen);
        p->setRenderHint(QPainter::Antialiasing, oldAntiAliasing);
    } else {
#endif // CSS3_TEXT
    // Qt interprets geometric units as end-point inclusive, while WebCore interprets geomtric units as endpoint exclusive.
    // This means we need to subtract one from the endpoint, or the line will be painted one pixel too long.
    if (p1.x() == p2.x())
        p->drawLine(p1, p2 - FloatSize(0, 1));
    else
        p->drawLine(p1, p2 - FloatSize(1, 0));
#if ENABLE(CSS3_TEXT)
    }
#endif // CSS3_TEXT

    if (patWidth)
        p->restore();

    p->setRenderHint(QPainter::Antialiasing, antiAlias);
}

// This method is only used to draw the little circles used in lists.
void GraphicsContext::drawEllipse(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    m_data->p()->drawEllipse(rect);
}

void GraphicsContext::drawConvexPolygon(size_t npoints, const FloatPoint* points, bool shouldAntialias)
{
    if (paintingDisabled())
        return;

    if (npoints <= 1)
        return;

    QPolygonF polygon(npoints);

    for (size_t i = 0; i < npoints; i++)
        polygon[i] = points[i];

    QPainter* p = m_data->p();

    const bool antiAlias = p->testRenderHint(QPainter::Antialiasing);
    p->setRenderHint(QPainter::Antialiasing, shouldAntialias);

    p->drawConvexPolygon(polygon);

    p->setRenderHint(QPainter::Antialiasing, antiAlias);
}

void GraphicsContext::clipConvexPolygon(size_t numPoints, const FloatPoint* points, bool antialiased)
{
    if (paintingDisabled())
        return;

    if (numPoints <= 1)
        return;

    QPainterPath path(points[0]);
    for (size_t i = 1; i < numPoints; ++i)
        path.lineTo(points[i]);
    path.setFillRule(Qt::WindingFill);

    QPainter* p = m_data->p();

    bool painterWasAntialiased = p->testRenderHint(QPainter::Antialiasing);

    if (painterWasAntialiased != antialiased)
        p->setRenderHint(QPainter::Antialiasing, antialiased);

    p->setClipPath(path, Qt::IntersectClip);

    if (painterWasAntialiased != antialiased)
        p->setRenderHint(QPainter::Antialiasing, painterWasAntialiased);
}

void GraphicsContext::fillPath(const Path& path)
{
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    QPainterPath platformPath = path.platformPath();
    platformPath.setFillRule(toQtFillRule(fillRule()));

    if (hasShadow()) {
        if (mustUseShadowBlur() || m_state.fillPattern || m_state.fillGradient)
        {
            ShadowBlur shadow(m_state);
            GraphicsContext* shadowContext = shadow.beginShadowLayer(this, platformPath.controlPointRect());
            if (shadowContext) {
                QPainter* shadowPainter = shadowContext->platformContext();
                if (m_state.fillPattern) {
                    shadowPainter->fillPath(platformPath, QBrush(m_state.fillPattern->createPlatformPattern()));
                } else if (m_state.fillGradient) {
                    QBrush brush(*m_state.fillGradient->platformGradient());
                    brush.setTransform(m_state.fillGradient->gradientSpaceTransform());
                    shadowPainter->fillPath(platformPath, brush);
                } else {
                    shadowPainter->fillPath(platformPath, p->brush());
                }
                shadow.endShadowLayer(this);
            }
        } else {
            QPointF offset(m_state.shadowOffset.width(), m_state.shadowOffset.height());
            p->translate(offset);
            QColor shadowColor = m_state.shadowColor;
            shadowColor.setAlphaF(shadowColor.alphaF() * p->brush().color().alphaF());
            p->fillPath(platformPath, shadowColor);
            p->translate(-offset);
        }
    }
    if (m_state.fillPattern) {
        p->fillPath(platformPath, QBrush(m_state.fillPattern->createPlatformPattern()));
    } else if (m_state.fillGradient) {
        QBrush brush(*m_state.fillGradient->platformGradient());
        brush.setTransform(m_state.fillGradient->gradientSpaceTransform());
        p->fillPath(platformPath, brush);
    } else
        p->fillPath(platformPath, p->brush());
}

inline static void fillPathStroke(QPainter* painter, const QPainterPath& platformPath, const QPen& pen)
{
    if (pen.color().alphaF() < 1.0) {
        QPainterPathStroker pathStroker;
        pathStroker.setJoinStyle(pen.joinStyle());
        pathStroker.setDashOffset(pen.dashOffset());
        pathStroker.setDashPattern(pen.dashPattern());
        pathStroker.setMiterLimit(pen.miterLimit());
        pathStroker.setCapStyle(pen.capStyle());
        pathStroker.setWidth(pen.widthF());

        QPainterPath stroke = pathStroker.createStroke(platformPath);
        painter->fillPath(stroke, pen.brush());
    } else {
        painter->strokePath(platformPath, pen);
    }
}

void GraphicsContext::strokePath(const Path& path)
{
    if (paintingDisabled())
        return;
    QPainter* p = m_data->p();
    QPen pen(p->pen());
    QPainterPath platformPath = path.platformPath();
    platformPath.setFillRule(toQtFillRule(fillRule()));

    if (hasShadow()) {
        if (mustUseShadowBlur() || m_state.strokePattern || m_state.strokeGradient)
        {
            ShadowBlur shadow(m_state);
            FloatRect boundingRect = platformPath.controlPointRect();
            boundingRect.inflate(pen.miterLimit() + pen.widthF());
            GraphicsContext* shadowContext = shadow.beginShadowLayer(this, boundingRect);
            if (shadowContext) {
                QPainter* shadowPainter = shadowContext->platformContext();
                if (m_state.strokeGradient) {
                    QBrush brush(*m_state.strokeGradient->platformGradient());
                    brush.setTransform(m_state.strokeGradient->gradientSpaceTransform());
                    QPen shadowPen(pen);
                    shadowPen.setBrush(brush);
                    fillPathStroke(shadowPainter, platformPath, shadowPen);
                } else {
                    fillPathStroke(shadowPainter, platformPath, pen);
                }
                shadow.endShadowLayer(this);
            }
        } else {
            QPointF offset(m_state.shadowOffset.width(), m_state.shadowOffset.height());
            p->translate(offset);
            QColor shadowColor = m_state.shadowColor;
            shadowColor.setAlphaF(shadowColor.alphaF() * pen.color().alphaF());
            QPen shadowPen(pen);
            shadowPen.setColor(shadowColor);
            fillPathStroke(p, platformPath, shadowPen);
            p->translate(-offset);
        }
    }

    if (m_state.strokePattern) {
        QBrush brush = m_state.strokePattern->createPlatformPattern();
        pen.setBrush(brush);
        fillPathStroke(p, platformPath, pen);
    } else if (m_state.strokeGradient) {
        QBrush brush(*m_state.strokeGradient->platformGradient());
        brush.setTransform(m_state.strokeGradient->gradientSpaceTransform());
        pen.setBrush(brush);
        fillPathStroke(p, platformPath, pen);
    } else
        fillPathStroke(p, platformPath, pen);
}

static inline void drawRepeatPattern(QPainter* p, PassRefPtr<Pattern> pattern, const FloatRect& rect)
{
    ASSERT(pattern);

    const QBrush brush = pattern->createPlatformPattern();
    if (brush.style() != Qt::TexturePattern)
        return;

    const bool repeatX = pattern->repeatX();
    const bool repeatY = pattern->repeatY();
    // Patterns must be painted so that the top left of the first image is anchored at
    // the origin of the coordinate space

    QRectF targetRect(rect);
    const int w = brush.texture().width();
    const int h = brush.texture().height();

    ASSERT(p);
    QRegion oldClip;
    if (p->hasClipping())
        oldClip = p->clipRegion();

    // The only type of transforms supported for the brush are translations.
    ASSERT(!brush.transform().isRotating());

    QRectF clip = targetRect;
    QRectF patternRect = brush.transform().mapRect(QRectF(0, 0, w, h));
    if (!repeatX) {
        clip.setLeft(patternRect.left());
        clip.setWidth(patternRect.width());
    }
    if (!repeatY) {
        clip.setTop(patternRect.top());
        clip.setHeight(patternRect.height());
    }
    if (!repeatX || !repeatY)
        p->setClipRect(clip);

    p->fillRect(targetRect, brush);

    if (!oldClip.isEmpty())
        p->setClipRegion(oldClip);
    else if (!repeatX || !repeatY)
        p->setClipping(false);
}

void GraphicsContext::fillRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    QRectF normalizedRect = rect.normalized();

    if (m_state.fillPattern) {
        if (hasShadow()) {
            ShadowBlur shadow(m_state);
            GraphicsContext* shadowContext = shadow.beginShadowLayer(this, normalizedRect);
            if (shadowContext) {
                QPainter* shadowPainter = shadowContext->platformContext();
                drawRepeatPattern(shadowPainter, m_state.fillPattern, normalizedRect);
                shadow.endShadowLayer(this);
            }
        }
        drawRepeatPattern(p, m_state.fillPattern, normalizedRect);
    } else if (m_state.fillGradient) {
        QBrush brush(*m_state.fillGradient->platformGradient());
        brush.setTransform(m_state.fillGradient->gradientSpaceTransform());
        if (hasShadow()) {
            ShadowBlur shadow(m_state);
            GraphicsContext* shadowContext = shadow.beginShadowLayer(this, normalizedRect);
            if (shadowContext) {
                QPainter* shadowPainter = shadowContext->platformContext();
                shadowPainter->fillRect(normalizedRect, brush);
                shadow.endShadowLayer(this);
            }
        }
        p->fillRect(normalizedRect, brush);
    } else {
        if (hasShadow()) {
            if (mustUseShadowBlur()) {
                ShadowBlur shadow(m_state);
                // drawRectShadowWithTiling does not work with rotations, and the fallback of
                // drawing though clipToImageBuffer() produces scaling artifacts for us.
                if (!getCTM().preservesAxisAlignment()) {
                    GraphicsContext* shadowContext = shadow.beginShadowLayer(this, normalizedRect);
                    if (shadowContext) {
                        QPainter* shadowPainter = shadowContext->platformContext();
                        shadowPainter->fillRect(normalizedRect, p->brush());
                        shadow.endShadowLayer(this);
                    }
                } else
                    shadow.drawRectShadow(this, rect, RoundedRect::Radii());
            } else {
                // Solid rectangle fill with no blur shadow or transformations applied can be done
                // faster without using the shadow layer at all.
                QColor shadowColor = m_state.shadowColor;
                shadowColor.setAlphaF(shadowColor.alphaF() * p->brush().color().alphaF());
                p->fillRect(normalizedRect.translated(QPointF(m_state.shadowOffset.width(), m_state.shadowOffset.height())), shadowColor);
            }
        }

        p->fillRect(normalizedRect, p->brush());
    }
}


void GraphicsContext::fillRect(const FloatRect& rect, const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled() || !color.isValid())
        return;

    QRectF platformRect(rect);
    QPainter* p = m_data->p();
    if (hasShadow()) {
        if (mustUseShadowBlur()) {
            ShadowBlur shadow(m_state);
            shadow.drawRectShadow(this, platformRect, RoundedRect::Radii());
        } else {
            QColor shadowColor = m_state.shadowColor;
            shadowColor.setAlphaF(shadowColor.alphaF() * p->brush().color().alphaF());
            p->fillRect(platformRect.translated(QPointF(m_state.shadowOffset.width(), m_state.shadowOffset.height())), shadowColor);
        }
    }
    p->fillRect(platformRect, QColor(color));
}

void GraphicsContext::fillRoundedRect(const IntRect& rect, const IntSize& topLeft, const IntSize& topRight, const IntSize& bottomLeft, const IntSize& bottomRight, const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled() || !color.isValid())
        return;

    Path path;
    path.addRoundedRect(rect, topLeft, topRight, bottomLeft, bottomRight);
    QPainter* p = m_data->p();
    if (hasShadow()) {
        if (mustUseShadowBlur()) {
            ShadowBlur shadow(m_state);
            shadow.drawRectShadow(this, rect, RoundedRect::Radii(topLeft, topRight, bottomLeft, bottomRight));
        } else {
            const QPointF shadowOffset(m_state.shadowOffset.width(), m_state.shadowOffset.height());
            p->translate(shadowOffset);
            p->fillPath(path.platformPath(), QColor(m_state.shadowColor));
            p->translate(-shadowOffset);
        }
    }
    p->fillPath(path.platformPath(), QColor(color));
}

void GraphicsContext::fillRectWithRoundedHole(const IntRect& rect, const RoundedRect& roundedHoleRect, const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled() || !color.isValid())
        return;

    Path path;
    path.addRect(rect);
    if (!roundedHoleRect.radii().isZero())
        path.addRoundedRect(roundedHoleRect);
    else
        path.addRect(roundedHoleRect.rect());

    QPainterPath platformPath = path.platformPath();
    platformPath.setFillRule(Qt::OddEvenFill);

    QPainter* p = m_data->p();
    if (hasShadow()) {
        if (mustUseShadowBlur()) {
            ShadowBlur shadow(m_state);
            shadow.drawInsetShadow(this, rect, roundedHoleRect.rect(), roundedHoleRect.radii());
        } else {
            const QPointF shadowOffset(m_state.shadowOffset.width(), m_state.shadowOffset.height());
            p->translate(shadowOffset);
            p->fillPath(platformPath, QColor(m_state.shadowColor));
            p->translate(-shadowOffset);
        }
    }

    p->fillPath(platformPath, QColor(color));
}

bool GraphicsContext::isInTransparencyLayer() const
{
    return m_data->layerCount;
}

void GraphicsContext::clip(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    m_data->p()->setClipRect(rect, Qt::IntersectClip);
}

void GraphicsContext::clip(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    m_data->p()->setClipRect(rect, Qt::IntersectClip);
}
IntRect GraphicsContext::clipBounds() const
{
    QPainter* p = m_data->p();
    QRectF clipRect;

    clipRect = p->transform().inverted().mapRect(p->window());

    if (p->hasClipping())
        clipRect = clipRect.intersected(m_data->clipBoundingRect());

    return enclosingIntRect(clipRect);
}

void GraphicsContext::clipPath(const Path& path, WindRule clipRule)
{
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    QPainterPath platformPath = path.platformPath();
    platformPath.setFillRule(clipRule == RULE_EVENODD ? Qt::OddEvenFill : Qt::WindingFill);
    p->setClipPath(platformPath, Qt::IntersectClip);
}

void drawFocusRingForPath(QPainter* p, const QPainterPath& path, const Color& color, bool antiAliasing)
{
    const bool antiAlias = p->testRenderHint(QPainter::Antialiasing);
    p->setRenderHint(QPainter::Antialiasing, antiAliasing);

    const QPen oldPen = p->pen();
    const QBrush oldBrush = p->brush();

    QPen nPen = p->pen();
    nPen.setColor(color);
    p->setBrush(Qt::NoBrush);
    nPen.setStyle(Qt::DotLine);

    p->strokePath(path, nPen);
    p->setBrush(oldBrush);
    p->setPen(oldPen);

    p->setRenderHint(QPainter::Antialiasing, antiAlias);
}

void GraphicsContext::drawFocusRing(const Path& path, int /* width */, int offset, const Color& color)
{
    // FIXME: Use 'offset' for something? http://webkit.org/b/49909

    if (paintingDisabled() || !color.isValid())
        return;

    drawFocusRingForPath(m_data->p(), path.platformPath(), color, m_data->antiAliasingForRectsAndLines);
}

/**
 * Focus ring handling for form controls is not handled here. Qt style in
 * RenderTheme handles drawing focus on widgets which 
 * need it. It is still handled here for links.
 */
void GraphicsContext::drawFocusRing(const Vector<IntRect>& rects, int width, int offset, const Color& color)
{
    if (paintingDisabled() || !color.isValid())
        return;

    unsigned rectCount = rects.size();

    if (!rects.size())
        return;

    int radius = (width - 1) / 2;
    QPainterPath path;
    for (unsigned i = 0; i < rectCount; ++i) {
        QRect rect = QRect((rects[i])).adjusted(-offset - radius, -offset - radius, offset + radius, offset + radius);
        // This is not the most efficient way to add a rect to a path, but if we don't create the tmpPath,
        // we will end up with ugly lines in between rows of text on anchors with multiple lines.
        QPainterPath tmpPath;
        tmpPath.addRoundedRect(rect, radius, radius);
        path = path.united(tmpPath);
    }
    drawFocusRingForPath(m_data->p(), path, color, m_data->antiAliasingForRectsAndLines);
}

void GraphicsContext::drawLineForText(const FloatPoint& origin, float width, bool)
{
    if (paintingDisabled())
        return;

    FloatPoint startPoint = origin;
    FloatPoint endPoint = origin + FloatSize(width, 0);

    // If paintengine type is X11 to avoid artifacts
    // like bug https://bugs.webkit.org/show_bug.cgi?id=42248
#if defined(Q_WS_X11)
    QPainter* p = m_data->p();
    if (p->paintEngine()->type() == QPaintEngine::X11) {
        // If stroke thickness is odd we need decrease Y coordinate by 1 pixel,
        // because inside method adjustLineToPixelBoundaries(...), which
        // called from drawLine(...), Y coordinate will be increased by 0.5f
        // and then inside Qt painting engine will be rounded to next greater
        // integer value.
        float strokeWidth = strokeThickness();
        if (static_cast<int>(strokeWidth) % 2) {
            startPoint.setY(startPoint.y() - 1);
            endPoint.setY(endPoint.y() - 1);
        }
    }
#endif // defined(Q_WS_X11)

    drawLine(roundedIntPoint(startPoint), roundedIntPoint(endPoint));
}


/*
 *   NOTE: This code is completely based upon the one from
 *   Source/WebCore/platform/graphics/cairo/DrawErrorUnderline.{h|cpp}
 *
 *   Draws an error underline that looks like one of:
 *
 *               H       E                H
 *      /\      /\      /\        /\      /\               -
 *    A/  \    /  \    /  \     A/  \    /  \              |
 *     \   \  /    \  /   /D     \   \  /    \             |
 *      \   \/  C   \/   /        \   \/   C  \            | height = heightSquares * square
 *       \      /\  F   /          \  F   /\   \           |
 *        \    /  \    /            \    /  \   \G         |
 *         \  /    \  /              \  /    \  /          |
 *          \/      \/                \/      \/           -
 *          B                         B
 *          |---|
 *        unitWidth = (heightSquares - 1) * square
 *
 *  The x, y, width, height passed in give the desired bounding box;
 *  x/width are adjusted to make the underline a integer number of units wide.
*/
static void drawErrorUnderline(QPainter *painter, qreal x, qreal y, qreal width, qreal height)
{
    const qreal heightSquares = 2.5;

    qreal square = height / heightSquares;
    qreal halfSquare = 0.5 * square;

    qreal unitWidth = (heightSquares - 1.0) * square;
    int widthUnits = static_cast<int>((width + 0.5 * unitWidth) / unitWidth);

    x += 0.5 * (width - widthUnits * unitWidth);
    width = widthUnits * unitWidth;

    qreal bottom = y + height;
    qreal top = y;

    QPainterPath path;

    // Bottom of squiggle.
    path.moveTo(x - halfSquare, top + halfSquare); // A

    int i = 0;
    for (i = 0; i < widthUnits; i += 2) {
        qreal middle = x + (i + 1) * unitWidth;
        qreal right = x + (i + 2) * unitWidth;

        path.lineTo(middle, bottom); // B

        if (i + 2 == widthUnits)
            path.lineTo(right + halfSquare, top + halfSquare); // D
        else if (i + 1 != widthUnits)
            path.lineTo(right, top + square); // C
    }

    // Top of squiggle.
    for (i -= 2; i >= 0; i -= 2) {
        qreal left = x + i * unitWidth;
        qreal middle = x + (i + 1) * unitWidth;
        qreal right = x + (i + 2) * unitWidth;

        if (i + 1 == widthUnits)
            path.lineTo(middle + halfSquare, bottom - halfSquare); // G
        else {
            if (i + 2 == widthUnits)
                path.lineTo(right, top); // E

            path.lineTo(middle, bottom - halfSquare); // F
        }

        path.lineTo(left, top); // H
    }

    painter->drawPath(path);
}


void GraphicsContext::drawLineForDocumentMarker(const FloatPoint& origin, float width, DocumentMarkerLineStyle style)
{
    if (paintingDisabled())
        return;

    QPainter* painter = platformContext();
    const QPen originalPen = painter->pen();

    switch (style) {
    case DocumentMarkerSpellingLineStyle:
        painter->setPen(Qt::red);
        break;
    case DocumentMarkerGrammarLineStyle:
        painter->setPen(Qt::green);
        break;
    default:
        return;
    }

    drawErrorUnderline(painter, origin.x(), origin.y(), width, cMisspellingLineThickness);
    painter->setPen(originalPen);
}

FloatRect GraphicsContext::roundToDevicePixels(const FloatRect& frect, RoundingMode)
{
    // It is not enough just to round to pixels in device space. The rotation part of the
    // affine transform matrix to device space can mess with this conversion if we have a
    // rotating image like the hands of the world clock widget. We just need the scale, so
    // we get the affine transform matrix and extract the scale.
    QPainter* painter = platformContext();
    QTransform deviceTransform = painter->deviceTransform();
    if (deviceTransform.isIdentity())
        return frect;

    qreal deviceScaleX = sqrtf(deviceTransform.m11() * deviceTransform.m11() + deviceTransform.m12() * deviceTransform.m12());
    qreal deviceScaleY = sqrtf(deviceTransform.m21() * deviceTransform.m21() + deviceTransform.m22() * deviceTransform.m22());

    QPoint deviceOrigin(frect.x() * deviceScaleX, frect.y() * deviceScaleY);
    QPoint deviceLowerRight(frect.maxX() * deviceScaleX, frect.maxY() * deviceScaleY);

    // Don't let the height or width round to 0 unless either was originally 0
    if (deviceOrigin.y() == deviceLowerRight.y() && frect.height())
        deviceLowerRight.setY(deviceLowerRight.y() + 1);
    if (deviceOrigin.x() == deviceLowerRight.x() && frect.width())
        deviceLowerRight.setX(deviceLowerRight.x() + 1);

    FloatPoint roundedOrigin = FloatPoint(deviceOrigin.x() / deviceScaleX, deviceOrigin.y() / deviceScaleY);
    FloatPoint roundedLowerRight = FloatPoint(deviceLowerRight.x() / deviceScaleX, deviceLowerRight.y() / deviceScaleY);
    return FloatRect(roundedOrigin, roundedLowerRight - roundedOrigin);
}

void GraphicsContext::setPlatformShadow(const FloatSize& size, float blur, const Color& color, ColorSpace colorSpace)
{
    // Qt doesn't support shadows natively, they are drawn manually in the draw*
    // functions
    if (m_state.shadowsIgnoreTransforms) {
        // Meaning that this graphics context is associated with a CanvasRenderingContext
        // We flip the height since CG and HTML5 Canvas have opposite Y axis
        m_state.shadowOffset = FloatSize(size.width(), -size.height());
    }
}

void GraphicsContext::clearPlatformShadow()
{
}

void GraphicsContext::pushTransparencyLayerInternal(const QRect &rect, qreal opacity, QPixmap& alphaMask)
{
    QPainter* p = m_data->p();

    QTransform deviceTransform = p->transform();
    QRect deviceClip = deviceTransform.mapRect(rect);

    alphaMask = alphaMask.transformed(deviceTransform);
    if (alphaMask.width() != deviceClip.width() || alphaMask.height() != deviceClip.height())
        alphaMask = alphaMask.scaled(deviceClip.width(), deviceClip.height());

    m_data->layers.push(new TransparencyLayer(p, deviceClip, 1.0, alphaMask));
}

void GraphicsContext::beginPlatformTransparencyLayer(float opacity)
{
    if (paintingDisabled())
        return;

    int x, y, w, h;
    x = y = 0;
    QPainter* p = m_data->p();
    const QPaintDevice* device = p->device();
    w = device->width();
    h = device->height();

    if (p->hasClipping()) {
        QRectF clip = m_data->clipBoundingRect();
        QRectF deviceClip = p->transform().mapRect(clip);
        x = int(qBound(qreal(0), deviceClip.x(), (qreal)w));
        y = int(qBound(qreal(0), deviceClip.y(), (qreal)h));
        w = int(qBound(qreal(0), deviceClip.width(), (qreal)w) + 2);
        h = int(qBound(qreal(0), deviceClip.height(), (qreal)h) + 2);
    }

    QPixmap emptyAlphaMask;
    m_data->layers.push(new TransparencyLayer(p, QRect(x, y, w, h), opacity, emptyAlphaMask));
    ++m_data->layerCount;
}

void GraphicsContext::endPlatformTransparencyLayer()
{
    if (paintingDisabled())
        return;

    TransparencyLayer* layer = m_data->layers.pop();
    if (!layer->alphaMask.isNull()) {
        layer->painter.resetTransform();
        layer->painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        layer->painter.drawPixmap(QPoint(), layer->alphaMask);
    } else
        --m_data->layerCount; // see the comment for layerCount
    layer->painter.end();

    QPainter* p = m_data->p();
    p->save();
    p->resetTransform();
    p->setOpacity(layer->opacity);
    p->drawPixmap(layer->offset, layer->pixmap);
    p->restore();

    delete layer;
}

bool GraphicsContext::supportsTransparencyLayers()
{
    return true;
}

void GraphicsContext::clearRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    QPainter::CompositionMode currentCompositionMode = p->compositionMode();
    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->fillRect(rect, Qt::transparent);
    p->setCompositionMode(currentCompositionMode);
}

void GraphicsContext::strokeRect(const FloatRect& rect, float lineWidth)
{
    if (paintingDisabled())
        return;

    Path path;
    path.addRect(rect);

    float previousStrokeThickness = strokeThickness();

    if (lineWidth != previousStrokeThickness)
        setStrokeThickness(lineWidth);

    strokePath(path);

    if (lineWidth != previousStrokeThickness)
        setStrokeThickness(previousStrokeThickness);
}

void GraphicsContext::setLineCap(LineCap lc)
{
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    QPen nPen = p->pen();
    nPen.setCapStyle(toQtLineCap(lc));
    p->setPen(nPen);
}

void GraphicsContext::setLineDash(const DashArray& dashes, float dashOffset)
{
    QPainter* p = m_data->p();
    QPen pen = p->pen();
    unsigned dashLength = dashes.size();
    if (dashLength) {
        QVector<qreal> pattern;
        unsigned count = dashLength;
        if (dashLength % 2)
            count *= 2;

        float penWidth = narrowPrecisionToFloat(double(pen.widthF()));
        if (penWidth <= 0.f)
            penWidth = 1.f;

        for (unsigned i = 0; i < count; i++)
            pattern.append(dashes[i % dashLength] / penWidth);

        pen.setDashPattern(pattern);
        pen.setDashOffset(dashOffset / penWidth);
    } else
        pen.setStyle(Qt::SolidLine);
    p->setPen(pen);
}

void GraphicsContext::setLineJoin(LineJoin lj)
{
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    QPen nPen = p->pen();
    nPen.setJoinStyle(toQtLineJoin(lj));
    p->setPen(nPen);
}

void GraphicsContext::setMiterLimit(float limit)
{
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    QPen nPen = p->pen();
    nPen.setMiterLimit(limit);
    p->setPen(nPen);
}

void GraphicsContext::setAlpha(float opacity)
{
    if (paintingDisabled())
        return;
    QPainter* p = m_data->p();
    p->setOpacity(opacity);
}

void GraphicsContext::setPlatformCompositeOperation(CompositeOperator op, BlendMode blendMode)
{
    if (paintingDisabled())
        return;

    ASSERT(op == WebCore::CompositeSourceOver || blendMode == WebCore::BlendModeNormal);

    if (op == WebCore::CompositeSourceOver)
        m_data->p()->setCompositionMode(toQtCompositionMode(blendMode));
    else
        m_data->p()->setCompositionMode(toQtCompositionMode(op));
}

void GraphicsContext::clip(const Path& path, WindRule windRule)
{
    if (paintingDisabled())
        return;

    QPainterPath clipPath = path.platformPath();
    clipPath.setFillRule(toQtFillRule(windRule));
    m_data->p()->setClipPath(clipPath, Qt::IntersectClip);
}

void GraphicsContext::canvasClip(const Path& path, WindRule windRule)
{
    clip(path, windRule);
}

void GraphicsContext::clipOut(const Path& path)
{
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    QPainterPath clippedOut = path.platformPath();
    QPainterPath newClip;
    newClip.setFillRule(Qt::OddEvenFill);
    if (p->hasClipping()) {
        newClip.addRect(m_data->clipBoundingRect());
        newClip.addPath(clippedOut);
        p->setClipPath(newClip, Qt::IntersectClip);
    } else {
        QRect windowRect = p->transform().inverted().mapRect(p->window());
        newClip.addRect(windowRect);
        newClip.addPath(clippedOut.intersected(newClip));
        p->setClipPath(newClip);
    }
}

void GraphicsContext::translate(float x, float y)
{
    if (paintingDisabled())
        return;

    m_data->p()->translate(x, y);
}

void GraphicsContext::rotate(float radians)
{
    if (paintingDisabled())
        return;

    QTransform rotation = QTransform().rotateRadians(radians);
    m_data->p()->setTransform(rotation, true);
}

void GraphicsContext::scale(const FloatSize& s)
{
    if (paintingDisabled())
        return;

    m_data->p()->scale(s.width(), s.height());
}

void GraphicsContext::clipOut(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    QPainterPath newClip;
    newClip.setFillRule(Qt::OddEvenFill);
    if (p->hasClipping()) {
        newClip.addRect(m_data->clipBoundingRect());
        newClip.addRect(QRect(rect));
        p->setClipPath(newClip, Qt::IntersectClip);
    } else {
        QRect clipOutRect(rect);
        QRect window = p->transform().inverted().mapRect(p->window());
        clipOutRect &= window;
        newClip.addRect(window);
        newClip.addRect(clipOutRect);
        p->setClipPath(newClip);
    }
}

void GraphicsContext::concatCTM(const AffineTransform& transform)
{
    if (paintingDisabled())
        return;

    m_data->p()->setWorldTransform(transform, true);
}

void GraphicsContext::setCTM(const AffineTransform& transform)
{
    if (paintingDisabled())
        return;

    m_data->p()->setWorldTransform(transform);
}

#if ENABLE(3D_RENDERING)
TransformationMatrix GraphicsContext::get3DTransform() const
{
    if (paintingDisabled())
        return TransformationMatrix();

    return platformContext()->worldTransform();
}

void GraphicsContext::concat3DTransform(const TransformationMatrix& transform)
{
    if (paintingDisabled())
        return;

    m_data->p()->setWorldTransform(transform, true);
}

void GraphicsContext::set3DTransform(const TransformationMatrix& transform)
{
    if (paintingDisabled())
        return;

    m_data->p()->setWorldTransform(transform, false);
}
#endif

void GraphicsContext::setURLForRect(const KURL&, const IntRect&)
{
    notImplemented();
}

void GraphicsContext::setPlatformStrokeColor(const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled() || !color.isValid())
        return;

    QPainter* p = m_data->p();
    QPen newPen(p->pen());
    m_data->solidColor.setColor(color);
    newPen.setBrush(m_data->solidColor);
    p->setPen(newPen);
}

void GraphicsContext::setPlatformStrokeStyle(StrokeStyle strokeStyle)
{
    if (paintingDisabled())
        return;
    QPainter* p = m_data->p();
    QPen newPen(p->pen());
    newPen.setStyle(toQPenStyle(strokeStyle));
    p->setPen(newPen);
}

void GraphicsContext::setPlatformStrokeThickness(float thickness)
{
    if (paintingDisabled())
        return;
    QPainter* p = m_data->p();
    QPen newPen(p->pen());
    newPen.setWidthF(thickness);
    p->setPen(newPen);
}

void GraphicsContext::setPlatformFillColor(const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled() || !color.isValid())
        return;

    m_data->solidColor.setColor(color);
    m_data->p()->setBrush(m_data->solidColor);
}

void GraphicsContext::setPlatformShouldAntialias(bool enable)
{
    if (paintingDisabled())
        return;
    m_data->p()->setRenderHint(QPainter::Antialiasing, enable);
}

#if OS(WINDOWS)

HDC GraphicsContext::getWindowsContext(const IntRect& dstRect, bool supportAlphaBlend, bool mayCreateBitmap)
{
    // painting through native HDC is only supported for plugin, where mayCreateBitmap is always true
    Q_ASSERT(mayCreateBitmap);

    if (dstRect.isEmpty())
        return 0;

    // Create a bitmap DC in which to draw.
    BITMAPINFO bitmapInfo;
    bitmapInfo.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth         = dstRect.width();
    bitmapInfo.bmiHeader.biHeight        = dstRect.height();
    bitmapInfo.bmiHeader.biPlanes        = 1;
    bitmapInfo.bmiHeader.biBitCount      = 32;
    bitmapInfo.bmiHeader.biCompression   = BI_RGB;
    bitmapInfo.bmiHeader.biSizeImage     = 0;
    bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
    bitmapInfo.bmiHeader.biYPelsPerMeter = 0;
    bitmapInfo.bmiHeader.biClrUsed       = 0;
    bitmapInfo.bmiHeader.biClrImportant  = 0;

    void* pixels = 0;
    HBITMAP bitmap = ::CreateDIBSection(0, &bitmapInfo, DIB_RGB_COLORS, &pixels, 0, 0);
    if (!bitmap)
        return 0;

    HDC displayDC = ::GetDC(0);
    HDC bitmapDC = ::CreateCompatibleDC(displayDC);
    ::ReleaseDC(0, displayDC);

    ::SelectObject(bitmapDC, bitmap);

    // Fill our buffer with clear if we're going to alpha blend.
    if (supportAlphaBlend) {
        BITMAP bmpInfo;
        GetObject(bitmap, sizeof(bmpInfo), &bmpInfo);
        int bufferSize = bmpInfo.bmWidthBytes * bmpInfo.bmHeight;
        memset(bmpInfo.bmBits, 0, bufferSize);
    }

#if !OS(WINCE)
    // Make sure we can do world transforms.
    SetGraphicsMode(bitmapDC, GM_ADVANCED);

    // Apply a translation to our context so that the drawing done will be at (0,0) of the bitmap.
    XFORM xform;
    xform.eM11 = 1.0f;
    xform.eM12 = 0.0f;
    xform.eM21 = 0.0f;
    xform.eM22 = 1.0f;
    xform.eDx = -dstRect.x();
    xform.eDy = -dstRect.y();
    ::SetWorldTransform(bitmapDC, &xform);
#endif

    return bitmapDC;
}

void GraphicsContext::releaseWindowsContext(HDC hdc, const IntRect& dstRect, bool supportAlphaBlend, bool mayCreateBitmap)
{
    // painting through native HDC is only supported for plugin, where mayCreateBitmap is always true
    Q_ASSERT(mayCreateBitmap);

    if (hdc) {

        if (!dstRect.isEmpty()) {

            HBITMAP bitmap = static_cast<HBITMAP>(GetCurrentObject(hdc, OBJ_BITMAP));
            BITMAP info;
            GetObject(bitmap, sizeof(info), &info);
            ASSERT(info.bmBitsPixel == 32);

            QPixmap pixmap = qt_pixmapFromWinHBITMAP(bitmap, supportAlphaBlend ? HBitmapPremultipliedAlpha : HBitmapNoAlpha);
            m_data->p()->drawPixmap(dstRect, pixmap);

            ::DeleteObject(bitmap);
        }

        ::DeleteDC(hdc);
    }
}
#endif

void GraphicsContext::setImageInterpolationQuality(InterpolationQuality quality)
{
    m_data->imageInterpolationQuality = quality;

    switch (quality) {
    case InterpolationNone:
    case InterpolationLow:
        // use nearest-neigbor
        m_data->p()->setRenderHint(QPainter::SmoothPixmapTransform, false);
        break;

    case InterpolationMedium:
    case InterpolationHigh:
        // use the filter
        m_data->p()->setRenderHint(QPainter::SmoothPixmapTransform, true);
        break;

    case InterpolationDefault:
    default:
        m_data->p()->setRenderHint(QPainter::SmoothPixmapTransform, m_data->initialSmoothPixmapTransformHint);
        break;
    };
}

InterpolationQuality GraphicsContext::imageInterpolationQuality() const
{
    return m_data->imageInterpolationQuality;
}

void GraphicsContext::takeOwnershipOfPlatformContext()
{
    m_data->takeOwnershipOfPlatformContext();
}

}

// vim: ts=4 sw=4 et
