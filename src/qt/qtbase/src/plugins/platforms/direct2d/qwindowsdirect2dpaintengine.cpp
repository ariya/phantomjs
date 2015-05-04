/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowsdirect2dpaintengine.h"
#include "qwindowsdirect2dplatformpixmap.h"
#include "qwindowsdirect2dpaintdevice.h"
#include "qwindowsdirect2dcontext.h"
#include "qwindowsdirect2dhelpers.h"
#include "qwindowsdirect2dbitmap.h"
#include "qwindowsdirect2ddevicecontext.h"

#include "qwindowsfontengine.h"
#include "qwindowsfontdatabase.h"
#include "qwindowsintegration.h"

#include <QtCore/QtMath>
#include <QtCore/QStack>
#include <QtCore/QSettings>
#include <QtGui/private/qpaintengine_p.h>
#include <QtGui/private/qtextengine_p.h>
#include <QtGui/private/qfontengine_p.h>
#include <QtGui/private/qstatictext_p.h>

using Microsoft::WRL::ComPtr;

QT_BEGIN_NAMESPACE

// The enum values below are set as tags on the device context
// in the various draw methods. When EndDraw is called the device context
// will report the last set tag number in case of errors
// along with an error code

// Microsoft keeps a list of d2d error codes here:
// http://msdn.microsoft.com/en-us/library/windows/desktop/dd370979(v=vs.85).aspx
enum {
    D2DDebugDrawInitialStateTag = -1,
    D2DDebugFillTag = 1,
    D2DDebugFillRectTag,
    D2DDebugDrawRectsTag,
    D2DDebugDrawRectFsTag,
    D2DDebugDrawLinesTag,
    D2DDebugDrawLineFsTag,
    D2DDebugDrawEllipseTag,
    D2DDebugDrawEllipseFTag,
    D2DDebugDrawImageTag,
    D2DDebugDrawPixmapTag,
    D2DDebugDrawStaticTextItemTag,
    D2DDebugDrawTextItemTag
};

//Clipping flags
enum {
    SimpleSystemClip = 0x1
};

enum ClipType {
    AxisAlignedClip,
    LayerClip
};

// Since d2d is a float-based system we need to be able to snap our drawing to whole pixels.
// Applying the magical aliasing offset to coordinates will do so, just make sure that
// aliased painting is turned on on the d2d device context.
static const qreal MAGICAL_ALIASING_OFFSET = 0.5;

#define D2D_TAG(tag) d->dc()->SetTags(tag, tag)

Q_GUI_EXPORT QImage qt_imageForBrush(int brushStyle, bool invert);

static inline ID2D1Factory1 *factory()
{
    return QWindowsDirect2DContext::instance()->d2dFactory();
}

static inline D2D1_MATRIX_3X2_F transformFromLine(const QLineF &line, qreal penWidth)
{
    const qreal halfWidth = penWidth / 2;
    const qreal angle = -qDegreesToRadians(line.angle());
    QTransform transform = QTransform::fromTranslate(line.p1().x() + qSin(angle) * halfWidth,
                                                     line.p1().y() - qCos(angle) * halfWidth);
    transform.rotateRadians(angle);
    return to_d2d_matrix_3x2_f(transform);
}

class Direct2DPathGeometryWriter
{
public:
    Direct2DPathGeometryWriter()
        : m_inFigure(false)
        , m_roundCoordinates(false)
    {

    }

    bool begin()
    {
        HRESULT hr = factory()->CreatePathGeometry(&m_geometry);
        if (FAILED(hr)) {
            qWarning("%s: Could not create path geometry: %#x", __FUNCTION__, hr);
            return false;
        }

        hr = m_geometry->Open(&m_sink);
        if (FAILED(hr)) {
            qWarning("%s: Could not create geometry sink: %#x", __FUNCTION__, hr);
            return false;
        }

        return true;
    }

    void setWindingFillEnabled(bool enable)
    {
        if (enable)
            m_sink->SetFillMode(D2D1_FILL_MODE_WINDING);
        else
            m_sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
    }

    void setAliasingEnabled(bool enable)
    {
        m_roundCoordinates = enable;
    }

    bool isInFigure() const
    {
        return m_inFigure;
    }

    void moveTo(const QPointF &point)
    {
        if (m_inFigure)
            m_sink->EndFigure(D2D1_FIGURE_END_OPEN);

        m_sink->BeginFigure(adjusted(point), D2D1_FIGURE_BEGIN_FILLED);
        m_inFigure = true;
    }

    void lineTo(const QPointF &point)
    {
        m_sink->AddLine(adjusted(point));
    }

    void curveTo(const QPointF &p1, const QPointF &p2, const QPointF &p3)
    {
        D2D1_BEZIER_SEGMENT segment = {
            adjusted(p1),
            adjusted(p2),
            adjusted(p3)
        };

        m_sink->AddBezier(segment);
    }

    void close()
    {
        if (m_inFigure)
            m_sink->EndFigure(D2D1_FIGURE_END_OPEN);

        m_sink->Close();
    }

    ComPtr<ID2D1PathGeometry1> geometry() const
    {
        return m_geometry;
    }

private:
    D2D1_POINT_2F adjusted(const QPointF &point)
    {
        static const QPointF adjustment(MAGICAL_ALIASING_OFFSET,
                                        MAGICAL_ALIASING_OFFSET);

        if (m_roundCoordinates)
            return to_d2d_point_2f(point + adjustment);
        else
            return to_d2d_point_2f(point);
    }

    ComPtr<ID2D1PathGeometry1> m_geometry;
    ComPtr<ID2D1GeometrySink> m_sink;

    bool m_inFigure;
    bool m_roundCoordinates;
};

struct D2DVectorPathCache {
    ComPtr<ID2D1PathGeometry1> aliased;
    ComPtr<ID2D1PathGeometry1> antiAliased;

    static void cleanup_func(QPaintEngineEx *engine, void *data) {
        Q_UNUSED(engine);
        D2DVectorPathCache *e = static_cast<D2DVectorPathCache *>(data);
        delete e;
    }
};

class QWindowsDirect2DPaintEnginePrivate : public QPaintEngineExPrivate
{
    Q_DECLARE_PUBLIC(QWindowsDirect2DPaintEngine)
public:
    QWindowsDirect2DPaintEnginePrivate(QWindowsDirect2DBitmap *bm, QWindowsDirect2DPaintEngine::Flags flags)
        : bitmap(bm)
        , clipFlags(0)
        , flags(flags)
    {
        pen.reset();
        brush.reset();

        dc()->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    }

    QWindowsDirect2DBitmap *bitmap;
    QImage fallbackImage;

    unsigned int clipFlags;
    QStack<ClipType> pushedClips;
    QWindowsDirect2DPaintEngine::Flags flags;

    QPointF currentBrushOrigin;

    QHash< QFontDef, ComPtr<IDWriteFontFace> > fontCache;

    struct {
        bool emulate;
        QPen qpen;
        ComPtr<ID2D1Brush> brush;
        ComPtr<ID2D1StrokeStyle1> strokeStyle;
        ComPtr<ID2D1BitmapBrush1> dashBrush;

        inline void reset() {
            emulate = false;
            qpen = QPen();
            brush.Reset();
            strokeStyle.Reset();
            dashBrush.Reset();
        }
    } pen;

    struct {
        bool emulate;
        QBrush qbrush;
        ComPtr<ID2D1Brush> brush;

        inline void reset() {
            emulate = false;
            brush.Reset();
            qbrush = QBrush();
        }
    } brush;

    inline ID2D1DeviceContext *dc() const
    {
        Q_ASSERT(bitmap);
        return bitmap->deviceContext()->get();
    }

    inline D2D1_INTERPOLATION_MODE interpolationMode() const
    {
        Q_Q(const QWindowsDirect2DPaintEngine);
        return (q->state()->renderHints & QPainter::SmoothPixmapTransform) ? D2D1_INTERPOLATION_MODE_LINEAR
                                                                           : D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
    }

    inline D2D1_ANTIALIAS_MODE antialiasMode() const
    {
        Q_Q(const QWindowsDirect2DPaintEngine);
        return (q->state()->renderHints & QPainter::Antialiasing) ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE
                                                                  : D2D1_ANTIALIAS_MODE_ALIASED;
    }

    inline D2D1_LAYER_OPTIONS1 layerOptions() const
    {
        if (flags & QWindowsDirect2DPaintEngine::TranslucentTopLevelWindow)
            return D2D1_LAYER_OPTIONS1_NONE;
        else
            return D2D1_LAYER_OPTIONS1_INITIALIZE_FROM_BACKGROUND;
    }

    void updateTransform(const QTransform &transform)
    {
        dc()->SetTransform(to_d2d_matrix_3x2_f(transform));
    }

    void updateOpacity(qreal opacity)
    {
        if (brush.brush)
            brush.brush->SetOpacity(opacity);
        if (pen.brush)
            pen.brush->SetOpacity(opacity);
    }

    void pushClip(const QVectorPath &path)
    {
        Q_Q(QWindowsDirect2DPaintEngine);

        if (path.isEmpty()) {
            D2D_RECT_F rect = {0, 0, 0, 0};
            dc()->PushAxisAlignedClip(rect, antialiasMode());
            pushedClips.push(AxisAlignedClip);
        } else if (path.isRect() && (q->state()->matrix.type() <= QTransform::TxScale)) {
            const qreal * const points = path.points();
            D2D_RECT_F rect = {
                points[0], // left
                points[1], // top
                points[2], // right,
                points[5]  // bottom
            };

            dc()->PushAxisAlignedClip(rect, antialiasMode());
            pushedClips.push(AxisAlignedClip);
        } else {
            ComPtr<ID2D1PathGeometry1> geometry = vectorPathToID2D1PathGeometry(path);
            if (!geometry) {
                qWarning("%s: Could not convert vector path to painter path!", __FUNCTION__);
                return;
            }

            dc()->PushLayer(D2D1::LayerParameters1(D2D1::InfiniteRect(),
                                                   geometry.Get(),
                                                   antialiasMode(),
                                                   D2D1::IdentityMatrix(),
                                                   1.0,
                                                   NULL,
                                                   layerOptions()),
                            NULL);
            pushedClips.push(LayerClip);
        }
    }

    void clearClips()
    {
        while (!pushedClips.isEmpty()) {
            switch (pushedClips.pop()) {
            case AxisAlignedClip:
                dc()->PopAxisAlignedClip();
                break;
            case LayerClip:
                dc()->PopLayer();
                break;
            }
        }
    }

    void updateClipEnabled(bool enabled)
    {
        if (!enabled)
            clearClips();
        else if (pushedClips.isEmpty())
            replayClipOperations();
    }

    void clip(const QVectorPath &path, Qt::ClipOperation operation)
    {
        switch (operation) {
        case Qt::NoClip:
            clearClips();
            break;
        case Qt::ReplaceClip:
            clearClips();
            pushClip(path);
            break;
        case Qt::IntersectClip:
            pushClip(path);
            break;
        }
    }

    void updateCompositionMode(QPainter::CompositionMode mode)
    {
        switch (mode) {
        case QPainter::CompositionMode_Source:
            dc()->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_COPY);
            break;
        case QPainter::CompositionMode_SourceOver:
            dc()->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
            break;

        default:
            // Activating an unsupported mode at any time will cause the QImage
            // fallback to be used for the remainder of the active paint session
            dc()->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_COPY);
            flags |= QWindowsDirect2DPaintEngine::EmulateComposition;
            break;
        }
    }

    void updateBrush(const QBrush &newBrush)
    {
        Q_Q(const QWindowsDirect2DPaintEngine);

        if (qbrush_fast_equals(brush.qbrush, newBrush) && (brush.brush || brush.emulate))
            return;

        brush.brush = to_d2d_brush(newBrush, &brush.emulate);
        brush.qbrush = newBrush;

        if (brush.brush) {
            brush.brush->SetOpacity(q->state()->opacity);
            applyBrushOrigin(currentBrushOrigin);
        }
    }

    void updateBrushOrigin(const QPointF &brushOrigin)
    {
        negateCurrentBrushOrigin();
        applyBrushOrigin(brushOrigin);
    }

    void negateCurrentBrushOrigin()
    {
        if (brush.brush && !currentBrushOrigin.isNull()) {
            D2D1_MATRIX_3X2_F transform;
            brush.brush->GetTransform(&transform);

            brush.brush->SetTransform(*(D2D1::Matrix3x2F::ReinterpretBaseType(&transform))
                                      * D2D1::Matrix3x2F::Translation(-currentBrushOrigin.x(),
                                                                      -currentBrushOrigin.y()));
        }
    }

    void applyBrushOrigin(const QPointF &origin)
    {
        if (brush.brush && !origin.isNull()) {
            D2D1_MATRIX_3X2_F transform;
            brush.brush->GetTransform(&transform);

            brush.brush->SetTransform(*(D2D1::Matrix3x2F::ReinterpretBaseType(&transform))
                                      * D2D1::Matrix3x2F::Translation(origin.x(), origin.y()));
        }

        currentBrushOrigin = origin;
    }

    void updatePen(const QPen &newPen)
    {
        Q_Q(const QWindowsDirect2DPaintEngine);
        if (qpen_fast_equals(newPen, pen.qpen) && (pen.brush || pen.emulate))
            return;

        pen.reset();
        pen.qpen = newPen;

        if (newPen.style() == Qt::NoPen)
            return;

        pen.brush = to_d2d_brush(newPen.brush(), &pen.emulate);
        if (!pen.brush)
            return;

        pen.brush->SetOpacity(q->state()->opacity);

        D2D1_STROKE_STYLE_PROPERTIES1 props = {};

        switch (newPen.capStyle()) {
        case Qt::SquareCap:
            props.startCap = props.endCap = props.dashCap = D2D1_CAP_STYLE_SQUARE;
            break;
        case Qt::RoundCap:
            props.startCap = props.endCap = props.dashCap = D2D1_CAP_STYLE_ROUND;
            break;
        case Qt::FlatCap:
        default:
            props.startCap = props.endCap = props.dashCap = D2D1_CAP_STYLE_FLAT;
            break;
        }

        switch (newPen.joinStyle()) {
        case Qt::BevelJoin:
            props.lineJoin = D2D1_LINE_JOIN_BEVEL;
            break;
        case Qt::RoundJoin:
            props.lineJoin = D2D1_LINE_JOIN_ROUND;
            break;
        case Qt::MiterJoin:
        default:
            props.lineJoin = D2D1_LINE_JOIN_MITER;
            break;
        }

        props.miterLimit = newPen.miterLimit() * qreal(2.0); // D2D and Qt miter specs differ
        props.dashOffset = newPen.dashOffset();

        if (newPen.widthF() == 0)
            props.transformType = D2D1_STROKE_TRANSFORM_TYPE_HAIRLINE;
        else if (qt_pen_is_cosmetic(newPen, q->state()->renderHints))
            props.transformType = D2D1_STROKE_TRANSFORM_TYPE_FIXED;
        else
            props.transformType = D2D1_STROKE_TRANSFORM_TYPE_NORMAL;

        switch (newPen.style()) {
        case Qt::SolidLine:
            props.dashStyle = D2D1_DASH_STYLE_SOLID;
            break;

        case Qt::DotLine:
        case Qt::DashDotLine:
        case Qt::DashDotDotLine:
            // Try and match Qt's raster engine in output as closely as possible
            if (newPen.widthF() <= 1.0)
                props.startCap = props.endCap = props.dashCap = D2D1_CAP_STYLE_FLAT;

            // fall through
        default:
            props.dashStyle = D2D1_DASH_STYLE_CUSTOM;
            break;
        }

        HRESULT hr;

        if (props.dashStyle == D2D1_DASH_STYLE_CUSTOM) {
            QVector<qreal> dashes = newPen.dashPattern();
            QVector<FLOAT> converted(dashes.size());
            qreal penWidth = pen.qpen.widthF();
            qreal brushWidth = 0;
            for (int i = 0; i < dashes.size(); i++) {
                converted[i] = dashes[i];
                brushWidth += penWidth * dashes[i];
            }

            hr = factory()->CreateStrokeStyle(props, converted.constData(), converted.size(), &pen.strokeStyle);

            // Create a combined brush/dash pattern for optimized line drawing
            QWindowsDirect2DBitmap bitmap;
            bitmap.resize(ceil(brushWidth), ceil(penWidth));
            bitmap.deviceContext()->begin();
            bitmap.deviceContext()->get()->SetAntialiasMode(antialiasMode());
            bitmap.deviceContext()->get()->SetTransform(D2D1::IdentityMatrix());
            bitmap.deviceContext()->get()->Clear();
            const qreal offsetX = (qreal(bitmap.size().width()) - brushWidth) / 2;
            const qreal offsetY = qreal(bitmap.size().height()) / 2;
            bitmap.deviceContext()->get()->DrawLine(D2D1::Point2F(offsetX, offsetY),
                                                    D2D1::Point2F(brushWidth, offsetY),
                                                    pen.brush.Get(), penWidth, pen.strokeStyle.Get());
            bitmap.deviceContext()->end();
            D2D1_BITMAP_BRUSH_PROPERTIES1 bitmapBrushProperties = D2D1::BitmapBrushProperties1(
                        D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_CLAMP, D2D1_INTERPOLATION_MODE_LINEAR);
            hr = dc()->CreateBitmapBrush(bitmap.bitmap(), bitmapBrushProperties, &pen.dashBrush);
        } else {
            hr = factory()->CreateStrokeStyle(props, NULL, 0, &pen.strokeStyle);
        }

        if (FAILED(hr))
            qWarning("%s: Could not create stroke style: %#x", __FUNCTION__, hr);
    }

    ComPtr<ID2D1Brush> to_d2d_brush(const QBrush &newBrush, bool *needsEmulation)
    {
        HRESULT hr;
        ComPtr<ID2D1Brush> result;

        Q_ASSERT(needsEmulation);

        *needsEmulation = false;

        switch (newBrush.style()) {
        case Qt::NoBrush:
            break;

        case Qt::SolidPattern:
        {
            ComPtr<ID2D1SolidColorBrush> solid;

            hr = dc()->CreateSolidColorBrush(to_d2d_color_f(newBrush.color()), &solid);
            if (FAILED(hr)) {
                qWarning("%s: Could not create solid color brush: %#x", __FUNCTION__, hr);
                break;
            }

            hr = solid.As(&result);
            if (FAILED(hr))
                qWarning("%s: Could not convert solid color brush: %#x", __FUNCTION__, hr);
        }
            break;

        case Qt::Dense1Pattern:
        case Qt::Dense2Pattern:
        case Qt::Dense3Pattern:
        case Qt::Dense4Pattern:
        case Qt::Dense5Pattern:
        case Qt::Dense6Pattern:
        case Qt::Dense7Pattern:
        case Qt::HorPattern:
        case Qt::VerPattern:
        case Qt::CrossPattern:
        case Qt::BDiagPattern:
        case Qt::FDiagPattern:
        case Qt::DiagCrossPattern:
        {
            ComPtr<ID2D1BitmapBrush1> bitmapBrush;
            D2D1_BITMAP_BRUSH_PROPERTIES1 bitmapBrushProperties = {
                D2D1_EXTEND_MODE_WRAP,
                D2D1_EXTEND_MODE_WRAP,
                interpolationMode()
            };

            QImage brushImg = qt_imageForBrush(newBrush.style(), false);
            brushImg.setColor(0, newBrush.color().rgba());
            brushImg.setColor(1, qRgba(0, 0, 0, 0));

            QWindowsDirect2DBitmap bitmap;
            bool success = bitmap.fromImage(brushImg, Qt::AutoColor);
            if (!success) {
                qWarning("%s: Could not create Direct2D bitmap from Qt pattern brush image", __FUNCTION__);
                break;
            }

            hr = dc()->CreateBitmapBrush(bitmap.bitmap(),
                                         bitmapBrushProperties,
                                         &bitmapBrush);
            if (FAILED(hr)) {
                qWarning("%s: Could not create Direct2D bitmap brush for Qt pattern brush: %#x", __FUNCTION__, hr);
                break;
            }

            hr = bitmapBrush.As(&result);
            if (FAILED(hr))
                qWarning("%s: Could not convert Direct2D bitmap brush for Qt pattern brush: %#x", __FUNCTION__, hr);
        }
            break;

        case Qt::LinearGradientPattern:
            if (newBrush.gradient()->spread() != QGradient::PadSpread) {
                *needsEmulation = true;
            } else {
                ComPtr<ID2D1LinearGradientBrush> linear;
                const QLinearGradient *qlinear = static_cast<const QLinearGradient *>(newBrush.gradient());

                D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES linearGradientBrushProperties;
                ComPtr<ID2D1GradientStopCollection> gradientStopCollection;

                const QGradientStops &qstops = qlinear->stops();
                QVector<D2D1_GRADIENT_STOP> stops(qstops.count());

                linearGradientBrushProperties.startPoint = to_d2d_point_2f(qlinear->start());
                linearGradientBrushProperties.endPoint = to_d2d_point_2f(qlinear->finalStop());

                for (int i = 0; i < stops.size(); i++) {
                    stops[i].position = qstops[i].first;
                    stops[i].color = to_d2d_color_f(qstops[i].second);
                }

                hr = dc()->CreateGradientStopCollection(stops.constData(), stops.size(), &gradientStopCollection);
                if (FAILED(hr)) {
                    qWarning("%s: Could not create gradient stop collection for linear gradient: %#x", __FUNCTION__, hr);
                    break;
                }

                hr = dc()->CreateLinearGradientBrush(linearGradientBrushProperties, gradientStopCollection.Get(),
                                                     &linear);
                if (FAILED(hr)) {
                    qWarning("%s: Could not create Direct2D linear gradient brush: %#x", __FUNCTION__, hr);
                    break;
                }

                hr = linear.As(&result);
                if (FAILED(hr)) {
                    qWarning("%s: Could not convert Direct2D linear gradient brush: %#x", __FUNCTION__, hr);
                    break;
                }
            }
            break;

        case Qt::RadialGradientPattern:
            if (newBrush.gradient()->spread() != QGradient::PadSpread) {
                *needsEmulation = true;
            } else {
                ComPtr<ID2D1RadialGradientBrush> radial;
                const QRadialGradient *qradial = static_cast<const QRadialGradient *>(newBrush.gradient());

                D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES radialGradientBrushProperties;
                ComPtr<ID2D1GradientStopCollection> gradientStopCollection;

                const QGradientStops &qstops = qradial->stops();
                QVector<D2D1_GRADIENT_STOP> stops(qstops.count());

                radialGradientBrushProperties.center = to_d2d_point_2f(qradial->center());
                radialGradientBrushProperties.gradientOriginOffset = to_d2d_point_2f(qradial->focalPoint() - qradial->center());
                radialGradientBrushProperties.radiusX = qradial->radius();
                radialGradientBrushProperties.radiusY = qradial->radius();

                for (int i = 0; i < stops.size(); i++) {
                    stops[i].position = qstops[i].first;
                    stops[i].color = to_d2d_color_f(qstops[i].second);
                }

                hr = dc()->CreateGradientStopCollection(stops.constData(), stops.size(), &gradientStopCollection);
                if (FAILED(hr)) {
                    qWarning("%s: Could not create gradient stop collection for radial gradient: %#x", __FUNCTION__, hr);
                    break;
                }

                hr = dc()->CreateRadialGradientBrush(radialGradientBrushProperties, gradientStopCollection.Get(),
                                                     &radial);
                if (FAILED(hr)) {
                    qWarning("%s: Could not create Direct2D radial gradient brush: %#x", __FUNCTION__, hr);
                    break;
                }

                radial.As(&result);
                if (FAILED(hr)) {
                    qWarning("%s: Could not convert Direct2D radial gradient brush: %#x", __FUNCTION__, hr);
                    break;
                }
            }
            break;

        case Qt::ConicalGradientPattern:
            *needsEmulation = true;
            break;

        case Qt::TexturePattern:
        {
            ComPtr<ID2D1BitmapBrush1> bitmapBrush;
            D2D1_BITMAP_BRUSH_PROPERTIES1 bitmapBrushProperties = {
                D2D1_EXTEND_MODE_WRAP,
                D2D1_EXTEND_MODE_WRAP,
                interpolationMode()
            };

            QWindowsDirect2DPlatformPixmap *pp = static_cast<QWindowsDirect2DPlatformPixmap *>(newBrush.texture().handle());
            QWindowsDirect2DBitmap *bitmap = pp->bitmap();
            hr = dc()->CreateBitmapBrush(bitmap->bitmap(),
                                         bitmapBrushProperties,
                                         &bitmapBrush);

            if (FAILED(hr)) {
                qWarning("%s: Could not create texture brush: %#x", __FUNCTION__, hr);
                break;
            }

            hr = bitmapBrush.As(&result);
            if (FAILED(hr))
                qWarning("%s: Could not convert texture brush: %#x", __FUNCTION__, hr);
        }
            break;
        }

        if (result && !newBrush.transform().isIdentity())
            result->SetTransform(to_d2d_matrix_3x2_f(newBrush.transform()));

        return result;
    }

    ComPtr<ID2D1PathGeometry1> vectorPathToID2D1PathGeometry(const QVectorPath &path)
    {
        Q_Q(QWindowsDirect2DPaintEngine);

        const bool alias = !q->antiAliasingEnabled();

        QVectorPath::CacheEntry *cacheEntry = path.isCacheable() ? path.lookupCacheData(q)
                                                                 : Q_NULLPTR;

        if (cacheEntry) {
            D2DVectorPathCache *e = static_cast<D2DVectorPathCache *>(cacheEntry->data);
            if (alias && e->aliased)
                return e->aliased;
            else if (!alias && e->antiAliased)
                return e->antiAliased;
        }

        Direct2DPathGeometryWriter writer;
        if (!writer.begin())
            return NULL;

        writer.setWindingFillEnabled(path.hasWindingFill());
        writer.setAliasingEnabled(alias);

        const QPainterPath::ElementType *types = path.elements();
        const int count = path.elementCount();
        const qreal *points = path.points();

        Q_ASSERT(points);

        if (types) {
            qreal x, y;

            for (int i = 0; i < count; i++) {
                x = points[i * 2];
                y = points[i * 2 + 1];

                switch (types[i]) {
                case QPainterPath::MoveToElement:
                    writer.moveTo(QPointF(x, y));
                    break;

                case QPainterPath::LineToElement:
                    writer.lineTo(QPointF(x, y));
                    break;

                case QPainterPath::CurveToElement:
                {
                    Q_ASSERT((i + 2) < count);
                    Q_ASSERT(types[i+1] == QPainterPath::CurveToDataElement);
                    Q_ASSERT(types[i+2] == QPainterPath::CurveToDataElement);

                    i++;
                    const qreal x2 = points[i * 2];
                    const qreal y2 = points[i * 2 + 1];

                    i++;
                    const qreal x3 = points[i * 2];
                    const qreal y3 = points[i * 2 + 1];

                    writer.curveTo(QPointF(x, y), QPointF(x2, y2), QPointF(x3, y3));
                }
                    break;

                case QPainterPath::CurveToDataElement:
                    qWarning("%s: Unhandled Curve Data Element", __FUNCTION__);
                    break;
                }
            }
        } else {
            writer.moveTo(QPointF(points[0], points[1]));
            for (int i = 1; i < count; i++)
                writer.lineTo(QPointF(points[i * 2], points[i * 2 + 1]));
        }

        if (writer.isInFigure())
            if (path.hasImplicitClose())
                writer.lineTo(QPointF(points[0], points[1]));

        writer.close();
        ComPtr<ID2D1PathGeometry1> geometry = writer.geometry();

        if (path.isCacheable()) {
            if (!cacheEntry)
                cacheEntry = path.addCacheData(q, new D2DVectorPathCache, D2DVectorPathCache::cleanup_func);

            D2DVectorPathCache *e = static_cast<D2DVectorPathCache *>(cacheEntry->data);
            if (alias)
                e->aliased = geometry;
            else
                e->antiAliased = geometry;
        } else {
            path.makeCacheable();
        }

        return geometry;
    }

    void updateHints()
    {
        dc()->SetAntialiasMode(antialiasMode());
    }

    void drawGlyphRun(const D2D1_POINT_2F &pos,
                      IDWriteFontFace *fontFace,
                      const QFontDef &fontDef,
                      int numGlyphs,
                      const UINT16 *glyphIndices,
                      const FLOAT *glyphAdvances,
                      const DWRITE_GLYPH_OFFSET *glyphOffsets,
                      bool rtl)
    {
        Q_Q(QWindowsDirect2DPaintEngine);

        DWRITE_GLYPH_RUN glyphRun = {
            fontFace,          //    IDWriteFontFace           *fontFace;
            fontDef.pixelSize, //    FLOAT                     fontEmSize;
            numGlyphs,         //    UINT32                    glyphCount;
            glyphIndices,      //    const UINT16              *glyphIndices;
            glyphAdvances,     //    const FLOAT               *glyphAdvances;
            glyphOffsets,      //    const DWRITE_GLYPH_OFFSET *glyphOffsets;
            FALSE,             //    BOOL                      isSideways;
            rtl ? 1 : 0        //    UINT32                    bidiLevel;
        };

        const bool antiAlias = bool((q->state()->renderHints & QPainter::TextAntialiasing)
                                    && !(fontDef.styleStrategy & QFont::NoAntialias));
        const D2D1_TEXT_ANTIALIAS_MODE antialiasMode = (flags & QWindowsDirect2DPaintEngine::TranslucentTopLevelWindow)
                ? D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE : D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE;
        dc()->SetTextAntialiasMode(antiAlias ? antialiasMode : D2D1_TEXT_ANTIALIAS_MODE_ALIASED);

        dc()->DrawGlyphRun(pos,
                           &glyphRun,
                           NULL,
                           pen.brush.Get(),
                           DWRITE_MEASURING_MODE_GDI_CLASSIC);
    }

    ComPtr<IDWriteFontFace> fontFaceFromFontEngine(QFontEngine *fe)
    {
        const QFontDef fontDef = fe->fontDef;
        ComPtr<IDWriteFontFace> fontFace = fontCache.value(fontDef);
        if (fontFace)
            return fontFace;

        LOGFONT lf = QWindowsFontDatabase::fontDefToLOGFONT(fontDef);

        // Get substitute name
        static const char keyC[] = "HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes";
        const QString familyName = QString::fromWCharArray(lf.lfFaceName);
        const QString nameSubstitute = QSettings(QLatin1String(keyC), QSettings::NativeFormat).value(familyName, familyName).toString();
        memcpy(lf.lfFaceName, nameSubstitute.utf16(), sizeof(wchar_t) * qMin(nameSubstitute.length() + 1, LF_FACESIZE));

        ComPtr<IDWriteFont> dwriteFont;
        HRESULT hr = QWindowsDirect2DContext::instance()->dwriteGdiInterop()->CreateFontFromLOGFONT(&lf, &dwriteFont);
        if (FAILED(hr)) {
            qDebug("%s: CreateFontFromLOGFONT failed: %#x", __FUNCTION__, hr);
            return fontFace;
        }

        hr = dwriteFont->CreateFontFace(&fontFace);
        if (FAILED(hr)) {
            qDebug("%s: CreateFontFace failed: %#x", __FUNCTION__, hr);
            return fontFace;
        }

        if (fontFace)
            fontCache.insert(fontDef, fontFace);

        return fontFace;
    }
};

QWindowsDirect2DPaintEngine::QWindowsDirect2DPaintEngine(QWindowsDirect2DBitmap *bitmap, Flags flags)
    : QPaintEngineEx(*(new QWindowsDirect2DPaintEnginePrivate(bitmap, flags)))
{
    QPaintEngine::PaintEngineFeatures unsupported =
            // As of 1.1 Direct2D does not natively support complex composition modes
            // However, using Direct2D effects that implement them should be possible
            QPaintEngine::PorterDuff
            | QPaintEngine::BlendModes
            | QPaintEngine::RasterOpModes

            // As of 1.1 Direct2D does not natively support perspective transforms
            // However, writing a custom effect that implements them should be possible
            // The built-in 3D transform effect unfortunately changes output image size, making
            // it unusable for us.
            | QPaintEngine::PerspectiveTransform;

    gccaps &= ~unsupported;
}

bool QWindowsDirect2DPaintEngine::begin(QPaintDevice * pdev)
{
    Q_D(QWindowsDirect2DPaintEngine);

    d->bitmap->deviceContext()->begin();
    d->dc()->SetTransform(D2D1::Matrix3x2F::Identity());

    if (systemClip().rectCount() > 1) {
        QPainterPath p;
        p.addRegion(systemClip());

        ComPtr<ID2D1PathGeometry1> geometry = d->vectorPathToID2D1PathGeometry(qtVectorPathForPath(p));
        if (!geometry)
            return false;

        d->dc()->PushLayer(D2D1::LayerParameters1(D2D1::InfiniteRect(),
                                               geometry.Get(),
                                               d->antialiasMode(),
                                               D2D1::IdentityMatrix(),
                                               1.0,
                                               NULL,
                                               d->layerOptions()),
                        NULL);
    } else {
        QRect clip(0, 0, pdev->width(), pdev->height());
        if (!systemClip().isEmpty())
            clip &= systemClip().boundingRect();
        d->dc()->PushAxisAlignedClip(to_d2d_rect_f(clip), D2D1_ANTIALIAS_MODE_ALIASED);
        d->clipFlags |= SimpleSystemClip;
    }

    D2D_TAG(D2DDebugDrawInitialStateTag);

    setActive(true);
    return true;
}

bool QWindowsDirect2DPaintEngine::end()
{
    Q_D(QWindowsDirect2DPaintEngine);

    // Always clear all emulation-related things so we are in a clean state for our next painting run
    const bool emulatingComposition = d->flags.testFlag(EmulateComposition);
    d->flags &= ~QWindowsDirect2DPaintEngine::EmulateComposition;
    if (!d->fallbackImage.isNull()) {
        if (emulatingComposition)
            drawImage(d->fallbackImage.rect(), d->fallbackImage, d->fallbackImage.rect());
        d->fallbackImage = QImage();
    }

    // Pop any user-applied clipping
    d->clearClips();
    // Now the system clip from begin() above
    if (d->clipFlags & SimpleSystemClip) {
        d->dc()->PopAxisAlignedClip();
        d->clipFlags &= ~SimpleSystemClip;
    } else {
        d->dc()->PopLayer();
    }

    return d->bitmap->deviceContext()->end();
}

QPaintEngine::Type QWindowsDirect2DPaintEngine::type() const
{
    return QPaintEngine::Direct2D;
}

void QWindowsDirect2DPaintEngine::setState(QPainterState *s)
{
    Q_D(QWindowsDirect2DPaintEngine);

    QPaintEngineEx::setState(s);
    d->clearClips();

    clipEnabledChanged();
    penChanged();
    brushChanged();
    brushOriginChanged();
    opacityChanged();
    compositionModeChanged();
    renderHintsChanged();
    transformChanged();
}

void QWindowsDirect2DPaintEngine::draw(const QVectorPath &path)
{
    Q_D(QWindowsDirect2DPaintEngine);

    ComPtr<ID2D1Geometry> geometry = d->vectorPathToID2D1PathGeometry(path);
    if (!geometry) {
        qWarning("%s: Could not convert path to d2d geometry", __FUNCTION__);
        return;
    }

    const QBrush &brush = state()->brush;
    if (qbrush_style(brush) != Qt::NoBrush) {
        if (emulationRequired(BrushEmulation))
            rasterFill(path, brush);
        else
            fill(geometry.Get(), brush);
    }

    const QPen &pen = state()->pen;
    if (qpen_style(pen) != Qt::NoPen && qbrush_style(qpen_brush(pen)) != Qt::NoBrush) {
        if (emulationRequired(PenEmulation))
            QPaintEngineEx::stroke(path, pen);
        else
            stroke(geometry.Get(), pen);
    }
}

void QWindowsDirect2DPaintEngine::fill(const QVectorPath &path, const QBrush &brush)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugFillTag);

    if (path.isEmpty())
        return;

    ensureBrush(brush);
    if (emulationRequired(BrushEmulation)) {
        rasterFill(path, brush);
        return;
    }

    if (!d->brush.brush)
        return;

    ComPtr<ID2D1Geometry> geometry = d->vectorPathToID2D1PathGeometry(path);
    if (!geometry) {
        qWarning("%s: Could not convert path to d2d geometry", __FUNCTION__);
        return;
    }

    d->dc()->FillGeometry(geometry.Get(), d->brush.brush.Get());
}

void QWindowsDirect2DPaintEngine::fill(ID2D1Geometry *geometry, const QBrush &brush)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugFillTag);

    ensureBrush(brush);
    if (!d->brush.brush)
        return;

    d->dc()->FillGeometry(geometry, d->brush.brush.Get());
}

void QWindowsDirect2DPaintEngine::stroke(const QVectorPath &path, const QPen &pen)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugFillTag);

    if (path.isEmpty())
        return;

    ensurePen(pen);
    if (emulationRequired(PenEmulation)) {
        QPaintEngineEx::stroke(path, pen);
        return;
    }

    if (!d->pen.brush)
        return;

    ComPtr<ID2D1Geometry> geometry = d->vectorPathToID2D1PathGeometry(path);
    if (!geometry) {
        qWarning("%s: Could not convert path to d2d geometry", __FUNCTION__);
        return;
    }

    d->dc()->DrawGeometry(geometry.Get(), d->pen.brush.Get(), d->pen.qpen.widthF(), d->pen.strokeStyle.Get());
}

void QWindowsDirect2DPaintEngine::stroke(ID2D1Geometry *geometry, const QPen &pen)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugFillTag);

    ensurePen(pen);
    if (!d->pen.brush)
        return;

    d->dc()->DrawGeometry(geometry, d->pen.brush.Get(), d->pen.qpen.widthF(), d->pen.strokeStyle.Get());
}

void QWindowsDirect2DPaintEngine::clip(const QVectorPath &path, Qt::ClipOperation op)
{
    Q_D(QWindowsDirect2DPaintEngine);
    d->clip(path, op);
}

void QWindowsDirect2DPaintEngine::clipEnabledChanged()
{
    Q_D(QWindowsDirect2DPaintEngine);
    d->updateClipEnabled(state()->clipEnabled);
}

void QWindowsDirect2DPaintEngine::penChanged()
{
    Q_D(QWindowsDirect2DPaintEngine);
    d->updatePen(state()->pen);
}

void QWindowsDirect2DPaintEngine::brushChanged()
{
    Q_D(QWindowsDirect2DPaintEngine);
    d->updateBrush(state()->brush);
}

void QWindowsDirect2DPaintEngine::brushOriginChanged()
{
    Q_D(QWindowsDirect2DPaintEngine);
    d->updateBrushOrigin(state()->brushOrigin);
}

void QWindowsDirect2DPaintEngine::opacityChanged()
{
    Q_D(QWindowsDirect2DPaintEngine);
    d->updateOpacity(state()->opacity);
}

void QWindowsDirect2DPaintEngine::compositionModeChanged()
{
    Q_D(QWindowsDirect2DPaintEngine);
    d->updateCompositionMode(state()->compositionMode());
}

void QWindowsDirect2DPaintEngine::renderHintsChanged()
{
    Q_D(QWindowsDirect2DPaintEngine);
    d->updateHints();
}

void QWindowsDirect2DPaintEngine::transformChanged()
{
    Q_D(QWindowsDirect2DPaintEngine);
    d->updateTransform(state()->transform());
}

void QWindowsDirect2DPaintEngine::fillRect(const QRectF &rect, const QBrush &brush)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugFillRectTag);

    ensureBrush(brush);

    if (emulationRequired(BrushEmulation)) {
        QPaintEngineEx::fillRect(rect, brush);
    } else {
        QRectF r = rect.normalized();
        adjustForAliasing(&r);

        if (d->brush.brush)
            d->dc()->FillRectangle(to_d2d_rect_f(rect), d->brush.brush.Get());
    }
}

void QWindowsDirect2DPaintEngine::drawRects(const QRect *rects, int rectCount)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawRectsTag);

    ensureBrush();
    ensurePen();

    if (emulationRequired(BrushEmulation) || emulationRequired(PenEmulation)) {
        QPaintEngineEx::drawRects(rects, rectCount);
    } else {
        QRectF rect;
        for (int i = 0; i < rectCount; i++) {
            rect = rects[i].normalized();
            adjustForAliasing(&rect);

            D2D1_RECT_F d2d_rect = to_d2d_rect_f(rect);

            if (d->brush.brush)
                d->dc()->FillRectangle(d2d_rect, d->brush.brush.Get());

            if (d->pen.brush)
                d->dc()->DrawRectangle(d2d_rect, d->pen.brush.Get(), d->pen.qpen.widthF(), d->pen.strokeStyle.Get());
        }
    }
}

void QWindowsDirect2DPaintEngine::drawRects(const QRectF *rects, int rectCount)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawRectFsTag);

    ensureBrush();
    ensurePen();

    if (emulationRequired(BrushEmulation) || emulationRequired(PenEmulation)) {
        QPaintEngineEx::drawRects(rects, rectCount);
    } else {
        QRectF rect;
        for (int i = 0; i < rectCount; i++) {
            rect = rects[i].normalized();
            adjustForAliasing(&rect);

            D2D1_RECT_F d2d_rect = to_d2d_rect_f(rect);

            if (d->brush.brush)
                d->dc()->FillRectangle(d2d_rect, d->brush.brush.Get());

            if (d->pen.brush)
                d->dc()->DrawRectangle(d2d_rect, d->pen.brush.Get(), d->pen.qpen.widthF(), d->pen.strokeStyle.Get());
        }
    }
}

static bool isLinePositivelySloped(const QPointF &p1, const QPointF &p2)
{
    if (p2.x() > p1.x())
        return p2.y() < p1.y();

    if (p1.x() > p2.x())
        return p1.y() < p2.y();

    return false;
}

static void adjustLine(QPointF *p1, QPointF *p2)
{
    if (isLinePositivelySloped(*p1, *p2)) {
        p1->ry() -= qreal(1.0);
        p2->ry() -= qreal(1.0);
    }
}

void QWindowsDirect2DPaintEngine::drawLines(const QLine *lines, int lineCount)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawLinesTag);

    ensurePen();

    if (emulationRequired(PenEmulation)) {
        QPaintEngineEx::drawLines(lines, lineCount);
    } else if (d->pen.brush) {
        for (int i = 0; i < lineCount; i++) {
            QPointF p1 = lines[i].p1();
            QPointF p2 = lines[i].p2();

            // Match raster engine output
            if (p1 == p2 && d->pen.qpen.widthF() <= 1.0) {
                fillRect(QRectF(p1, QSizeF(d->pen.qpen.widthF(), d->pen.qpen.widthF())),
                         d->pen.qpen.brush());
                continue;
            }

            // Match raster engine output
            if (!antiAliasingEnabled())
                adjustLine(&p1, &p2);

            adjustForAliasing(&p1);
            adjustForAliasing(&p2);

            D2D1_POINT_2F d2d_p1 = to_d2d_point_2f(p1);
            D2D1_POINT_2F d2d_p2 = to_d2d_point_2f(p2);

            if (!d->pen.dashBrush || state()->renderHints.testFlag(QPainter::HighQualityAntialiasing)) {
                d->dc()->DrawLine(d2d_p1, d2d_p2, d->pen.brush.Get(), d->pen.qpen.widthF(), d->pen.strokeStyle.Get());
            } else {
                d->pen.dashBrush->SetTransform(transformFromLine(lines[i], d->pen.qpen.widthF()));
                d->dc()->DrawLine(d2d_p1, d2d_p2, d->pen.dashBrush.Get(), d->pen.qpen.widthF(), NULL);
            }
        }
    }
}

void QWindowsDirect2DPaintEngine::drawLines(const QLineF *lines, int lineCount)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawLineFsTag);

    ensurePen();

    if (emulationRequired(PenEmulation)) {
        QPaintEngineEx::drawLines(lines, lineCount);
    } else if (d->pen.brush) {
        for (int i = 0; i < lineCount; i++) {
            QPointF p1 = lines[i].p1();
            QPointF p2 = lines[i].p2();

            // Match raster engine output
            if (p1 == p2 && d->pen.qpen.widthF() <= 1.0) {
                fillRect(QRectF(p1, QSizeF(d->pen.qpen.widthF(), d->pen.qpen.widthF())),
                         d->pen.qpen.brush());
                continue;
            }

            // Match raster engine output
            if (!antiAliasingEnabled())
                adjustLine(&p1, &p2);

            adjustForAliasing(&p1);
            adjustForAliasing(&p2);

            D2D1_POINT_2F d2d_p1 = to_d2d_point_2f(p1);
            D2D1_POINT_2F d2d_p2 = to_d2d_point_2f(p2);

            if (!d->pen.dashBrush || state()->renderHints.testFlag(QPainter::HighQualityAntialiasing)) {
                d->dc()->DrawLine(d2d_p1, d2d_p2, d->pen.brush.Get(), d->pen.qpen.widthF(), d->pen.strokeStyle.Get());
            } else {
                d->pen.dashBrush->SetTransform(transformFromLine(lines[i], d->pen.qpen.widthF()));
                d->dc()->DrawLine(d2d_p1, d2d_p2, d->pen.dashBrush.Get(), d->pen.qpen.widthF(), NULL);
            }
        }
    }
}

void QWindowsDirect2DPaintEngine::drawEllipse(const QRectF &r)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawEllipseFTag);

    ensureBrush();
    ensurePen();

    if (emulationRequired(BrushEmulation) || emulationRequired(PenEmulation)) {
        QPaintEngineEx::drawEllipse(r);
    } else {
        QPointF p = r.center();
        adjustForAliasing(&p);

        D2D1_ELLIPSE ellipse = {
            to_d2d_point_2f(p),
            r.width() / 2.0,
            r.height() / 2.0
        };

        if (d->brush.brush)
            d->dc()->FillEllipse(ellipse, d->brush.brush.Get());

        if (d->pen.brush)
            d->dc()->DrawEllipse(ellipse, d->pen.brush.Get(), d->pen.qpen.widthF(), d->pen.strokeStyle.Get());
    }
}

void QWindowsDirect2DPaintEngine::drawEllipse(const QRect &r)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawEllipseTag);

    ensureBrush();
    ensurePen();

    if (emulationRequired(BrushEmulation) || emulationRequired(PenEmulation)) {
        QPaintEngineEx::drawEllipse(r);
    } else {
        QPointF p = r.center();
        adjustForAliasing(&p);

        D2D1_ELLIPSE ellipse = {
            to_d2d_point_2f(p),
            r.width() / 2.0,
            r.height() / 2.0
        };

        if (d->brush.brush)
            d->dc()->FillEllipse(ellipse, d->brush.brush.Get());

        if (d->pen.brush)
            d->dc()->DrawEllipse(ellipse, d->pen.brush.Get(), d->pen.qpen.widthF(), d->pen.strokeStyle.Get());
    }
}

void QWindowsDirect2DPaintEngine::drawImage(const QRectF &rectangle, const QImage &image,
                                            const QRectF &sr, Qt::ImageConversionFlags flags)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawImageTag);

    QPixmap pixmap = QPixmap::fromImage(image, flags);
    drawPixmap(rectangle, pixmap, sr);
}

void QWindowsDirect2DPaintEngine::drawPixmap(const QRectF &r,
                                             const QPixmap &pm,
                                             const QRectF &sr)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawPixmapTag);

    if (pm.isNull())
        return;

    if (pm.handle()->pixelType() == QPlatformPixmap::BitmapType) {
        QImage i = pm.toImage();
        i.setColor(0, qRgba(0, 0, 0, 0));
        i.setColor(1, d->pen.qpen.color().rgba());
        drawImage(r, i, sr);
        return;
    }

    if (d->flags.testFlag(EmulateComposition)) {
        const qreal points[] = {
            r.x(), r.y(),
            r.x() + r.width(), r.y(),
            r.x() + r.width(), r.y() + r.height(),
            r.x(), r.y() + r.height()
        };
        const QVectorPath vp(points, 4, 0, QVectorPath::RectangleHint);
        QBrush brush(sr.isValid() ? pm.copy(sr.toRect()) : pm);
        brush.setTransform(QTransform::fromTranslate(r.x(), r.y()));
        rasterFill(vp, brush);
        return;
    }

    QWindowsDirect2DPlatformPixmap *pp = static_cast<QWindowsDirect2DPlatformPixmap *>(pm.handle());
    QWindowsDirect2DBitmap *bitmap = pp->bitmap();

    ensurePen();

    if (bitmap->bitmap() != d->bitmap->bitmap()) {
        // Good, src bitmap != dst bitmap
        if (sr.isValid())
            d->dc()->DrawBitmap(bitmap->bitmap(),
                                to_d2d_rect_f(r), state()->opacity,
                                d->interpolationMode(),
                                to_d2d_rect_f(sr));
        else
            d->dc()->DrawBitmap(bitmap->bitmap(),
                                to_d2d_rect_f(r), state()->opacity,
                                d->interpolationMode());
    } else {
        // Ok, so the source pixmap and destination pixmap is the same.
        // D2D is not fond of this scenario, deal with it through
        // an intermediate bitmap
        QWindowsDirect2DBitmap intermediate;

        if (sr.isValid()) {
            bool r = intermediate.resize(sr.width(), sr.height());
            if (!r) {
                qWarning("%s: Could not resize intermediate bitmap to source rect size", __FUNCTION__);
                return;
            }

            D2D1_RECT_U d2d_sr =  to_d2d_rect_u(sr.toRect());
            HRESULT hr = intermediate.bitmap()->CopyFromBitmap(NULL,
                                                               bitmap->bitmap(),
                                                               &d2d_sr);
            if (FAILED(hr)) {
                qWarning("%s: Could not copy source rect area from source bitmap to intermediate bitmap: %#x", __FUNCTION__, hr);
                return;
            }
        } else {
            bool r = intermediate.resize(bitmap->size().width(),
                                         bitmap->size().height());
            if (!r) {
                qWarning("%s: Could not resize intermediate bitmap to source bitmap size", __FUNCTION__);
                return;
            }

            HRESULT hr = intermediate.bitmap()->CopyFromBitmap(NULL,
                                                               bitmap->bitmap(),
                                                               NULL);
            if (FAILED(hr)) {
                qWarning("%s: Could not copy source bitmap to intermediate bitmap: %#x", __FUNCTION__, hr);
                return;
            }
        }

        d->dc()->DrawBitmap(intermediate.bitmap(),
                            to_d2d_rect_f(r), state()->opacity,
                            d->interpolationMode());
    }
}

void QWindowsDirect2DPaintEngine::drawStaticTextItem(QStaticTextItem *staticTextItem)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawStaticTextItemTag);

    if (staticTextItem->numGlyphs == 0)
        return;

    ensurePen();

    // If we can't support the current configuration with Direct2D, fall back to slow path
    if (emulationRequired(PenEmulation)) {
        QPaintEngineEx::drawStaticTextItem(staticTextItem);
        return;
    }

    ComPtr<IDWriteFontFace> fontFace = d->fontFaceFromFontEngine(staticTextItem->fontEngine());
    if (!fontFace) {
        qWarning("%s: Could not find font - falling back to slow text rendering path.", __FUNCTION__);
        QPaintEngineEx::drawStaticTextItem(staticTextItem);
        return;
    }

    QVarLengthArray<UINT16> glyphIndices(staticTextItem->numGlyphs);
    QVarLengthArray<FLOAT> glyphAdvances(staticTextItem->numGlyphs);
    QVarLengthArray<DWRITE_GLYPH_OFFSET> glyphOffsets(staticTextItem->numGlyphs);

    for (int i = 0; i < staticTextItem->numGlyphs; i++) {
        glyphIndices[i] = UINT16(staticTextItem->glyphs[i]); // Imperfect conversion here

        // This looks  a little funky because the positions are precalculated
        glyphAdvances[i] = 0;
        glyphOffsets[i].advanceOffset = staticTextItem->glyphPositions[i].x.toReal();
        // Qt and Direct2D seem to disagree on the direction of the ascender offset...
        glyphOffsets[i].ascenderOffset = staticTextItem->glyphPositions[i].y.toReal() * -1;
    }

    d->drawGlyphRun(D2D1::Point2F(0, 0),
                    fontFace.Get(),
                    staticTextItem->fontEngine()->fontDef,
                    staticTextItem->numGlyphs,
                    glyphIndices.constData(),
                    glyphAdvances.constData(),
                    glyphOffsets.constData(),
                    false);
}

void QWindowsDirect2DPaintEngine::drawTextItem(const QPointF &p, const QTextItem &textItem)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawTextItemTag);

    const QTextItemInt &ti = static_cast<const QTextItemInt &>(textItem);
    if (ti.glyphs.numGlyphs == 0)
        return;

    ensurePen();

    // If we can't support the current configuration with Direct2D, fall back to slow path
    if (emulationRequired(PenEmulation)) {
        QPaintEngine::drawTextItem(p, textItem);
        return;
    }

    ComPtr<IDWriteFontFace> fontFace = d->fontFaceFromFontEngine(ti.fontEngine);
    if (!fontFace) {
        qWarning("%s: Could not find font - falling back to slow text rendering path.", __FUNCTION__);
        QPaintEngine::drawTextItem(p, textItem);
        return;
    }

    QVarLengthArray<UINT16> glyphIndices(ti.glyphs.numGlyphs);
    QVarLengthArray<FLOAT> glyphAdvances(ti.glyphs.numGlyphs);
    QVarLengthArray<DWRITE_GLYPH_OFFSET> glyphOffsets(ti.glyphs.numGlyphs);

    for (int i = 0; i < ti.glyphs.numGlyphs; i++) {
        glyphIndices[i] = UINT16(ti.glyphs.glyphs[i]); // Imperfect conversion here
        glyphAdvances[i] = ti.glyphs.effectiveAdvance(i).toReal();
        glyphOffsets[i].advanceOffset = ti.glyphs.offsets[i].x.toReal();

        // XXX Should we negate the y value like for static text items?
        glyphOffsets[i].ascenderOffset = ti.glyphs.offsets[i].y.toReal();
    }

    const bool rtl = (ti.flags & QTextItem::RightToLeft);
    const QPointF offset(rtl ? ti.width.toReal() : 0, 0);

    d->drawGlyphRun(to_d2d_point_2f(p + offset),
                    fontFace.Get(),
                    ti.fontEngine->fontDef,
                    ti.glyphs.numGlyphs,
                    glyphIndices.constData(),
                    glyphAdvances.constData(),
                    glyphOffsets.constData(),
                    rtl);
}

void QWindowsDirect2DPaintEngine::ensureBrush()
{
    ensureBrush(state()->brush);
}

void QWindowsDirect2DPaintEngine::ensureBrush(const QBrush &brush)
{
    Q_D(QWindowsDirect2DPaintEngine);
    d->updateBrush(brush);
}

void QWindowsDirect2DPaintEngine::ensurePen()
{
    ensurePen(state()->pen);
}

void QWindowsDirect2DPaintEngine::ensurePen(const QPen &pen)
{
    Q_D(QWindowsDirect2DPaintEngine);
    d->updatePen(pen);
}

void QWindowsDirect2DPaintEngine::rasterFill(const QVectorPath &path, const QBrush &brush)
{
    Q_D(QWindowsDirect2DPaintEngine);

    if (d->fallbackImage.isNull()) {
        if (d->flags.testFlag(EmulateComposition)) {
            QWindowsDirect2DPaintEngineSuspender suspender(this);
            d->fallbackImage = d->bitmap->toImage();
        } else {
            d->fallbackImage = QImage(d->bitmap->size(), QImage::Format_ARGB32_Premultiplied);
            d->fallbackImage.fill(Qt::transparent);
        }
    }

    QImage &img = d->fallbackImage;
    QPainter p;
    QPaintEngine *engine = img.paintEngine();

    if (engine->isExtended() && p.begin(&img)) {
        p.setRenderHints(state()->renderHints);
        p.setCompositionMode(state()->compositionMode());
        p.setOpacity(state()->opacity);
        p.setBrushOrigin(state()->brushOrigin);
        p.setBrush(state()->brush);
        p.setPen(state()->pen);

        QPaintEngineEx *extended = static_cast<QPaintEngineEx *>(engine);
        foreach (const QPainterClipInfo &info, state()->clipInfo) {
            extended->state()->matrix = info.matrix;
            extended->transformChanged();

            switch (info.clipType) {
            case QPainterClipInfo::RegionClip:
                extended->clip(info.region, info.operation);
                break;
            case QPainterClipInfo::PathClip:
                extended->clip(info.path, info.operation);
                break;
            case QPainterClipInfo::RectClip:
                extended->clip(info.rect, info.operation);
                break;
            case QPainterClipInfo::RectFClip:
                qreal right = info.rectf.x() + info.rectf.width();
                qreal bottom = info.rectf.y() + info.rectf.height();
                qreal pts[] = { info.rectf.x(), info.rectf.y(),
                                right, info.rectf.y(),
                                right, bottom,
                                info.rectf.x(), bottom };
                QVectorPath vp(pts, 4, 0, QVectorPath::RectangleHint);
                extended->clip(vp, info.operation);
                break;
            }
        }

        extended->state()->matrix = state()->matrix;
        extended->transformChanged();

        extended->fill(path, brush);
        if (!p.end())
            qWarning("%s: Paint Engine end returned false", __FUNCTION__);

        if (!d->flags.testFlag(EmulateComposition)) { // Emulated fallback will be flattened in end()
            d->updateClipEnabled(false);
            d->updateTransform(QTransform());
            drawImage(img.rect(), img, img.rect());
            d->fallbackImage = QImage();
            transformChanged();
            clipEnabledChanged();
        }
    } else {
        qWarning("%s: Could not fall back to QImage", __FUNCTION__);
    }
}

bool QWindowsDirect2DPaintEngine::emulationRequired(EmulationType type) const
{
    Q_D(const QWindowsDirect2DPaintEngine);

    if (d->flags.testFlag(EmulateComposition))
        return true;

    if (!state()->matrix.isAffine())
        return true;

    switch (type) {
    case PenEmulation:
        return d->pen.emulate;
        break;
    case BrushEmulation:
        return d->brush.emulate;
        break;
    }

    return false;
}

bool QWindowsDirect2DPaintEngine::antiAliasingEnabled() const
{
    return state()->renderHints & QPainter::Antialiasing;
}

void QWindowsDirect2DPaintEngine::adjustForAliasing(QRectF *rect)
{
   if (!antiAliasingEnabled()) {
       rect->adjust(MAGICAL_ALIASING_OFFSET,
                    MAGICAL_ALIASING_OFFSET,
                    MAGICAL_ALIASING_OFFSET,
                    MAGICAL_ALIASING_OFFSET);
   }
}

void QWindowsDirect2DPaintEngine::adjustForAliasing(QPointF *point)
{
    static const QPointF adjustment(MAGICAL_ALIASING_OFFSET,
                                    MAGICAL_ALIASING_OFFSET);

    if (!antiAliasingEnabled())
        (*point) += adjustment;
}

void QWindowsDirect2DPaintEngine::suspend()
{
    end();
}

void QWindowsDirect2DPaintEngine::resume()
{
    begin(paintDevice());
    clipEnabledChanged();
    penChanged();
    brushChanged();
    brushOriginChanged();
    opacityChanged();
    compositionModeChanged();
    renderHintsChanged();
    transformChanged();
}

class QWindowsDirect2DPaintEngineSuspenderImpl
{
    Q_DISABLE_COPY(QWindowsDirect2DPaintEngineSuspenderImpl)
    QWindowsDirect2DPaintEngine *m_engine;
    bool m_active;
public:
    QWindowsDirect2DPaintEngineSuspenderImpl(QWindowsDirect2DPaintEngine *engine)
        : m_engine(engine)
        , m_active(engine->isActive())
    {
        if (m_active)
            m_engine->suspend();
    }

    ~QWindowsDirect2DPaintEngineSuspenderImpl()
    {
        if (m_active)
            m_engine->resume();
    }
};

class QWindowsDirect2DPaintEngineSuspenderPrivate
{
public:
    QWindowsDirect2DPaintEngineSuspenderPrivate(QWindowsDirect2DPaintEngine *engine)
        : engineSuspender(engine)
        , dcSuspender(static_cast<QWindowsDirect2DPaintEnginePrivate *>(engine->d_ptr.data())->bitmap->deviceContext())
    {
    }

    QWindowsDirect2DPaintEngineSuspenderImpl engineSuspender;
    QWindowsDirect2DDeviceContextSuspender dcSuspender;
};

QWindowsDirect2DPaintEngineSuspender::QWindowsDirect2DPaintEngineSuspender(QWindowsDirect2DPaintEngine *engine)
    : d_ptr(new QWindowsDirect2DPaintEngineSuspenderPrivate(engine))
{

}

QWindowsDirect2DPaintEngineSuspender::~QWindowsDirect2DPaintEngineSuspender()
{
}

void QWindowsDirect2DPaintEngineSuspender::resume()
{
    d_ptr.reset();
}

QT_END_NAMESPACE
